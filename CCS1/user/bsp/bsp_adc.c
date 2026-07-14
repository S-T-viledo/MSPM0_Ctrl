#include "user/bsp/bsp_adc.h"

#include "user/board/board_config.h"

#define BSP_ADC_TIMEOUT                 (100000U)
#define BSP_ADC_DEFAULT_SAMPLE_TIME     (50U)

#if BOARD_HAS_LIGHT_AO
static const bsp_adc_channel_t g_light_adc_channel = BOARD_LIGHT_ADC_CHANNEL;
#endif

#if BOARD_HAS_TRACK_ADC
static const bsp_adc_channel_t g_track_adc_channel = BOARD_TRACK_ADC_CHANNEL;
#endif

static uint32_t bsp_adc_mem_interrupt_mask(DL_ADC12_MEM_IDX mem)
{
    switch (mem) {
        case DL_ADC12_MEM_IDX_0:
            return DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED;
        case DL_ADC12_MEM_IDX_1:
            return DL_ADC12_INTERRUPT_MEM1_RESULT_LOADED;
        case DL_ADC12_MEM_IDX_2:
            return DL_ADC12_INTERRUPT_MEM2_RESULT_LOADED;
        case DL_ADC12_MEM_IDX_3:
            return DL_ADC12_INTERRUPT_MEM3_RESULT_LOADED;
        case DL_ADC12_MEM_IDX_4:
            return DL_ADC12_INTERRUPT_MEM4_RESULT_LOADED;
        case DL_ADC12_MEM_IDX_5:
            return DL_ADC12_INTERRUPT_MEM5_RESULT_LOADED;
        case DL_ADC12_MEM_IDX_6:
            return DL_ADC12_INTERRUPT_MEM6_RESULT_LOADED;
        case DL_ADC12_MEM_IDX_7:
            return DL_ADC12_INTERRUPT_MEM7_RESULT_LOADED;
        case DL_ADC12_MEM_IDX_8:
            return DL_ADC12_INTERRUPT_MEM8_RESULT_LOADED;
        case DL_ADC12_MEM_IDX_9:
            return DL_ADC12_INTERRUPT_MEM9_RESULT_LOADED;
        case DL_ADC12_MEM_IDX_10:
            return DL_ADC12_INTERRUPT_MEM10_RESULT_LOADED;
        case DL_ADC12_MEM_IDX_11:
            return DL_ADC12_INTERRUPT_MEM11_RESULT_LOADED;
        default:
            return 0U;
    }
}

static bool bsp_adc_convert(const bsp_adc_channel_t *channel,
    uint16_t *raw, bool *timeout_flag)
{
    uint32_t timeout = BSP_ADC_TIMEOUT;
    uint32_t int_mask;

    if (timeout_flag != 0) {
        *timeout_flag = false;
    }

    if ((channel == 0) || (channel->inst == 0) || (raw == 0)) {
        return false;
    }

    int_mask = bsp_adc_mem_interrupt_mask(channel->mem);
    if (int_mask == 0U) {
        return false;
    }

    DL_ADC12_clearInterruptStatus(channel->inst, int_mask);
    DL_ADC12_enableConversions(channel->inst);
    DL_ADC12_startConversion(channel->inst);

    while ((DL_ADC12_getRawInterruptStatus(channel->inst, int_mask) == 0U) &&
           (timeout > 0U)) {
        timeout--;
    }

    if (timeout == 0U) {
        if (timeout_flag != 0) {
            *timeout_flag = true;
        }
        DL_ADC12_enableConversions(channel->inst);
        return false;
    }

    *raw = DL_ADC12_getMemResult(channel->inst, channel->mem);
    DL_ADC12_clearInterruptStatus(channel->inst, int_mask);
    DL_ADC12_enableConversions(channel->inst);
    return true;
}

void bsp_adc_init(void)
{
#if BOARD_HAS_LIGHT_AO
    bsp_adc_init_single_sample(g_light_adc_channel.inst,
        BSP_ADC_DEFAULT_SAMPLE_TIME);
#endif
#if BOARD_HAS_TRACK_ADC
    bsp_adc_init_single_sample(g_track_adc_channel.inst,
        BSP_ADC_DEFAULT_SAMPLE_TIME);
#endif
}

