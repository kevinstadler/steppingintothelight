// sensor/led number constant used for all the loops.
const int N = 4;

const int TRIG[] = { 2, 4, 6, 8 };
const int ECHO[] = { 3, 5, 7, 9 };

// Uno PWM pins: 3, 5, 6, 9, 10, 11 (higher freq on 5, 6)
const int LEDS[] = { A0, A1, A2, A3 };
//const int LEDS[] = { 11, 10, 9, 6 };
//const int LEDS[] = { 13, 12, 11, 10 }; // Mega PWM pins: 2-13 (higher freq on 4, 13)

// add resistance to fluctuating measurements:
// how many consecutive 'on' or 'off' triggers required to switch?
const int CONSECUTIVE_ONOFFS[] = { 2, 2 };
// 0: be conservative with turning on again
// 1: (almost) immediately turn off after receiving under-threshold values


// LED control variables
bool ledStatus[N];
// in us or mm, so won't exceed 20.000
int distanceThresholds[N];
// add hysteresis
int readsUntilSwitch[N];

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

    while (distanceThresholds[i] <= 0) {
      distanceThresholds[i] = readDistance(i);
    }
    Serial.println(distanceThresholds[i]);
    // turn LED off when object comes 30cm closer than at initialisation
    distanceThresholds[i] = distanceThresholds[i] - 1000;
  }

  digitalWrite(LED_BUILTIN, LOW);
}

unsigned long readDistance(int i) {
  // 65k error signal takes 154ms to produce, so force '0' first
  return readDistance(i, 80000);
}

unsigned long readDistance(int i, unsigned long timeout) {
  digitalWrite(TRIG[i], HIGH);
  // at least 10us of high signal triggers 8x40khz square wave (200us)
  delayMicroseconds(12);
  digitalWrite(TRIG[i], LOW);
  // "gives up and returns 0 if no complete pulse was received within the timeout"
  // something about pulseIn() actually terminating 25ms earlier than timeout..
  return pulseIn(ECHO[i], HIGH, timeout + 20000);
}

void loop() {
  for (int i = 0; i < N; i++) {

    // make read as short as possible: response from PWM module takes
    // 8ms + 2*the distance to the object (duration of signal+echo)
    unsigned long dist = readDistance(i, 8000 + 2*distanceThresholds[i]);
    // TODO if dist == 0 (read error/interrupt), store current time for delay

    bool clearView = dist > distanceThresholds[i];
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
  }
  // avoid Heisenbug where too quickly discarding overtakes the US-100
}

void setLED(int ledId, bool state) {
  ledStatus[ledId] = state;
  digitalWrite(LEDS[ledId], state);
//  analogWrite(LEDS[ledId], state * 127);
}
