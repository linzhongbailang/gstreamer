#ifndef _OVERLAY_H_
#define _OVERLAY_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif 

int overlay_text(uint8_t *dst_data, 
    uint32_t dst_w, uint32_t dst_h, 
    uint32_t start_x, uint32_t start_y, 
    const char *text);

int overlay_image(uint8_t *dst_data, 
    uint32_t dst_w, uint32_t dst_h, 
    uint32_t start_x, uint32_t start_y, 
    const char *image_name);

#ifdef __cplusplus
}
#endif 

#endif /* UTILS_OVERLAY_H */