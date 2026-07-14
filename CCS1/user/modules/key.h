#ifndef USER_MODULES_KEY_H_
#define USER_MODULES_KEY_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void key_init(void);
bool key_is_pressed(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_MODULES_KEY_H_ */
