#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <string.h>
#include <Arduino.h>
#include <Adafruit_BME680.h>
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

//--novo--//
#include <WebServer.h>
#include <LittleFS.h>
#include "DatabaseConnection.h"

#define TEST_AP false

WebServer servidor(80);
DatabaseConnection banco;
const char *ssid = "E-Planta";
const char *password = "12345678";
const char *ssid_connect = NULL;     //"Projeto";
const char *password_connect = NULL; //"2022-11-07";
bool wifi_selecionado = false;
JsonDocument dadosSalvos;
//-------//

WiFiClientSecure conexaoSegura;
MQTTClient mqtt;
GxEPD2_290_T94_V2 modeloTela(10, 14, 15, 16);
GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> tela(modeloTela);
U8G2_FOR_ADAFRUIT_GFX fontes;
QRCode qrcode(tela);
Adafruit_BME680 sensorBME;
int sensor_luz = 18;
unsigned long instante_anterior = 0;
bool home = false;
String ssid_f = "Projeto";
String senha_f = "2022-11-07";
int clientes;
int sensor_umid_solo = 4;
int temperatura;
int umidade_ar;
int leituraAnalogica;
int porcentagemLuz;
float umidade_solo;
IPAddress IP;

//GFButton botao(5);

//---novo---//
void lerDados()
{
    File arquivo = LittleFS.open("/dados.json", "r");
    if (!arquivo)
    {
        return;
    }

    deserializeJson(dadosSalvos, arquivo);
    arquivo.close();
    ssid_connect = dadosSalvos["wifi"]["ssid"];
    delay(500);
    password_connect = dadosSalvos["wifi"]["senha"];
}

void salvarDados()
{
    File arquivo = LittleFS.open("/dados.json", "w");
    if (!arquivo)
    {
        return;
    }

    serializeJson(dadosSalvos, Serial);
    serializeJson(dadosSalvos, arquivo);
    arquivo.close();
}

void reconectarWiFi()
{
    if (WiFi.status() != WL_CONNECTED && wifi_selecionado)
    {
        Serial.println(String(ssid_connect) + " | " + String(password_connect));
        delay(500);
        WiFi.begin(ssid_connect, password_connect);
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
        Serial.print("Conectando MQTT...");
        while (!mqtt.connected())
        {
            mqtt.connect("eplanta", "aula", "zowmad-tavQez");
            Serial.print(".");
            delay(1000);
        }
        Serial.println(" conectado!");
    }
}

void desenhaQRcode(String link)
{
    qrcode.scale = 4;
    qrcode.draw(link, -10, 80);

    tela.display(true);
}

void pagina_configuracoes()
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

void pagina_inicial()
{
    home = true;

    tela.fillScreen(GxEPD_WHITE);
    fontes.setFont(u8g2_font_lubR14_te);

    // nome da planta
    fontes.setFontMode(1);
    fontes.setCursor(0, 40);
    String nome_planta = dadosSalvos["planta"]["nome"];
    fontes.print(nome_planta);
    fontes.setFont(u8g2_font_luBS18_tf);

    // luminosidade
    fontes.setFontMode(1);
    fontes.setCursor(62, 115);
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

    fontes.setFont(u8g2_font_open_iconic_all_6x_t);
    fontes.setFontMode(1);
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

void pagina_bem_vindo()
{
    home = false;
    tela.fillScreen(GxEPD_WHITE);
    fontes.setFont(u8g2_font_helvB18_te);
    fontes.setFont(u8g2_font_helvB18_te);
    fontes.setFontMode(1);
    fontes.setCursor(0, 35);
    fontes.print("Bem-vindo");

    // qrcode
    // String wifi_qr_code = "WIFI:T:WPA2;S:" + (String)ssid + ";P:" + (String)password + ";;";
    String link = "http://" + IP.toString() + "/plantas";

    qrcode.scale = 4;
    qrcode.draw(link, 0, 140);
    fontes.setFont(u8g2_font_helvB14_te);
    fontes.setFontMode(1);
    fontes.setCursor(0,75);
    fontes.print("Wifi: E-Planta");
    fontes.setCursor(0,100);
    fontes.setFont(u8g2_font_helvB10_te);
    fontes.setFontMode(1);
    fontes.print("Senha: 12345678");

    tela.display(true);
}

void pagina_default()
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

void pagina_envia()
{
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
}

void conectarWifiHandler()
{
    if (servidor.hasArg("plain"))
    {
        String body = servidor.arg("plain");

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);

        if (error)
        {
            Serial.println("Falha na análise do JSON");
            servidor.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON inválido\"}");
            return;
        }

        ssid_connect = doc["ssid"];
        password_connect = doc["password"];

        // Tenta conectar ao WiFi
        WiFi.begin(ssid_connect, password_connect);
        if (WiFi.waitForConnectResult() == WL_CONNECTED)
        {
            wifi_selecionado = true;
            dadosSalvos["wifi"]["ssid"] = ssid_connect;
            dadosSalvos["wifi"]["senha"] = password_connect;
            salvarDados();
            Serial.println("Conectado com sucesso ao WiFi");
            pagina_inicial();
            servidor.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Conexão bem-sucedida\"}");
        }
        else
        {
            Serial.println("Falha na conexão WiFi");
            ssid_connect = NULL;
            password_connect = NULL;
            wifi_selecionado = false;
            servidor.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Falha ao conectar\"}");
        }
    }
    else
    {
        servidor.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Dados ausentes\"}");
    }
}

int forca_wifi(int rssi)
{
    if (rssi >= -55)
        return 4;
    if (rssi >= -67)
        return 3;
    if (rssi >= -90)
        return 2;
    return 1;
}

