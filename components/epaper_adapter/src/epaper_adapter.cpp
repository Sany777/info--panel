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

static char text_buf[150];
static const int USE_SCREEN_WIDTH = SCREEN_WIDTH-10;
static sFONT *font_list[] = {&Font12, &Font16, &Font20, &Font24, &Font34, &Font48, &Font64};



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
        paint_red->DrawStringAt(hor, ver, str, font, color == RED); 
        if(color != RED){
            paint->DrawStringAt(hor, ver, str, font, color);  
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
        epaper_print_str(hor, ver, font_size, color, text_buf);
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

void epaper_update()
{
    epd.WaitUntilIdle();
    epd.DisplayBlack(screen);
    epd.WaitUntilIdle();
    epd.DisplayRed(screen_red);
    epd.update();
}

void epaper_clear()
{
    memset(screen_red, 0, sizeof(screen_red));
    memset(screen, 0xff, sizeof(screen));
}


void draw_rect(int hor_0, int ver_0, int hor_1, int ver_1, int color, int filled)
{
    if(filled){
        paint->DrawFilledRectangle(hor_0, ver_0, hor_1, ver_1, color);
    } else {
        paint->DrawRectangle(hor_0, ver_0, hor_1, ver_1, color);
    }
}

void draw_circle(int hor, int ver, int radius, int color, int filled)
{
    if(filled){
        paint_red->DrawFilledCircle(hor, ver, radius, color == RED);
        if(color != RED){
            paint->DrawFilledCircle(hor, ver, radius, color);
        }
    } else {
        paint_red->DrawCircle(hor, ver, radius, color == RED);
        if(color != RED){
            paint->DrawCircle(hor, ver, radius, color);
        }
    }
}

void draw_line(int hor_0, int ver_0, int hor_1, int ver_1, int color)
{
    if(color != RED){
        paint->DrawLine(hor_0, ver_0, hor_1, ver_1, color);
    }
    paint_red->DrawLine(hor_0, ver_0, hor_1, ver_1, color == RED);
}

void draw_horizontal_line(int hor_0, int hor_1, int ver, int width, int color)
{
    if(color != RED){
        paint->DrawFilledRectangle(hor_0, ver, hor_1, ver+width, color);
    }
    paint_red->DrawFilledRectangle(hor_0, ver, hor_1, ver+width, color == RED);
}

void draw_vertical_line(int ver_0, int ver_1, int hor, int width, int color)
{
    paint->DrawFilledRectangle(hor, ver_0, hor+width, ver_1, color);
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