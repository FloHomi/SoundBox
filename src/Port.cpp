#include <Arduino.h>
#include "settings.h"

#include "Port.h"

#include "Log.h"

#include <Wire.h>

// Infos:
// PCA9555 has 16 channels that are subdivided into 2 ports with 8 channels each.
// Every channels is represented by a bit.
// Examples for ESPuino-configuration:
// 100 => port 0 channel/bit 0
// 107 => port 0 channel/bit 7
// 108 => port 1 channel/bit 0
// 115 => port 1 channel/bit 7

#ifdef PORT_EXPANDER_ENABLE
extern TwoWire i2cBusTwo;

uint8_t Port_ExpanderPortsInputChannelStatus[2];
static uint8_t Port_ExpanderPortsOutputChannelStatus[2]; // Stores current configuration of output-channels locally
void Port_ExpanderHandler(void);
uint8_t Port_ChannelToBit(const uint8_t _channel);
void Port_WriteInitMaskForOutputChannels(void);
void Port_Test(void);

	#if (PE_INTERRUPT_PIN >= 0 && PE_INTERRUPT_PIN <= MAX_GPIO)
		#define PE_INTERRUPT_PIN_ENABLE
void IRAM_ATTR PORT_ExpanderISR(void);
bool Port_AllowReadFromPortExpander = false;
	#endif
#endif

#if defined(INVERT_POWER)
	#define POWER_INVERT  true
#else
	#define POWER_INVERT  false
#endif

void Port_Init(void) {
#ifdef PORT_EXPANDER_ENABLE
	Port_Test();
	Port_WriteInitMaskForOutputChannels();
#endif

#ifdef PE_INTERRUPT_PIN_ENABLE
	pinMode(PE_INTERRUPT_PIN, INPUT_PULLUP);
	// ISR gets enabled in Port_ExpanderHandler()
	Log_Println(portExpanderInterruptEnabled, LOGLEVEL_NOTICE);
#endif

#ifdef PORT_EXPANDER_ENABLE
	Port_AllowReadFromPortExpander = true;
	Port_ExpanderHandler();
#endif
}

void Port_Cyclic(void) {
#ifdef PORT_EXPANDER_ENABLE
	Port_ExpanderHandler();
#endif
}

// Wrapper: reads from GPIOs (via digitalRead()) or from port-expander (if enabled)
// Behaviour like digitalRead(): returns true if not pressed and false if pressed
bool Port_Read(const uint8_t _channel) {
	switch (_channel) {
		case 0 ... MAX_GPIO: // GPIO
			return digitalRead(_channel);

#ifdef PORT_EXPANDER_ENABLE
		case 100 ... 107: // Port-expander (port 0)
			return (Port_ExpanderPortsInputChannelStatus[0] & (1 << (_channel - 100))); // Remove offset 100 (return false if pressed)

		case 108 ... 115: // Port-expander (port 1)
			return (Port_ExpanderPortsInputChannelStatus[1] & (1 << (_channel - 108))); // Remove offset 100 + 8 (return false if pressed)

#endif

		default: // Everything else (doesn't make sense at all) isn't supposed to be pressed
			return true;
	}
}

