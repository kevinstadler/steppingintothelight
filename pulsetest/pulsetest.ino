const int TRIG = 42;
const int ECHO = 18;

void setup() {
  Serial.begin(115200);
  pinMode(TRIG, OUTPUT);
  digitalWrite(TRIG, LOW);
  pinMode(ECHO, INPUT);

  attachInterrupt(digitalPinToInterrupt(ECHO), interrupt, RISING);
}

//////////////////// interrupt

volatile unsigned long t, u;

void loop() {
  Serial.println("---");
  digitalWrite(TRIG, HIGH);
  // at least 10us of high signal (triggers 8 40khz square wave)
  delayMicroseconds(12);
  t = micros();
  digitalWrite(TRIG, LOW);
  u = micros();
  unsigned long dist = pulseIn(ECHO, HIGH);
  unsigned long v = micros();
  Serial.println("pulse");
  Serial.println(v - u);
  Serial.println(dist);
  delay(1000);
}

// can't test both at the same time because the interrupt messes with pulseIn()...
// min: interrupt 7ms, pulse takes 6.5ms to return 200
// 2m: interrupt 18ms, pulse takes 30ms to return 12000
// 4m: interrupt ?, pulse takes 42ms ro return 17000
// error: interrupt 88ms, pulse takes 154ms to return 65k+

void interrupt() {
  unsigned long v = micros();
  Serial.println("interrupt");
  Serial.println(v - t);
  Serial.println(v - u);
}

//////////////////// PWM
void loopPWM() {
  digitalWrite(TRIG, HIGH);
  // at least 10us of high signal (triggers 8 40khz square wave)
  delayMicroseconds(12);
  digitalWrite(TRIG, LOW);
  unsigned long t = micros();
  unsigned long dist = pulseIn(ECHO, HIGH, 80000);
  Serial.println("--");
  Serial.println(dist);
  Serial.println(micros() - t);
  delay(500);
}

// min: takes 8ms to return 179
// 2m: takes 28ms to return 11000
// 4m: takes 42ms to return 17000
// error: takes 154ms to return 65320 or 65314 or 65546

// so the max timeout i need is 8ms + 2*distance

// pulsein 50000 timeout already terminates at 32000
// pulsein 80000 timeout terminates at 55000

// so timeout should be 8ms + 2*distance + 25000
