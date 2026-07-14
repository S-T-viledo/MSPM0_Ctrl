#ifndef USER_BOARD_BOARD_CONFIG_H_
#define USER_BOARD_BOARD_CONFIG_H_

#include "ti_msp_dl_config.h"

/*
 * Board logical resources.
 *
 * Keep module code independent from concrete pins/peripherals. When changing
 * boards or SysConfig names, update this file first.
 */

#if defined(GPIO_LEDS_PORT) && defined(GPIO_LEDS_USER_LED_1_PIN)
#define BOARD_HAS_LED1             (1)
#define BOARD_LED1_PORT            GPIO_LEDS_PORT
#define BOARD_LED1_PIN             GPIO_LEDS_USER_LED_1_PIN
#define BOARD_LED1_ACTIVE_HIGH     (1)
#elif defined(GPIO_GRP_LED_PORT) && defined(GPIO_GRP_LED_LED1_PIN)
#define BOARD_HAS_LED1             (1)
#define BOARD_LED1_PORT            GPIO_GRP_LED_PORT
#define BOARD_LED1_PIN             GPIO_GRP_LED_LED1_PIN
#define BOARD_LED1_ACTIVE_HIGH     (1)
#elif defined(GPIO_LED_PORT) && defined(GPIO_LED_PIN)
#define BOARD_HAS_LED1             (1)
#define BOARD_LED1_PORT            GPIO_LED_PORT
#define BOARD_LED1_PIN             GPIO_LED_PIN
#define BOARD_LED1_ACTIVE_HIGH     (1)
#elif defined(GPIO_GRP_0_PORT) && defined(GPIO_GRP_0_PIN_1_PIN)
#define BOARD_HAS_LED1             (1)
#define BOARD_LED1_PORT            GPIO_GRP_0_PORT
#define BOARD_LED1_PIN             GPIO_GRP_0_PIN_1_PIN
#define BOARD_LED1_ACTIVE_HIGH     (1)
#else
#define BOARD_HAS_LED1             (0)
#endif

#if defined(GPIO_SWITCHES_PORT) && defined(GPIO_SWITCHES_USER_SWITCH_1_PIN)
#define BOARD_HAS_KEY1             (1)
#define BOARD_KEY1_PORT            GPIO_SWITCHES_PORT
#define BOARD_KEY1_PIN             GPIO_SWITCHES_USER_SWITCH_1_PIN
#define BOARD_KEY1_ACTIVE_LOW      (1)
#elif defined(GPIO_KEY_PORT) && defined(GPIO_KEY_PIN)
#define BOARD_HAS_KEY1             (1)
#define BOARD_KEY1_PORT            GPIO_KEY_PORT
#define BOARD_KEY1_PIN             GPIO_KEY_PIN
#define BOARD_KEY1_ACTIVE_LOW      (1)
#elif defined(GPIO_GRP_0_PORT) && defined(GPIO_GRP_0_PIN_0_PIN)
#define BOARD_HAS_KEY1             (1)
#define BOARD_KEY1_PORT            GPIO_GRP_0_PORT
#define BOARD_KEY1_PIN             GPIO_GRP_0_PIN_0_PIN
#define BOARD_KEY1_ACTIVE_LOW      (0)
#else
#define BOARD_HAS_KEY1             (0)
#endif

#if defined(UART_Bluetooth_INST)
#define BOARD_HAS_DEBUG_UART       (1)
#define BOARD_DEBUG_UART_INST      UART_Bluetooth_INST
#elif defined(UART_0_INST)
#define BOARD_HAS_DEBUG_UART       (1)
#define BOARD_DEBUG_UART_INST      UART_0_INST
#elif defined(UART0_INST)
#define BOARD_HAS_DEBUG_UART       (1)
#define BOARD_DEBUG_UART_INST      UART0_INST
#else
#define BOARD_HAS_DEBUG_UART       (0)
#endif

#define BOARD_HAS_UART0            BOARD_HAS_DEBUG_UART
#if BOARD_HAS_DEBUG_UART
#define BOARD_UART0_INST           BOARD_DEBUG_UART_INST
#endif

#if defined(UART_K230_INST)
#define BOARD_HAS_K230_UART        (1)
#define BOARD_K230_UART_INST       UART_K230_INST
#else
#define BOARD_HAS_K230_UART        (0)
#endif

