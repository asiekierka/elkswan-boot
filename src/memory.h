#ifndef MEMORY_H_
#define MEMORY_H_

#define SETUP_SEG 0x0040
#define DATA_SEG  0x0061

#define SETUP_LEN 0x0200
#define DATA_LEN_MAX (0x3000 - (DATA_SEG << 4))
#define BOOTOPTS_MAX 1024

#define setupb(i) (*((uint8_t ws_iram*) ((SETUP_SEG << 4) + (i))))
#define setupw(i) (*((uint16_t ws_iram*) ((SETUP_SEG << 4) + (i))))
#define data_ptr ((uint8_t ws_iram*) (DATA_SEG << 4))

#define setup_opt_base setupw(0x1de)
#define setup_romfs_base setupw(0x1e0)
#define setup_xms_kbytes setupw(0x1ea)
#define setup_root_dev setupw(0x1fc)

#define ROMFS_DEV 0x0600

#endif /* MEMORY_H_ */