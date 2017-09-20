#include <Homie.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

#define FW_NAME "homie-h801"
#define FW_VERSION "2.0.0"

/* Magic sequence for Autodetectable Binary Upload */
const char *__FLAGGED_FW_NAME = "\xbf\x84\xe4\x13\x54" FW_NAME "\x93\x44\x6b\xa7\x75";
const char *__FLAGGED_FW_VERSION = "\x6a\x3f\x3e\x0e\xe1" FW_VERSION "\xb0\x30\x48\xd4\x1a";
/* End of magic sequence for Autodetectable Binary Upload */

// EEPROM structure
struct EEpromDataStruct {
  int initialState; // 1 - ON, other - OFF
  int keepAliveValue; // 0 - disabled, keepalive time - seconds
};

EEpromDataStruct EEpromData;

unsigned long keepAliveReceived=0;

// Light
// the payload that represents enabled/disabled state, by default
const char* LIGHT_ON = "ON";
const char* LIGHT_OFF = "OFF";


#define RGB_LIGHT_RED_PIN    15
#define RGB_LIGHT_GREEN_PIN  13
#define RGB_LIGHT_BLUE_PIN   12
#define W1_PIN               14
#define W2_PIN               4

#define GREEN_PIN    1
#define RED_PIN      5

// Miscellaneous
// Default number of flashes if no value was given
#define CONFIG_DEFAULT_FLASH_LENGTH 2
// Number of seconds for one transition in colorfade mode
#define CONFIG_COLORFADE_TIME_SLOW 10
#define CONFIG_COLORFADE_TIME_FAST 3

const char* on_cmd = "ON";
const char* off_cmd = "OFF";

const bool led_invert = false;

const int BUFFER_SIZE = JSON_OBJECT_SIZE(15);

// Maintained state for reporting to HA
byte red = 255;
byte green = 255;
byte blue = 255;
byte brightness = 255;

// Real values to write to the LEDs (ex. including brightness and state)
byte realRed = 0;
byte realGreen = 0;
byte realBlue = 0;

bool stateOn = false;

// Globals for fade/transitions
bool startFade = false;
unsigned long lastLoop = 0;
int transitionTime = 0;
bool inFade = false;
int loopCount = 0;
int stepR, stepG, stepB;
int redVal, grnVal, bluVal;

// Globals for flash
bool flash = false;
bool startFlash = false;
int flashLength = 0;
unsigned long flashStartTime = 0;
byte flashRed = red;
byte flashGreen = green;
byte flashBlue = blue;
byte flashBrightness = brightness;

// Globals for colorfade
bool colorfade = false;
int currentColor = 0;
// {red, grn, blu}
const byte colors[][3] = {
  {255, 0, 0},
  {0, 255, 0},
  {0, 0, 255},
  {255, 80, 0},
  {163, 0, 255},
  {0, 255, 255},
  {255, 255, 0}
};
const int numColors = 7;


// // store the state of the rgb light (colors, brightness, ...)
// boolean m_rgb_state = false;
// uint8_t m_rgb_brightness = 255;
// uint8_t m_rgb_red = 255;
// uint8_t m_rgb_green = 255;
// uint8_t m_rgb_blue = 255;
//
// boolean m_w1_state = false;
// uint8_t m_w1_brightness = 255;
//
// boolean m_w2_state = false;
// uint8_t m_w2_brightness = 255;

// Publish to $node/$type a description of the topic
HomieNode RGBLEDNode("RGB", "light");
HomieNode W1LEDNode("W1", "light");
HomieNode W2LEDNode("W2", "light");

HomieNode keepAliveNode("keepalive", "keepalive");

// Keepalive tick handler
bool keepAliveTickHandler(const HomieRange& range, const String& value) {
  keepAliveReceived=millis();
  return true;
}

// Keepalive mode
bool keepAliveValueHandler(const HomieRange& range, const String& value) {
  int oldValue = EEpromData.keepAliveValue;
  if (value.toInt() > 0)
  {
    EEpromData.keepAliveValue = value.toInt();
  }
  if (value=="0")
  {
    EEpromData.keepAliveValue = 0;
  }
  if (oldValue!=EEpromData.keepAliveValue)
  {
    EEPROM.put(0, EEpromData);
    EEPROM.commit();
  }
}