#if defined(I2C_OLED_INST)
#define BOARD_HAS_I2C_OLED         (1)
#define BOARD_I2C_OLED_INST        I2C_OLED_INST
#if defined(GPIO_I2C_OLED_SCL_PIN)
#define BOARD_I2C_OLED_SCL_PIN     GPIO_I2C_OLED_SCL_PIN
#endif
#if defined(GPIO_I2C_OLED_SDA_PIN)
#define BOARD_I2C_OLED_SDA_PIN     GPIO_I2C_OLED_SDA_PIN
#endif
#else
#define BOARD_HAS_I2C_OLED         (0)
#endif

#if defined(PWM_MOTOR_INST) && defined(GPIO_PWM_MOTOR_C0_IDX) && \
    defined(GPIO_PWM_MOTOR_C1_IDX) && \
    defined(GPIO_MOTOR_DIRECTION_PIN_AIN1_PORT) && \
    defined(GPIO_MOTOR_DIRECTION_PIN_AIN1_PIN) && \
    defined(GPIO_MOTOR_DIRECTION_PIN_AIN2_PORT) && \
    defined(GPIO_MOTOR_DIRECTION_PIN_AIN2_PIN) && \
    defined(GPIO_MOTOR_DIRECTION_PIN_BIN1_PORT) && \
    defined(GPIO_MOTOR_DIRECTION_PIN_BIN1_PIN) && \
    defined(GPIO_MOTOR_DIRECTION_PIN_BIN2_PORT) && \
    defined(GPIO_MOTOR_DIRECTION_PIN_BIN2_PIN)
#define BOARD_HAS_CAR_MOTOR        (1)
#define BOARD_MOTOR_PWM_INST       PWM_MOTOR_INST
#define BOARD_MOTOR_PWMA_CC_INDEX  GPIO_PWM_MOTOR_C0_IDX
#define BOARD_MOTOR_PWMB_CC_INDEX  GPIO_PWM_MOTOR_C1_IDX
#define BOARD_MOTOR_AIN1_PORT      GPIO_MOTOR_DIRECTION_PIN_AIN1_PORT
#define BOARD_MOTOR_AIN1_PIN       GPIO_MOTOR_DIRECTION_PIN_AIN1_PIN
#define BOARD_MOTOR_AIN2_PORT      GPIO_MOTOR_DIRECTION_PIN_AIN2_PORT
#define BOARD_MOTOR_AIN2_PIN       GPIO_MOTOR_DIRECTION_PIN_AIN2_PIN
#define BOARD_MOTOR_BIN1_PORT      GPIO_MOTOR_DIRECTION_PIN_BIN1_PORT
#define BOARD_MOTOR_BIN1_PIN       GPIO_MOTOR_DIRECTION_PIN_BIN1_PIN
#define BOARD_MOTOR_BIN2_PORT      GPIO_MOTOR_DIRECTION_PIN_BIN2_PORT
#define BOARD_MOTOR_BIN2_PIN       GPIO_MOTOR_DIRECTION_PIN_BIN2_PIN
#else
#define BOARD_HAS_CAR_MOTOR        (0)
#endif

#if defined(UART_BLDC_INST)
#define BOARD_HAS_F32C_GIMBAL      (1)
#define BOARD_F32C_UART_INST       UART_BLDC_INST
#define BOARD_F32C_UART_IRQN       UART_BLDC_INST_INT_IRQN
#elif defined(UART_BLDC_X_INST)
#define BOARD_HAS_F32C_GIMBAL      (1)
#define BOARD_F32C_UART_INST       UART_BLDC_X_INST
#define BOARD_F32C_UART_IRQN       UART_BLDC_X_INST_INT_IRQN
#else
#define BOARD_HAS_F32C_GIMBAL      (0)
#endif

#if defined(ADC12_0_INST) && defined(ADC12_0_ADCMEM_0)
#define BOARD_HAS_LIGHT_AO         (1)
#define BOARD_LIGHT_ADC_INST       ADC12_0_INST
#define BOARD_LIGHT_ADC_MEM        ADC12_0_ADCMEM_0
#define BOARD_LIGHT_ADC_MAX        (4095U)
#define BOARD_LIGHT_ADC_CHANNEL    { BOARD_LIGHT_ADC_INST, BOARD_LIGHT_ADC_MEM, BOARD_LIGHT_ADC_MAX }
#else
#define BOARD_HAS_LIGHT_AO         (0)
#define BOARD_LIGHT_ADC_MAX        (4095U)
#endif

