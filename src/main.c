/* Includes */

#include <wonderful.h>
#include <ws.h>
#include "minix.h"
#include "console.h"
#include "memory.h"

#ifdef __WONDERFUL_WWITCH__
#include <sys/bios.h>
#include <sys/filesys.h>
#include <sys/indirect.h>
#include <sys/oswork.h>

#define TARGET_WWITCH
#else
#include <nile.h>
#include <nilefs.h>

#define TARGET_NILESWAN
#endif

/* Launch entrypoint state */

uint16_t entry_seg;
uint16_t entry_ofs;
uint16_t text_size;
uint16_t data_size;
uint16_t bss_size;

__attribute__((noreturn))
extern void launch_entrypoint(void);

/* Target variables */

#if defined(TARGET_WWITCH)

#elif defined(TARGET_NILESWAN)
#define NILESWAN_KERNEL_BANK 2
#define NILESWAN_BOOTOPTS_BANK 3

__attribute__((section(".iram_4000")))
FATFS fs;
__attribute__((section(".iram_4400")))
FIL fsf;
#endif

/* Target helpers */

#ifndef __WONDERFUL_WWITCH__
__attribute__((section(".iramx_0400")))
uint8_t mem_buffer[0x4000 - 0x400];

void comm_open(void) {
	ws_uart_open(WS_UART_BAUD_RATE_38400);
}
#endif

void fatfs_check(int result) {
	if (result)
		panic("FatFs error %d", result);
}

/* Strings */

static const char __wf_rom s_mono_model_not_supported[] = "\"mono\" model not supported";
static const char __wf_rom s_system_invalid[] = "system invalid";
static const char __wf_rom s_data_too_big[] = "data too big";
static const char __wf_rom s_invalid_data_alignment[] = "invalid data alignment";
static const char __wf_rom s_system_not_found[] = "system not found";

/* Shared helpers */

void system_init(void) {
	if (ws_system_is_color_model()) {
		ws_system_set_mode(WS_SYSTEM_CTRL_COLOR_MODE_COLOR_2BPP);
		WS_SCREEN_COLOR_MEM(0)[0] = 0x000;
		WS_SCREEN_COLOR_MEM(0)[1] = 0xAAA; // zx0 local font
		WS_SCREEN_COLOR_MEM(0)[3] = 0xAAA; // FreyaBIOS font
	} else {
		panic(s_mono_model_not_supported);
	}
}

void apply_minix_header(minix_hdr_t __far* hdr) {
	if (!minix_validate_header(hdr))
		panic(s_system_invalid);
	if (hdr->data_len > DATA_LEN_MAX)
		panic(s_data_too_big);

	text_size = hdr->text_len;
	data_size = hdr->data_len;
	bss_size = hdr->bss_len;

	entry_seg += hdr->entry_seg;
	entry_ofs = hdr->entry_ofs;
}

static void __far* ptr_to_seg_or_panic(void __far *ptr) {
	if (!ptr)
		return ptr;
	if (FP_OFF(ptr) & 15)
		panic(s_invalid_data_alignment);
	return MK_FP(FP_SEG(ptr) + (FP_OFF(ptr) >> 4), 0);
}

/* Main function */

