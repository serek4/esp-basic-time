#include <esp-basic-time.h>
#include <esp-basic-wifi.h>

#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASS "your-wifi-password"

#define TIMEZONE 1    // Central European Time (Europe/Warsaw)

BasicWiFi wifi(WIFI_SSID, WIFI_PASS);
BasicTime NTPclient(TIMEZONE);

long loopDelay = -25000;

void setup() {
	Serial.begin(115200);
	Serial.println();
	wifi.setup();
	NTPclient.setup();
	if (wifi.waitForConnection() == BasicWiFi::connected) {
		NTPclient.waitForNTP();
	}
	Serial.println("setup done!");
}

void loop() {
	NTPclient.handle();
	if (millis() - loopDelay >= 30000) {
		Serial.println(NTPclient.dateTimeString());
		loopDelay = millis();
	}
	delay(10);
}
