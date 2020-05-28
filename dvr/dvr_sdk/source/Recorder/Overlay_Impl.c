#include "Overlay.h"
#include "Overlay_Table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FALSE 0
#define TRUE 1

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
/*
 * A OVER B alpha compositing operation, with:
 *  alphaS: source pixel alpha
 *  colorS: source pixel color
 *  alphaD: destination pixel alpha
 *  colorD: destination pixel color
 *  alphaD: blended pixel alpha
 */
#define OVER(alphaS, colorS, alphaD, colorD, alphaF) \
    MIN((colorS * alphaS + colorD * alphaD * (255 - alphaS) / 255) / alphaF, 255)

#define OVERLAY_BLEND overlay_blend_optimize

static int unpack_nv12(uint8_t *src, uint32_t src_w, uint32_t src_h,
    uint32_t x, uint32_t y,
    uint8_t *dst, uint32_t dst_w)
{
    if (x & 1 || dst_w & 1)
    {
        printf("'x', 'y' or 'dst_w' is not correct!%d\n", y);
        return FALSE;
    }

    if (x + dst_w > src_w || y > src_h)
    {
        return FALSE;
    }

    const uint8_t *sy = src + y*src_w + x;
    const uint8_t *suv = src + src_h*src_w + (y / 2)*src_w + x;
    uint8_t *d = dst;

    unsigned int i;
    for (i = 0; i < (dst_w >> 1); i++)
    {
        d[i * 8 + 0] = sy[i * 2 + 0];
        d[i * 8 + 1] = suv[i * 2 + 0];
        d[i * 8 + 2] = suv[i * 2 + 1];
        d[i * 8 + 3] = 0xff;
        d[i * 8 + 4] = sy[i * 2 + 1];
        d[i * 8 + 5] = suv[i * 2 + 0];
        d[i * 8 + 6] = suv[i * 2 + 1];
        d[i * 8 + 7] = 0xff;
    }

    return TRUE;
}


static int pack_nv12(uint8_t *src, uint32_t src_w,
    uint8_t *dst, uint32_t dst_w, uint32_t dst_h,
    uint32_t x, uint32_t y)
{
    if (x & 1 || dst_w & 1)
    {
        printf("'x', 'y' or 'dst_w' is not correct!\n");
        return FALSE;
    }

    if (x + src_w > dst_w || y > dst_h)
    {
        return FALSE;
    }

    uint8_t *dy = dst + y*dst_w + x;
    uint8_t *duv = dst + dst_h*dst_w + (y / 2)*dst_w + x;
    uint8_t *s = src;

    unsigned int i;
    for (i = 0; i < (src_w >> 1); i++)
    {
        dy[i * 2 + 0] = s[i * 8 + 0];
        dy[i * 2 + 1] = s[i * 8 + 4];
        duv[i * 2 + 0] = s[i * 8 + 1];
        duv[i * 2 + 1] = s[i * 8 + 2];
    }

    return TRUE;
}

static void overlay_blend(uint8_t *src, uint32_t src_w, uint32_t src_h,
    uint32_t x, uint32_t y,
    uint8_t *dst, uint32_t dst_w, uint32_t dst_h)
{
    uint8_t *tmp_src;
    uint8_t *tmp_dst = (uint8_t *)malloc(src_w * 4 * sizeof(uint8_t));
    //uint8_t tmp_dst[src_w * 4];

    unsigned int i, dst_yoff, src_yoff = 0;
    for (dst_yoff = y; dst_yoff < y + src_h; dst_yoff++, src_yoff++)
    {
        unpack_nv12(dst, dst_w, dst_h, x, dst_yoff, tmp_dst, src_w);
        tmp_src = src + src_yoff*src_w * 4;

        // blend
        for (i = 0; i < src_w * 4; i += 4)
        {
            uint8_t src_a = tmp_src[i + 3];
            uint8_t dst_a = tmp_dst[i + 3];
            uint8_t final_alpha;

            if (!src_a)
                continue;

            // obtain final alpha
            final_alpha = src_a + dst_a * (255 - src_a) / 255;
            tmp_dst[i + 0] = OVER(src_a, tmp_src[i + 0], dst_a, tmp_dst[i + 0], final_alpha);
            tmp_dst[i + 1] = OVER(src_a, tmp_src[i + 1], dst_a, tmp_dst[i + 1], final_alpha);
            tmp_dst[i + 2] = OVER(src_a, tmp_src[i + 2], dst_a, tmp_dst[i + 2], final_alpha);
        }
        pack_nv12(tmp_dst, src_w, dst, dst_w, dst_h, x, dst_yoff);
    }
    free(tmp_dst);
}