#if defined(TRACK_INST) && defined(TRACK_ADCMEM_0)
#define BOARD_HAS_TRACK_ADC        (1)
#define BOARD_TRACK_ADC_INST       TRACK_INST
#define BOARD_TRACK_ADC_MEM        TRACK_ADCMEM_0
#define BOARD_TRACK_ADC_MAX        (4095U)
#define BOARD_TRACK_ADC_CHANNEL    { BOARD_TRACK_ADC_INST, BOARD_TRACK_ADC_MEM, BOARD_TRACK_ADC_MAX }
#else
#define BOARD_HAS_TRACK_ADC        (0)
#define BOARD_TRACK_ADC_MAX        (4095U)
#endif

#if defined(GPIO_GRP_TRACK_PORT) && defined(GPIO_GRP_TRACK_S0_PIN) && \
    defined(GPIO_GRP_TRACK_S1_PIN) && defined(GPIO_GRP_TRACK_S2_PIN)
#define BOARD_HAS_TRACK_SELECT     (1)
#define BOARD_TRACK_S0_PORT        GPIO_GRP_TRACK_PORT
#define BOARD_TRACK_S0_PIN         GPIO_GRP_TRACK_S0_PIN
#define BOARD_TRACK_S1_PORT        GPIO_GRP_TRACK_PORT
#define BOARD_TRACK_S1_PIN         GPIO_GRP_TRACK_S1_PIN
#define BOARD_TRACK_S2_PORT        GPIO_GRP_TRACK_PORT
#define BOARD_TRACK_S2_PIN         GPIO_GRP_TRACK_S2_PIN
#else
#define BOARD_HAS_TRACK_SELECT     (0)
#endif

#if defined(GPIO_LIGHT_DO_PORT) && defined(GPIO_LIGHT_DO_PIN)
#define BOARD_HAS_LIGHT_DO         (1)
#define BOARD_LIGHT_DO_PORT        GPIO_LIGHT_DO_PORT
#define BOARD_LIGHT_DO_PIN         GPIO_LIGHT_DO_PIN
#define BOARD_LIGHT_DO_ACTIVE_LOW  (0)
#elif defined(GPIO_LIGHT_PORT) && defined(GPIO_LIGHT_DO_PIN)
#define BOARD_HAS_LIGHT_DO         (1)
#define BOARD_LIGHT_DO_PORT        GPIO_LIGHT_PORT
#define BOARD_LIGHT_DO_PIN         GPIO_LIGHT_DO_PIN
#define BOARD_LIGHT_DO_ACTIVE_LOW  (0)
#else
#define BOARD_HAS_LIGHT_DO         (0)
#endif

#if defined(SPI_0_INST)
#define BOARD_HAS_SPI0             (1)
#define BOARD_SPI0_INST            SPI_0_INST
#elif defined(SPI0_INST)
#define BOARD_HAS_SPI0             (1)
#define BOARD_SPI0_INST            SPI0_INST
#else
#define BOARD_HAS_SPI0             (0)
#endif

#if defined(GPIO_SPI_0_SCLK_PORT) && defined(GPIO_SPI_0_SCLK_PIN) && \
    defined(GPIO_SPI_0_IOMUX_SCLK) && defined(GPIO_SPI_0_PICO_PORT) && \
    defined(GPIO_SPI_0_PICO_PIN) && defined(GPIO_SPI_0_IOMUX_PICO) && \
    defined(GPIO_SPI_0_POCI_PORT) && defined(GPIO_SPI_0_POCI_PIN) && \
    defined(GPIO_SPI_0_IOMUX_POCI)
