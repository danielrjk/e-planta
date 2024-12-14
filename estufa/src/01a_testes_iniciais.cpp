#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiClientSecure.h>
#include "certificados.h"
#include <wifi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <uri/UriBraces.h>
#include <time.h>
#include <ESP32Servo.h>
#include <Adafruit_BME680.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <Adafruit_BME680.h>


Adafruit_BME680 sensorBME;


static const int servoPin = 6;
static const int servoPin2 = 4;
const int relay = 21;
const int relayBomba = 7;

Servo servo1;
Servo servo2;

WebServer servidor(80); // porta 80 (padrão do HTTP)
JsonDocument dados;
unsigned long instanteAnterior = 0;
int LUZ = 18;
int pinoLuz = 0;
WiFiClientSecure conexaoSegura;
String chaveTelegram = "7513651643:AAGYN0U_q_9IlHCtbWVLmKxwZZEUykCwB2E";
String idDoChat = "5689474343";
String enderecoBase = "https://api.telegram.org/bot" + chaveTelegram;

struct tm tempo;

float tempEstufa = 0.0;  // ºC
float umidadeEstufa = 0.0;
bool estufaAberta = false;//

// O Led RGB está conectado ao pino 18 do Franzininho
#define PIN 5
// Há apenas um LED
#define NUMPIXELS   60


U8G2_FOR_ADAFRUIT_GFX fontes;
GxEPD2_290_T94_V2 modeloTela(10, 14, 15, 16);
GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> tela(modeloTela);


// Instância do objeto "Adafruit_NeoPixel"
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

bool CHOVE = 0;

void reconectarWifi()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.begin("Projeto", "2022-11-07");
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

void AbrirVent()
{
    servo1.attach(servoPin);
    servo1.write(0);
    servo2.attach(servoPin2);
    servo2.write(0);
    delay(1000);
    servo1.detach();
    servo2.detach();
    estufaAberta = true;

 
}

void FecharVent()
{
    servo1.attach(servoPin);
    servo1.write(45);
    servo2.attach(servoPin2);
    servo2.write(45);
    delay(1000);
    servo1.detach();
    servo2.detach();
    estufaAberta = false;

    
}

void ligarLuz()
{digitalWrite(relay, LOW);}

void desligarLuz()
{digitalWrite(relay, HIGH);}

void ligarBomba()
{digitalWrite(relayBomba, LOW);}

void desligarBomba()
{digitalWrite(relayBomba, HIGH);}

void color (int r, int g, int b, float brilho){
    for(int i = 0; i<NUMPIXELS;i++){
    pixels.setPixelColor(i, int(r*255*brilho),int(r*255*brilho),int(r*255*brilho));
    pixels.show();  // envia o pixel atualizado para o hardware
}
}


void RegularTemperatura ()
{
    int tempe = dados["temperatura"];
    bool regulTemp = dados["regularTemperatura"];
    int tempe1 = tempe-2;
    int tempe2 = tempe+2;
    Serial.println(tempe);
    if (regulTemp)
    {
        if (tempEstufa<float(tempe1))
        {
            Serial.println("ligarLampada");
            ligarLuz();
        }
        else if (tempEstufa>float(tempe2))
        {
            Serial.println("desligarLampada");
            desligarLuz();
        }
    }
}

void RegularVentilacao ()
{
    int umidade = dados["umidade"];
    bool abrirVent = dados["abrirVent"];
    int umidade1 = umidade-2;
    int umidade2 = umidade+2;
    Serial.println(umidade);
    if (abrirVent)
    {
        if (umidadeEstufa<float(umidade1) && estufaAberta)
        {
            Serial.println("fechar vent");
            FecharVent();
            ligarBomba();
        }
        else if (umidadeEstufa>float(umidade2) && !estufaAberta)
        {
            Serial.println("abrir vent");
            AbrirVent();
            desligarBomba();
        }
    }
}

