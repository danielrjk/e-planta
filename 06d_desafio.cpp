#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "DatabaseConnection.h"

#define TEST_AP false

WebServer servidor(80);

DatabaseConnection banco;

const char *ssid = "E-Planta";
const char *password = "12345678";

const char *ssid_connect = "Projeto";
const char *passord_connect = "2022-11-07";

bool wifi_selecionado = false;

void reconectarWiFi()
{
    if (WiFi.status() != WL_CONNECTED && (wifi_selecionado || !TEST_AP))
    {
        WiFi.begin(ssid_connect, passord_connect);
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
        passord_connect = doc["password"];

        // Tenta conectar ao WiFi
        WiFi.begin(ssid_connect, passord_connect);
        if (WiFi.waitForConnectResult() == WL_CONNECTED)
        {
            wifi_selecionado = true;
            Serial.println("Conectado com sucesso ao WiFi");
            servidor.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Conexão bem-sucedida\"}");
        }
        else
        {
            Serial.println("Falha na conexão WiFi");
            ssid_connect = "";
            passord_connect = "";
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

void setup()
{
    Serial.begin(115200);
    delay(100);
    if (TEST_AP)
    {
        WiFi.softAP(ssid, password);
        IPAddress IP = WiFi.softAPIP();
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
    servidor.begin();

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
}

void loop()
{
    reconectarWiFi();
    servidor.handleClient();
}
