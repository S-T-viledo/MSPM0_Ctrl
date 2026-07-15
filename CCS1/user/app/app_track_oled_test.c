#include "user/app/app_track_oled_test.h"

#include <stdbool.h>
#include <stdint.h>

#include "user/bsp/bsp_gpio.h"
#include "user/bsp/bsp_timer.h"
#include "user/bsp/bsp_uart.h"
#include "user/modules/oled.h"
#include "user/modules/track_sensor.h"

static bool g_oled_ok;
static uint32_t g_tick;

static void u16_to_4digits(uint16_t value, char out[5])
{
    if (value > 9999U) {
        value = 9999U;
    }

    out[0] = (char) ('0' + ((value / 1000U) % 10U));
    out[1] = (char) ('0' + ((value / 100U) % 10U));
    out[2] = (char) ('0' + ((value / 10U) % 10U));
    out[3] = (char) ('0' + (value % 10U));
    out[4] = '\0';
}

static void i16_to_string(int16_t value, char out[7])
{
    uint16_t mag;

    if (value == TRACK_SENSOR_POSITION_INVALID) {
        out[0] = 'I';
        out[1] = 'N';
        out[2] = 'V';
        out[3] = '\0';
        return;
    }

    if (value < 0) {
        out[0] = '-';
        mag = (uint16_t) (-value);
    } else {
        out[0] = '+';
        mag = (uint16_t) value;
    }

    u16_to_4digits(mag, &out[1]);
    out[5] = '\0';
}

static void make_mask_string(uint8_t mask, char out[7])
{
    uint8_t i;

    for (i = 0U; i < TRACK_SENSOR_USED_CHANNELS; i++) {
        out[i] = ((mask & (uint8_t) (1U << i)) != 0U) ? '1' : '0';
    }
    out[TRACK_SENSOR_USED_CHANNELS] = '\0';
}

static void make_raw_pair_line(char line[18], uint8_t left_index,
    uint16_t left_raw, uint8_t right_index, uint16_t right_raw)
{
    char left[5];
    char right[5];

    u16_to_4digits(left_raw, left);
    u16_to_4digits(right_raw, right);

    line[0] = (char) ('1' + left_index);
    line[1] = ' ';
    line[2] = left[0];
    line[3] = left[1];
    line[4] = left[2];
    line[5] = left[3];
    line[6] = ' ';
    line[7] = ' ';
    line[8] = (char) ('1' + right_index);
    line[9] = ' ';
    line[10] = right[0];
    line[11] = right[1];
    line[12] = right[2];
    line[13] = right[3];
    line[14] = '\0';
}

static uint8_t read_select_mask(void)
{
    return track_sensor_debug_read_select_mask();
}

static uint8_t run_select_self_test(void)
{
    uint8_t ch;
    uint8_t fail_mask = 0U;

    for (ch = 0U; ch < TRACK_SENSOR_TOTAL_CHANNELS; ch++) {
        track_sensor_debug_select_channel(ch);
        bsp_delay_us(10U);
        if (track_sensor_debug_read_select_mask() != ch) {
            fail_mask |= (uint8_t) (1U << ch);
        }
    }

    return fail_mask;
}

static void calc_min_max(const uint16_t raw[TRACK_SENSOR_TOTAL_CHANNELS],
    uint16_t *min_raw, uint16_t *max_raw)
{
    uint8_t i;

    *min_raw = raw[0];
    *max_raw = raw[0];

    for (i = 1U; i < TRACK_SENSOR_TOTAL_CHANNELS; i++) {
        if (raw[i] < *min_raw) {
            *min_raw = raw[i];
        }
        if (raw[i] > *max_raw) {
            *max_raw = raw[i];
        }
    }
}

