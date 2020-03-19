// sensor/led number constant used for all the loops.
const int N = 4;

const int TRIG[] = { 42, 43, 44, 45 };
const int ECHO[] = { 18, 19, 20, 21 }; // 2 and 3 also interrupt

// Mega PWM pins: 2-13 (higher freq on 4, 13)
const int LEDS[] = { 4, 5, 6, 7 };

// add resistance to fluctuating measurements:
// how many consecutive 'on' or 'off' triggers required to switch?
const int CONSECUTIVE_ONOFFS[] = { 2, 2 };
// 0: be conservative with turning on again
// 1: (almost) immediately turn off after receiving under-threshold values


// LED control variables
bool ledStatus[N];
// add hysteresis
byte readsUntilSwitch[N];
// time from trigger until echo onset exceed 40.000
int distanceThresholds[N];

volatile unsigned long triggers[N];

void sound(int i) {
  digitalWrite(TRIG[i], HIGH);
  // at least 10us of high signal triggers 8x40khz square wave (200us)
  delayMicroseconds(11);
  digitalWrite(TRIG[i], LOW);
  triggers[i] = micros();
}

void loop() {
  for (int i = 0; i < N; i++) {
    sound(i);
  }
  delay(100);
}

void echo(int i) {
  long elapsed = micros() - triggers[i];
  // 3 cases: 
  // 1. smaller than threshold, turn off
  // 2. error (80000+ response), turn off
  // 3. in between that: clear view, keep on
  unsigned long diff = abs(elapsed - distanceThresholds[i]);
  // random variation can be up to 1200? 
  bool clearView = diff < 1200;
  if (ledStatus[i] == clearView) {
    readsUntilSwitch[i] = CONSECUTIVE_ONOFFS[clearView];
  } else {
    if (readsUntilSwitch[i] > 1) {
      readsUntilSwitch[i]--;
    } else {
      setLED(i, clearView);
      readsUntilSwitch[i] = CONSECUTIVE_ONOFFS[clearView];
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

void setup() {
  // light onboard led during calibration
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);

  for (int i = 0; i < N; i++) {
    pinMode(TRIG[i], OUTPUT);
    pinMode(ECHO[i], INPUT);
    pinMode(LEDS[i], OUTPUT);
    setLED(i, HIGH);

    // calibration interrupt
    attachEchoInterrupt(i, [] () { calibration = micros(); });

    while (true) {
      calibration = micros();
      sound(i);
      // onset of error signal takes 88ms max
      delay(100);
      unsigned long wait = calibration - triggers[i];
      if (wait < 80000) {
        distanceThresholds[i] = wait;
        break;
      } else {
      }
    }

    attachEchoInterrupt(i, callbacks[i]);
  }

  digitalWrite(LED_BUILTIN, LOW);
}

void setLED(int ledId, bool state) {
  ledStatus[ledId] = state;
  digitalWrite(LEDS[ledId], state);
//  analogWrite(LEDS[ledId], state * 127);
}

void attachEchoInterrupt(byte i, void(*function)(void)) {
  attachInterrupt(digitalPinToInterrupt(ECHO[i]), function, RISING);
}
