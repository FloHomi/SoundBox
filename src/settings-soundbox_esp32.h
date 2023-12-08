#ifndef __ESPUINO_SOUNDBOX_ESP32
	#define __ESPUINO_SOUNDBOX_ESP32
	#include "Arduino.h"

	#include "Port.h"

//     //######################### INFOS ####################################
//     /* This is not a develboard-specific config-file. It's intended for your own use.
//     It's been originally derived from lolin32, but just change it according your needs!
    /*
    PCB: tba
	Infos: Uses ESP32-WROVER, boost-buck-converter, port-expander, 2x MAX98357a, MS6324 + TDA1308 (headphone), 
	Caveats: Don't forget to verify polarity of your battery-connector. It needs to fit the polarity printed on the PCB!
	Settings: Make sure to enable at least PORT_EXPANDER_ENABLE. PLAY_MONO_SPEAKER should be disabled.
	Status: Test in progress...
	*/

	//################## GPIO-configuration ##############################
	#ifdef SD_MMC_1BIT_MODE
		// uSD-card-reader (via SD-MMC 1Bit)
		//
		// SD_MMC uses fixed pins
		//  (MOSI)    15  CMD
		//  (SCK)     14  SCK
		//  (MISO)     2  D0
	#else
		// SPI-SD IS NOT SUPPORTED BY THIS PCB
		#define SPISD_CS		99		// GPIO for chip select (SD)
		#ifndef SINGLE_SPI_ENABLE
			#define SPISD_MOSI	99		// GPIO for master out slave in (SD) => not necessary for single-SPI
			#define SPISD_MISO	99		// GPIO for master in slave ou (SD) => not necessary for single-SPI
			#define SPISD_SCK	99		// GPIO for clock-signal (SD) => not necessary for single-SPI
		#endif
	#endif

	// RFID (via SPI)
	#define RST_PIN                         22		// Not necessary for RC522 but has to be set anyway; so let's use a dummy-number
	#define RFID_CS                         21		// GPIO for chip select (RFID)
	#define RFID_MOSI                       23		// GPIO for master out slave in (RFID)
	#define RFID_MISO                       19		// GPIO for master in slave out (RFID)
	#define RFID_SCK                        18		// GPIO for clock-signal (RFID)

	// RFID (PN5180 only; not necessary for RC522)
	#ifdef RFID_READER_TYPE_PN5180
		#define RFID_BUSY		33		// PN5180 BUSY PIN
		#define RFID_RST		22		// PN5180 RESET PIN
		#define RFID_IRQ		32		// PN5180 IRQ PIN (only needed for low power card detection)
	#endif

	// I2S (DAC)
	#define I2S_DOUT                        26		// Digital out (I2S)
	#define I2S_BCLK                        27		// BCLK (I2S)
	#define I2S_LRC                         25		// LRC (I2S)

	// Rotary encoder
	#ifdef USEROTARY_ENABLE
		//#define REVERSE_ROTARY			// To reverse encoder's direction; switching CLK / DT in hardware does the same
		#define ROTARYENCODER_CLK	39          	// rotary encoder's CLK
		#define ROTARYENCODER_DT	34		// rotary encoder's DT
	#endif

	// Amp enable
	#define GPIO_PA_EN                      103         	// To enable amp for loudspeaker (GPIO or port-channel)
	//#define GPIO_HP_EN                    101         	// To enable amp for headphones (GPIO or port-channel)

	// Control-buttons
	#define ROTARYENCODER_BUTTON		4		// rotary encoder's button
	#define NEXT_BUTTON			106		// Button 1: GPIO to detect next
	#define PREVIOUS_BUTTON			108 		// Button 2: GPIO to detect previous
	#define PAUSEPLAY_BUTTON		110		// Button 3: GPIO to detect pause/play
	#define BUTTON_4			111		// Button 4: unnamed optional button
	#define BUTTON_5			112		// Button 5: unnamed optional button
	#define BUTTON_6			113		// Button 6: unnamed optional button
	#define BUTTON_7			114		// Button 7: unnamed optional button
	#define BUTTON_8			115		// Button 7: unnamed optional button

	//#define BUTTONS_LED                 	99         	// Powers the LEDs of the buttons. (ext.7)
	#define BUTTON0_LED                 	105         	// Powers the LEDs of the buttons. (ext.7)
	#define BUTTON1_LED                 	107         	// Powers the LEDs of the buttons. (ext.7)
	#define BUTTON2_LED                 	109         	// Powers the LEDs of the buttons. (ext.7)

	// Channels of port-expander can be read cyclic or interrupt-driven. It's strongly recommended to use the interrupt-way!
	// Infos: https://forum.espuino.de/t/einsatz-des-port-expanders-pca9555/306
	#ifdef PORT_EXPANDER_ENABLE
		#define PE_INTERRUPT_PIN	36		// GPIO that is used to receive interrupts from port-expander
	#endif

	// I2C-configuration (necessary at least for port-expander - don't change!)
	#ifdef I2C_2_ENABLE
		#define ext_IIC_CLK			0		// i2c-SCL (clock)   
		#define ext_IIC_DATA		5		// i2c-SDA (data) 
	#endif

	// Wake-up button => this also is the interrupt-pin if port-expander is enabled!
	// Please note: only RTC-GPIOs (0, 4, 12, 13, 14, 15, 25, 26, 27, 32, 33, 34, 35, 36, 39, 99) can be used! Set to 99 to DISABLE.
	// Please note #2: this button can be used as interrupt-pin for port-expander. If so, all pins connected to port-expander can wake up ESPuino.
	#define WAKEUP_BUTTON			4 		// Defines the button that is used to wake up ESPuino from deepsleep.

	// (optional) Power-control
	#define POWER 					101
	#if !defined(POWER)
	#define LOW_POWER				13         	// GPIO used to drive transistor-circuit, that switches off peripheral devices while ESP32-deepsleep
	#define HIGH_POWER				101          	// GPIO used to drive transistor-circuit, that switches off peripheral devices while ESP32-deepsleep
	#endif // !defined(POWER)
	#if defined(LOW_POWER) || defined(HIGH_POWER) || defined (POWER)
		#define INVERT_POWER				// If enabled, use inverted logic for POWER circuit, that means peripherals are turned off by writing HIGH
		//#warning "use inverted logic for POWER circuit, that means peripherals are turned off by writing HIGH"
	#endif
	

	// (optional) Neopixel
	#define LED_PIN				12		// GPIO for Neopixel-signaling

	// Headphone-detection
	#ifdef HEADPHONE_ADJUST_ENABLE
		//#define DETECT_HP_ON_HIGH			// Per default headphones are supposed to be connected if HT_DETECT is LOW. DETECT_HP_ON_HIGH will change this behaviour to HIGH.
		#define HP_DETECT		104         	// GPIO that detects, if there's a plug in the headphone jack or not
	#endif


	// (Optional) remote control via infrared
	#ifdef IR_CONTROL_ENABLE
		#define IRLED_PIN		  5		// GPIO where IR-receiver is connected (only tested with VS1838B)
		#define IR_DEBOUNCE		200		// Interval in ms to wait at least for next signal (not used for actions volume up/down)

		// Actions available. Use your own remote control and have a look at the console for "Command=0x??". E.g. "Protocol=NEC Address=0x17F Command=0x68 Repeat gap=39750us"
		// Make sure to define a hex-code not more than once as this will lead to a compile-error
		// https://forum.espuino.de/t/neues-feature-fernsteuerung-per-infrarot-fernbedienung/265
		#define RC_PLAY			0x68            // command for play
		#define RC_PAUSE		0x67            // command for pause
		#define RC_NEXT			0x6b            // command for next track of playlist
		#define RC_PREVIOUS		0x6a            // command for previous track of playlist
		#define RC_FIRST		0x6c            // command for first track of playlist
		#define RC_LAST			0x6d            // command for last track of playlist
		#define RC_VOL_UP		0x1a            // Command for volume up (one step)
		#define RC_VOL_DOWN		0x1b            // Command for volume down (one step)
		#define RC_MUTE			0x1c            // Command to mute ESPuino
		#define RC_SHUTDOWN		0x2a            // Command for deepsleep
		#define RC_BLUETOOTH		0x72            // Command to enable/disable bluetooth
		#define RC_FTP			0x65            // Command to enable FTP-server
	#endif
#endif // __ESPUINO_SOUNDBOX_ESP32





