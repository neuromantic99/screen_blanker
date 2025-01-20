/*
Code originally written by Ali Haydarogly, February 2022, for Adafruit Grand Central M4 board
Adapted for Teensy 4.0 by Michael Krumin, March 2023
*/

// pin connected to the screen backlight
const byte pulse_pin = 13;

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

// My scanner is 7910 Hz, my line period is 63.17 us my spatial fill fraction is 0.9 
// int delay_rising = 21000; //21e3 at 600MHz == 35 microseconds 
// int delay_falling = 21300;

// computed ratio
int delay_rising = 31835; 
int delay_falling = 32290;

// random attempts
// int delay_rising = 5000; 
// int delay_falling = 5300;

// number of ticks in a pulse triggered by 
// a rising/falling edge
// This will depend on the scanners frequency and the temporal fill fraction of the acquisition parameters
// Spatial fill fraction of 0.9 --> temporal 0.713 --> 11 microseconds is just right for a single U-Turn duration

// int pulse_ticks_rising = 6600; 
// int pulse_ticks_falling = 6600;

// computer ratio
int pulse_ticks_rising = 10000; 
int pulse_ticks_falling = 10000;

// int pulse_ticks_rising = 30000; 
// int pulse_ticks_falling = 30000;




int current_time_r, current_time_f, current_time, previous_time;
int next_rising_pulse_start_tick;
int next_falling_pulse_start_tick;
// int current_pulse_start_tick;
int current_pulse_end_tick;
int pulse_on = 0;
int diff = 0;
     
void setup() {
//Serial.begin(115200);
  ARM_DEMCR |= ARM_DEMCR_TRCENA;
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
  pulse_on = 0;
  //max_delay = max(delay_rising, delay_falling) + pulse_ticks + 1000;
  pinMode(pulse_pin, OUTPUT);
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
    // the following if block is repeated twice, once for a rising edge-triggered
    // pulse and once for a falling edge-triggered pulse. This is so you can set 
    // different pulse delays/times, and also it can handle the case where the falling edge 
    // triggered pulse happened after the next rising edge.

    // check if it is time to turn the pulse on
    if (check_time(current_time, next_rising_pulse_start_tick)){
          pulse_on = 1;
          // calculate the end time of the pulse
          //current_pulse_start_tick = current_time;
          current_pulse_end_tick = current_time + pulse_ticks_rising;
          // set the next pulse start tick to a large impossible number so it doesn't retrigger another pulse
          // one second from now, should never realistically trigger another pulse while the resonant scanner is running
          next_rising_pulse_start_tick = current_time + sys_clock;
          digitalWrite(pulse_pin, HIGH);
      }
    if (check_time(current_time, next_falling_pulse_start_tick)){
          pulse_on = 1;
          //current_pulse_start_tick = current_time;
          current_pulse_end_tick = current_time + pulse_ticks_falling;
          next_falling_pulse_start_tick = current_time + sys_clock;
          digitalWrite(pulse_pin, HIGH);
      }
  }   
  // if the pulse is currently on
  if (pulse_on == 1){
    // check if it is time to turn the pulse off
    if (check_time(current_time, current_pulse_end_tick)){
          pulse_on = 0;
          digitalWrite(pulse_pin, LOW);
      }
  }
}

void pulse_rising() {
/*
 * Interrupt routine for rising edge. set the next rising pulse start time.
 */
  current_time_r = ARM_DWT_CYCCNT;
  // Some lines useful for debugging, but they are slowing things down a bit
  //Serial.print(current_time_r-previous_time);
  //Serial.print(current_time_r);
  //Serial.print('\n');
  next_rising_pulse_start_tick = current_time_r + delay_rising;
  //previous_time = current_time_r;
}
void pulse_falling(){
  /*
   * Interrupt routine for falling edge. Set the next falling pulse start time.
   */
  current_time_f = ARM_DWT_CYCCNT;
  next_falling_pulse_start_tick = current_time_f + delay_falling;
}
