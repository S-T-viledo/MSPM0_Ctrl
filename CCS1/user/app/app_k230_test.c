#include "user/app/app_k230_test.h"

#include "user/bsp/bsp_gpio.h"
#include "user/bsp/bsp_timer.h"
#include "user/bsp/bsp_uart.h"
#include "user/modules/k230.h"

#define APP_K230_POLL_PERIOD_MS         (10U)
#define APP_K230_SPEEDUP_PERIOD_MS      (2000U)
#define APP_K230_START_HALF_PERIOD_MS   (500U)
#define APP_K230_MIN_HALF_PERIOD_MS     (50U)

void app_k230_test_init(void)
{
    k230_init();
    bsp_led1_off();
    (void) k230_send_text_frame("MSPM0 K230 READY");
}

void app_k230_test_task(void)
{
    static bool rx_seen = false;
    static uint32_t last_rx_bytes = 0UL;
    static uint32_t half_period_ms = APP_K230_START_HALF_PERIOD_MS;
    static uint32_t blink_elapsed_ms = 0UL;
    static uint32_t speedup_elapsed_ms = 0UL;
    k230_status_t status;
    const k230_frame_t *frame;

    k230_poll();
    status = k230_get_status();

    if (status.rx_bytes != last_rx_bytes) {
        last_rx_bytes = status.rx_bytes;
        rx_seen = true;
    }

    if (k230_has_frame()) {
        frame = k230_get_frame();
        (void) k230_send_ack(frame->command);
        k230_clear_frame();
    }

    if (k230_has_line()) {
        k230_clear_line();
    }

    if (rx_seen) {
        blink_elapsed_ms += APP_K230_POLL_PERIOD_MS;
        speedup_elapsed_ms += APP_K230_POLL_PERIOD_MS;

        if (blink_elapsed_ms >= half_period_ms) {
            bsp_led1_toggle();
            blink_elapsed_ms = 0U;
        }

        if (speedup_elapsed_ms >= APP_K230_SPEEDUP_PERIOD_MS) {
            if (half_period_ms > APP_K230_MIN_HALF_PERIOD_MS) {
                half_period_ms /= 2U;
                if (half_period_ms < APP_K230_MIN_HALF_PERIOD_MS) {
                    half_period_ms = APP_K230_MIN_HALF_PERIOD_MS;
                }
            }
            speedup_elapsed_ms = 0U;
        }
    } else {
        bsp_led1_off();
    }

    bsp_delay_ms(APP_K230_POLL_PERIOD_MS);
}