//bool processJson(char* message) {
bool processJson(String message) {
  // Plagiarised from https://github.com/corbanmailloux/esp-mqtt-rgb-led/blob/master/mqtt_esp8266_rgb/mqtt_esp8266_rgb.ino#L185
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  RGBLEDNode.setProperty("message").send("JSON Processing");
  RGBLEDNode.setProperty("message").send(message);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    RGBLEDNode.setProperty("message").send("JSON Parsing failed");
    return false;
  }

  if (root.containsKey("state")) {
    if (strcmp(root["state"], on_cmd) == 0) {
      stateOn = true;
    }
    else if (strcmp(root["state"], off_cmd) == 0) {
      stateOn = false;
    }
  }

  // If "flash" is included, treat RGB and brightness differently
  if (root.containsKey("flash") ||
       (root.containsKey("effect") && strcmp(root["effect"], "flash") == 0)) {

    if (root.containsKey("flash")) {
      flashLength = (int)root["flash"] * 1000;
    }
    else {
      flashLength = CONFIG_DEFAULT_FLASH_LENGTH * 1000;
    }

    if (root.containsKey("brightness")) {
      flashBrightness = root["brightness"];
    }
    else {
      flashBrightness = brightness;
    }

    if (root.containsKey("color")) {
      flashRed = root["color"]["r"];
      flashGreen = root["color"]["g"];
      flashBlue = root["color"]["b"];
    }
    else {
      flashRed = red;
      flashGreen = green;
      flashBlue = blue;
    }

    flashRed = map(flashRed, 0, 255, 0, flashBrightness);
    flashGreen = map(flashGreen, 0, 255, 0, flashBrightness);
    flashBlue = map(flashBlue, 0, 255, 0, flashBrightness);

    flash = true;
    startFlash = true;
  }
  else if (root.containsKey("effect") &&
      (strcmp(root["effect"], "colorfade_slow") == 0 || strcmp(root["effect"], "colorfade_fast") == 0)) {
    flash = false;
    colorfade = true;
    currentColor = 0;
    if (strcmp(root["effect"], "colorfade_slow") == 0) {
      transitionTime = CONFIG_COLORFADE_TIME_SLOW;
    }
    else {
      transitionTime = CONFIG_COLORFADE_TIME_FAST;
    }
  }
  else if (colorfade && !root.containsKey("color") && root.containsKey("brightness")) {
    // Adjust brightness during colorfade
    // (will be applied when fading to the next color)
    brightness = root["brightness"];
  }
  else { // No effect
    flash = false;
    colorfade = false;

    if (root.containsKey("color")) {
      red = root["color"]["r"];
      green = root["color"]["g"];
      blue = root["color"]["b"];
    }

    if (root.containsKey("brightness")) {
      brightness = root["brightness"];
    }

    if (root.containsKey("transition")) {
      transitionTime = root["transition"];
    }
    else {
      transitionTime = 0;
    }
  }

  return true;
}

void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["state"] = (stateOn) ? on_cmd : off_cmd;
  JsonObject& color = root.createNestedObject("color");
  color["r"] = red;
  color["g"] = green;
  color["b"] = blue;

  root["brightness"] = brightness;

  if (colorfade) {
    if (transitionTime == CONFIG_COLORFADE_TIME_SLOW) {
      root["effect"] = "colorfade_slow";
    }
    else {
      root["effect"] = "colorfade_fast";
    }
  }
  else {
    root["effect"] = "null";
  }

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  RGBLEDNode.setProperty("message").send("in sendState");
  //RGBLEDNode.setProperty("state").send('{"state": "ON"}');
  //client.publish(light_state_topic, buffer, true);

}

void loopHandler() {
    if (flash) {
      if (startFlash) {
        startFlash = false;
        flashStartTime = millis();
      }

      if ((millis() - flashStartTime) <= flashLength) {
        if ((millis() - flashStartTime) % 1000 <= 500) {
          setColor(flashRed, flashGreen, flashBlue);
        }
        else {
          setColor(0, 0, 0);
          // If you'd prefer the flashing to happen "on top of"
          // the current color, uncomment the next line.
          // setColor(realRed, realGreen, realBlue);
        }
      }
      else {
        flash = false;
        setColor(realRed, realGreen, realBlue);
      }
    }
    else if (colorfade && !inFade) {
      realRed = map(colors[currentColor][0], 0, 255, 0, brightness);
      realGreen = map(colors[currentColor][1], 0, 255, 0, brightness);
      realBlue = map(colors[currentColor][2], 0, 255, 0, brightness);
      currentColor = (currentColor + 1) % numColors;
      startFade = true;
    }

    if (startFade) {
      // If we don't want to fade, skip it.
      if (transitionTime == 0) {
        setColor(realRed, realGreen, realBlue);

        redVal = realRed;
        grnVal = realGreen;
        bluVal = realBlue;

        startFade = false;
      }
      else {
        loopCount = 0;
        stepR = calculateStep(redVal, realRed);
        stepG = calculateStep(grnVal, realGreen);
        stepB = calculateStep(bluVal, realBlue);

        inFade = true;
      }
    }

    if (inFade) {
      startFade = false;
      unsigned long now = millis();
      if (now - lastLoop > transitionTime) {
        if (loopCount <= 1020) {
          lastLoop = now;

          redVal = calculateVal(stepR, redVal, loopCount);
          grnVal = calculateVal(stepG, grnVal, loopCount);
          bluVal = calculateVal(stepB, bluVal, loopCount);

          setColor(redVal, grnVal, bluVal); // Write current values to LED pins

          Serial.print("Loop count: ");
          Serial.println(loopCount);
          loopCount++;
        }
        else {
          inFade = false;
        }
      }
    }
}

