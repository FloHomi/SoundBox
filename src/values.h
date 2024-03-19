#ifndef VALUES_H
#define VALUES_H


// Operation Mode
typedef enum {
    OPMODE_NORMAL = 0, // Normal mode
    OPMODE_BLUETOOTH_SINK = 1, // Bluetooth sink mode. Player acts as as bluetooth speaker. WiFi is deactivated. Music from SD and webstreams can't be played.
    OPMODE_BLUETOOTH_SOURCE = 2 // Bluetooth sourcemode. Player sennds audio to bluetooth speaker/headset. WiFi is deactivated. Music from SD and webstreams can't be played.
} OperationModeType;

// Track-Control
typedef enum {
    NO_ACTION = 0, // Dummy to unset track-control-command
    STOP = 1, // Stop play
    PLAY = 2, // Start play (currently not used)
    PAUSEPLAY = 3, // Pause/play
    NEXTTRACK = 4, // Next track of playlist
    PREVIOUSTRACK = 5, // Previous track of playlist
    FIRSTTRACK = 6, // First track of playlist
    LASTTRACK = 7 // Last track of playlist
} TrackControlType;

// Playmodes
typedef enum {
    NO_PLAYLIST = 0, // If no playlist is active
    SINGLE_TRACK = 1, // Play a single track
    SINGLE_TRACK_LOOP = 2, // Play a single track in infinite-loop
    SINGLE_TRACK_OF_DIR_RANDOM = 12, // Play a single track of a directory and fall asleep subsequently
    AUDIOBOOK = 3, // Single track, can save last play-position
    AUDIOBOOK_LOOP = 4, // Single track as infinite-loop, can save last play-position
    ALL_TRACKS_OF_DIR_SORTED = 5, // Play all files of a directory (alph. sorted)
    ALL_TRACKS_OF_DIR_RANDOM = 6, // Play all files of a directory (randomized)
    ALL_TRACKS_OF_DIR_SORTED_LOOP = 7, // Play all files of a directory (alph. sorted) in infinite-loop
    ALL_TRACKS_OF_DIR_RANDOM_LOOP = 9, // Play all files of a directory (randomized) in infinite-loop
    RANDOM_SUBDIRECTORY_OF_DIRECTORY = 13, // Picks a random subdirectory from a given directory and do ALL_TRACKS_OF_DIR_SORTED
    RANDOM_SUBDIRECTORY_OF_DIRECTORY_ALL_TRACKS_OF_DIR_RANDOM = 14, // Picks a random subdirectory from a given directory and do ALL_TRACKS_OF_DIR_RANDOM
    WEBSTREAM = 8, // Play webradio-stream
    LOCAL_M3U = 11, // Plays items (webstream or files) with addresses/paths from a local m3u-file
    PLAYER_BUSY = 10 // Used if playlist is created
} PlayModeType;

