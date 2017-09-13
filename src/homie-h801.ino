#include <Homie.h>
#include <EEPROM.h>

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

// store the state of the rgb light (colors, brightness, ...)
boolean m_rgb_state = false;
uint8_t m_rgb_brightness = 255;
uint8_t m_rgb_red = 255;
uint8_t m_rgb_green = 255;
uint8_t m_rgb_blue = 255;

boolean m_w1_state = false;
uint8_t m_w1_brightness = 255;

boolean m_w2_state = false;
uint8_t m_w2_brightness = 255;

HomieNode RGBLEDNode("RGB", "value");
HomieNode W1LEDNode("W1", "value");
HomieNode W2LEDNode("W2", "value");

HomieNode TestNode("test", "value");

HomieNode keepAliveNode("keepalive", "keepalive");

// Keepliave tick handler
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

void RGBLEDStateHandler(const HomieRange& range, const String& value) {
}

void W1LEDStateHandler(const HomieRange& range, const String& value) {
}

void W2LEDStateHandler(const HomieRange& range, const String& value) {
}

void TestNodeStateHandler(const HomieRange& range, const String& value) {}

void onHomieEvent(const HomieEvent& event) {
}

void loopHandler() {
}

void setupHandler() {
        TestNode.setProperty("foo").send("bar");
        //RGBLEDNode.setProperty("unit").send("C");
        //W1LEDNode.setProperty("unit").send("%");
        //W2LEDNode.setProperty("light").send("foo");
}

void setup() {

        Homie_setFirmware(FW_NAME, FW_VERSION);
        Homie.setLedPin(RED_PIN, HIGH);
        Homie.setSetupFunction(setupHandler);
        Homie.setLoopFunction(loopHandler);

        TestNode.advertise("testnode").settable(TestNodeStateHandler);
        // RGBLED.advertise("RGBLEDState").settable(RGBLEDStateHandler);
        // W1LEDNode.advertise("W1LEDState").settable(W1LEDStateHandler);
        // W2LED.advertise("W2LEDState").settable(W2LEDStateHandler);
        keepAliveNode.advertise("tick").settable(keepAliveTickHandler);
        keepAliveNode.advertise("keepAliveValue").settable(keepAliveValueHandler);

        //Homie.onEvent(onHomieEvent);
        Homie.setup();
}

void loop() {
        Homie.loop();
}
