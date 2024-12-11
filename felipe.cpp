#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <string.h>
#include <Arduino.h>
#include <Adafruit_BME680.h>
#include <WiFi.h>
// #include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include "certificados.h"
#include "QRCode.h"
#include "logo.h"
#include <ArduinoJson.h>
#include <stdlib.h>
#include <stdint.h>
#include <GFButton.h>

WiFiClientSecure conexaoSegura;
MQTTClient mqtt;
GxEPD2_290_T94_V2 modeloTela(10, 14, 15, 16);
GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> tela(modeloTela);
U8G2_FOR_ADAFRUIT_GFX fontes;
QRCode qrcode(tela);
Adafruit_BME680 sensorBME;
// AsyncWebServer server(80);
int sensor_luz = 18;
unsigned long instante_anterior = 0;
bool home = false;
String ssid = "Projeto";
String senha = "2022-11-07";
int clientes;
int sensor_umid_solo = 4;
int temperatura;
int umidade_ar;
int leituraAnalogica;
int porcentagemLuz;
float umidade_solo;



void envia_dados(int temp_ar, int luz, int umid_ar, float umid_solo)
{
    Serial.println("dados enviados");
    JsonDocument dados;
    dados["temperatura_ar"] = temp_ar;
    dados["umidade_ar"] = umid_ar;
    dados["luminosidade"] = luz;
    dados["umidade_solo"] = umid_solo;
    String texto;
    serializeJson(dados, texto);
    mqtt.publish("dados", texto);
}

void reconectarWifi()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.begin(ssid, senha);
        Serial.print("Conectando ao WiFi...");
        while (WiFi.status() != WL_CONNECTED)
        {
            Serial.print(".");
            delay(1000);
        }

        Serial.print("conectado!\nEndereço IP: ");
        Serial.println(WiFi.localIP());
    }
}

void reconectarMQTT()
{
    if (!mqtt.connected())
    {
        Serial.println("Conectando MQTT...");
        while (!mqtt.connected())
        {
            mqtt.connect("projeto", "aula", "zowmad-tavQez");
            Serial.print(".");
            delay(1000);
        }
        Serial.println(" conectado!");

        mqtt.subscribe("dados");
    }
}

void setup()
{
    // serial
    Serial.begin(115200);
    delay(500);

    // botao
    pinMode(5,INPUT);

    // sensor umidade solo
    pinMode(sensor_umid_solo, INPUT);

    // wifi
    reconectarWifi();
    conexaoSegura.setCACert(certificado1);

    // mqtt
    mqtt.begin("mqtt.janks.dev.br", 8883, conexaoSegura);

    // ap
    WiFi.softAP(ssid, senha);
    Serial.println("Iniciando ponto de acesso...");
    Serial.print("IP do ponto de acesso: ");
    Serial.println(WiFi.softAPIP());
    clientes = WiFi.softAPgetStationNum();
    Serial.print("Número de dispositivos conectados: ");
    Serial.println(clientes);
    // server.begin();

    // sensor
    if (!sensorBME.begin())
    {
        Serial.println("Erro no sensor BME");
        while (true)
            ;
    }

    sensorBME.setTemperatureOversampling(BME680_OS_8X);
    sensorBME.setHumidityOversampling(BME680_OS_2X);
    sensorBME.setPressureOversampling(BME680_OS_4X);
    sensorBME.setIIRFilterSize(BME680_FILTER_SIZE_3);
    sensorBME.setGasHeater(320, 150);

    // tela
    tela.init();
    tela.setRotation(2);
    tela.fillScreen(GxEPD_WHITE);
    tela.display(false);

    // fontes
    fontes.begin(tela);
    fontes.setForegroundColor(GxEPD_BLACK);

    reconectarMQTT();
}