bool RGBLEDStateHandler(const HomieRange& range, const String& value) {

  if (!processJson(value)) {
    return 0;
  }

  if (stateOn) {
    // Update lights
    realRed = map(red, 0, 255, 0, brightness);
    realGreen = map(green, 0, 255, 0, brightness);
    realBlue = map(blue, 0, 255, 0, brightness);
  }
  else {
    realRed = 0;
    realGreen = 0;
    realBlue = 0;
  }

  startFade = true;
  inFade = false; // Kill the current fade

  sendState();

}

bool W1LEDStateHandler(const HomieRange& range, const String& value) {
}

bool W2LEDStateHandler(const HomieRange& range, const String& value) {
}

bool TestNodeStateHandler(const HomieRange& range, const String& value) {}

void onHomieEvent(const HomieEvent& event) {
}

void setupHandler() {
        RGBLEDNode.setProperty("state").send("foo");
        W1LEDNode.setProperty("state").send("foo");
        W2LEDNode.setProperty("state").send("foo");
}

void setup() {

        Homie_setFirmware(FW_NAME, FW_VERSION);
        Homie.setLedPin(RED_PIN, HIGH);
        Homie.setSetupFunction(setupHandler);
        Homie.setLoopFunction(loopHandler);

        RGBLEDNode.advertise("light").settable(RGBLEDStateHandler);
        W1LEDNode.advertise("light").settable(W1LEDStateHandler);
        W2LEDNode.advertise("light").settable(W2LEDStateHandler);
        keepAliveNode.advertise("tick").settable(keepAliveTickHandler);
        keepAliveNode.advertise("keepAliveValue").settable(keepAliveValueHandler);

        Homie.onEvent(onHomieEvent);
        Homie.setup();
}

void loop() {
        Homie.loop();
}

void setColor(int inR, int inG, int inB) {
  if (led_invert) {
    inR = (255 - inR);
    inG = (255 - inG);
    inB = (255 - inB);
  }

  RGBLEDNode.setProperty("message").send("setting colours");
  // FIXME
  // analogWrite(redPin, inR);
  // analogWrite(greenPin, inG);
  // analogWrite(bluePin, inB);

  Serial.println("Setting LEDs:");
  Serial.print("r: ");
  Serial.print(inR);
  Serial.print(", g: ");
  Serial.print(inG);
  Serial.print(", b: ");
  Serial.println(inB);
}

// From https://www.arduino.cc/en/Tutorial/ColorCrossfader
/* BELOW THIS LINE IS THE MATH -- YOU SHOULDN'T NEED TO CHANGE THIS FOR THE BASICS
*
* The program works like this:
* Imagine a crossfade that moves the red LED from 0-10,
*   the green from 0-5, and the blue from 10 to 7, in
*   ten steps.
*   We'd want to count the 10 steps and increase or
*   decrease color values in evenly stepped increments.
*   Imagine a + indicates raising a value by 1, and a -
*   equals lowering it. Our 10 step fade would look like:
*
*   1 2 3 4 5 6 7 8 9 10
* R + + + + + + + + + +
* G   +   +   +   +   +
* B     -     -     -
*
* The red rises from 0 to 10 in ten steps, the green from
* 0-5 in 5 steps, and the blue falls from 10 to 7 in three steps.
*
* In the real program, the color percentages are converted to
* 0-255 values, and there are 1020 steps (255*4).
*
* To figure out how big a step there should be between one up- or
* down-tick of one of the LED values, we call calculateStep(),
* which calculates the absolute gap between the start and end values,
* and then divides that gap by 1020 to determine the size of the step
* between adjustments in the value.
*/
int calculateStep(int prevValue, int endValue) {
    int step = endValue - prevValue; // What's the overall gap?
    if (step) {                      // If its non-zero,
        step = 1020/step;            //   divide by 1020
    }

    return step;
}

/* The next function is calculateVal. When the loop value, i,
*  reaches the step size appropriate for one of the
*  colors, it increases or decreases the value of that color by 1.
*  (R, G, and B are each calculated separately.)
*/
int calculateVal(int step, int val, int i) {
    if ((step) && i % step == 0) { // If step is non-zero and its time to change a value,
        if (step > 0) {              //   increment the value if step is positive...
            val += 1;
        }
        else if (step < 0) {         //   ...or decrement it if step is negative
            val -= 1;
        }
    }

    // Defensive driving: make sure val stays in the range 0-255
    if (val > 255) {
        val = 255;
    }
    else if (val < 0) {
        val = 0;
    }

    return val;
}
