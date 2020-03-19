## stepping into the light

(a brightly lit room in which you always stand in the dark)

### hardware

#### visitor detection

"light cell size" resolution will depend mostly on detection capability:

1. laser (~$5 per sensor, very directed)
2. ultrasonic (from $1.5 per sensor, detection angle supposedly down to 15°)
3. infrared (very wide angle?)

Try out ultrasonic first, comparison:
https://www.evelta.com/blog/a-beginners-guide-to-ultrasonic-sensing-technology/

US-100 has good reviews, but angle is given as 15° (but is this +- 15° or +- 7.5° ??)

"grid/cell size" depends on detection angle as well as distance from floor (`detection circle diameter = tan(theta) * height`)

| height | 1m | 2m | 2.5m | 3m | 3.5m | 4m |
| -----  | -- | -- | ---- | -- | ---- | -- |
|  15°   | 27 | 54 |  68  | 81 |  95  | 108 |
|  20°   | 36 | 72 |  90  | 108 | 126 | 144 |
|  30°   | 54 | 108 | 136 | 162  | 190 | 216 |

#### lighting

LED + hazer

* https://www.wired.com/2016/03/obsessive-quest-worlds-sharpest-led-spotlight/

4° LED spot is top, more realistic for consumer maybe 6-10°?

needs *hard* light source (https://www.youtube.com/watch?v=yhmkt_HYTVs)

#### control/switching

If the LEDs take DC, can use a Power FET switch (cheaper and doesn't wear out like a relay): http://arduinoinfo.mywikis.net/wiki/ArduinoPower#Power_Control:Dimming.2FSpeed

The opto-coupled switch circuits will require 5V control (not 3.3 like newer Arduinos).

If US-100 (with serial interface) turns out to be best, could use one of several Arduinos to control it

* Arduino Mega (4 hardware serial ports)
* Arduino Uno, .... (1 hardware + 3 software serial ports, possible on any pin that supports pin change interrupt)
* ESP8266? (if I want/need Wifi support for some reason)

### design considerations

* 4 sensors+lights per controlling unit seems reasonable
  * reading of distances can be staged to avoid clashes
  * if ultrasonic echoes clash between adjacent cells, connect different controllers to each other (one corner is the "master", triggers adjacent units through single connection)
* control LED brightness through PWM

setup/detection:

* on startup, unit reads all distances as "default" values, set a "detection" threshold of the first value minus 20?

### ultrasonic sensor readouts

The Serial interface of the US-100 and even the PWM one (reading distance using `pulseIn()` with a threshold timeout) are nice, but take/block too long when there is more than one sensor on the Arduino (see `pwm.ino` code). Since we are not interested in the precise distance at all but just want to decide ASAP if the threshold has been undercut, simply sending the (staged) triggers of all sensors and waiting for the onset of a response using interrupts (and checking the time elapsed since the trigger) should make for the fastest and only version that allows staging/interleaving. (Only works on the Mega because Uno etc only have 2 interrupt pins.)

### interrupt-based on Uno:

HW interrupt: 2+3
Pin Change Interrupt: 8-13 port, 14-19 (=A0-A5) port
LED control: 4, 5, 6, 7