void bsp_adc_init_single_sample(ADC12_Regs *inst, uint16_t sample_time)
{
    if (inst == 0) {
        return;
    }

    DL_ADC12_disableConversions(inst);
    DL_ADC12_initSingleSample(inst,
        DL_ADC12_REPEAT_MODE_DISABLED,
        DL_ADC12_SAMPLING_SOURCE_AUTO,
        DL_ADC12_TRIG_SRC_SOFTWARE,
        DL_ADC12_SAMP_CONV_RES_12_BIT,
        DL_ADC12_SAMP_CONV_DATA_FORMAT_UNSIGNED);
    DL_ADC12_setPowerDownMode(inst, DL_ADC12_POWER_DOWN_MODE_MANUAL);
    DL_ADC12_setSampleTime0(inst, sample_time);
    DL_ADC12_enableConversions(inst);
}

bool bsp_adc_is_channel_available(const bsp_adc_channel_t *channel)
{
    return ((channel != 0) && (channel->inst != 0) &&
            (bsp_adc_mem_interrupt_mask(channel->mem) != 0U));
}

bool bsp_adc_read_channel(const bsp_adc_channel_t *channel, uint16_t *raw)
{
    return bsp_adc_convert(channel, raw, 0);
}

uint8_t bsp_adc_raw_to_percent(uint16_t raw, uint16_t max_value)
{
    uint32_t scaled;

    if (max_value == 0U) {
        return 0U;
    }

    scaled = ((uint32_t) raw * 100U) / max_value;
    if (scaled > 100U) {
        scaled = 100U;
    }

    return (uint8_t) scaled;
}

void bsp_adc_get_debug(const bsp_adc_channel_t *channel, bsp_adc_debug_t *debug)
{
    uint16_t sample = 0U;
    bool timeout = false;
    uint32_t mem_index;
    uint32_t int_mask;

    if (debug == 0) {
        return;
    }

    debug->inst_addr = 0U;
    debug->ctl0 = 0U;
    debug->ctl1 = 0U;
    debug->ctl2 = 0U;
    debug->scomp0 = 0U;
    debug->mem = 0U;
    debug->memctl0 = 0U;
    debug->memres0 = 0U;
    debug->status = 0U;
    debug->ris = 0U;
    debug->iidx = 0U;
    debug->available = false;
    debug->timeout = false;
    debug->sample = 0U;

    if (!bsp_adc_is_channel_available(channel)) {
        return;
    }

    (void) bsp_adc_convert(channel, &sample, &timeout);

    mem_index = (uint32_t) channel->mem;
    int_mask = bsp_adc_mem_interrupt_mask(channel->mem);

    debug->inst_addr = (uint32_t) channel->inst;
    debug->ctl0 = channel->inst->ULLMEM.CTL0;
    debug->ctl1 = channel->inst->ULLMEM.CTL1;
    debug->ctl2 = channel->inst->ULLMEM.CTL2;
    debug->scomp0 = channel->inst->ULLMEM.SCOMP0;
    debug->mem = mem_index;
    debug->memctl0 = channel->inst->ULLMEM.MEMCTL[mem_index];
    debug->memres0 = channel->inst->ULLMEM.MEMRES[mem_index];
    debug->status = channel->inst->ULLMEM.STATUS;
    debug->ris = channel->inst->ULLMEM.CPU_INT.RIS & int_mask;
    debug->iidx = channel->inst->ULLMEM.CPU_INT.IIDX;
    debug->available = true;
    debug->timeout = timeout;
    debug->sample = sample;
}

bool bsp_adc_light_is_available(void)
{
#if BOARD_HAS_LIGHT_AO
    return bsp_adc_is_channel_available(&g_light_adc_channel);
#else
    return false;
#endif
}

bool bsp_adc_light_read_raw(uint16_t *raw)
{
#if BOARD_HAS_LIGHT_AO
    return bsp_adc_read_channel(&g_light_adc_channel, raw);
#else
    (void) raw;
    return false;
#endif
}

void bsp_adc_light_get_debug(bsp_adc_debug_t *debug)
{
#if BOARD_HAS_LIGHT_AO
    bsp_adc_get_debug(&g_light_adc_channel, debug);
#else
    bsp_adc_get_debug(0, debug);
#endif
}
