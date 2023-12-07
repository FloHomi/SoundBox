#include <Arduino.h>
#include "settings.h"

#include "Battery.h"


#ifdef MEASURE_BATTERY_BQ2589X
	#include "Log.h"
	#include "Mqtt.h"
	#include "Led.h"
	#include "System.h"
	#include <Wire.h>
	#include <bq2589x.h>


bq2589x BatteryManager;

#define SOC_ESTIMATION_REF_POINTS 13
typedef struct {
    uint16_t voltage_ref_points_mV[SOC_ESTIMATION_REF_POINTS];
    int soc_ref_points_pct[SOC_ESTIMATION_REF_POINTS];
} SOC_Estimator;

	// https://lygte-info.dk/info/BatteryChargePercent%20UK.html
	// at a mean between 1A/3A discharge measured after 1 hour.  Error after 10sec-30sec is very little.  Important is that the battery is not charged or discharged at this time.
	// SOH is assuemd to be 100% at 4.2V
SOC_Estimator s_soc_estimator = {
	{3000, 3100, 3200 , 3300, 3400, 3500, 3600, 3700, 3800, 3900, 4000, 4100, 4200}, // voltage_ref_points
	#if defined(USE_CGR18650CH_2250)
	{0,	  0,   0 ,   0,   0,  9,  22,  52,  64,  75,  84,  93,  100} // soc_ref_points for CGR18650CH 2250mAH
	#elif defined(USE_Sanyo_18650_2600)
	{0,	  0,   0 ,   0,   0,  0,  2,  12,  42,  62,  79,  91,  100} // soc_ref_points for Sanyo 18650 2600mAH
	#else
	{0,	  0,   0 ,   0,   6,  15,  33,  50,  59,  72,  83,  94,  100} // soc_ref_points for NCR18650A 3100mAH || NCR18650B 3400mAH 
	#endif
};




extern TwoWire i2cBusTwo;

void Battery_InitInner() {
	int err = BatteryManager.begin(&i2cBusTwo);
	if(err != BQ2589X_OK) {
		Log_Println("BQ2589X init failed", LOGLEVEL_ERROR);
		return;
	}

	delay(50); // wait for the chip to be ready
	err = BatteryManager.adc_start(false); // start adc in continous mode
	delay(50); // wait for the adc to be ready

	err |= BatteryManager.disable_otg();
	//BatteryManager.set_minSysVoltage(3500);  //not implemented in lib
	err |= BatteryManager.set_charge_current(s_batteryChargeCurrent_mA);
	err |= BatteryManager.set_chargevoltage(s_batteryChargeVoltage_mV);
	if(err != BQ2589X_OK) {
		Log_Println("BQ2589X configuration failed", LOGLEVEL_ERROR);
		return;
	}
}

void Battery_CyclicInner() {
	
	static uint32_t lastWatchdogResetTimestamp = 0;
	//reset whatchdog every couple sec. (10s) to prevent reset
	if(millis() - lastWatchdogResetTimestamp > 10000) {
		lastWatchdogResetTimestamp = millis();
		BatteryManager.reset_watchdog_timer();
	}
	
	
}

float Battery_GetVoltage(void) {
	//return Battery voltage in mV and convert to float volt
	return BatteryManager.adc_read_battery_volt()/1000.0;
	
}

void Battery_PublishMQTT() {
	#ifdef MQTT_ENABLE
	float voltage = Battery_GetVoltage();
	char vstr[6];
	snprintf(vstr, 6, "%.2f", voltage);
	publishMqtt(topicBatteryVoltage, vstr, false);

	float soc = Battery_EstimateLevel() * 100;
	snprintf(vstr, 6, "%.2f", soc);
	publishMqtt(topicBatterySOC, vstr, false);
	#endif
}

void Battery_LogStatus(void) {
	Log_Printf(LOGLEVEL_INFO, currentBattVoltageMsg, Battery_GetVoltage());
	Log_Printf(LOGLEVEL_INFO, currentSysVoltageMsg, BatteryManager.adc_read_sys_volt()/1000.0);
	Log_Printf(LOGLEVEL_INFO, currentChargeMsg, Battery_EstimateLevel() * 100);
	Log_Printf(LOGLEVEL_INFO, batteryCurrentMsg, BatteryManager.adc_read_charge_current());
	Log_Printf(LOGLEVEL_INFO, batteryTempMsg, BatteryManager.adc_read_temperature());

}

float Battery_EstimateLevel(void) 
{	
	// get soc estator pointer
	SOC_Estimator *estimator = &s_soc_estimator;
	// Get the measured voltage
	uint16_t measured_voltage = BatteryManager.adc_read_battery_volt();
    // Ensure that the measured voltage is within the range of reference points
    if (measured_voltage < estimator->voltage_ref_points_mV[0] || measured_voltage > estimator->voltage_ref_points_mV[SOC_ESTIMATION_REF_POINTS-1]) {
		Log_Printf(LOGLEVEL_ERROR, "Measured voltage is outside the valid range\n");
        return -1;  // Or handle the error in a way suitable for your application
    }

    // Find the two closest reference points for linear interpolation
    int lower_index = -1, upper_index = -1;
    for (int i = 0; i < SOC_ESTIMATION_REF_POINTS; ++i) {
        if (estimator->voltage_ref_points_mV[i] <= measured_voltage)
            lower_index = i;
        if (estimator->voltage_ref_points_mV[i] >= measured_voltage) {
            upper_index = i;
            break;
        }
    }

    // Linear interpolation formula
    float lower_voltage = estimator->voltage_ref_points_mV[lower_index];
    float upper_voltage = estimator->voltage_ref_points_mV[upper_index];
    int lower_soc = estimator->soc_ref_points_pct[lower_index];
    int upper_soc = estimator->soc_ref_points_pct[upper_index];

    float estimated_soc = lower_soc + ((measured_voltage - lower_voltage) / (upper_voltage - lower_voltage)) *
                                               (upper_soc - lower_soc);

    return estimated_soc/100.0;
}

bool Battery_IsLow(void) {
	float vBatt = Battery_GetVoltage();
	float capacity = Battery_EstimateLevel()*100.0;

	bool isLow = vBatt < s_warningLowVoltage_mV/1000.0;
	isLow |= capacity < s_batteryLow_pct;
	return isLow;
}

bool Battery_IsCritical(void) {
	float vBatt = Battery_GetVoltage();
	float capacity = Battery_EstimateLevel()*100.0;

	bool isCritical = vBatt < s_warningCriticalVoltage_mV/1000.0;
	isCritical |= capacity < s_batteryCritical_pct;
	return isCritical;
}
#endif