static void draw_track_main_screen(const track_sensor_data_t *data)
{
    char line[18];
    char mask[7];
    char pos[7];
    char threshold[5];
    track_sensor_config_t config = track_sensor_get_config();

    oled_clear();

    if (!data->available) {
        oled_draw_string(0U, 0U, "TRACK NO ADC");
        oled_draw_string(0U, 12U, "CHECK SYSCFG");
        oled_draw_string(0U, 24U, "ADC0 PA14");
        oled_draw_string(0U, 36U, "S0 PA16");
        oled_draw_string(0U, 48U, "S1 PA2 S2 PA23");
        return;
    }

    make_mask_string(data->active_mask, mask);
    i16_to_string(data->position, pos);
    u16_to_4digits(config.threshold, threshold);

    oled_draw_string(0U, 0U, data->valid ? "TRACK 6CH OK" : "TRACK READ FAIL");

    line[0] = 'B';
    line[1] = ' ';
    line[2] = mask[0];
    line[3] = mask[1];
    line[4] = mask[2];
    line[5] = mask[3];
    line[6] = mask[4];
    line[7] = mask[5];
    line[8] = ' ';
    line[9] = 'C';
    line[10] = (char) ('0' + data->active_count);
    line[11] = '\0';
    oled_draw_string(0U, 10U, line);

    line[0] = 'P';
    line[1] = ' ';
    line[2] = pos[0];
    line[3] = pos[1];
    line[4] = pos[2];
    line[5] = pos[3];
    line[6] = pos[4];
    line[7] = ' ';
    line[8] = 'T';
    line[9] = threshold[0];
    line[10] = threshold[1];
    line[11] = threshold[2];
    line[12] = threshold[3];
    line[13] = '\0';
    oled_draw_string(0U, 20U, line);

    make_raw_pair_line(line, 0U, data->used_raw[0], 1U, data->used_raw[1]);
    oled_draw_string(0U, 30U, line);
    make_raw_pair_line(line, 2U, data->used_raw[2], 3U, data->used_raw[3]);
    oled_draw_string(0U, 40U, line);
    make_raw_pair_line(line, 4U, data->used_raw[4], 5U, data->used_raw[5]);
    oled_draw_string(0U, 50U, line);
}

static void draw_track_debug_screen(const track_sensor_data_t *data)
{
    char line[18];
    char min_s[5];
    char max_s[5];
    uint16_t min_raw;
    uint16_t max_raw;
    uint8_t sel = read_select_mask();
    uint8_t sel_fail = run_select_self_test();

    oled_clear();

    calc_min_max(data->raw, &min_raw, &max_raw);
    u16_to_4digits(min_raw, min_s);
    u16_to_4digits(max_raw, max_s);

    line[0] = 'S';
    line[1] = 'E';
    line[2] = 'L';
    line[3] = ' ';
    line[4] = (sel & 0x01U) ? '1' : '0';
    line[5] = (sel & 0x02U) ? '1' : '0';
    line[6] = (sel & 0x04U) ? '1' : '0';
    line[7] = ' ';
    line[8] = 'S';
    line[9] = 'T';
    line[10] = sel_fail ? 'F' : 'O';
    line[11] = sel_fail ? 'A' : 'K';
    line[12] = sel_fail ? 'I' : '\0';
    line[13] = sel_fail ? 'L' : '\0';
    line[14] = '\0';
    oled_draw_string(0U, 0U, line);

    line[0] = 'M';
    line[1] = 'N';
    line[2] = ' ';
    line[3] = min_s[0];
    line[4] = min_s[1];
    line[5] = min_s[2];
    line[6] = min_s[3];
    line[7] = ' ';
    line[8] = 'M';
    line[9] = 'X';
    line[10] = ' ';
    line[11] = max_s[0];
    line[12] = max_s[1];
    line[13] = max_s[2];
    line[14] = max_s[3];
    line[15] = '\0';
    oled_draw_string(0U, 10U, line);

    make_raw_pair_line(line, 0U, data->raw[0], 1U, data->raw[1]);
    oled_draw_string(0U, 20U, line);
    make_raw_pair_line(line, 2U, data->raw[2], 3U, data->raw[3]);
    oled_draw_string(0U, 30U, line);
    make_raw_pair_line(line, 4U, data->raw[4], 5U, data->raw[5]);
    oled_draw_string(0U, 40U, line);
    make_raw_pair_line(line, 6U, data->raw[6], 7U, data->raw[7]);
    oled_draw_string(0U, 50U, line);
}