void RGBLed()
{
    String cor = dados["corRGB"];
    float brilho = float(dados["brilho"])/100;
    if (cor == "vermelho")
    {
        neopixelWrite(RGB_BUILTIN, int(255*brilho), 0, 0);
        for(int i = 0; i<NUMPIXELS;i++){
            pixels.setPixelColor(i, int(255*brilho),0,0);
            pixels.show();  // envia o pixel atualizado para o hardware
        }

    }
    else if (cor == "azul")
    {
        neopixelWrite(RGB_BUILTIN, 0, 0, int(255*brilho));
        for(int i = 0; i<NUMPIXELS;i++){
            pixels.setPixelColor(i,0,0, int(255*brilho));
            pixels.show();  // envia o pixel atualizado para o hardware
        }
    }
    else if (cor == "bicolor")
    {
        neopixelWrite(RGB_BUILTIN, int(255*brilho),0,int(255*brilho));
        for(int i = 0; i<NUMPIXELS;i++){
            if (i%2 == 0)
             {pixels.setPixelColor(i, int(255*brilho),0, 0);}
            else {pixels.setPixelColor(i, 0,0, int(255*brilho));}
            pixels.show();  // envia o pixel atualizado para o hardware
        }
    }
    else if (cor == "branco")
    {
        neopixelWrite(RGB_BUILTIN, int(255*brilho), int(255*brilho), int(255*brilho));
        for(int i = 0; i<NUMPIXELS;i++){
            pixels.setPixelColor(i, int(255*brilho),int(255*brilho), int(255*brilho));
            pixels.show();  // envia o pixel atualizado para o hardware
        }
    }
    else if (cor == "amarelo")
    {
        neopixelWrite(RGB_BUILTIN, int(255*brilho), int(255*brilho), 0);
        for(int i = 0; i<NUMPIXELS;i++){
            pixels.setPixelColor(i, int(255*brilho),int(255*brilho), 0);
            pixels.show();  // envia o pixel atualizado para o hardware
        }
    }
    else
    {
        neopixelWrite(RGB_BUILTIN, 0, 0, 0);
        for(int i = 0; i<NUMPIXELS;i++){
            pixels.setPixelColor(i, 0,0,0);
            pixels.show();  // envia o pixel atualizado para o hardware
        }
    }
}

void paginaComArquivoHTML()
{
    File arquivo = LittleFS.open("/ajustes.html", "r");
    if (!arquivo)
    {
        servidor.send(500, "text/html", "Erro ao abrir HTML");
        return;
    }
    String html = arquivo.readString();
    arquivo.close();
    html.replace("{{input-" + (String)dados["corRGB"] + "}}", "checked");
    html.replace("{{t1}}", (String)dados["horarioAcenderTudo"]);
    html.replace("{{t2}}", (String)dados["horarioApagarTudo"]);
    html.replace("{{brilho}}", (String)dados["brilho"]);
    html.replace("{{temperatura}}", (String)dados["temperatura"]);
    if ((bool)dados["regularTemperatura"]) {
        html.replace("{{input-regularTemperatura}}", "checked");
    } else {
        html.replace("{{input-regularTemperatura}}", "");
    }
    html.replace("{{umidade}}", (String)dados["umidade"]);
    if ((bool)dados["abrirVent"]) {
        html.replace("{{input-abrirVent}}", "checked");
    } else {
        html.replace("{{input-abrirVent}}", "");
    }
    servidor.send(200, "text/html", html);
} 

void tratarDadosSubmetidos()
{

    File arquivo = LittleFS.open("/ajustes.json", "w"); // "write"
    dados["corRGB"] = servidor.arg("cor");
    dados["horarioAcenderTudo"] = servidor.arg("t1");
    dados["horarioApagarTudo"] = servidor.arg("t2");
    dados["brilho"] = servidor.arg("brilho").toInt();
    dados["temperatura"] = servidor.arg("temperatura").toInt();
    dados["umidade"] = servidor.arg("umidade").toInt();
    String regularTemperatura = servidor.arg("regularTemperatura");
    if (regularTemperatura == "true")
    {
        dados["regularTemperatura"] = true;
    }
    else
    {
        dados["regularTemperatura"] = false;
    }
    String abrirVent = servidor.arg("abrirVent");
    if (abrirVent == "true" )
    {
        dados["abrirVent"] = true;
        
    
    }
    else
    {
        dados["abrirVent"] = false;
    
    
    }
    serializeJson(dados, arquivo);
    arquivo.close();
    // faz alguma coisa com esses dados
    // redireciona para outra página
    servidor.sendHeader("Location", "/ajustes");
    servidor.send(303);
    RGBLed();
}

void horario()
{
    getLocalTime(&tempo);
    Serial.println(&tempo, "%H:%M:%S");
    char buffer[100]; 
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &tempo); 
    String tempoString = String(buffer);
    if (tempoString==dados["horarioAcenderTudo"])
    {
      RGBLed() ;
    }   
    else if (tempoString==dados["horarioApagarTudo"])
    {
     neopixelWrite(RGB_BUILTIN, 0, 0, 0);
      for(int i = 0; i<NUMPIXELS;i++){
            pixels.setPixelColor(i, 0,0,0);
            pixels.show();  // envia o pixel atualizado para o hardware
        }
    }

}