// RFID-modifcation-types
typedef enum {
    CMD_NOTHING = 0, // Do Nothing

    //settings
    CMD_LOCK_BUTTONS_MOD = 100, // Locks all buttons and rotary encoder
    CMD_SLEEP_TIMER_MOD_15 = 101, // Puts uC into deepsleep after 15 minutes + LED-DIMM
    CMD_SLEEP_TIMER_MOD_30 = 102, // Puts uC into deepsleep after 30 minutes + LED-DIMM
    CMD_SLEEP_TIMER_MOD_60 = 103, // Puts uC into deepsleep after 60 minutes + LED-DIMM
    CMD_SLEEP_TIMER_MOD_120 = 104, // Puts uC into deepsleep after 120 minutes + LED-DIMM
    CMD_SLEEP_AFTER_END_OF_TRACK = 105, // Puts uC into deepsleep after track is finished + LED-DIMM
    CMD_SLEEP_AFTER_END_OF_PLAYLIST = 106, // Puts uC into deepsleep after playlist is finished + LED-DIMM
    CMD_SLEEP_AFTER_5_TRACKS = 107, // Puts uC into deepsleep after five tracks + LED-DIMM
    CMD_REPEAT_PLAYLIST = 110, // Changes active playmode to endless-loop (for a playlist)
    CMD_REPEAT_TRACK = 111, // Changes active playmode to endless-loop (for a single track)
    CMD_DIMM_LEDS_NIGHTMODE = 120, // Changes LED-brightness
    CMD_TOGGLE_WIFI_STATUS = 130, // Toggles WiFi-status
    CMD_TOGGLE_BLUETOOTH_SINK_MODE = 140, // Toggles Normal/Bluetooth sink Mode
    CMD_TOGGLE_BLUETOOTH_SOURCE_MODE = 141, // Toggles Normal/Bluetooth source Mode
    CMD_TOGGLE_MODE					= 142, // Toggles Normal => Bluetooth sink => Bluetooth source => Normal Mode
    CMD_ENABLE_FTP_SERVER = 150, // Enables FTP-server
    CMD_TELL_IP_ADDRESS = 151, // Command: ESPuino announces its IP-address via speech
    CMD_TELL_CURRENT_TIME = 152, // Command: ESPuino announces current time via speech

    //player
    CMD_PLAYPAUSE = 170, // Command: play/pause
    CMD_PREVTRACK = 171, // Command: previous track
    CMD_NEXTTRACK = 172, // Command: next track
    CMD_FIRSTTRACK = 173, // Command: first track
    CMD_LASTTRACK = 174, // Command: last track
    CMD_VOLUMEINIT = 175, // Command: set volume to initial value
    CMD_VOLUMEUP = 176, // Command: increase volume by 1
    CMD_VOLUMEDOWN = 177, // Command: lower volume by 1
    CMD_MEASUREBATTERY = 178, // Command: Measure battery-voltage
    CMD_SLEEPMODE = 179, // Command: Go to deepsleep
    CMD_SEEK_FORWARDS = 180, // Command: jump forwards (time period to jump (in seconds) is configured via settings.h: jumpOffset)
    CMD_SEEK_BACKWARDS = 181, // Command: jump backwards (time period to jump (in seconds) is configured via settings.h: jumpOffset)
    CMD_STOP = 182, // Command: stops playback
    CMD_RESTARTSYSTEM = 183, // Command: restart System

    //buttons
    CMD_BUTTON_0_ID_SHORT = 200, // Command: Button 0 short press
    CMD_BUTTON_0_ID_LONG = 201, // Command: Button 0 long press
    CMD_BUTTON_1_ID_SHORT = 202, // Command: Button 1 short press
    CMD_BUTTON_1_ID_LONG = 203, // Command: Button 1 long press
    CMD_BUTTON_2_ID_SHORT = 204, // Command: Button 2 short press
    CMD_BUTTON_2_ID_LONG = 205, // Command: Button 2 long press
    CMD_BUTTON_3_ID_SHORT = 206, // Command: Button 3 short press
    CMD_BUTTON_3_ID_LONG = 207, // Command: Button 3 long press
    CMD_BUTTON_4_ID_SHORT = 208, // Command: Button 4 short press
    CMD_BUTTON_4_ID_LONG = 209, // Command: Button 4 long press
    CMD_BUTTON_5_ID_SHORT = 210, // Command: Button 5 short press
    CMD_BUTTON_5_ID_LONG = 211, // Command: Button 5 long press
    CMD_BUTTON_6_ID_SHORT = 212, // Command: Button 6 short press
    CMD_BUTTON_6_ID_LONG = 213, // Command: Button 6 long press
    CMD_BUTTON_7_ID_SHORT = 214, // Command: Button 7 short press
    CMD_BUTTON_7_ID_LONG = 215, // Command: Button 7 long press
    CMD_BUTTON_8_ID_SHORT = 216, // Command: Button 8 short press
    CMD_BUTTON_8_ID_LONG = 217, // Command: Button 8 long press
    } RFIDModificationType;

// Repeat-Modes
typedef enum {
    NO_REPEAT = 0, // No repeat
    TRACK = 1, // Repeat current track (infinite loop)
    PLAYLIST = 2, // Repeat whole playlist (infinite loop)
    TRACK_N_PLAYLIST = 3 // Repeat both (infinite loop)
} RepeatModeType;

// Seek-modes
typedef enum {
    SEEK_NORMAL = 0, // Normal play
    SEEK_FORWARDS = 1, // Seek forwards
    SEEK_BACKWARDS = 2 // Seek backwards
    SEEK_POS_PERCENT = 3 // Seek to position (0-100)
} SeekModesType;

// TTS
typedef enum {
    TTS_NONE = 0, // Do nothng (IDLE)
    TTS_IP_ADDRESS = 1, // Tell IP-address
    TTS_CURRENT_TIME = 2 // Tell current time
} TTSModeType;

// supported languages
#define DE 1
#define EN 2


// Debug
#define PRINT_TASK_STATS 199 // Prints task stats for debugging, needs CONFIG_FREERTOS_USE_TRACE_FACILITY=y and CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y in sdkconfig.defaults

#endif // VALUES_H