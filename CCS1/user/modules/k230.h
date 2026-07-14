#ifndef USER_MODULES_K230_H_
#define USER_MODULES_K230_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define K230_RX_LINE_MAX    (96U)
#define K230_FRAME_SOF      (0xA5U)
#define K230_FRAME_EOF      (0x5AU)
#define K230_FRAME_MAX_PAYLOAD (64U)

typedef enum {
    K230_CMD_PING = 0x01U,
    K230_CMD_PONG = 0x02U,
    K230_CMD_TEXT = 0x10U,
    K230_CMD_CONTROL = 0x20U,
    K230_CMD_ACK = 0x7FU
} k230_command_t;

typedef struct {
    uint8_t command;
    uint8_t length;
    uint8_t payload[K230_FRAME_MAX_PAYLOAD];
    uint8_t checksum;
} k230_frame_t;

typedef struct {
    bool uart_available;
    bool line_ready;
    bool frame_ready;
    bool overflow;
    uint32_t rx_bytes;
    uint32_t tx_bytes;
    uint32_t rx_frames;
    uint32_t rx_frame_errors;
    const char *line;
} k230_status_t;

void k230_init(void);
bool k230_is_available(void);
void k230_poll(void);
k230_status_t k230_get_status(void);
bool k230_has_line(void);
const char *k230_get_line(void);
void k230_clear_line(void);
bool k230_has_frame(void);
const k230_frame_t *k230_get_frame(void);
void k230_clear_frame(void);
uint8_t k230_calc_checksum(uint8_t command, const uint8_t *payload, uint8_t length);
bool k230_send_frame(uint8_t command, const uint8_t *payload, uint8_t length);
bool k230_send_ack(uint8_t command);
bool k230_send_text_frame(const char *str);
void k230_send_bytes(const uint8_t *data, size_t length);
void k230_send_string(const char *str);
void k230_send_line(const char *str);
void k230_send_ping(uint32_t sequence);

#ifdef __cplusplus
}
#endif

#endif /* USER_MODULES_K230_H_ */
