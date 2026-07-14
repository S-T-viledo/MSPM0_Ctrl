#include "user/modules/oled.h"

#include <stddef.h>
#include <stdint.h>

#include "user/bsp/bsp_i2c.h"
#include "user/bsp/bsp_timer.h"

#define OLED_PAGES                 (OLED_HEIGHT / 8U)
#define OLED_BUFFER_SIZE           (OLED_WIDTH * OLED_PAGES)
#define OLED_CONTROL_CMD           (0x00U)
#define OLED_CONTROL_DATA          (0x40U)
#define OLED_DATA_CHUNK            (16U)

static uint8_t g_oled_buffer[OLED_BUFFER_SIZE];
static bool g_oled_ready;
static uint8_t g_oled_addr = OLED_DEFAULT_ADDR;

static bool oled_write_cmds(const uint8_t *cmds, uint8_t count)
{
    uint8_t packet[17];
    uint8_t i;

    if ((cmds == NULL) || (count == 0U) || (count > 16U)) {
        return false;
    }

    packet[0] = OLED_CONTROL_CMD;
    for (i = 0U; i < count; i++) {
        packet[i + 1U] = cmds[i];
    }

    return bsp_i2c_oled_write(g_oled_addr, packet, (size_t) count + 1U);
}

static bool oled_write_data(const uint8_t *data, uint8_t count)
{
    uint8_t packet[OLED_DATA_CHUNK + 1U];
    uint8_t i;

    if ((data == NULL) || (count == 0U) || (count > OLED_DATA_CHUNK)) {
        return false;
    }

    packet[0] = OLED_CONTROL_DATA;
    for (i = 0U; i < count; i++) {
        packet[i + 1U] = data[i];
    }

    return bsp_i2c_oled_write(g_oled_addr, packet, (size_t) count + 1U);
}

