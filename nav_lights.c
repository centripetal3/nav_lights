/* nav_lights.c: Airplane navigation lights */

#define F_CPU 1000000UL  // 1 MHz
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include <stdint.h>
#include "pin.h"
#include "bool.h"

#define delay_ms(ms) _delay_ms(ms)
#define delay_us(ms) _delay_us(ms)

pin_t strobe_a;
pin_t strobe_b;
pin_t left_wing;
pin_t right_wing;
pin_t data;

#define strobe_off_time 1000
#define strobe_on_time 60
#define pulse_width_threshold 92
#define deadband 4

/* ISR variables */
volatile uint8_t elapsed = 0;


/* Interrupts *******************************************************/
ISR(PCINT0_vect)
{
  static uint8_t clock_start = 0;
  uint8_t clock_end;

  /* Sample the input pin */
  if (pin_read(&data) == 1)
  {
    /* Pin went high - begin counting */
    clock_start = TCNT1;
  }
  else
  {
    /* Pin went low - stop count and set result */
    clock_end = TCNT1;
    if (clock_end < clock_start)
      elapsed = (255 - clock_start) + clock_end;
    else
      elapsed = (clock_end - clock_start);
  }
}

/* Functions ********************************************************/
uint8_t read_pulse_width(void)
{
  uint8_t result;

  cli();
  /* Make a copy of this volatile data while interrupts are disabled */
  result = elapsed;
  sei();
  return result;
}

/********************************************************************/
bool monitored_pause(uint16_t time_ms)
{
  const uint16_t step_size = 50;
  bool broken = false;
  
  while (!broken && time_ms > step_size)
  {
    delay_ms(step_size);
    if (read_pulse_width() < pulse_width_threshold)
      broken = true;
    else
      time_ms -= step_size;
  }
  return !broken;
}

/********************************************************************/
void strobe_animate(void)
{
  pin_high(&strobe_a);
  delay_ms(strobe_on_time);
  pin_low(&strobe_a);
  delay_ms(strobe_on_time);
  pin_high(&strobe_b);
  delay_ms(strobe_on_time);
  pin_low(&strobe_b);    
}

/********************************************************************/
void configure_pins(void)
{
  /* Pin setup */
  pin_init(&strobe_a, B, 2);
  pin_init(&strobe_b, B, 4);
  pin_init(&left_wing, B, 1);
  pin_init(&right_wing, B, 0);
  pin_init(&data, B, 3);
  pin_output(&strobe_a);
  pin_output(&strobe_b);
  pin_output(&left_wing);
  pin_output(&right_wing);
}

bool probe_for_radio(pin_t* data, pin_t* status_light)
/* Determine if we have a radio connected */
{
  const uint8_t max_loops = 10;
  bool found = false;
  uint8_t loops;

  /* Enable pull-up on the data pin */
  pin_high(data);

  /* Wait for a radio signal */
  for (loops = 0; !found && loops < max_loops; loops++)
  {
    pin_high(status_light);
    delay_ms(100);
    pin_low(status_light);
    found = (pin_read(data) == 0);
    if (!found)
      delay_ms(900);
  }
  
  /* If a radio is connected, then a pull-up is not necessary */
  if (found)
    pin_low(data);
    
  return found;
}

void configure_timer_interrupts(void)
{
  /* Active TIMER1 with prescaler=16, used for measuring input signal pulse 
   * width. Input signals are from 760us to 2280us. */
  set(TCCR1, CS10);
  set(TCCR1, CS12);

  /* Enable interrupts for PCINT3 (pin B3, the data pin) */
  set(PCMSK, PCINT3);
  set(GIMSK, PCIE);
  sei();
}

void handle_lights(bool radio_is_connected)
{
  bool lights_active = false;
  bool lights_requested = false;
  uint8_t pulse_width = 0;

  /* Determine if lights are requested */
  if (!radio_is_connected)
    lights_requested = true; // always run the lights if we don't have a radio
  else    
  {
    pulse_width = read_pulse_width();
    if (pulse_width >= (pulse_width_threshold + deadband))
      lights_requested = true;
    else if (pulse_width < pulse_width_threshold)
      lights_requested = false;
  }

  /* Handle state transitions */
  if (!lights_active && lights_requested)
  {
    pin_high(&left_wing);
    pin_high(&right_wing);
    lights_active = true;
  }
  else if (lights_active && !lights_requested)
  {
    pin_low(&left_wing);
    pin_low(&right_wing);
    lights_active = false;
  }

  /* Strobe or sleep */
  if (lights_active)
  {
    if (!radio_is_connected)
    {
      delay_ms(strobe_off_time);
      strobe_animate();
    }
    else if (monitored_pause(strobe_off_time))
    {
      strobe_animate();
    }
  }
  else
    sleep_mode();
}

/********************************************************************/
int main(void)
{
  bool radio_is_connected;
  
  configure_pins();
  radio_is_connected = probe_for_radio(&data, &left_wing);
  
  if (radio_is_connected)
    configure_timer_interrupts();

  while (true)
    handle_lights(radio_is_connected);

  return 0;
}

