#include "esp-basic-time.hpp"

AsyncUDP NTPudp;

String BasicTime::_NTPServerAddress = DEFAULT_NTP_SERVER_ADDRESS;
uint16_t BasicTime::_NTPServerPort = DEFAULT_NTP_PORT;
int BasicTime::_timezone = DEFAULT_TIMEZONE;
bool BasicTime::_waitingForNTP = false;
u_long BasicTime::_requestSendedAt;
bool BasicTime::_gotNTPserverIP = false;
IPAddress BasicTime::_NTPServerIP;
time_t BasicTime::_NTPSyncInterval = NTP_NORMAL_SYNC_INTERVAL;           // timeSet sync interval
time_t BasicTime::_NTPReSyncInterval = NTP_SHORT_SYNC_INTERVAL;          // timeNeedsSync sync interval
time_t BasicTime::_NTPnoSyncInterval = NTP_NO_TIME_SET_SYNC_INTERVAL;    // timeNotSet sync interval
void (*BasicTime::_logger)(String logLevel, String msg) = nullptr;

BasicTime::BasicTime(int timezone)
    : _connectingIndicator(nullptr) {
	_timezone = timezone;
}
BasicTime::BasicTime(const char* NTP_server_address, int timezone)
    : _connectingIndicator(nullptr) {
	_NTPServerAddress = NTP_server_address;
	_timezone = timezone;
}
BasicTime::BasicTime(const char* NTP_server_address, int NTP_server_port, int timezone)
    : _connectingIndicator(nullptr) {
	_NTPServerAddress = NTP_server_address;
	_NTPServerPort = NTP_server_port;
	_timezone = timezone;
}
void BasicTime::addLogger(void (*logger)(String logLevel, String msg)) {
	_logger = logger;
}
void BasicTime::setup() {
	setSyncInterval(_NTPReSyncInterval);
	setSyncProvider(requestNtpTime);
	NTPudp.onPacket(_NTPrequestCallback);
}
void BasicTime::setWaitingFunction(void (*connectingIndicator)(u_long onTime, u_long offTime)) {
	_connectingIndicator = connectingIndicator;
}
bool BasicTime::waitForNTP(int waitTime) {
	u_long startWaitingAt = millis();
	BASIC_TIME_PRINT("Waiting for NTP connection");
	while (timeStatus() != timeSet) {
		BASIC_TIME_PRINT(".");
		handle();
		if (_connectingIndicator == nullptr) {
			delay(TIME_NO_BLINK);
		} else {
			(*_connectingIndicator)(TIME_BLINK_ON, TIME_BLINK_OFF);
		}
		if (millis() - startWaitingAt > waitTime * 1000) {
			BASIC_TIME_PRINTLN("Can't connect to NTP server!");
			return false;
			break;
		}
	}
	return true;
}
//request time from NTP server
time_t BasicTime::requestNtpTime() {
	if (_gotNTPserverIP) {
		BASIC_TIME_PRINTLN("Syncing time with NTP");
		_sendNTPpacket(_NTPServerIP, _NTPServerPort);
	}
	return 0;
}
// response from NTP server
void BasicTime::_NTPrequestCallback(AsyncUDPPacket& packet) {    // response packet
	// convert four bytes starting at location 40 to a long integer
	u_long secsSince1900 = (unsigned long)packet.data()[40] << 24;
	secsSince1900 |= (unsigned long)packet.data()[41] << 16;
	secsSince1900 |= (unsigned long)packet.data()[42] << 8;
	secsSince1900 |= (unsigned long)packet.data()[43];
	setTime(secsSince1900 - 2208988800UL);    // unix time - 70 years
	_NTPsyncInterval("Time synced with NTP server\nNext sync in ");
	NTPudp.close();
	_waitingForNTP = false;
}
// sync time response checker
void BasicTime::handle() {
	if (!_gotNTPserverIP && WiFi.isConnected()) {    // waiting for WiFi connection to get NTP server IP
		_gotNTPserverIP = WiFi.hostByName(_NTPServerAddress.c_str(), _NTPServerIP);
		if (timeStatus() != timeSet) {
			requestNtpTime();
		}
	}
	if (_waitingForNTP && millis() - _requestSendedAt >= NTP_TIMEOUT) {    // NTP packet sended waiting for response
		NTPudp.close();
		_NTPsyncInterval("No response from NTP server\nNext sync in ");
		_waitingForNTP = false;
	}
}
//sending UDP packet to NTP server
bool BasicTime::_sendNTPpacket(IPAddress& address, uint16_t port) {    // send an NTP request to the time server
	if (NTPudp.connect(address, port)) {
		byte packetBuffer[NTP_PACKET_SIZE];    // buffer to hold outgoing packet
		memset(packetBuffer, 0, NTP_PACKET_SIZE);
		packetBuffer[0] = 0b11100011;    // LI, Version, Mode
		packetBuffer[1] = 0;             // Stratum, or type of clock
		packetBuffer[2] = 6;             // Polling Interval
		packetBuffer[3] = 0xEC;          // Peer Clock Precision
		                                 // 8 bytes of zero for Root Delay & Root Dispersion
		packetBuffer[12] = 49;
		packetBuffer[13] = 0x4E;
		packetBuffer[14] = 49;
		packetBuffer[15] = 52;
		NTPudp.write(packetBuffer, NTP_PACKET_SIZE);
		setSyncInterval(_NTPReSyncInterval);    // set short sync interval when waiting for server response
		_requestSendedAt = millis();
		_waitingForNTP = true;
		return true;
	} else {
		_NTPsyncInterval("Can't connect to NTP server\nNext sync in ");
		return false;
	}
}
void BasicTime::_NTPsyncInterval(const char* message) {
	String logMessage = message;
	switch (timeStatus()) {
		case timeNotSet:
			setSyncInterval(_NTPnoSyncInterval);    // set very short sync interval if time was never synced
			logMessage += (String)(_NTPnoSyncInterval / SECS_PER_MIN) + "m";
			break;
		case timeNeedsSync:
			setSyncInterval(_NTPReSyncInterval);    // set short sync interval if time was set at least once
			logMessage += (String)(_NTPReSyncInterval / SECS_PER_HOUR) + "h";
			break;
		case timeSet:
			if (millis() < NTP_FIRST_SYNC_INTERVAL_THRESHOLD) {
				setSyncInterval(_NTPnoSyncInterval);    // set very short sync interval on first sync
				logMessage += (String)(_NTPnoSyncInterval / SECS_PER_MIN) + "m";
			} else {
				setSyncInterval(_NTPSyncInterval);    // set long sync interval on successful sync
				logMessage += (String)(_NTPSyncInterval / SECS_PER_HOUR) + "h";
			}
			break;

		default:
			break;
	}
	BASIC_TIME_PRINTLN(logMessage);
	logMessage.replace("\n", ". ");
	if (_logger != nullptr) { (*_logger)("ntp", logMessage); }
}
//converting timestamp to human readable date string (RRRR-MM-DD)
String BasicTime::dateString(time_t timestamp) {
	String date = "timeNotSet";
	if (timeStatus() != timeNotSet) {
		if (isDST(timestamp)) timestamp += 1 * SECS_PER_HOUR;    // summer time +1h
		timestamp += _timezone * SECS_PER_HOUR;                  // time zone + 1h
		date = (String)(year(timestamp));
		date += "-";
		if (month(timestamp) < 10) date += "0";
		date += (String)(month(timestamp));
		date += "-";
		if (day(timestamp) < 10) date += "0";
		date += (String)(day(timestamp));
	}
	return date;
}
//converting timestamp to human readable time string (hh:mm:ss)[24h]
String BasicTime::timeString(time_t timestamp) {
	String time = "";
	if (timeStatus() != timeNotSet) {
		if (isDST(timestamp)) timestamp += 1 * SECS_PER_HOUR;    // summer time + 1h
		timestamp += _timezone * SECS_PER_HOUR;                  // time zone + 1h
	}
	if (hour(timestamp) < 10) time += "0";
	time += (String)hour(timestamp);
	time += ":";
	if (minute(timestamp) < 10) time += "0";
	time += (String)(minute(timestamp));
	time += ":";
	if (second(timestamp) < 10) time += "0";
	time += (String)(second(timestamp));
	if (timeStatus() == timeNeedsSync) {
		time += "*";
	}
	return time;
}
//converting timestamp to human readable date time string (RRRR-MM-DD hh:mm:ss)[24h]
String BasicTime::dateTimeString(time_t timestamp) {
	return dateString(timestamp) + " " + timeString(timestamp);
}
//European Central Summer Time (CEST) check
bool BasicTime::isDST(time_t timestamp) {
	timestamp += _timezone * SECS_PER_HOUR;                   // time zone offset
	if (month(timestamp) >= 10 || month(timestamp) <= 3) {    // winter time/standard time from october to march
		if (month(timestamp) == 3 || month(timestamp) == 10) {
			int previousSunday = day(timestamp) - weekday(timestamp);
			if (previousSunday >= LAST_SUNDAY_OF_THE_MONTH) {
				if (weekday(timestamp) == 1) {    // last sunday of month
					if (hour(timestamp) < 2) {    // until 2:00 (2:00->3:00 in march and 3:00->2:00 in october)
						return month(timestamp) == 3 ? false : true;
					}
				}
				return month(timestamp) == 3 ? true : false;
			}
			return month(timestamp) == 3 ? false : true;
		}
		return false;
	}
	return true;
}
