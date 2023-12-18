#pragma once


constexpr uint8_t cardIdSize = 4u;
constexpr uint8_t CARD_ID_STR_SIZE = (cardIdSize * 3u) + 1u;
constexpr uint8_t BUTTON_ID_STR_SIZE = 12; //sizeof("BTNxx_SHORT") + 1u;


#if CARD_ID_STR_SIZE > BUTTON_ID_STR_SIZE
	constexpr uint8_t ID_STRING_SIZE = CARD_ID_STR_SIZE;
#else
	constexpr uint8_t ID_STRING_SIZE = BUTTON_ID_STR_SIZE;
#endif

extern char gCurrentId[ID_STRING_SIZE];

#ifndef PAUSE_WHEN_RFID_REMOVED
	#ifdef DONT_ACCEPT_SAME_RFID_TWICE // ignore feature silently if PAUSE_WHEN_RFID_REMOVED is active
		#define DONT_ACCEPT_SAME_RFID_TWICE_ENABLE
	#endif
#endif

#ifdef DONT_ACCEPT_SAME_RFID_TWICE_ENABLE
void Rfid_ResetOldRfid(void);
#endif

void Rfid_Init(void);
void Rfid_Cyclic(void);
void Rfid_Exit(void);
void Rfid_TaskPause(void);
void Rfid_TaskResume(void);
void Rfid_WakeupCheck(void);
void Rfid_PreferenceLookupHandler(void);