// Configures OUTPUT-mode for GPIOs (non port-expander)
// Output-mode for port-channels is done via Port_WriteInitMaskForOutputChannels()
void Port_Write(const uint8_t _channel, const bool _newState, const bool _initGpio) {
#ifdef GPIO_PA_EN
	if (_channel == GPIO_PA_EN) {
		if (_newState) {
			Log_Println(paOn, LOGLEVEL_NOTICE);
		} else {
			Log_Println(paOff, LOGLEVEL_NOTICE);
		}
	}
#endif

#ifdef GPIO_HP_EN
	if (_channel == GPIO_HP_EN) {
		if (_newState) {
			Log_Println(hpOn, LOGLEVEL_NOTICE);
		} else {
			Log_Println(hpOff, LOGLEVEL_NOTICE);
		}
	}
#endif

	// Make init only for GPIO but not for PE (because PE is already done earlier)
	if (_initGpio) {
		if (_channel <= MAX_GPIO) {
			pinMode(_channel, OUTPUT);
		}
	}

	switch (_channel) {
		case 0 ... MAX_GPIO: { // GPIO
			digitalWrite(_channel, _newState);
			break;
		}

#ifdef PORT_EXPANDER_ENABLE
		case 100 ... 115: {
			uint8_t portOffset = 0;
			if (_channel >= 108 && _channel <= 115) {
				portOffset = 1;
			}

			uint8_t oldPortBitmask = Port_ExpanderPortsOutputChannelStatus[portOffset];
			uint8_t newPortBitmask;

			i2cBusTwo.beginTransmission(expanderI2cAddress);
			i2cBusTwo.write(0x02); // Pointer to output configuration-register
			if (_newState) {
				newPortBitmask = (oldPortBitmask | (1 << Port_ChannelToBit(_channel)));
				Port_ExpanderPortsOutputChannelStatus[portOffset] = newPortBitmask; // Write back new status
			} else {
				newPortBitmask = (oldPortBitmask & ~(1 << Port_ChannelToBit(_channel)));
				Port_ExpanderPortsOutputChannelStatus[portOffset] = newPortBitmask; // Write back new status
			}
			i2cBusTwo.write(Port_ExpanderPortsOutputChannelStatus[0]);
			i2cBusTwo.write(Port_ExpanderPortsOutputChannelStatus[1]);
			i2cBusTwo.endTransmission();
			break;
		}
#endif

		default: {
			break;
		}
	}
}

#ifdef PORT_EXPANDER_ENABLE
// Translates digitalWrite-style "GPIO" to bit
uint8_t Port_ChannelToBit(const uint8_t _channel) {
    if (_channel >= 100 && _channel <= 115) {
        return (_channel - 100) % 8;
    } else {
        return 255; // not valid!
    }
}

void configMaskForOutput(uint16_t* IOMask, uint16_t* stateMask, uint8_t channel, bool invert) {
	if(channel >= 100 && channel <= 115){
		channel -= 100;
		if(invert){
			*stateMask |= (1 << channel);
		}else{
			*stateMask &= ~(1 << channel);
		}	
		*IOMask &= ~(1 << channel);
	}
}

// Writes initial port-configuration (I/O) for port-expander PCA9555
// If no output-channel is necessary, nothing has to be configured as all channels are in input-mode as per default (255)
// So every bit representing an output-channel needs to be set to 0.
void Port_WriteInitMaskForOutputChannels(void) {
	uint16_t ioMask = 0xFFFF; // 0xFFFF => all channels set to input;
	uint16_t stateMask = 0x0000; // Bit configured as 0 for an output-channels means: logic LOW

	#if defined(GPIO_PA_EN) // Set as output to enable/disable amp for loudspeaker
	configMaskForOutput(&ioMask, &stateMask, GPIO_PA_EN, false);
	#endif

	#if defined( GPIO_HP_EN) // Set as output to enable/disable amp for headphones
	configMaskForOutput(&ioMask, &stateMask, GPIO_HP_EN, false);
	#endif

	#if defined(POWER) // Set as output to trigger mosfet/power-pin for powering peripherals. Hint: logic is inverted if INVERT_POWER is enabled.
	configMaskForOutput(&ioMask, &stateMask, POWER, POWER_INVERT);
	#endif
	
	#if defined(BUTTONS_LED)
	configMaskForOutput(&ioMask, &stateMask, BUTTONS_LED, false);
	#endif
	#if defined(BUTTON0_LED)
	configMaskForOutput(&ioMask, &stateMask, BUTTON0_LED, false);
	#endif
	#if defined(BUTTON1_LED)
	configMaskForOutput(&ioMask, &stateMask, BUTTON1_LED, false);
	#endif
	#if defined(BUTTON2_LED)
	configMaskForOutput(&ioMask, &stateMask, BUTTON2_LED, false);
	#endif

	Log_Printf(LOGLEVEL_DEBUG, "Port_WriteInitMaskForOutputChannels: ioMask: %u, stateMask: %u", ioMask, stateMask);
	i2cBusTwo.beginTransmission(expanderI2cAddress);
	i2cBusTwo.write(0x06); // Pointer to configuration of input/output
	i2cBusTwo.write(ioMask & 0xFF); // port0
	i2cBusTwo.write(ioMask >> 8); // port1
	i2cBusTwo.endTransmission();

	// Write low/high-config to all output-channels. Channels that are configured as input are silently/automatically ignored by PCA9555
	i2cBusTwo.beginTransmission(expanderI2cAddress);
	i2cBusTwo.write(0x02); // Pointer to configuration of output-channels (high/low)
	i2cBusTwo.write(stateMask & 0xFF); // port0
	i2cBusTwo.write(stateMask >> 8); // port1
	i2cBusTwo.endTransmission();
	
}

