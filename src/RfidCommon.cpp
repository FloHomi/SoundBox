#include <Arduino.h>
#include "settings.h"

#include "AudioPlayer.h"
#include "Cmd.h"
#include "Common.h"
#include "Log.h"
#include "MemX.h"
#include "Mqtt.h"
#include "Queues.h"
#include "Rfid.h"
#include "System.h"
#include "Web.h"
#include "String.h"

unsigned long Rfid_LastRfidCheckTimestamp = 0;
char gCurrentId[ID_STRING_SIZE] = ""; // No crap here as otherwise it could be shown in GUI
#ifdef DONT_ACCEPT_SAME_RFID_TWICE_ENABLE
char gOldRfidTagId[ID_STRING_SIZE] = "X"; // Init with crap
#endif

// check if we have RFID-reader enabled
#if defined(RFID_READER_TYPE_MFRC522_SPI) || defined(RFID_READER_TYPE_MFRC522_I2C) || defined(RFID_READER_TYPE_PN5180)
	#define RFID_READER_ENABLED 1
#endif

// Tries to lookup RFID-tag-string in NVS and extracts parameter from it if found
void Rfid_PreferenceLookupHandler(void) {
#if defined(RFID_READER_ENABLED)
	char newId[ID_STRING_SIZE];
	char _file[255];
	uint32_t _lastPlayPos = 0;
	uint16_t _trackLastPlayed = 0;
	uint32_t _playMode = 1;

	BaseType_t newQueueReceived = xQueueReceive(gRfidCardQueue, &newId, 0) == pdPASS;
	if(newQueueReceived != pdPASS) { // If no new card was read, try to read button-queue, just one queue at a time
    	newQueueReceived = xQueueReceive(gButtonIdQueue, &newId, 0) == pdPASS;
	}

	if (newQueueReceived) {
		System_UpdateActivityTimer();
		strncpy(gCurrentId, newId, ID_STRING_SIZE - 1);
		Log_Printf(LOGLEVEL_INFO, "%s: %s", rfidTagReceived, gCurrentId);
		Web_SendWebsocketData(0, 10); // Push new rfidTagId to all websocket-clients
		String idStr = "-1";
		if (gPrefsRfid.isKey(gCurrentId)) {
			idStr = gPrefsRfid.getString(gCurrentId, "-1"); // Try to lookup rfidId in NVS
		}
		if (!idStr.compareTo("-1")) {
			Log_Println(rfidTagUnknownInNvs, LOGLEVEL_ERROR);
			System_IndicateError();
	#ifdef DONT_ACCEPT_SAME_RFID_TWICE_ENABLE
			strncpy(gOldRfidTagId, gCurrentId, ID_STRING_SIZE - 1); // Even if not found in NVS: accept it as card last applied
	#endif
			// allow to escape from bluetooth mode with an unknown card, switch back to normal mode
			System_SetOperationMode(OPMODE_NORMAL);
			return;
		}

		char *token;
		uint8_t i = 1;
		token = strtok((char *) idStr.c_str(), stringDelimiter);
		while (token != NULL) { // Try to extract data from string after lookup
			if (i == 1) {
				strncpy(_file, token, sizeof(_file) / sizeof(_file[0]));
			} else if (i == 2) {
				_lastPlayPos = strtoul(token, NULL, 10);
			} else if (i == 3) {
				_playMode = strtoul(token, NULL, 10);
			} else if (i == 4) {
				_trackLastPlayed = strtoul(token, NULL, 10);
			}
			i++;
			token = strtok(NULL, stringDelimiter);
		}

		if (i != 5) {
			Log_Println(errorOccuredNvs, LOGLEVEL_ERROR);
			System_IndicateError();
		} else {
			// Only pass file to queue if strtok revealed 3 items
			if (_playMode >= 100) {
				// Modification-cards can change some settings (e.g. introducing track-looping or sleep after track/playlist).
				Cmd_Action(_playMode);
			} else {
	#ifdef DONT_ACCEPT_SAME_RFID_TWICE_ENABLE
				if (strncmp(gCurrentId, gOldRfidTagId, ID_STRING_SIZE - 1) == 0) {
					Log_Printf(LOGLEVEL_ERROR, dontAccepctSameRfid, gCurrentId);
					// System_IndicateError(); // Enable to have shown error @neopixel every time
					return;
				} else {
					strncpy(gOldRfidTagId, gCurrentId, ID_STRING_SIZE - 1);
				}
	#endif
	#ifdef MQTT_ENABLE
				publishMqtt(topicRfidState, gCurrentId, false);
	#endif

	#ifdef BLUETOOTH_ENABLE
				// if music rfid was read, go back to normal mode
				if (System_GetOperationMode() == OPMODE_BLUETOOTH_SINK) {
					System_SetOperationMode(OPMODE_NORMAL);
				}
	#endif

				AudioPlayer_TrackQueueDispatcher(_file, _lastPlayPos, _playMode, _trackLastPlayed);
			}
		}
	}
#endif
}

#ifdef DONT_ACCEPT_SAME_RFID_TWICE_ENABLE
void Rfid_ResetOldRfid() {
	strncpy(gOldRfidTagId, "X", cardIdStringSize - 1);
}
#endif

#if defined(RFID_READER_ENABLED)
extern TaskHandle_t rfidTaskHandle;
#endif

void Rfid_TaskPause(void) {
#if defined(RFID_READER_ENABLED)
	vTaskSuspend(rfidTaskHandle);
#endif
}
void Rfid_TaskResume(void) {
#if defined(RFID_READER_ENABLED)
	vTaskResume(rfidTaskHandle);
#endif
}