static void overlay_blend_optimize(uint8_t *src, uint32_t src_w, uint32_t src_h,
    uint32_t x, uint32_t y,
    uint8_t *dst, uint32_t dst_w, uint32_t dst_h)
{
    uint8_t *tmp_src;
    uint8_t *dst_y, *dst_uv;

    unsigned int i, dst_yoff, src_yoff = 0;
    for (dst_yoff = y; dst_yoff < y + src_h; dst_yoff++, src_yoff++)
    {
        tmp_src = src + src_yoff * src_w * 4;
        dst_y = dst + dst_yoff * dst_w + x;
        dst_uv = dst + dst_h * dst_w + (dst_yoff >> 1) * dst_w + x;

        // blend
        for (i = 0; i < src_w * 4; i += 8)
        {
            uint8_t src_a = tmp_src[i + 3];
            uint8_t src_a2 = tmp_src[i + 7];
            uint8_t dst_a = 255;
            uint8_t final_alpha, final_alpha2;

            if (src_a)
            {
                // obtain final alpha
                final_alpha = src_a + dst_a * (255 - src_a) / 255;
                final_alpha2 = src_a2 + dst_a * (255 - src_a2) / 255;

                *dst_y = OVER(src_a, tmp_src[i + 0], dst_a, *dst_y, final_alpha);
                *dst_uv = OVER(src_a, tmp_src[i + 1], dst_a, *dst_uv, final_alpha);
                *(dst_uv + 1) = OVER(src_a, tmp_src[i + 2], dst_a, *(dst_uv + 1), final_alpha);

                *(dst_y + 1) = OVER(src_a2, tmp_src[i + 4], dst_a, *(dst_y + 1), final_alpha2);
                *dst_uv = OVER(src_a2, tmp_src[i + 5], dst_a, *dst_uv, final_alpha2);
                *(dst_uv + 1) = OVER(src_a2, tmp_src[i + 6], dst_a, *(dst_uv + 1), final_alpha2);
            }
            dst_y += 2;
            dst_uv += 2;
        }
    }
}

int overlay_text(uint8_t *dst_data,
    uint32_t dst_w, uint32_t dst_h,
    uint32_t start_x, uint32_t start_y,
    const char *text)
{
    int i, j, k;

    TableInfo *text_table = (TableInfo *)malloc(strlen(text) * sizeof(TableInfo));
    int table_num = sizeof(table_info) / sizeof(table_info[0]);
    int text_length = strlen(text);
    int table_size = text_length;

    // get text table according to text string
    for (i = 0; i < text_length; i++)
    {
        for (j = 0; j < table_num; j++)
        {
            if (table_info[j]->text == text[i])
            {
                memcpy(&text_table[i], table_info[j], sizeof(TableInfo));
                break;
            }
        }
    }

    for (k = 0; k < table_size; k++)
    {
        OVERLAY_BLEND(text_table[k].table, text_table[k].table_width, text_table[k].table_height, start_x, start_y, dst_data, dst_w, dst_h);
        start_x += text_table[k].table_width;
    }

    free(text_table);
    return TRUE;
}

int overlay_image(uint8_t *dst_data,
    uint32_t dst_w, uint32_t dst_h,
    uint32_t start_x, uint32_t start_y,
    const char *image_name)
{
    int i;

    TableInfo image_table;
    int table_num = sizeof(table_info) / sizeof(table_info[0]);

    // get image table according to image name
    for (i = 0; i < table_num; i++)
    {
        if (!strcmp(table_info[i]->table_name, image_name))
        {
            memcpy(&image_table, table_info[i], sizeof(TableInfo));
            break;
        }
    }

    if (strcmp(image_table.table_name, image_name))
    {
        printf("couldn't find table with name '%s'\n", image_name);
        return FALSE;
    }

    // overlay image on dst image
    OVERLAY_BLEND(image_table.table, image_table.table_width, image_table.table_height, start_x, start_y, dst_data, dst_w, dst_h);
    start_x += image_table.table_width;
    return TRUE;
}