#ifndef USER_MODULES_BLUETOOTH_H_
#define USER_MODULES_BLUETOOTH_H_

#include <stdint.h>

void bluetooth_init(void);
void bluetooth_send_char(uint8_t data);
void bluetooth_send_string(char *str);
uint8_t bluetooth_available(void);
uint8_t bluetooth_read_char(void);

#endif /* USER_MODULES_BLUETOOTH_H_ */