void paginaConectividade()
{
    File arquivo = LittleFS.open("/listagem_wifi.html", "r");
    if (!arquivo)
    {
        servidor.send(500, "text/html", "Erro no HTML");
        return;
    }
    String html = arquivo.readString();
    arquivo.close();

    String listagem = "";
    int n = WiFi.scanNetworks();
    if (n == 0)
    {
        listagem = "<h1>NENHUM WIFI ENCONTRADO</h1>";
    }
    else
    {
        for (int i = 0; i < n; ++i)
        {
            String nome = WiFi.SSID(i).c_str();
            int forca = forca_wifi(WiFi.RSSI(i));
            bool aberto;
            switch (WiFi.encryptionType(i))
            {
            case WIFI_AUTH_OPEN:
                aberto = true;
                break;
            default:
                aberto = false;
                break;
            }

            listagem += "<li>";
            listagem += "<button onclick=\"showPasswordBox('" + nome + "', " + (aberto ? "false" : "true") + ")\"><span class=\"wifi-icon\" data-type=";
            listagem += String(forca);
            listagem += "></span>";
            if (!aberto)
            {
                listagem += "<span class=\"lock-icon\"></span>";
            }
            listagem += nome;
            listagem += "</button></li>";
        }
        html.replace("{{listagem_wifi}}", listagem);
    }

    servidor.send(200, "text/html", html);
}

void paginaSelecionarPlanta()
{
    File arquivo = LittleFS.open("/plantas.html", "r");
    if (!arquivo)
    {
        servidor.send(500, "text/html", "Erro no HTML");
        return;
    }
    pagina_configuracoes();
    String html = arquivo.readString();
    arquivo.close();
    servidor.send(200, "text/html", html);
}

void listarTodasPlantas()
{
    try
    {
        JsonDocument resultados = banco.execute("SELECT * FROM plantas");
        String texto_json;
        serializeJson(resultados, texto_json);
        servidor.send(200, "text/json", texto_json);
    }
    catch (const std::exception &e)
    {
        Serial.println(e.what());
    }
}

void selecionarPlanta()
{
    if (!servidor.hasArg("plain"))
    {
        servidor.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Dados ausentes.\"}");
        return;
    }

    String body = servidor.arg("plain");

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
        servidor.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON inválido.\"}");
        return;
    }

    dadosSalvos["planta"] = doc;
    salvarDados();
    Serial.println(body);
    servidor.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Planta configurada com sucesso.\"}");
}

//---------//

void botaoClicado(GFButton &botao) {
    Serial.println("clicou");
    pagina_bem_vindo();
}



bool ligou = false;

void setup()
{
    // serial
    Serial.begin(115200);
    delay(500);

    // botao
    pinMode(5, INPUT);

    // sensor umidade solo
    pinMode(sensor_umid_solo, INPUT);


    if (!LittleFS.begin())
    {
        Serial.println("LittleFS falhou!");
        while (true)
        {
        };
    }

    try
    {
        banco.open("/littlefs/banco.db");
    }
    catch (const std::exception &e)
    {
        Serial.println(e.what());
        while (true)
            ;
    }

    // sensor BME
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

    //botao.setPressHandler(botaoClicado);

    // tela
    tela.init();
    tela.setRotation(2);
    tela.fillScreen(GxEPD_WHITE);
    tela.display(false);

    // fontes
    fontes.begin(tela);
    fontes.setForegroundColor(GxEPD_BLACK);

    lerDados();
    if (ssid_connect != NULL)
    {
        Serial.println("entrou aqui");
        wifi_selecionado = true;
        ssid_connect = dadosSalvos["wifi"]["ssid"];
        password_connect = dadosSalvos["wifi"]["senha"];
        Serial.println("ssid_connect: " + String(ssid_connect) + "\npassword_connect: " + String(password_connect));
        delay(500);
    }

    Serial.println("WiFi selecionado: " + String(wifi_selecionado));
    serializeJson(dadosSalvos, Serial);
    Serial.print('\n');

    //---novo---//
    if (!wifi_selecionado)
    {
        WiFi.softAP(ssid, password);
        IP = WiFi.softAPIP();
        Serial.println(IP);
    }
    else
    {
        reconectarWiFi();
    }

    delay(100);

    servidor.on("/wifi", HTTP_GET, paginaConectividade);
    servidor.on("/plantas", HTTP_GET, paginaSelecionarPlanta);
    servidor.on("/conectar_wifi", HTTP_POST, conectarWifiHandler);
    servidor.on("/api/plantas", HTTP_GET, listarTodasPlantas);
    servidor.on("/api/selecionar_planta", HTTP_POST, selecionarPlanta);
    servidor.begin();

    // mqtt
    if (wifi_selecionado)
    {
        mqtt.begin("mqtt.janks.dev.br", 8883, conexaoSegura);
        conexaoSegura.setCACert(certificado1);
        reconectarMQTT();
    }

    if (!sensorBME.performReading())
    {
        Serial.println("Failed to perform reading :(");
        return;
    }

    if (wifi_selecionado) {
        pagina_inicial();
    } else {
        pagina_default();
    }
}

void loop()
{
    unsigned long instante_atual = millis();
    //botao.process();

    if(instante_atual > instante_anterior + 5000 && wifi_selecionado){
        pagina_inicial();
        instante_anterior = instante_atual;
    }

    if (instante_atual > 17000 && !ligou) {
        ligou = true;
        pagina_bem_vindo();
    }

    if (!sensorBME.performReading())
    {
        Serial.println("Failed to perform reading :(");
        return;
    }

    if (wifi_selecionado)
    {
        reconectarWiFi();
        reconectarMQTT();
        mqtt.loop();
    }
    else
    {
        servidor.handleClient();
    }
}