void setup()
{
    Serial.begin(115200);
    delay(1500);

    reconectarWifi();
    conexaoSegura.setCACert(certificado1);
    conexaoSegura.setCACert(certificado2);
    pixels.begin();
    pixels.clear();

        // EPAPER
    tela.init();
    tela.setRotation(3);
    tela.fillScreen(GxEPD_WHITE);
    tela.display(true);

    fontes.begin(tela);
    fontes.setForegroundColor(GxEPD_BLACK);

   if (!sensorBME.begin()) {
    Serial.println("Erro no sensor BME");
    while (true);
    }
    sensorBME.setTemperatureOversampling(BME680_OS_8X);
    sensorBME.setHumidityOversampling(BME680_OS_2X);
    sensorBME.setPressureOversampling(BME680_OS_4X);
    sensorBME.setIIRFilterSize(BME680_FILTER_SIZE_3);
    sensorBME.setGasHeater(320, 150);
    if (!LittleFS.begin())
    {
        Serial.println("Falha ao montar o sistema de arquivos!");
        while (true)
        {
        }; // trava programa aqui em caso de erro
    }
    File arquivo = LittleFS.open("/ajustes.json", "r"); // "read"
    if (!arquivo)
    {
        Serial.println("Erro ao abrir o arquivo JSON!");
        while (true)
        {
        }; // trava programa aqui em caso de erro
    }

    configTzTime("<-03>3", "a.ntp.br", "pool.ntp.org");

    deserializeJson(dados, arquivo);
    RGBLed();
    //mettre neopixel

    arquivo.close();
    servidor.on("/ajustes", HTTP_GET, paginaComArquivoHTML);
    servidor.on("/ajustes", HTTP_POST, tratarDadosSubmetidos);
    servidor.begin();
    pinMode(relay, OUTPUT);
    digitalWrite(relay, HIGH);
    pinMode(relayBomba, OUTPUT);
    digitalWrite(relayBomba, HIGH);
    FecharVent();
}

void loop()
{
    reconectarWifi();
    servidor.handleClient();
    sensorBME.performReading();
    unsigned long instanteAtual = millis();
    if (instanteAtual > instanteAnterior + 5000)
    {
        instanteAnterior = instanteAtual;
        tempEstufa = sensorBME.temperature;  // ºC
        umidadeEstufa = sensorBME.humidity;         // %
        Serial.println(tempEstufa);
        Serial.println(umidadeEstufa);
        RegularTemperatura();
        RegularVentilacao();
        horario();
    }

    tela.fillScreen(GxEPD_WHITE);
    tela.fillRect(-10, 120, 350, 130, GxEPD_BLACK);
    fontes.setFont(u8g2_font_open_iconic_all_4x_t);
    fontes.setFontMode(1);

    for (int i = 0; i < 5; i++)
    {
        if (i != 2)
        {
            fontes.drawGlyph(20 + (296 / 5) * i, 128, 0xfe);
            fontes.drawGlyph(20 + (296 / 5) * i, 100, 0x81);
        }
        else
        {
            fontes.drawGlyph(20 + (296 / 5) * i, 80, 0x103);
        }
    }

    tela.drawRect(10, 3, 15, 20, GxEPD_BLACK);
    tela.fillRect(10, 15, 15, 18, GxEPD_BLACK);
    fontes.setFont(u8g2_font_helvB10_te);
    fontes.setFontMode(1);
    fontes.setCursor(30, 20);
    fontes.print((String)sensorBME.temperature + "°C");

    fontes.setFont(u8g2_font_open_iconic_all_4x_t);
    fontes.setFontMode(1);
    fontes.drawGlyph(178, 33, 0x8d);
    fontes.drawGlyph(88, 33, 0x98);
    fontes.setFont(u8g2_font_helvB12_te);
    fontes.setFontMode(1);
    fontes.setCursor(118, 20);
    fontes.print((String)sensorBME.humidity + "%");
    fontes.setCursor(213, 20);
    fontes.print((String)(sensorBME.pressure / 100) + " hPa");

    int leitura = analogRead(18);
    int porcentagemLuz = map(leitura, 0, 4095, 0, 100);

    if (porcentagemLuz >= 60)
    {
        CHOVE = 1;
    }
    else
    {
        CHOVE = 0;
    }

    fontes.setCursor(140, 100);
    fontes.print((String)porcentagemLuz + "%");

    if (CHOVE)
    {
        tela.fillScreen(GxEPD_WHITE);
        fontes.setFont(u8g2_font_open_iconic_all_4x_t);
        fontes.setFontMode(1);

        for (int i = 0; i < 5; i++)
        {
            fontes.drawGlyph(20 + (296 / 5) * i, 128, 0xfe);
            fontes.drawGlyph(20 + (296 / 5) * i, 100, 0x81);
        }

        for (int i = 0; i < 5; i++)
        {
            fontes.drawGlyph(20 + (296 / 5) * i, 35, 0xf1);
        }
    }

    tela.fillRect(-10, 120, 350, 130, GxEPD_BLACK);
    tela.display(true);

}