#define BOARD_HAS_SPI0_GPIO_PINS   (1)
#define BOARD_SPI0_SCLK_PORT       GPIO_SPI_0_SCLK_PORT
#define BOARD_SPI0_SCLK_PIN        GPIO_SPI_0_SCLK_PIN
#define BOARD_SPI0_SCLK_IOMUX      GPIO_SPI_0_IOMUX_SCLK
#define BOARD_SPI0_MOSI_PORT       GPIO_SPI_0_PICO_PORT
#define BOARD_SPI0_MOSI_PIN        GPIO_SPI_0_PICO_PIN
#define BOARD_SPI0_MOSI_IOMUX      GPIO_SPI_0_IOMUX_PICO
#define BOARD_SPI0_MISO_PORT       GPIO_SPI_0_POCI_PORT
#define BOARD_SPI0_MISO_PIN        GPIO_SPI_0_POCI_PIN
#define BOARD_SPI0_MISO_IOMUX      GPIO_SPI_0_IOMUX_POCI
#else
#define BOARD_HAS_SPI0_GPIO_PINS   (0)
#endif

#if defined(GPIO_W25QXX_CS_PORT) && defined(GPIO_W25QXX_CS_PIN)
#define BOARD_HAS_W25QXX_CS        (1)
#define BOARD_W25QXX_CS_PORT       GPIO_W25QXX_CS_PORT
#define BOARD_W25QXX_CS_PIN        GPIO_W25QXX_CS_PIN
#if defined(GPIO_W25QXX_CS_IOMUX)
#define BOARD_W25QXX_CS_IOMUX      GPIO_W25QXX_CS_IOMUX
#endif
#elif defined(GPIO_W25QXX_PORT) && defined(GPIO_W25QXX_CS_PIN)
#define BOARD_HAS_W25QXX_CS        (1)
#define BOARD_W25QXX_CS_PORT       GPIO_W25QXX_PORT
#define BOARD_W25QXX_CS_PIN        GPIO_W25QXX_CS_PIN
#if defined(GPIO_W25QXX_CS_IOMUX)
#define BOARD_W25QXX_CS_IOMUX      GPIO_W25QXX_CS_IOMUX
#endif
#elif defined(GPIO_FLASH_CS_PORT) && defined(GPIO_FLASH_CS_PIN)
#define BOARD_HAS_W25QXX_CS        (1)
#define BOARD_W25QXX_CS_PORT       GPIO_FLASH_CS_PORT
#define BOARD_W25QXX_CS_PIN        GPIO_FLASH_CS_PIN
#if defined(GPIO_FLASH_CS_IOMUX)
#define BOARD_W25QXX_CS_IOMUX      GPIO_FLASH_CS_IOMUX
#endif
#elif defined(GPIO_FLASH_PORT) && defined(GPIO_FLASH_CS_PIN)
#define BOARD_HAS_W25QXX_CS        (1)
#define BOARD_W25QXX_CS_PORT       GPIO_FLASH_PORT
#define BOARD_W25QXX_CS_PIN        GPIO_FLASH_CS_PIN
#if defined(GPIO_FLASH_CS_IOMUX)
#define BOARD_W25QXX_CS_IOMUX      GPIO_FLASH_CS_IOMUX
#endif
#elif defined(GPIO_SPI_0_CS_PORT) && defined(GPIO_SPI_0_CS_PIN)
#define BOARD_HAS_W25QXX_CS        (1)
#define BOARD_W25QXX_CS_PORT       GPIO_SPI_0_CS_PORT
#define BOARD_W25QXX_CS_PIN        GPIO_SPI_0_CS_PIN
#if defined(GPIO_SPI_0_IOMUX_CS)
#define BOARD_W25QXX_CS_IOMUX      GPIO_SPI_0_IOMUX_CS
#endif
#elif defined(GPIO_GRP_0_PORT) && defined(GPIO_GRP_0_PIN_1_PIN) && \
    defined(GPIO_GRP_0_PIN_1_IOMUX)
#define BOARD_HAS_W25QXX_CS        (1)
#define BOARD_W25QXX_CS_PORT       GPIO_GRP_0_PORT
#define BOARD_W25QXX_CS_PIN        GPIO_GRP_0_PIN_1_PIN
#define BOARD_W25QXX_CS_IOMUX      GPIO_GRP_0_PIN_1_IOMUX
#else
#define BOARD_HAS_W25QXX_CS        (0)
#endif

#if defined(CPUCLK_FREQ)
#define BOARD_CPUCLK_HZ            CPUCLK_FREQ
#else
#define BOARD_CPUCLK_HZ            (32000000UL)
#endif

#endif /* USER_BOARD_BOARD_CONFIG_H_ */