static void oled_get_glyph(char ch, uint8_t glyph[5])
{
    uint8_t i;

    for (i = 0U; i < 5U; i++) {
        glyph[i] = 0x00U;
    }

    if ((ch >= 'a') && (ch <= 'z')) {
        ch = (char) (ch - ('a' - 'A'));
    }

    switch (ch) {
        case '0': glyph[0]=0x3E; glyph[1]=0x51; glyph[2]=0x49; glyph[3]=0x45; glyph[4]=0x3E; break;
        case '1': glyph[0]=0x00; glyph[1]=0x42; glyph[2]=0x7F; glyph[3]=0x40; glyph[4]=0x00; break;
        case '2': glyph[0]=0x42; glyph[1]=0x61; glyph[2]=0x51; glyph[3]=0x49; glyph[4]=0x46; break;
        case '3': glyph[0]=0x21; glyph[1]=0x41; glyph[2]=0x45; glyph[3]=0x4B; glyph[4]=0x31; break;
        case '4': glyph[0]=0x18; glyph[1]=0x14; glyph[2]=0x12; glyph[3]=0x7F; glyph[4]=0x10; break;
        case '5': glyph[0]=0x27; glyph[1]=0x45; glyph[2]=0x45; glyph[3]=0x45; glyph[4]=0x39; break;
        case '6': glyph[0]=0x3C; glyph[1]=0x4A; glyph[2]=0x49; glyph[3]=0x49; glyph[4]=0x30; break;
        case '7': glyph[0]=0x01; glyph[1]=0x71; glyph[2]=0x09; glyph[3]=0x05; glyph[4]=0x03; break;
        case '8': glyph[0]=0x36; glyph[1]=0x49; glyph[2]=0x49; glyph[3]=0x49; glyph[4]=0x36; break;
        case '9': glyph[0]=0x06; glyph[1]=0x49; glyph[2]=0x49; glyph[3]=0x29; glyph[4]=0x1E; break;
        case 'A': glyph[0]=0x7E; glyph[1]=0x11; glyph[2]=0x11; glyph[3]=0x11; glyph[4]=0x7E; break;
        case 'B': glyph[0]=0x7F; glyph[1]=0x49; glyph[2]=0x49; glyph[3]=0x49; glyph[4]=0x36; break;
        case 'C': glyph[0]=0x3E; glyph[1]=0x41; glyph[2]=0x41; glyph[3]=0x41; glyph[4]=0x22; break;
        case 'D': glyph[0]=0x7F; glyph[1]=0x41; glyph[2]=0x41; glyph[3]=0x22; glyph[4]=0x1C; break;
        case 'E': glyph[0]=0x7F; glyph[1]=0x49; glyph[2]=0x49; glyph[3]=0x49; glyph[4]=0x41; break;
        case 'F': glyph[0]=0x7F; glyph[1]=0x09; glyph[2]=0x09; glyph[3]=0x09; glyph[4]=0x01; break;
        case 'G': glyph[0]=0x3E; glyph[1]=0x41; glyph[2]=0x49; glyph[3]=0x49; glyph[4]=0x7A; break;
        case 'H': glyph[0]=0x7F; glyph[1]=0x08; glyph[2]=0x08; glyph[3]=0x08; glyph[4]=0x7F; break;
        case 'I': glyph[0]=0x00; glyph[1]=0x41; glyph[2]=0x7F; glyph[3]=0x41; glyph[4]=0x00; break;
        case 'J': glyph[0]=0x20; glyph[1]=0x40; glyph[2]=0x41; glyph[3]=0x3F; glyph[4]=0x01; break;
        case 'K': glyph[0]=0x7F; glyph[1]=0x08; glyph[2]=0x14; glyph[3]=0x22; glyph[4]=0x41; break;
        case 'L': glyph[0]=0x7F; glyph[1]=0x40; glyph[2]=0x40; glyph[3]=0x40; glyph[4]=0x40; break;
        case 'M': glyph[0]=0x7F; glyph[1]=0x02; glyph[2]=0x0C; glyph[3]=0x02; glyph[4]=0x7F; break;
        case 'N': glyph[0]=0x7F; glyph[1]=0x04; glyph[2]=0x08; glyph[3]=0x10; glyph[4]=0x7F; break;
        case 'O': glyph[0]=0x3E; glyph[1]=0x41; glyph[2]=0x41; glyph[3]=0x41; glyph[4]=0x3E; break;
        case 'P': glyph[0]=0x7F; glyph[1]=0x09; glyph[2]=0x09; glyph[3]=0x09; glyph[4]=0x06; break;
        case 'Q': glyph[0]=0x3E; glyph[1]=0x41; glyph[2]=0x51; glyph[3]=0x21; glyph[4]=0x5E; break;
        case 'R': glyph[0]=0x7F; glyph[1]=0x09; glyph[2]=0x19; glyph[3]=0x29; glyph[4]=0x46; break;
        case 'S': glyph[0]=0x46; glyph[1]=0x49; glyph[2]=0x49; glyph[3]=0x49; glyph[4]=0x31; break;
        case 'T': glyph[0]=0x01; glyph[1]=0x01; glyph[2]=0x7F; glyph[3]=0x01; glyph[4]=0x01; break;
        case 'U': glyph[0]=0x3F; glyph[1]=0x40; glyph[2]=0x40; glyph[3]=0x40; glyph[4]=0x3F; break;
        case 'V': glyph[0]=0x1F; glyph[1]=0x20; glyph[2]=0x40; glyph[3]=0x20; glyph[4]=0x1F; break;
        case 'W': glyph[0]=0x3F; glyph[1]=0x40; glyph[2]=0x38; glyph[3]=0x40; glyph[4]=0x3F; break;
        case 'X': glyph[0]=0x63; glyph[1]=0x14; glyph[2]=0x08; glyph[3]=0x14; glyph[4]=0x63; break;
        case 'Y': glyph[0]=0x07; glyph[1]=0x08; glyph[2]=0x70; glyph[3]=0x08; glyph[4]=0x07; break;
        case 'Z': glyph[0]=0x61; glyph[1]=0x51; glyph[2]=0x49; glyph[3]=0x45; glyph[4]=0x43; break;
        case '-': glyph[0]=0x08; glyph[1]=0x08; glyph[2]=0x08; glyph[3]=0x08; glyph[4]=0x08; break;
        case '_': glyph[0]=0x40; glyph[1]=0x40; glyph[2]=0x40; glyph[3]=0x40; glyph[4]=0x40; break;
        case '.': glyph[0]=0x00; glyph[1]=0x60; glyph[2]=0x60; glyph[3]=0x00; glyph[4]=0x00; break;
        case ':': glyph[0]=0x00; glyph[1]=0x36; glyph[2]=0x36; glyph[3]=0x00; glyph[4]=0x00; break;
        case ' ': break;
        default: glyph[0]=0x7F; glyph[1]=0x41; glyph[2]=0x5D; glyph[3]=0x41; glyph[4]=0x7F; break;
    }
}

