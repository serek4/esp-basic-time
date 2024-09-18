#include <esp-basic-time.h>
#include <esp-basic-wifi.h>

#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASS "your-wifi-password"

#define TIMEZONE 1    // Central European Time (Europe/Warsaw)

BasicWiFi wifi(WIFI_SSID, WIFI_PASS);
BasicTime NTPclient(TIMEZONE);

u_long loopDelay = 0;

void setup() {
	Serial.begin(115200);
	Serial.println();
	wifi.onGotIP([](GOT_IP_HANDLER_ARGS) { NTPclient.setNetworkReady(true); });
	wifi.onDisconnected([](DISCONNECTED_HANDLER_ARGS) { NTPclient.setNetworkReady(false); });
	wifi.setup();
	NTPclient.setup();
	if (wifi.waitForConnection() == BasicWiFi::wifi_got_ip) {
		NTPclient.waitForNTP();
	}
	Serial.println("setup done!");
}

void loop() {
	NTPclient.handle();
	if (millis() - loopDelay >= 30000) {
		loopDelay = millis();
		Serial.println(NTPclient.dateTimeString());
	}
}
