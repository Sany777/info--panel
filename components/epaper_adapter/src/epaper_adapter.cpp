#include "epaper_adapter.h"

#include "epaper.h"
#include "epdpaint.h"
#include "fonts.h"
#include "epd2.9.h"

#include "cstring"
#include "cstdio"

#include "esp_log.h"

#define SCREEN_HEIGHT   128
#define SCREEN_WIDTH    296

static Paint *paint, *paint_red;
static Epd epd;

static unsigned char screen[SCREEN_WIDTH * SCREEN_HEIGHT/8];
static unsigned char screen_red[SCREEN_WIDTH * SCREEN_HEIGHT/8];
static unsigned char update_screen_indicator[SCREEN_WIDTH * SCREEN_HEIGHT/8];


static char text_buf[150];
static const int USE_SCREEN_WIDTH = SCREEN_WIDTH-4;
static sFONT *font_list[] = {&Font12, &Font16, &Font20};


void epaper_init()
{
    if(!paint){
        paint = new Paint(screen, epd.width, epd.height);
        paint_red = new Paint(screen_red, epd.width, epd.height);
        paint->SetRotate(ROTATE_270);
        paint_red->SetRotate(ROTATE_270);
    }
    epd.LDirInit();

}


static sFONT* get_font(size_t font_id)
{
    if(font_id >= FONT_SIZE_MAX){
        return NULL;
    }
    return font_list[font_id];
}

void epaper_print_str(int hor, int ver, font_size_t font_size, color_t color, const char *str)
{
    sFONT * font = get_font(font_size);
    if(font){
        (color == RED ? paint_red : paint)->DrawStringAt(hor, ver, str, font, color); 
    }
}


void epaper_display_image(int x, int y, int w, int h,  color_t color, const unsigned char *bitmap) 
{
    Paint *p = color == RED ? paint_red : paint;
    int16_t byteWidth = (w + 7) / 8;
    uint8_t byte = 0;

    for (uint16_t j = 0; j < h; j++){
      for (uint16_t i = 0; i < w; i++ ){
        if (i & 7) byte <<= 1;
        else
        {
          byte = bitmap[j * byteWidth + i / 8];
        }
        if (! (byte & 0x80) ){
            p->DrawPixel(x + i, y + j, color);
        }
      }
    }
}

static sFONT* get_fit_font(font_size_t font_id, size_t char_num)
{
    size_t ind = font_id;
    sFONT * font = NULL;
    size_t str_width = 0;
    do{
        if(ind >= FONT_SIZE_MAX)break;
        font = get_font(ind);
        ind -= 1;
        str_width = font->Width * char_num;
    }while(str_width>USE_SCREEN_WIDTH);
    return font;
}

void epaper_print_centered_str(int ver, font_size_t font_size, color_t color, const char *str)
{
    int hor = 5, str_width;
    const size_t str_len = strlen(str);
    sFONT * font = get_fit_font(font_size, str_len);
    if(font){
        str_width = font->Width * str_len;
        if(str_width < USE_SCREEN_WIDTH){
            hor += (USE_SCREEN_WIDTH - str_width) / 2;  
        }
        epaper_print_str(hor, ver, font_size, color, str);
    }
}

void epaper_printf_centered(int ver, font_size_t font_size, color_t color, const char *format, ...)
{
    va_list args;
    va_start (args, format);
    vsnprintf (text_buf, sizeof(text_buf), format, args);
    va_end (args);
    epaper_print_centered_str(ver, font_size, color, text_buf);
}

void epaper_printf(int hor, int ver, font_size_t font_size, color_t color, const char *format, ...)
{
    va_list args;
    va_start (args, format);
    vsnprintf (text_buf, sizeof(text_buf), format, args);
    va_end (args);
    epaper_print_str(hor, ver, font_size, color, text_buf);
}

void epaper_refresh()
{
    epd.DisplayFrame();
}

bool epaper_update()
{
    if(memcmp(update_screen_indicator, screen, sizeof(screen)) != 0){
        epd.WaitUntilIdle();
        epd.DisplayBlack(screen);
        epd.DisplayRed(screen_red);
        epd.DisplayFrame();
        memcpy(update_screen_indicator, screen, sizeof(screen));
        epd.WaitUntilIdle();
        return true;
    }
    return false;
}

void epaper_clear()
{
    memset(screen_red, 0, sizeof(screen_red));
    memset(screen, 0xff, sizeof(screen));
}


void draw_rect(int hor_0, int ver_0, int hor_1, int ver_1, color_t color, int filled)
{
    if(filled){
        (color == RED ? paint_red : paint)->DrawFilledRectangle(hor_0, ver_0, hor_1, ver_1, color);
    } else {
        (color == RED ? paint_red : paint)->DrawRectangle(hor_0, ver_0, hor_1, ver_1, color);
    }
}

void draw_circle(int hor, int ver, int radius, color_t color, int filled)
{
    if(filled){
        (color == RED ? paint_red : paint)->DrawFilledCircle(hor, ver, radius, 1);
    } else {
        (color == RED ? paint_red : paint)->DrawCircle(hor, ver, radius, 1);
    }
}

void draw_line(int hor_0, int ver_0, int hor_1, int ver_1, color_t color)
{
    (color == RED ? paint_red : paint)->DrawLine(hor_0, ver_0, hor_1, ver_1, 1);
}

void draw_horizontal_line(int hor_0, int hor_1, int ver, int width, color_t color)
{
    (color == RED ? paint_red : paint)->DrawFilledRectangle(hor_0, ver, hor_1, ver+width, 1);
}

void draw_vertical_line(int ver_0, int ver_1, int hor, int width, color_t color)
{
    (color == RED ? paint_red : paint)->DrawFilledRectangle(hor, ver_0, hor+width, ver_1, color);
}


void epaper_set_rotate(int rotate)
{
    if(rotate >= 0 && rotate < 4){
        paint->SetRotate(rotate);       
    }
}


void epaper_display_part()
{
    epd.WaitUntilIdle();
    epd.DisplayBlack(screen);
}


void epaper_wait()
{
    epd.WaitUntilIdle();
}