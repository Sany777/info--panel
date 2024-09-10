#ifndef EPAPER_ADAPTER_H
#define EPAPER_ADAPTER_H

#include "stdarg.h"
#include "stdbool.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    BLACK,
    WHITE,
    RED
}color_t;

typedef enum FontSize{
    FONT_SIZE_12 = 0,
    FONT_SIZE_16,
    FONT_SIZE_20,
    FONT_SIZE_34,
    FONT_SIZE_MAX
}font_size_t ;


void epaper_init();
void epaper_printf(int hor, int ver, font_size_t font_size, color_t color, const char *format, ...);
void epaper_refresh();
void epaper_clear();
void draw_rect(int hor_0, int ver_0, int hor_1, int ver_1, color_t color, int filled);
void draw_circle(int hor, int ver, int radius, color_t color, int filled);
void draw_line(int hor_0, int ver_0, int hor_1, int ver_1, color_t color);
void draw_horizontal_line(int hor_0, int hor_1, int ver, int width, color_t color);
void draw_vertical_line(int ver_0, int ver_1, int hor, int width, color_t color);
void epaper_set_rotate(int cur_rotate);
void epaper_print_str(int hor, int ver, font_size_t font_size, color_t color, const char *str);
void epaper_print_centered_str(int ver, font_size_t font_size, color_t color, const char *str);
void epaper_printf_centered(int ver, font_size_t font_size, color_t color, const char *format, ...);
void epaper_display_part();
bool epaper_update();
void epaper_wait();
void epaper_display_image(int x, int y, int w, int h,  color_t color, const unsigned char *bitmap);





#ifdef __cplusplus
}
#endif













#endif