const byte pulse_pin = 13;

const byte interrupt_pin_rising = 7;
const byte interrupt_pin_falling = 8;

void setup() {
  // put your setup code here, to run once:
  pinMode(pulse_pin, OUTPUT);
  pinMode(interrupt_pin_rising, INPUT);
  pinMode(interrupt_pin_falling, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
        digitalWrite(pulse_pin, HIGH);
        delay(1000);
        digitalWrite(pulse_pin, LOW);
        delay(1000);
}