bool oled_init(void)
{
    static const uint8_t addrs[] = {0x3CU, 0x3DU};
    static const uint8_t init1[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12
    };
    static const uint8_t init2[] = {
        0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6,
        0x2E, 0xAF
    };
    uint8_t i;

    g_oled_ready = false;

    if (!bsp_i2c_oled_is_available()) {
        return false;
    }

    bsp_delay_ms(50U);

    for (i = 0U; i < (uint8_t) (sizeof(addrs) / sizeof(addrs[0])); i++) {
        g_oled_addr = addrs[i];
        if (oled_write_cmds(init1, (uint8_t) sizeof(init1)) &&
            oled_write_cmds(init2, (uint8_t) sizeof(init2))) {
            oled_clear();
            g_oled_ready = oled_display();
            return g_oled_ready;
        }
    }

    g_oled_addr = OLED_DEFAULT_ADDR;
    return false;
}

bool oled_is_ready(void)
{
    return g_oled_ready;
}

uint8_t oled_get_address(void)
{
    return g_oled_addr;
}

bool oled_display(void)
{
    static const uint8_t addr_cmds[] = {0x21, 0x00, 0x7F, 0x22, 0x00, 0x07};
    uint16_t offset = 0U;

    if (!oled_write_cmds(addr_cmds, (uint8_t) sizeof(addr_cmds))) {
        return false;
    }

    while (offset < OLED_BUFFER_SIZE) {
        if (!oled_write_data(&g_oled_buffer[offset], OLED_DATA_CHUNK)) {
            return false;
        }
        offset += OLED_DATA_CHUNK;
    }

    return true;
}

void oled_clear(void)
{
    oled_fill(false);
}

void oled_fill(bool on)
{
    uint16_t i;
    uint8_t value = on ? 0xFFU : 0x00U;

    for (i = 0U; i < OLED_BUFFER_SIZE; i++) {
        g_oled_buffer[i] = value;
    }
}

void oled_set_pixel(uint8_t x, uint8_t y, bool on)
{
    uint16_t index;
    uint8_t mask;

    if ((x >= OLED_WIDTH) || (y >= OLED_HEIGHT)) {
        return;
    }

    index = (uint16_t) x + ((uint16_t) (y / 8U) * OLED_WIDTH);
    mask = (uint8_t) (1U << (y & 0x07U));

    if (on) {
        g_oled_buffer[index] |= mask;
    } else {
        g_oled_buffer[index] &= (uint8_t) ~mask;
    }
}

void oled_draw_char(uint8_t x, uint8_t y, char ch)
{
    uint8_t glyph[5];
    uint8_t col;
    uint8_t row;

    oled_get_glyph(ch, glyph);

    for (col = 0U; col < 5U; col++) {
        for (row = 0U; row < 7U; row++) {
            if ((glyph[col] & (1U << row)) != 0U) {
                oled_set_pixel((uint8_t) (x + col), (uint8_t) (y + row), true);
            }
        }
    }
}

void oled_draw_string(uint8_t x, uint8_t y, const char *str)
{
    uint8_t cursor = x;

    if (str == NULL) {
        return;
    }

    while ((*str != '\0') && (cursor < (OLED_WIDTH - 5U))) {
        oled_draw_char(cursor, y, *str);
        cursor = (uint8_t) (cursor + 6U);
        str++;
    }
}

void oled_draw_hline(uint8_t x, uint8_t y, uint8_t width, bool on)
{
    uint8_t i;

    for (i = 0U; i < width; i++) {
        oled_set_pixel((uint8_t) (x + i), y, on);
    }
}

void oled_draw_vline(uint8_t x, uint8_t y, uint8_t height, bool on)
{
    uint8_t i;

    for (i = 0U; i < height; i++) {
        oled_set_pixel(x, (uint8_t) (y + i), on);
    }
}

void oled_draw_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool on)
{
    if ((width == 0U) || (height == 0U)) {
        return;
    }

    oled_draw_hline(x, y, width, on);
    oled_draw_hline(x, (uint8_t) (y + height - 1U), width, on);
    oled_draw_vline(x, y, height, on);
    oled_draw_vline((uint8_t) (x + width - 1U), y, height, on);
}