void main(void) {
	/* System initialization */

	console_init();
	cputs("elkswan");
	system_init();

	/* Target iniialization */

#ifdef TARGET_WWITCH
	// ELKS may corrupt FreyaBIOS resume flags
	sys_set_resume(0);
#endif

#ifdef TARGET_NILESWAN
	nile_io_unlock();
	nile_bank_unlock();

	// FIXME: Why is this necessary?
	nilefs_eject();
#endif

	/* Loading kernel - first stage */

#ifdef TARGET_WWITCH
	FsIL fsIL;

	{
		asm volatile("" ::: "memory");
		uint16_t old_bank = bank_get_map(BANK_SRAM);
		asm volatile("" ::: "memory");
		bank_set_map(BANK_SRAM, BANK_OSWORK);
		asm volatile("" ::: "memory");
		memcpy(&fsIL, rom0_fs->il, sizeof(FsIL));
		asm volatile("" ::: "memory");
		bank_set_map(BANK_SRAM, old_bank);
		asm volatile("" ::: "memory");
	}

	void __far *system_ptr = ptr_to_seg_or_panic(fsIL._mmap(rom0_fs, "system"));
	if (system_ptr == NULL)
		panic(s_system_not_found);

	void __far *bootopts_ptr = ptr_to_seg_or_panic(fsIL._mmap(rom0_fs, "bootopts"));
	void __far *romfs_ptr = ptr_to_seg_or_panic(fsIL._mmap(rom0_fs, "romfs"));
	if (!romfs_ptr)
		romfs_ptr = fsIL._mmap(rom0_fs, "romfs.bin");
#endif

#ifdef TARGET_NILESWAN
	char blank = 0;

	// Mount file system
	fatfs_check(f_mount(&fs, &blank, 1));
#endif

	/* Take over system - disables FreyaBIOS etc. */

	ia16_disable_irq();
	ws_int_disable_all();
	ws_int_ack_all();
	outportb(WS_TIMER_CTRL_PORT, 0);

	/* Initialize setup */

	memset(MK_FP(SETUP_SEG, 0), 0, SETUP_LEN);

#ifdef TARGET_WWITCH
	setup_xms_kbytes = 3*64;
	setup_root_dev = ROMFS_DEV;

	entry_seg = FP_SEG(system_ptr) + 2;
	apply_minix_header(system_ptr);
	void __far* system_ptr_data = MK_FP(FP_SEG(system_ptr) + 2 + (text_size >> 4), text_size & 15);
	memcpy(data_ptr, system_ptr_data, data_size);

	if (bootopts_ptr)
		setup_opt_base = FP_SEG(bootopts_ptr);

	if (romfs_ptr)
		setup_romfs_base = FP_SEG(romfs_ptr);
#endif

#ifdef TARGET_NILESWAN
	// TODO: implement nileswan layout
	setup_xms_kbytes = 8*64;

	uint16_t bytes_read;

	// Read /boot/system
	int result = f_open(&fsf, "/boot/system", FA_OPEN_EXISTING | FA_READ);
	if (result == FR_NO_FILE || result == FR_NO_PATH)
		panic(s_system_not_found);
	else
		fatfs_check(result);

	fatfs_check(f_read(&fsf, data_ptr, 32, &bytes_read));

	entry_seg = 0x2000;
	ws_bank_rom0_set(NILESWAN_KERNEL_BANK);

	apply_minix_header(data_ptr);

	ws_bank_with_ram(NILESWAN_KERNEL_BANK, {
		ws_bank_with_flash(1, {
			for (uint16_t text_pos = 0; text_pos < text_size; text_pos += 16384) {
				uint16_t next_step = text_size - text_pos;
				if (next_step > 16384)
					next_step = 16384;

				fatfs_check(f_read(&fsf, MK_FP(0x1000, text_pos), next_step, &bytes_read));
				cputs(".");
			}

			fatfs_check(f_read(&fsf, data_ptr, data_size, &bytes_read));
			cputs(".");

			f_close(&fsf);
		});
	});

	// Read /bootopts
	result = f_open(&fsf, "/bootopts", FA_OPEN_EXISTING | FA_READ);
	if (!(result == FR_NO_FILE || result == FR_NO_PATH)) {
		if (f_size(&fsf) > BOOTOPTS_MAX)
			panic("bootopts too big");

		ws_bank_with_ram(NILESWAN_BOOTOPTS_BANK, {
			ws_bank_with_flash(1, {
				memset(MK_FP(0x1000, 0x0000), 0, BOOTOPTS_MAX);
				fatfs_check(f_read(&fsf, MK_FP(0x1000, 0x0000), BOOTOPTS_MAX, &bytes_read));
			});
		});
		cputs(".");

		f_close(&fsf);

		setup_opt_base = 0x3000;
		ws_bank_rom1_set(NILESWAN_BOOTOPTS_BANK);
	}

	// Read /boot/romfs.bin
	result = f_open(&fsf, "/boot/romfs.bin", FA_OPEN_EXISTING | FA_READ);
	if (!(result == FR_NO_FILE || result == FR_NO_PATH)) {
		// TODO: Support slightly larger ROMFS by switching linear bank at launch_entrypoint() 
		if (f_size(&fsf) > (768-32)*1024)
			panic("romfs too big");

		uint8_t romfs_bank = (inportb(0xC0) << 4) + 4;
		ws_bank_with_flash(1, {
			while (!f_eof(&fsf)) {
				uint32_t ofs = f_tell(&fsf);
				ws_bank_with_ram(romfs_bank + (ofs >> 16), {
					fatfs_check(f_read(&fsf, MK_FP(0x1000, ofs & 0xFFFF), 32768, &bytes_read));
				});
				cputs(".");
			}
		});

		f_close(&fsf);

		setup_romfs_base = 0x4000;
		setup_root_dev = ROMFS_DEV;
	} else {
		// FIXME: FAT support
		setup_root_dev = ROMFS_DEV;
	}
#endif

	cprintf("\nentry at %04x:%04x", entry_seg, entry_ofs);

	// Clean system state for launch:
	// - Reset display state
	// - Open serial port (for debugging)
	ws_display_set_control(0);
	comm_open();

	// Jump
	launch_entrypoint();
}
