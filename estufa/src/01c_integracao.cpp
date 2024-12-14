
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>

static const int servoPin = 6;


Servo servo1;


// O Led RGB está conectado ao pino 18 do Franzininho
#define PIN 5
// Há apenas um LED
#define NUMPIXELS   3

// Instância do objeto "Adafruit_NeoPixel"
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


const int relay = 16;
const int relay1 = 4;

void color (int r, int g, int b, float brilho){
    for(int i = 0; i<NUMPIXELS;i++){
    pixels.setPixelColor(i, int(r*255*brilho),int(r*255*brilho),int(r*255*brilho));
    pixels.show();  // envia o pixel atualizado para o hardware
}
}


void setup()
{
    Serial.begin(115200);
    delay(1500);
    servo1.attach(servoPin);
    pixels.begin();
    pixels.clear();
    neopixelWrite(RGB_BUILTIN, int(255*0.5), 0, 0);
        for(int i = 0; i<NUMPIXELS;i++){
            pixels.setPixelColor(i, int(255*0.5),0,0);
            pixels.show();  // envia o pixel atualizado para o hardware
        
        }
    pinMode(relay, OUTPUT);
    digitalWrite(relay, LOW);
    pinMode(relay1, OUTPUT);
    digitalWrite(relay1, LOW);


}

void loop()
{
    servo1.write(0);
    digitalWrite(relay, LOW);
    digitalWrite(relay1, LOW);
    delay(1000);
    servo1.write(90);
    digitalWrite(relay, HIGH);
    digitalWrite(relay1, HIGH);
    delay(1000);

}
