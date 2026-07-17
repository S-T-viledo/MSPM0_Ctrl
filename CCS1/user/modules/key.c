#include "user/modules/key.h"

#include "ti_msp_dl_config.h"

#define KEY_DEBOUNCE_TICKS         (640000UL)
#define KEY_SYSTICK_RELOAD         (0x00FFFFFFUL)
#define KEY_1_MASK                 (1U << 0U)
#define KEY_2_MASK                 (1U << 1U)
#define KEY_3_MASK                 (1U << 2U)
#define KEY_4_MASK                 (1U << 3U)

static uint8_t g_last_raw;
static uint8_t g_stable;
static uint32_t g_change_tick;

static uint8_t key_read_raw(void)
{
    uint32_t pins = DL_GPIO_readPins(GPIO_GRP_KEY_PORT,
        GPIO_GRP_KEY_KEY1_PIN | GPIO_GRP_KEY_KEY2_PIN |
        GPIO_GRP_KEY_KEY3_PIN | GPIO_GRP_KEY_KEY4_PIN);
    uint8_t pressed = 0U;

    /* Keys are wired active-low and use the SysConfig pull-ups. */
    if ((pins & GPIO_GRP_KEY_KEY1_PIN) == 0U) {
        pressed |= KEY_1_MASK;
    }
    if ((pins & GPIO_GRP_KEY_KEY2_PIN) == 0U) {
        pressed |= KEY_2_MASK;
    }
    if ((pins & GPIO_GRP_KEY_KEY3_PIN) == 0U) {
        pressed |= KEY_3_MASK;
    }
    if ((pins & GPIO_GRP_KEY_KEY4_PIN) == 0U) {
        pressed |= KEY_4_MASK;
    }

    return pressed;
}

void key_init(void)
{
    g_last_raw = key_read_raw();
    g_stable = g_last_raw;
    g_change_tick = SysTick->VAL;
}

bool key_is_pressed(void)
{
    return (key_read_raw() != 0U);
}

key_event_t key_get_event(void)
{
    uint8_t raw = key_read_raw();
    uint8_t new_presses;

    if (raw != g_last_raw) {
        g_last_raw = raw;
        g_change_tick = SysTick->VAL;
        return KEY_EVENT_NONE;
    }

    if (raw == g_stable) {
        return KEY_EVENT_NONE;
    }

    if (((g_change_tick - SysTick->VAL) & KEY_SYSTICK_RELOAD) <
        KEY_DEBOUNCE_TICKS) {
        return KEY_EVENT_NONE;
    }

    new_presses = (uint8_t) (raw & (uint8_t) ~g_stable);
    g_stable = raw;

    if ((new_presses & KEY_1_MASK) != 0U) {
        return KEY_EVENT_1;
    }
    if ((new_presses & KEY_2_MASK) != 0U) {
        return KEY_EVENT_2;
    }
    if ((new_presses & KEY_3_MASK) != 0U) {
        return KEY_EVENT_3;
    }
    if ((new_presses & KEY_4_MASK) != 0U) {
        return KEY_EVENT_4;
    }

    return KEY_EVENT_NONE;
}
