
#include "bmp085.h"

struct bmp085 b;

void setup()
{
    Serial.begin(9600);
    bmp085_init(&b);
}

void loop()
{

    b.oss = 3;
    bmp085_read_sensors(&b);

    Serial.print("temp: ");
    Serial.print(b.t);
    Serial.print("dC pressure: ");
    Serial.print(b.ppa);
    Serial.print("Pa ");
    Serial.print(b.patm);
    Serial.println("atm");

    delay(5000);

}
