#include "user/app/app_motor_test.h"

#include <stdbool.h>
#include <stdint.h>

#include "user/bsp/bsp_gpio.h"
#include "user/bsp/bsp_timer.h"
#include "user/bsp/bsp_uart.h"
#include "user/modules/oled.h"
#include "user/modules/car_motor.h"

#define MTR_OLED_DIV      (4U)
#define MTR_STEP_TICKS     (400U)  /* 4 seconds per step */

static bool     g_oled_ok;
static uint32_t g_tick;

static const char *g_step_names[] = {
    "STOP",
    "A FWD 50%",
    "A REV 50%",
    "B FWD 50%",
    "B REV 50%",
    "BOTH FWD 50%",
    "DONE"
};
#define MTR_NUM_STEPS  (7U)

static void mtr_set_step(uint32_t step)
{
    car_motor_stop_all();

    switch (step) {
    case 0: /* STOP */
        break;
    case 1: /* A forward */
        car_motor_run(CAR_MOTOR_A, CAR_MOTOR_FORWARD, 50U);
        break;
    case 2: /* A reverse */
        car_motor_run(CAR_MOTOR_A, CAR_MOTOR_REVERSE, 50U);
        break;
    case 3: /* B forward */
        car_motor_run(CAR_MOTOR_B, CAR_MOTOR_FORWARD, 50U);
        break;
    case 4: /* B reverse */
        car_motor_run(CAR_MOTOR_B, CAR_MOTOR_REVERSE, 50U);
        break;
    case 5: /* both forward */
        car_motor_run_both(CAR_MOTOR_FORWARD, CAR_MOTOR_FORWARD, 50U);
        break;
    case 6: /* DONE */
    default:
        break;
    }
}

void app_motor_test_init(void)
{
    bsp_uart0_write_string("\r\nMotor test\r\n");

    g_oled_ok = oled_init();
    car_motor_init();

    if (car_motor_is_available()) {
        bsp_uart0_write_string("Motor ready\r\n");
    } else {
        bsp_uart0_write_string("Motor NOT available\r\n");
    }
}

void app_motor_test_task(void)
{
    uint32_t step = (g_tick / MTR_STEP_TICKS) % MTR_NUM_STEPS;

    if ((g_tick % MTR_STEP_TICKS) == 0U) {
        mtr_set_step(step);
    }

    if (g_oled_ok && ((g_tick % MTR_OLED_DIV) == 0U)) {
        oled_clear();

        if (car_motor_is_available()) {
            oled_draw_string(0U, 0U,  "MOTOR TEST");
            oled_draw_string(0U, 16U, g_step_names[step]);
        } else {
            oled_draw_string(0U, 0U,  "MOTOR NOT AVAIL");
            oled_draw_string(0U, 16U, "CHECK SYSCFG");
        }

        g_oled_ok = oled_display();
    }

    bsp_led1_toggle();
    g_tick++;
    bsp_delay_ms(10U);
}
