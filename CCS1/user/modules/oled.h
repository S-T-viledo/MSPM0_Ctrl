#ifndef USER_MODULES_OLED_H_
#define USER_MODULES_OLED_H_

#include <stdbool.h>
#include <stdint.h>

#define OLED_WIDTH                 (128U)
#define OLED_HEIGHT                (64U)
#define OLED_DEFAULT_ADDR          (0x3CU)

bool oled_init(void);
bool oled_is_ready(void);
uint8_t oled_get_address(void);
bool oled_display(void);
void oled_clear(void);
void oled_fill(bool on);
void oled_set_pixel(uint8_t x, uint8_t y, bool on);
void oled_draw_char(uint8_t x, uint8_t y, char ch);
void oled_draw_string(uint8_t x, uint8_t y, const char *str);
void oled_draw_hline(uint8_t x, uint8_t y, uint8_t width, bool on);
void oled_draw_vline(uint8_t x, uint8_t y, uint8_t height, bool on);
void oled_draw_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool on);

#endif /* USER_MODULES_OLED_H_ */