// Some channels are configured as output before shutdown in order to avoid unwanted interrupts while ESP32 sleeps
void Port_MakeSomeChannelsOutputForShutdown(void) {
	uint16_t ioMask = 0xFFFF; // 0xFFFF => all channels set to input;
	uint16_t stateMask = 0x0000; // Bit configured as 0 for an output-channels means: logic LOW


	// init status cache with values from HW
	i2cBusTwo.beginTransmission(expanderI2cAddress);
	i2cBusTwo.write(0x02); // Pointer to first output-register
	i2cBusTwo.endTransmission(false);
	i2cBusTwo.requestFrom(expanderI2cAddress, 2, true); // ...and read the contents
	if (i2cBusTwo.available() == 2) {
		stateMask = i2cBusTwo.read();
		stateMask |= i2cBusTwo.read() << 8;
	}


	#if defined(HP_DETECT) // https://forum.espuino.de/t/lolin-d32-pro-mit-sd-mmc-pn5180-max-fuenf-buttons-und-port-expander-smd/638/33
	configMaskForOutput(&ioMask, &stateMask, HP_DETECT, false);
	#endif

	// There's no possibility to get current I/O-status from PCA9555. So we just re-set it again for OUTPUT-pins.
	#if defined(GPIO_PA_EN)
	configMaskForOutput(&ioMask, &stateMask, GPIO_PA_EN, false);
	#endif

	#if defined(GPIO_HP_EN)
	configMaskForOutput(&ioMask, &stateMask, GPIO_HP_EN, false);
	#endif

	#if defined(POWER) // Set as output to trigger mosfet/power-pin for powering peripherals. Hint: logic is inverted if INVERT_POWER is enabled.
	configMaskForOutput(&ioMask, &stateMask, POWER, POWER_INVERT);
	#endif

	#if defined(BUTTONS_LED)
	configMaskForOutput(&ioMask, &stateMask, BUTTONS_LED, false);
	#endif
	#if defined(BUTTON0_LED)
	configMaskForOutput(&ioMask, &stateMask, BUTTON0_LED, false);
	#endif
	#if defined(BUTTON1_LED)
	configMaskForOutput(&ioMask, &stateMask, BUTTON1_LED, false);
	#endif
	#if defined(BUTTON2_LED)
	configMaskForOutput(&ioMask, &stateMask, BUTTON2_LED, false);
	#endif

	i2cBusTwo.beginTransmission(expanderI2cAddress);
	i2cBusTwo.write(0x06); // Pointer to configuration of input/output
	i2cBusTwo.write(ioMask & 0xFF); // port0
	i2cBusTwo.write(ioMask >> 8); // port1
	i2cBusTwo.endTransmission();

	// Write low/high-config to all output-channels. Channels that are configured as input are silently/automatically ignored by PCA9555
	i2cBusTwo.beginTransmission(expanderI2cAddress);
	i2cBusTwo.write(0x02); // Pointer to configuration of output-channels (high/low)
	i2cBusTwo.write(stateMask & 0xFF); // port0
	i2cBusTwo.write(stateMask >> 8); // port1
	i2cBusTwo.endTransmission();
}