static void draw_track_hold_screen(uint8_t channel, uint16_t raw)
{
    char line[18];
    char raw_s[5];
    char mv_s[5];
    uint32_t mv = ((uint32_t) raw * 3300UL) / 4095UL;
    uint8_t sel = read_select_mask();

    u16_to_4digits(raw, raw_s);
    u16_to_4digits((uint16_t) mv, mv_s);

    oled_clear();
    oled_draw_string(0U, 0U, "TRACK HOLD TEST");

    line[0] = 'C';
    line[1] = 'H';
    line[2] = (char) ('1' + channel);
    line[3] = ' ';
    line[4] = 'S';
    line[5] = (sel & 0x01U) ? '1' : '0';
    line[6] = (sel & 0x02U) ? '1' : '0';
    line[7] = (sel & 0x04U) ? '1' : '0';
    line[8] = '\0';
    oled_draw_string(0U, 12U, line);

    line[0] = 'R';
    line[1] = 'A';
    line[2] = 'W';
    line[3] = ' ';
    line[4] = raw_s[0];
    line[5] = raw_s[1];
    line[6] = raw_s[2];
    line[7] = raw_s[3];
    line[8] = '\0';
    oled_draw_string(0U, 24U, line);

    line[0] = 'M';
    line[1] = 'V';
    line[2] = ' ';
    line[3] = mv_s[0];
    line[4] = mv_s[1];
    line[5] = mv_s[2];
    line[6] = mv_s[3];
    line[7] = '\0';
    oled_draw_string(0U, 36U, line);

    oled_draw_string(0U, 50U, "MEASURE PA14");
}

void app_track_oled_test_init(void)
{
    track_sensor_config_t config = {
        2000U,
        false
    };

    bsp_uart0_write_string("\r\nMSPM0 track OLED test start\r\n");
    bsp_uart0_write_string("TRACK ADC=PA14 S0=PA16 S1=PA2 S2=PA23\r\n");

    track_sensor_init();
    track_sensor_set_config(&config);

    g_oled_ok = oled_init();
    if (g_oled_ok) {
        bsp_uart0_write_string("OLED init ok\r\n");
    } else {
        bsp_uart0_write_string("OLED init fail, UART debug only\r\n");
    }
}

void app_track_oled_test_task(void)
{
    uint8_t page = 0U; /* locked to main page */
    uint8_t hold_channel = 0U;
    uint16_t hold_raw = 0U;
    track_sensor_data_t data;

    if (page == 2U) {
        data.available = track_sensor_is_available();
        data.valid = track_sensor_debug_read_channel(hold_channel, &hold_raw);
        data.active_mask = 0U;
        data.active_count = 0U;
        data.position = TRACK_SENSOR_POSITION_INVALID;
    } else {
        data = track_sensor_read();
    }

    if (g_oled_ok) {
        if (page == 0U) {
            draw_track_main_screen(&data);
        } else if (page == 1U) {
            draw_track_debug_screen(&data);
        } else {
            draw_track_hold_screen(hold_channel, hold_raw);
        }
        g_oled_ok = oled_display();
    }

    if ((g_tick % 4UL) == 0UL) {
        if (page == 2U) {
            uart0_printf("track hold ch=%u sel=%u raw=%u mv=%u\r\n",
                (uint32_t) hold_channel + 1UL,
                read_select_mask(),
                hold_raw,
                ((uint32_t) hold_raw * 3300UL) / 4095UL);
        } else {
            uart0_printf("track av=%u valid=%u sel=%u mask=0x%02X count=%u pos=%d raw=%u,%u,%u,%u,%u,%u,%u,%u\r\n",
                data.available ? 1U : 0U,
                data.valid ? 1U : 0U,
                read_select_mask(),
                data.active_mask,
                data.active_count,
                data.position,
                data.raw[0],
                data.raw[1],
                data.raw[2],
                data.raw[3],
                data.raw[4],
                data.raw[5],
                data.raw[6],
                data.raw[7]);
        }
    }

    bsp_led1_toggle();
    g_tick++;
    bsp_delay_ms(250U);
}
