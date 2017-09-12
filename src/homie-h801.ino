#include <Homie.h>

#define FW_NAME "homie-h801"
#define FW_VERSION "2.0.0"

/* Magic sequence for Autodetectable Binary Upload */
const char *__FLAGGED_FW_NAME = "\xbf\x84\xe4\x13\x54" FW_NAME "\x93\x44\x6b\xa7\x75";
const char *__FLAGGED_FW_VERSION = "\x6a\x3f\x3e\x0e\xe1" FW_VERSION "\xb0\x30\x48\xd4\x1a";
/* End of magic sequence for Autodetectable Binary Upload */

float temperature;
float humidity;
const int TEMP_INTERVAL = 10;                   // seconds
unsigned long last_temp_sent = 0;

HomieNode temperatureNode("temperature", "temperature");
HomieNode humidityNode("humidity", "humidity");

HomieNode RGBLed("RGB", "value");
HomieNode W1LED("W1", "value");
HomieNode W2LED("W2", "value");


void loopHandler() {

        if (millis() - last_temp_sent >= TEMP_INTERVAL * 1000UL
            || last_temp_sent == 0)
        {

                //sht30.get();
                //temperature = sht30.cTemp;
                //humidity = sht30.humidity;

                Serial.print("Temperature: ");
                Serial.print(temperature);
                Serial.println(" °C");

                Serial.print("Humidity: ");
                Serial.print(humidity);
                Serial.println(" %");

                if (temperatureNode.setProperty("degrees").send(String(temperature))) {
                            last_temp_sent = millis();
                }
                //Homie.setNodeProperty(humidityNode, "percent", String(humidity), true);
                humidityNode.setProperty("percent").send(String(humidity));
                delay(1000);
        }

}

void setupHandler() {
        temperatureNode.setProperty("unit").send("C");
        humidityNode.setProperty("unit").send("%");
}

void setup() {

        Homie_setFirmware(FW_NAME, FW_VERSION);

        Homie.setSetupFunction(setupHandler);
        Homie.setLoopFunction(loopHandler);

        Homie.setup();
}

void loop() {
        Homie.loop();
}
