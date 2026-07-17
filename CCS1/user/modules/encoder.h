#ifndef USER_MODULES_ENCODER_H_
#define USER_MODULES_ENCODER_H_

#include <stdint.h>

#define ENCODER_COUNTS_PER_REV    (1560U)

void encoder_init(void);
int32_t encoder_get_left(void);
int32_t encoder_get_right(void);
void encoder_reset_left(void);
void encoder_reset_right(void);
void encoder_reset_both(void);
int32_t encoder_get_left_rpm(void);
int32_t encoder_get_right_rpm(void);
void encoder_poll_left(void);
void encoder_update(void);

#endif /* USER_MODULES_ENCODER_H_ */
