/*
Code originally written by Ali Haydarogly, February 2022, for Adafruit Grand Central M4 board
Adapted for Teensy 4.0 by Michael Krumin, March 2023
Adapted for gated PMTs by James Rowland Jan 2025
*/

// pin connected to the screen backlight
const byte screen_pin = 13;
// pin connected to the pmt gate
const byte gate_pin = 14;

// rising and falling interrupt pins should both be tied 
// to the same sync signal from the resonant mirrors
const byte interrupt_pin_rising = 7;
const byte interrupt_pin_falling = 8;

// Useful parameter - number of CPU ticks in a second
int sys_clock = 6e8;

// Timing parameters:
// For Teensy 4.0 running at 600 MHz
// 6000 ticks is 10.0 us
// 3000 ticks is 5.0 us
// 1000 ticks is 1.67 us
// 600  ticks is 1.0 us

// number of ticks from a rising/falling edge until the pulse is triggered by that edge
// Resonant mirror edge usually appears in the middle of a U-turn
// So we need to wait until the end of that line to turn on the screens
// For 12 kHz scanners, line time is ~41.67 us

// JR: My scanner is 7910 Hz, my line period is 63.17 us my spatial fill fraction is 0.9 
// JR: This is the original setting for 12 kHz scanners
// int delay_rising = 21000; //21e3 at 600MHz == 35 microseconds 
// int delay_falling = 21300;

// This is the delays computed for a 7910 Hz scanner
int delay_rising = 31835; 
int delay_falling = 32290;

// number of ticks in a pulse triggered by 
// a rising/falling edge
// This will depend on the scanners frequency and the temporal fill fraction of the acquisition parameters
// Spatial fill fraction of 0.9 --> temporal 0.713 --> 11 microseconds is just right for a single U-Turn duration

// JR This is the original number of ticks
// int pulse_ticks_rising = 6600; 
// int pulse_ticks_falling = 6600;

// JR: increased for my slower scanner (this still may be slightly too long, but we lose a 
// negligable amount of image at the edge)
int pulse_ticks_rising = 10000; 
int pulse_ticks_falling = 10000;

// My PMT takes 1 us to fully switch from 0 -> 100% (600 ticks at 600 MHz) tinyurl.com/msj8b2j6
// TODO: check that this works when disabling the gate (assumes that the screens have a similar switching time)
const int gate_switiching_time = 600;

int current_time_r, current_time_f, current_time, previous_time;
int next_rising_pulse_start_tick;
int next_falling_pulse_start_tick;
int current_pulse_end_tick;
// Probably unneccesary
int gate_on = 0;
int pulse_on = 0;
int diff = 0;
     
void setup() {
  ARM_DEMCR |= ARM_DEMCR_TRCENA;
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
  pulse_on = 0;
  gate_on = 0;
  pinMode(screen_pin, OUTPUT);
  pinMode(gate_pin, OUTPUT);
  pinMode(interrupt_pin_rising, INPUT_PULLUP);
  pinMode(interrupt_pin_falling, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interrupt_pin_rising), pulse_rising, RISING);
  attachInterrupt(digitalPinToInterrupt(interrupt_pin_falling), pulse_falling, FALLING);
}

int check_time(int current_time, int target_time){
/*  Teensy CPU cycle counts are pretty straightforward
    They count up, and wraparound as usual int (+-2^31), which means that calculating difference 
    between two times does not suffer from wraparound (unless you do a full cycle of 2^32 ticks)
    At clock speeds of 600 MHz it takes 7.1583 seconds to wraparound 2^32 int ticks, 
    which is plenty and safe, given that our typical diff will be in 10s of microseconds  
*/
  diff = current_time - target_time;
  // Still too early to emit a pulse
  if (diff < 0) {return 0;}

  // Now this is time to emit a pulse
  if (diff >= 0) {return 1;}

  // we shouldn't really reach this line, but return 0 just in case
  return 0;
}

void loop() {
  current_time = ARM_DWT_CYCCNT;
  // if we are in between pulses
  if (pulse_on == 0){
    // check if it is time to turn the gate on (1 µs before the screen)
    if (check_time(current_time, next_rising_pulse_start_tick - gate_switiching_time)){
          gate_on = 1;
          digitalWrite(gate_pin, HIGH);
      }
    if (check_time(current_time, next_falling_pulse_start_tick - gate_switiching_time)){
          gate_on = 1;
          digitalWrite(gate_pin, HIGH);
      }

    // check if it is time to turn the screen on
    if (check_time(current_time, next_rising_pulse_start_tick)){
          pulse_on = 1;
          // calculate the end time of the pulse
          //current_pulse_start_tick = current_time;
          current_pulse_end_tick = current_time + pulse_ticks_rising;
          // set the next pulse start tick to a large impossible number so it doesn't retrigger another pulse
          // one second from now, should never realistically trigger another pulse while the resonant scanner is running
          next_rising_pulse_start_tick = current_time + sys_clock;
          digitalWrite(screen_pin, HIGH);
      }
    if (check_time(current_time, next_falling_pulse_start_tick)){
          pulse_on = 1;
          current_pulse_end_tick = current_time + pulse_ticks_falling;
          next_falling_pulse_start_tick = current_time + sys_clock;
          digitalWrite(screen_pin, HIGH);
      }
  }   
  // if the pulse is currently on
  if (pulse_on == 1){
    // check if it is time to turn the screen off
    if (check_time(current_time, current_pulse_end_tick)){
          pulse_on = 0;
          digitalWrite(screen_pin, LOW);
      }
    // check if it is time to turn the gate off (1 µs after the screen)
    if (check_time(current_time, current_pulse_end_tick + gate_switiching_time)){
          gate_on = 0;
          digitalWrite(gate_pin, LOW);
      }
  }
}

void pulse_rising() {
/*
 * Interrupt routine for rising edge. set the next rising pulse start time.
 */
  current_time_r = ARM_DWT_CYCCNT;
  next_rising_pulse_start_tick = current_time_r + delay_rising;
}
void pulse_falling(){
  /*
   * Interrupt routine for falling edge. Set the next falling pulse start time.
   */
  current_time_f = ARM_DWT_CYCCNT;
  next_falling_pulse_start_tick = current_time_f + delay_falling;
}
