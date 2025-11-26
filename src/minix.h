#ifndef MINIX_H_
#define MINIX_H_

#include <stdbool.h>
#include <stdint.h>

#define MINIX_AOUT_MAGIC 0x04300301UL

typedef struct {
    uint32_t magic;
    uint8_t header_len;
    uint8_t reserved_1;
    uint16_t version;
    uint32_t text_len;
    uint32_t data_len;
    uint32_t bss_len;
    uint16_t entry_ofs;
    uint16_t entry_seg;
    uint16_t chmem;
    uint16_t min_stack;
    uint32_t syms;
} minix_hdr_t;

bool minix_validate_header(minix_hdr_t __far *hdr);

#endif /* MINIX_H_ */