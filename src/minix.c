#include "minix.h"

bool minix_validate_header(minix_hdr_t __far *hdr) {
    return
        hdr->magic == MINIX_AOUT_MAGIC
        && hdr->header_len == 32;
}