#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include "certificados.h"
#include <wifi.h>
#include <WebServer.h>
#include <GFButton.h> 
#include <HTTPClient.h>
#include <uri/UriBraces.h>
WebServer servidor(80); // porta 80 (padrão do HTTP)
JsonDocument dados;
unsigned long instanteAnterior = 0;
int LUZ = 18;
int pinoLuz = 0;
WiFiClientSecure conexaoSegura; 
String chaveTelegram = "7513651643:AAGYN0U_q_9IlHCtbWVLmKxwZZEUykCwB2E";
String idDoChat = "5689474343";
String enderecoBase = "https://api.telegram.org/bot" + chaveTelegram; 

GFButton sensorDeMovimento(21);

int leds[] = {42, 41, 40};


void reconectarWifi() {
    if (WiFi.status() != WL_CONNECTED) 
        {
        WiFi.begin("Projeto", "2022-11-07");
        Serial.print("Conectando ao WiFi...");
        while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
        }

        Serial.print("conectado!\nEndereço IP: ");
        Serial.println(WiFi.localIP());
        
    }
}

void RGBLed()
{
    String cor = dados["corRGB"] ;
    if (cor=="vermelho")
    { neopixelWrite(RGB_BUILTIN,255,0,0);}
    else if (cor=="verde")
      {neopixelWrite(RGB_BUILTIN, 0,255,0);}
    else if (cor=="amarelo")
      {neopixelWrite(RGB_BUILTIN, 255,255,0);}
    else if (cor=="rosa")
      {neopixelWrite(RGB_BUILTIN,255,0,255);}
    else 
    {
    neopixelWrite(RGB_BUILTIN,0,0,0);
    }
  
}
void sensorLuz()
{
    int leituraAnalogica = analogRead(LUZ); // entre 0 e 4096
    int porcentagemLuz = map(leituraAnalogica, 0, 4095, 0, 100);
    Serial.print(porcentagemLuz);
    Serial.println("%");
    if (porcentagemLuz < 20)
    {
        digitalWrite(leds[pinoLuz], LOW); 
    }
    else 
    {
            digitalWrite(leds[pinoLuz], HIGH);
    }
}
void mandarMensagem() {
    JsonDocument dados1;
    dados1["chat_id"] = idDoChat;
    String mensagem = "Movimento";
    dados1["text"] = mensagem;
    String dadosString;
    serializeJson(dados1, dadosString);
    String enderecoMensagemTexto = enderecoBase + "/sendMessage";
    HTTPClient requisicao;
    requisicao.begin(conexaoSegura, enderecoMensagemTexto);
    requisicao.addHeader("Content-Type", "application/json");
    int codigoDoResultado = requisicao.POST(dadosString);
    String resposta = requisicao.getString();
    Serial.println(resposta);
    if (codigoDoResultado != 200) {
    Serial.println("Erro ao enviar mensagem!");
    }
}

void movimentoDetectado (GFButton& sensor) { 
    Serial.println("Detectei movimento!"); 
    if (dados["alertarMovimento"] ==true)
    {
        mandarMensagem();
    }


} 
void inerciaDetectada (GFButton& sensor) { 
    Serial.println("Detectei inércia!"); 
} 

void receberParametrosLed () {
    String estado = servidor.pathArg(0);
    int i = servidor.pathArg(1).toInt();
    if (estado=="ligar")
    {
         digitalWrite(leds[i], LOW);
    }
    else
    {
        digitalWrite(leds[i], HIGH);
    }
    servidor.send(200, "text/html", "ok!");
}

void setup()
{
    Serial.begin(115200);
    delay(1500);
    
    reconectarWifi(); 
    conexaoSegura.setCACert(certificado1); 
    conexaoSegura.setCACert(certificado2);
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

    deserializeJson(dados, arquivo);
    RGBLed();
    pinoLuz = dados["ledSensorLuz"];
    for (int i=0; i<=2; i++)
    {
        pinMode(leds[i], OUTPUT);
    }
    
    arquivo.close();
    sensorDeMovimento.setReleaseHandler(movimentoDetectado); 
    sensorDeMovimento.setPressHandler(inerciaDetectada);
    servidor.on(UriBraces("/led/{}/{}"), HTTP_GET, receberParametrosLed); 
    servidor.begin(); 
}



void loop()
{
     reconectarWifi(); 
    servidor.handleClient(); 
unsigned long instanteAtual = millis();
    if (instanteAtual > instanteAnterior + 1000) 
    {
        instanteAnterior = instanteAtual;
        sensorLuz();
        
    
    }
    sensorDeMovimento.process();
}