void loop()
{
    if (Serial.available() > 0)
    {

        String texto = Serial.readStringUntil('\n');
        texto.trim();
        if (texto.startsWith("default"))
        {
            home = false;
            tela.fillScreen(GxEPD_WHITE);
            fontes.setFont(u8g2_font_helvB24_te);
            fontes.setFontMode(1);
            fontes.setCursor(-3, 45);
            fontes.print("E-Planta");
            tela.drawXBitmap(-35, 60, logo_bits, 200, 200, GxEPD_BLACK);
            tela.display(true);
        }
        else if (texto.startsWith("bem"))
        {
            home = false;
            tela.fillScreen(GxEPD_WHITE);
            fontes.setFont(u8g2_font_helvB18_te);
            fontes.setFontMode(1);
            fontes.setCursor(0, 45);
            fontes.print("Bem-vindo");

            // qrcode
            String wifi_qr_code = "WIFI:T:WPA2;S:" + ssid + ";P:" + senha + ";;";
            qrcode.scale = 4;
            qrcode.draw(wifi_qr_code, -10, 80);

            tela.display(true);
        }
        else if (texto.startsWith("home"))
        {
            home = true;
            if (!sensorBME.performReading())
            {
                Serial.println("Failed to perform reading :(");
                return;
            }
            tela.fillScreen(GxEPD_WHITE);
            fontes.setFont(u8g2_font_helvB18_te);

            // nome da planta
            fontes.setFontMode(1);
            fontes.setCursor(0, 45);
            fontes.print("nome");

            // luminosidade
            fontes.setFontMode(1);
            fontes.setCursor(65, 115);
            int leituraAnalogica = analogRead(sensor_luz);
            porcentagemLuz = map(leituraAnalogica, 0, 4096, 0, 100);
            fontes.print((String)porcentagemLuz + "%");

            // umidade ar
            fontes.setFontMode(1);
            fontes.setCursor(65, 185);
            umidade_ar = sensorBME.humidity;
            fontes.print((String)umidade_ar + "%");

            // temperatura
            fontes.setFontMode(1);
            fontes.setCursor(65, 255);
            temperatura = sensorBME.temperature;
            fontes.print((String)temperatura + "°C");

            // umidade solo
            umidade_solo = analogRead(sensor_umid_solo);
            umidade_solo = map(umidade_solo, 4095, 2150, 0, 100);
            if (umidade_solo > 100)
            {
                umidade_solo = 100;
            }
            Serial.println(umidade_solo);

            fontes.setFont(u8g2_font_open_iconic_all_6x_t);
            fontes.setFontMode(1);
            // plantinha
            fontes.drawGlyph(75, 55, 0x00fe);
            // sol
            fontes.drawGlyph(0, 125, 0x0103);
            // gota
            fontes.drawGlyph(0, 195, 0x0098);
            // termometro
            // fontes.drawGlyph(0, 265, 0x008d);
            tela.fillRect(10, 225, 25, 40, GxEPD_BLACK);
            tela.fillRect(15, 230, 15, 30, GxEPD_WHITE);
            tela.fillRect(20, 245, 05, 10, GxEPD_BLACK);
            tela.display(true);
        }
        else if (texto.startsWith("config"))
        {
            home = false;
            tela.fillScreen(GxEPD_WHITE);
            fontes.setFont(u8g2_font_helvB14_te);
            fontes.setFontMode(1);
            fontes.setCursor(0, 65);
            fontes.print("Configurando");
            fontes.setFont(u8g2_font_open_iconic_all_8x_t);
            fontes.setFontMode(1);
            fontes.drawGlyph(25, 170, 0x011a);
            tela.display(true);
        }
        else if (texto.startsWith("envia"))
        {

            reconectarWifi();
            reconectarMQTT();
            temperatura = sensorBME.temperature;
            umidade_ar = sensorBME.humidity;
            leituraAnalogica = analogRead(sensor_luz);
            porcentagemLuz = map(leituraAnalogica, 0, 4096, 0, 100);
            umidade_solo = analogRead(sensor_umid_solo);
            umidade_solo = map(umidade_solo, 4095, 2150, 0, 100);
            if (umidade_solo > 100)
            {
                umidade_solo = 100;
            }
            envia_dados(temperatura, porcentagemLuz, umidade_ar, umidade_solo);
            // reconectarWifi();
            // reconectarMQTT();
        }
    }
    unsigned long instante_atual = millis();
    if (instante_atual > instante_anterior + 1000){
        if(digitalRead(5)==1){
            ESP.restart();
        }
        instante_anterior = instante_atual;
    }
    reconectarWifi();
    reconectarMQTT();
}
