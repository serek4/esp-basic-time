#pragma once

#ifdef ARDUINO_ARCH_ESP32
#include <AsyncUDP.h>
#include <WiFi.h>
#elif defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncUDP.h>
#endif
#include <TimeLib.h>

// #define BASIC_TIME_DEBUG
// debug printing macros
// clang-format off
#ifdef BASIC_TIME_DEBUG
#define DEBUG_PRINTER Serial
#define BASIC_TIME_PRINT(...) { DEBUG_PRINTER.print(__VA_ARGS__); }
#define BASIC_TIME_PRINTLN(...) { DEBUG_PRINTER.println(__VA_ARGS__); }
#define BASIC_TIME_PRINTF(...) { DEBUG_PRINTER.printf(__VA_ARGS__); }
#else
#define BASIC_TIME_PRINT(...) {}
#define BASIC_TIME_PRINTLN(...) {}
#define BASIC_TIME_PRINTF(...) {}
#endif
// clang-format on

#define TIME_BLINK_ON 100
#define TIME_BLINK_OFF 150
#define TIME_NO_BLINK TIME_BLINK_ON + TIME_BLINK_OFF

#define DEFAULT_NTP_SERVER_ADDRESS "pool.ntp.org"
#define DEFAULT_NTP_PORT 123
#define DEFAULT_TIMEZONE 0    // UTC
#define NTP_PACKET_SIZE 48    // NTP time stamp is in the first 48 bytes of the message
#define NTP_TIMEOUT 1500
#define LAST_SUNDAY_OF_THE_MONTH 25 - 1    // last Sunday of march and october (or other 31-day month)
#define NTP_NORMAL_SYNC_INTERVAL 12 * SECS_PER_HOUR
#define NTP_SHORT_SYNC_INTERVAL 1 * SECS_PER_HOUR
#define NTP_NO_TIME_SET_SYNC_INTERVAL 5 * SECS_PER_MIN
#define NTP_FIRST_SYNC_INTERVAL_THRESHOLD NTP_NO_TIME_SET_SYNC_INTERVAL * 1000

class BasicTime {
  public:
	BasicTime(int timezone);
	BasicTime(const char* NTP_server_address, int timezone);
	BasicTime(const char* NTP_server_address, int NTP_server_port, int timezone);

	void addLogger(void (*logger)(String logLevel, String msg));
	void setup();
	void setWaitingFunction(void (*connectingIndicator)(u_long onTime, u_long offTime));
	bool waitForNTP(int waitTime = 10);
	void handle();
	static time_t requestNtpTime();
	static String dateString(time_t timeStamp = now());
	static String timeString(time_t timeStamp = now());
	static String dateTimeString(time_t timeStamp = now());
	static bool isDST(time_t timeStamp = now());

  private:
	static String _NTPServerAddress;
	static IPAddress _NTPServerIP;
	static uint16_t _NTPServerPort;
	static int _timezone;
	static bool _waitingForNTP;
	static u_long _requestSendedAt;
	static bool _gotNTPserverIP;
	static time_t _NTPSyncInterval;      // timeSet sync interval
	static time_t _NTPReSyncInterval;    // timeNeedsSync sync interval
	static time_t _NTPnoSyncInterval;    // timeNotSet sync interval

	void (*_connectingIndicator)(u_long onTime, u_long offTime);
	static void (*_logger)(String logLevel, String msg);
	static void _NTPrequestCallback(AsyncUDPPacket& packet);
	static bool _sendNTPpacket(IPAddress& address, uint16_t port);
	static void _NTPsyncInterval(const char* message);
};
