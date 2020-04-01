// sensor/led number constant used for all the loops.
const int N = 4;

#include <EnableInterrupt.h>

#if defined(ARDUINO_AVR_UNO) //// UNO ////
const int TRIG[] = { 4, 5, 11, 8 };
// PC interrupts
const int ECHO[] = { 2, 3, 10, A0 };
// Uno PWM pins: 3, 5, 6, 9, 10, 11 (higher freq on 5, 6)
const int LEDS[] = { A1, A2, A3, A4 };
#else //// MEGA ////
const int TRIG[] = { 42, 43, 44, 45 };
const int ECHO[] = { 18, 19, 20, 21 };
// Mega PWM pins: 2-13 (higher freq on 4, 13)
const int LEDS[] = { 4, 5, 6, 7 };
#endif

// add resistance to fluctuating measurements:
// how many consecutive 'on' or 'off' triggers required to switch?
const int CONSECUTIVE_ONOFFS[] = { 2, 2 };
// 0: be conservative with turning on again
// 1: (almost) immediately turn off after receiving under-threshold values

// LED control variables
bool ledStatus[N];
// add hysteresis for error/timeout reads
byte readsUntilSwitch[N];
// time from trigger until echo onset exceed 40.000
unsigned int distanceThresholds[N];
// set by setup as max threshold + 1, in ms
byte interTriggerDelay;

// 20.000 ~ 2m
// 1000 was ok for US-100
const int LIMIT = 3000;

volatile unsigned long triggers[N];

void sound(byte i) {
//  digitalWrite(TRIG[i], LOW);
//  delayMicroseconds(2);
  digitalWrite(TRIG[i], HIGH);
  // at least 10us of high signal triggers 8x40khz square wave (200us)
  delayMicroseconds(11);
  digitalWrite(TRIG[i], LOW);
  triggers[i] = micros();
}

void loop() {
  for (byte i = 0; i < N; i++) {
    sound(i);
    // add 10ms or more if you're using US-015 (what a piece of shit)
    delay(interTriggerDelay);
  }
}

void echo(byte i) {
  long elapsed = micros() - triggers[i];
  if (elapsed < 1000) {
    Serial.println(i);
    Serial.println(elapsed);
    return;
  }
  // 3 cases:
  // 1. smaller than threshold, turn off
  // 2. error (80000+ response), turn off (with hysteresis)
  // 3. in between that: clear view, keep on
  unsigned long diff = abs(elapsed - distanceThresholds[i]);

  bool clearView = diff < LIMIT;
//  if (!clearView) {
//    Serial.println(i);
//    Serial.println(elapsed - distanceThresholds[i]);
//  }
  if (ledStatus[i] == clearView) {
    readsUntilSwitch[i] = CONSECUTIVE_ONOFFS[clearView];
  } else {
    if (readsUntilSwitch[i] > 1) {
      readsUntilSwitch[i]--;
    } else {
      setLED(i, clearView);
    }
  }
  // TODO if elapsed > 80000 (read error/interrupt) and number of
  // sensors is small, store current time for delayed read of
  // next interrupt (or just delay() here to avoid heisenbug)
}

void (*callbacks[])(void) = {
  [] () { echo(0); },
  [] () { echo(1); },
  [] () { echo(2); },
  [] () { echo(3); },
};

volatile unsigned long calibration;

void attachCalibration(byte i, byte interruptSignal) {
  attachEchoInterrupt(i, interruptSignal, [] () { calibration = micros(); });
}

void setup() {
  // light onboard led during calibration
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);

  for (byte i = 0; i < N; i++) {
    pinMode(TRIG[i], OUTPUT);
    pinMode(ECHO[i], INPUT);
    pinMode(LEDS[i], OUTPUT);
    setLED(i, HIGH);

    // calibration interrupt
    byte interruptSignal = RISING; // default for US-100
    attachCalibration(i, interruptSignal);

    while (true) {
      calibration = 0;
      sound(i);
      // onset of error signal takes 88ms max
      delay(100);
      unsigned long wait = calibration - triggers[i];

      if (wait < 3000) {
        // too early to be true -- probably a US-015
        interruptSignal = FALLING;
        attachCalibration(i, interruptSignal);
      } else if (wait < 80000) {
        distanceThresholds[i] = wait;
        break;
      }
      // signal failed attempt
      setLED(i, LOW);
      delay(50);
      setLED(i, HIGH);
      Serial.println(wait);
    }
    Serial.println(distanceThresholds[i]);
    attachEchoInterrupt(i, interruptSignal, callbacks[i]);

    interTriggerDelay = max(interTriggerDelay,
                          1 + distanceThresholds[i] / 1000);
  }
  Serial.println(interTriggerDelay);
  Serial.println("-----");

  digitalWrite(LED_BUILTIN, LOW);
}

void setLED(byte i, bool state) {
  ledStatus[i] = state;
//  digitalWrite(LEDS[i], state);
  analogWrite(LEDS[i], state * 20);
  readsUntilSwitch[i] = CONSECUTIVE_ONOFFS[state];
}

void attachEchoInterrupt(byte i, byte interruptSignal, void(*function)(void)) {
  //  | PINCHANGEINTERRUPT
  enableInterrupt(ECHO[i], function, interruptSignal);

  // original HW interruptsinterruptSignal
//  attachInterrupt(digitalPinToInterrupt(ECHO[i]), function, interruptSignal);
}
