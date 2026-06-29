#ifndef __LED_BLINK_H__
#define __LED_BLINK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

//Function prototypes
void configure_led(void);
void blink_led(uint8_t state);

#ifdef __cplusplus
}
#endif

#endif