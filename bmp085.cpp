
/*

  bmp085 library for the Arduino.

  This library implements the following features:

   - reads uncompensated temperature and pressure readings
   - calibrates readings with correction coefficients from internal eeprom

  Author:          Petre Rodan <2b4eda@subdimension.ro>
  Available from:  https://github.com/rodan/bmp085
  License:         GNU GPLv3

  The bmp085 is a digital pressure sensor with i2c interface, temperature 
  measurement, and a factory calibrated 300-1100 hPa pressure range.

  GNU GPLv3 license:
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
   
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
   
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
   
*/

#include <Wire.h>
#include "bmp085.h"

uint8_t bmp085_get_uint8_t(uint8_t addr)
{
    uint8_t rv;

    Wire.beginTransmission((uint8_t) BMP085_I2C_ADDR);
    Wire.write((uint8_t) addr);
    Wire.endTransmission();

    Wire.requestFrom((uint8_t) BMP085_I2C_ADDR, (uint8_t) 1);
    delay(4); // this sensor might be missing, do not block execution before read()
    rv = Wire.read();

    return rv;
}

uint16_t bmp085_get_uint16_t(uint8_t addr)
{
    uint8_t t1, t2;

    Wire.beginTransmission((uint8_t) BMP085_I2C_ADDR);
    Wire.write((uint8_t) addr);
    Wire.endTransmission();

    Wire.requestFrom((uint8_t) BMP085_I2C_ADDR, (uint8_t) 2);
    delay(4); // this sensor might be missing, do not block execution before read()
    t1 = Wire.read();
    t2 = Wire.read();

    return (t1 << 8) + t2;
}

void bmp085_get_cal_param(struct bmp085 *b)
{
    b->ac1 = bmp085_get_uint16_t(0xaa);
    b->ac2 = bmp085_get_uint16_t(0xac);
    b->ac3 = bmp085_get_uint16_t(0xae);
    b->ac4 = bmp085_get_uint16_t(0xb0);
    b->ac5 = bmp085_get_uint16_t(0xb2);
    b->ac6 = bmp085_get_uint16_t(0xb4);
    b->b1 = bmp085_get_uint16_t(0xb6);
    b->b2 = bmp085_get_uint16_t(0xb8);
    b->mb = bmp085_get_uint16_t(0xba);
    b->mc = bmp085_get_uint16_t(0xbc);
    b->md = bmp085_get_uint16_t(0xbe);
}

void bmp085_init(struct bmp085 *b)
{
    bmp085_get_cal_param(b);
}

void bmp085_read_sensors(struct bmp085 *b)
{
    bmp085_get_ut(b);
    bmp085_get_temperature(b);

    bmp085_get_up(b);
    bmp085_get_pressure(b);
    b->patm = b->ppa / 101325.0;
}

// read uncompensated temperature value
void bmp085_get_ut(struct bmp085 *b)
{
    Wire.beginTransmission((uint8_t) BMP085_I2C_ADDR);
    Wire.write(0xf4);
    Wire.write(0x2e);
    Wire.endTransmission();

    delay(5);

    b->ut = bmp085_get_uint16_t(0xf6);
}

// calculate calibrated temperature value
void bmp085_get_temperature(struct bmp085 *b)
{
    int16_t rv;
    int32_t x1, x2;

    x1 = (((int32_t) b->ut - (int32_t) b->ac6) * (int32_t) b->ac5) >> 15;
    x2 = ((int32_t) b->mc << 11) / (x1 + b->md);
    b->b5 = x1 + x2;

    rv = (b->b5 + 8) >> 4;

    b->t = rv / 10.0;
}

// read uncompensated pressure value
// oss - oversampling setting 0-3
void bmp085_get_up(struct bmp085 *b)
{
    uint8_t msb, lsb, xlsb;

    Wire.beginTransmission((uint8_t) BMP085_I2C_ADDR);
    Wire.write(0xf4);
    Wire.write(0x34 + (b->oss << 6));
    Wire.endTransmission();

    // conversion time depends on oversampling value
    delay(7 * b->oss + 5);

    msb = bmp085_get_uint8_t(0xf6);
    lsb = bmp085_get_uint8_t(0xf7);
    xlsb = bmp085_get_uint8_t(0xf8);

    b->up =
        (((uint32_t) msb << 16) | ((uint32_t) lsb << 8) | (uint32_t) xlsb) >> (8
                                                                               -
                                                                               b->
                                                                               oss);
}

void bmp085_get_pressure(struct bmp085 *b)
{
    int32_t rv;
    int32_t x1, x2, x3;
    int32_t b3, b6;
    uint32_t b4, b7;

    b6 = (int32_t) b->b5 - 4000;

    x1 = ((int32_t) b->b2 * (b6 * b6) >> 12) >> 11;
    x2 = ((int32_t) b->ac2 * b6) >> 11;
    x3 = x1 + x2;
    b3 = (((((int32_t) b->ac1) * 4 + x3) << b->oss) + 2) >> 2;

    x1 = ((int32_t) b->ac3 * b6) >> 13;
    x2 = ((int32_t) b->b1 * ((b6 * b6) >> 12)) >> 16;
    x3 = ((x1 + x2) + 2) >> 2;
    b4 = ((uint32_t) b->ac4 * (uint32_t) (x3 + 32768)) >> 15;

    b7 = ((uint32_t) ((uint32_t) b->up - b3) * (50000 >> b->oss));
    if (b7 < 0x80000000)
        rv = (b7 << 1) / b4;
    else
        rv = (b7 / b4) << 1;

    x1 = (rv >> 8) * (rv >> 8);
    x1 = (x1 * 3038) >> 16;
    x2 = (-7357 * rv) >> 16;
    rv += (x1 + x2 + 3791) >> 4;

    b->ppa = rv;
}
