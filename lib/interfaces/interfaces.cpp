#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include "ADS7828.h"

#include "interfaces.h"
#include "sensor_data.h"

const int bat_therm_pin = PB0;
const int cell_pins[N_BATTERY_CELLS] = {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PC0, PC1, PC2, PC3};
const uint8_t thermistor_map[N_THERMISTORS] = {3,2,1,0,7,6,5,4};
TwoWire wire1(PB7,PB6);

Adafruit_BME680 bme(&wire1); // I2C
ADS7828 ext_adc(72,&wire1,ADS7828_SINGLE_ENDED, 1, 0);

void setup_interfaces(){
    bme.begin(0x77);
    //Setup ADC
    analogReadResolution(ADC_N_BITS);
    for(int i = 0; i < N_BATTERY_CELLS; i++){
        pinMode(cell_pins[i], INPUT_ANALOG);
    }
}

void update_interfaces(){

    sensor_data.battery_cell_voltage[0] = battery_calc_cell_v(analogRead(cell_pins[0]), 0);
    for(int i = 1; i < N_BATTERY_CELLS; i++){
        sensor_data.battery_cell_voltage[i] = battery_calc_cell_v(analogRead(cell_pins[i]), analogRead(cell_pins[i-1]));
    }

    sensor_data.battery_temp = thermistor_calc_temp(analogRead(bat_therm_pin));

    for(int i = 0; i < N_THERMISTORS; i++){
        sensor_data.thermistors[i] = thermistor_calc_temp(ext_adc.read(thermistor_map[i]));
    }

    if(wire1.getWriteError()){
        wire1.clearWriteError();
    }

    bme.performReading();
    sensor_data.ambiant_temp = bme.readTemperature();
    sensor_data.humidity = bme.readHumidity();
    
}

float thermistor_calc_temp(int adc_reading){
    //Find thermistor resistance from rdivider equation
    float resistance;
    resistance = THERM_PULL_UP/((ADC_MAX_VALUE / adc_reading) - 1);
    
    //Find thermistor temperature from steinart equation
    float temp;
    temp = (THERM_B*THERM_T0)/(THERM_B+THERM_T0*log(resistance/THERM_R0));
    //Degree K to degree C
    temp -= 273.15;

    return(temp);
}

float battery_calc_cell_v(uint16_t cell_reading, uint16_t prev_cell_reading){
    float result;
    result = ((cell_reading - prev_cell_reading)/ADC_MAX_VALUE) * ADC_VCC * CELL_RDIV_RATIO;
    return result;
}

float lowpass_filter(float previous, float input, float tau, float dt){
    float alpha = dt / (tau + dt);
    float output = alpha * input + (1 - alpha) * previous;
    return output;
}