// Reads input-registers from port-expander and writes output into global cache-array
// Datasheet: https://www.nxp.com/docs/en/data-sheet/PCA9555.pdf
void Port_ExpanderHandler(void) {
	static uint32_t inputChanged = 0; // Used to debounce once in case of register-change
	static uint32_t inputPrev = 0;

	// If interrupt-handling is active, only read port-expander's registers if interrupt was fired
	// or the next time if there was a change
	#ifdef PE_INTERRUPT_PIN_ENABLE
	if (Port_AllowReadFromPortExpander) {
		Port_AllowReadFromPortExpander = false;
	} else if (!inputChanged) {
		return;
	}
	#endif

	i2cBusTwo.beginTransmission(expanderI2cAddress);
	i2cBusTwo.write(0x00); // Pointer to input-register...
	uint8_t error = i2cBusTwo.endTransmission(false);
	if (error != 0) {
		Log_Printf(LOGLEVEL_ERROR, "Error in endTransmission(): %d", error);
		i2cBusTwo.endTransmission(true);

	#ifdef PE_INTERRUPT_PIN_ENABLE
		Port_AllowReadFromPortExpander = true;
	#endif

		return;
	}
	i2cBusTwo.requestFrom(expanderI2cAddress, 2u); // ...and read its bytes

	if (i2cBusTwo.available() == 2) {
		uint32_t inputCurr = 0;
		for (uint8_t i = 0; i < 2; i++) {
			inputCurr |= i2cBusTwo.read() << 8 * i; // Cache current readout
		}

		// Check if input-register changed. If so, don't use the changed bits immediately
		// but wait another cycle instead (=> rudimentary debounce).
		// Added because there've been "ghost"-events occasionally with Arduino2 (https://forum.espuino.de/t/aktueller-stand-esp32-arduino-2/1389/55)
		inputChanged = inputPrev ^ inputCurr;

		uint32_t inputStable = 0;
		for (uint8_t i = 0; i < 2; i++) {
			inputStable |= Port_ExpanderPortsInputChannelStatus[i] << 8 * i;
		}

		// update bits that were stable since the last run
		inputStable &= inputChanged;
		inputStable |= (~inputChanged & inputCurr);

		for (uint8_t i = 0; i < 2; i++) {
			Port_ExpanderPortsInputChannelStatus[i] = (inputStable >> 8 * i) & 0xff;
			// Serial.printf("%u Debug: PE-Port: %u  Status: %u\n", millis(), i, Port_ExpanderPortsInputChannelStatus[i]);
		}

		inputPrev = inputCurr;
	}

	#ifdef PE_INTERRUPT_PIN_ENABLE
	// input is stable; go back to interrupt mode
	if (!inputChanged) {
		attachInterrupt(digitalPinToInterrupt(PE_INTERRUPT_PIN), PORT_ExpanderISR, ONLOW);
	}
	#endif
}

// Make sure ports are read finally at shutdown in order to clear any active IRQs that could cause re-wakeup immediately
void Port_Exit(void) {
	Port_MakeSomeChannelsOutputForShutdown();
	i2cBusTwo.beginTransmission(expanderI2cAddress);
	i2cBusTwo.write(0x00); // Pointer to input-registers...
	i2cBusTwo.endTransmission();
	i2cBusTwo.requestFrom(expanderI2cAddress, 2u); // ...and read its bytes

	if (i2cBusTwo.available() == 2) {
		for (uint8_t i = 0; i < 2; i++) {
			Port_ExpanderPortsInputChannelStatus[i] = i2cBusTwo.read();
		}
	}
}

// Tests if port-expander can be detected at address configured
void Port_Test(void) {
	i2cBusTwo.beginTransmission(expanderI2cAddress);
	i2cBusTwo.write(0x02);
	if (!i2cBusTwo.endTransmission()) {
		Log_Println(portExpanderFound, LOGLEVEL_NOTICE);
	} else {
		Log_Println(portExpanderNotFound, LOGLEVEL_ERROR);
	}
}

	#ifdef PE_INTERRUPT_PIN_ENABLE
void IRAM_ATTR PORT_ExpanderISR(void) {
	// check if the interrupt pin is actually low and only if it is
	// trigger the handler (there are a lot of false calls to this ISR
	// where the interrupt pin isn't low...)
	Port_AllowReadFromPortExpander = !digitalRead(PE_INTERRUPT_PIN);
	// until the interrupt is handled we don't need any more ISR calls
	if (Port_AllowReadFromPortExpander) {
		detachInterrupt(digitalPinToInterrupt(PE_INTERRUPT_PIN));
	}
}
	#endif
#endif
