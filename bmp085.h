#ifndef __bmp085_h_
#define __bmp085_h_

#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

// i2c slave address
// wire needs only the 7 MSB for the address
#define BMP085_I2C_ADDR             0x77

struct bmp085 {
    // calibration coefficients
    int16_t ac1, ac2, ac3, b1, b2, mb, mc, md, b5;
    uint16_t ac4, ac5, ac6;

    // uncompensated values
    int16_t ut;
    uint32_t up;

    // oversampling setting
    uint8_t oss;

    // calculated values
    float t;                    // temperature in degC
    uint32_t ppa;               // pressure in Pa
    float patm;                 // pressure in atm
};

uint8_t bmp085_get_uint8_t(uint8_t addr);
uint16_t bmp085_get_uint16_t(uint8_t addr);
void bmp085_get_cal_param(struct bmp085 *b);

void bmp085_init(struct bmp085 *b);
void bmp085_read_sensors(struct bmp085 *b);

void bmp085_get_ut(struct bmp085 *b);
void bmp085_get_up(struct bmp085 *b);
void bmp085_get_temperature(struct bmp085 *b);
void bmp085_get_pressure(struct bmp085 *b);

#endif
