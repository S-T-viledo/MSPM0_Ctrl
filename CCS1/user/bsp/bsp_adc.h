#ifndef USER_BSP_BSP_ADC_H_
#define USER_BSP_BSP_ADC_H_

#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ADC12_Regs *inst;
    DL_ADC12_MEM_IDX mem;
    uint16_t max_value;
} bsp_adc_channel_t;

typedef struct {
    uint32_t inst_addr;
    uint32_t ctl0;
    uint32_t ctl1;
    uint32_t ctl2;
    uint32_t scomp0;
    uint32_t mem;
    uint32_t memctl0;
    uint32_t memres0;
    uint32_t status;
    uint32_t ris;
    uint32_t iidx;
    bool available;
    bool timeout;
    uint16_t sample;
} bsp_adc_debug_t;

void bsp_adc_init(void);
void bsp_adc_init_single_sample(ADC12_Regs *inst, uint16_t sample_time);
bool bsp_adc_is_channel_available(const bsp_adc_channel_t *channel);
bool bsp_adc_read_channel(const bsp_adc_channel_t *channel, uint16_t *raw);
uint8_t bsp_adc_raw_to_percent(uint16_t raw, uint16_t max_value);
void bsp_adc_get_debug(const bsp_adc_channel_t *channel, bsp_adc_debug_t *debug);

bool bsp_adc_light_is_available(void);
bool bsp_adc_light_read_raw(uint16_t *raw);
void bsp_adc_light_get_debug(bsp_adc_debug_t *debug);

#ifdef __cplusplus
}
#endif

#endif /* USER_BSP_BSP_ADC_H_ */
