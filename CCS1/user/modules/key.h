#ifndef USER_MODULES_KEY_H_
#define USER_MODULES_KEY_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void key_init(void);
bool key_is_pressed(void);

typedef enum {
    KEY_EVENT_NONE = 0U,
    KEY_EVENT_1,
    KEY_EVENT_2,
    KEY_EVENT_3,
    KEY_EVENT_4
} key_event_t;

/* Returns one event per debounced press; holding a key does not repeat it. */
key_event_t key_get_event(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_MODULES_KEY_H_ */
