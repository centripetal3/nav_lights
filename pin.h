/* pin.h: AVR pin, port, and data direction abstraction library */

#ifndef pin_h
#define pin_h

#include <stdint.h>
#include "bool.h"

/* The pin_t object */
typedef struct
{
  volatile uint8_t* data_direction_register;
  volatile uint8_t* port_register;
  volatile uint8_t* pin_register;
  uint8_t position;
} pin_t;

#define pin_static_def(port, position) {&DDR##port, &PORT##port, &PIN##port, position}
#define pin_def(port, position) ((pin_t)pin_static_def(port, position))
/* Defines a new pin macro for the given port and position. E.g.:
 *   #define green_led pin_def(B, 6)
 *   pin_t pin_a0 = pin_static_def(A, 0); */

#define pin_output(pin) (*(pin.data_direction_register) |= _BV(pin.position))
/* Sets the pin to output mode. */

#define pin_input(pin) (*(pin.data_direction_register) &= ~_BV(pin.position))
/* Sets the pin to input mode */

#define pin_high(pin) (*(pin.port_register) |= _BV(pin.position))
/* If the pin is in output mode, drives to +Vcc. 
 * If the pin is in input mode, activates a weak pullup to +Vcc */

#define pin_low(pin) (*(pin.port_register) &= ~_BV(pin.position))
/* If the pin is in output mode, drives to GND 
 * If the pin is in input mode, goes into high-impedance */

#define pin_read(pin) ((*(pin.pin_register) & _BV(pin.position)) >> pin.position)
/* Sample the value on the pin and returns 0 (for GND) or 1 (for +Vcc) */

#define pin_is_low(pin) (pin_read(pin) == 0)
#define pin_is_high(pin) (pin_read(pin) == 1)
/* Symbolic definitions for common comparisons */

#define pin_write(pin, value) \
  do \
  { \
    if ((value) == 0) \
      pin_low(pin); \
    else \
      pin_high(pin); \
  } while (false)
/* Calls pin_low() if value == 0, otherwise calls pin_high() */

#endif

