#ifndef __ADC_APP_H_
#define __ADC_APP_H_

#include "stdint.h"

extern float R_temp;
extern float PT100_temp;
extern float adc_raw_voltage;
float PT100_ResistanceToTemp(float R);
float PT100_BoardVoltageToTemp(float voltage);
float PT100_TempToResistance(float temp);
void adc_task(void);

#endif /* __ADC_APP_H_ */
