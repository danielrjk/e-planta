#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "DatabaseConnection.h"
#include <esp_camera.h>

#define TEST_AP true

WebServer servidor(80);
DatabaseConnection banco;

camera_config_t config = {
    .pin_pwdn = -1, .pin_reset = -1, .pin_xclk = 15, .pin_sscb_sda = 4, .pin_sscb_scl = 5, .pin_d7 = 16, .pin_d6 = 17, .pin_d5 = 18, .pin_d4 = 12, .pin_d3 = 10, .pin_d2 = 8, .pin_d1 = 9, .pin_d0 = 11, .pin_vsync = 6, .pin_href = 7, .pin_pclk = 13, .xclk_freq_hz = 20000000, .ledc_timer = LEDC_TIMER_0, .ledc_channel = LEDC_CHANNEL_0, .pixel_format = PIXFORMAT_JPEG, .frame_size = FRAMESIZE_SVGA, .jpeg_quality = 10, .fb_count = 2};

const char *ssid = "E-Planta";
const char *password = "12345678";

const char *ssid_connect = "Projeto";
const char *password_connect = "2022-11-07";

bool wifi_selecionado = false;

JsonDocument dadosSalvos;

void lerDados() {
    File arquivo = LittleFS.open("/dados.json", "r");
    if (!arquivo)
    {
        return;
    }

    deserializeJson(dadosSalvos, arquivo);
    arquivo.close();
}

void salvarDados() {
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
    if (WiFi.status() != WL_CONNECTED && (wifi_selecionado || !TEST_AP))
    {
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
            servidor.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Conexão bem-sucedida\"}");
        }
        else
        {
            Serial.println("Falha na conexão WiFi");
            ssid_connect = "";
            password_connect = "";
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

void paginaCamera()
{
    File arquivo = LittleFS.open("/camera.html", "r");
    if (!arquivo)
    {
        servidor.send(500, "text/html", "Erro no HTML");
        return;
    }
    String html = arquivo.readString();
    arquivo.close();
    servidor.send(200, "text/html", html);
}

void paginaStream() {
    WiFiClient client = servidor.client();

    // Envia cabeçalhos HTTP
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    client.println();

    while (client.connected()) {
        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Erro ao capturar frame");
            continue;
        }

        // Envia cada frame com cabeçalhos específicos
        client.printf("--frame\r\n");
        client.printf("Content-Type: image/jpeg\r\n");
        client.printf("Content-Length: %u\r\n\r\n", fb->len);
        client.write(fb->buf, fb->len);
        client.printf("\r\n");

        esp_camera_fb_return(fb);
    }
}

void selecionarPlanta() {
    if (!servidor.hasArg("plain")) {
        servidor.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Dados ausentes.\"}");
        return;
    }

    String body = servidor.arg("plain");

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
        servidor.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON inválido.\"}");
        return;
    }

    dadosSalvos["planta"] = doc;
    salvarDados();
    Serial.println(body);
    servidor.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Planta configurada com sucesso.\"}");
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
    servidor.on("/camera", HTTP_GET, paginaCamera);
    servidor.on("/stream", HTTP_GET, paginaStream);
    servidor.on("/conectar_wifi", HTTP_POST, conectarWifiHandler);
    servidor.on("/api/plantas", HTTP_GET, listarTodasPlantas);
    servidor.on("/api/selecionar_planta", HTTP_POST, selecionarPlanta);
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

    //config.grab_mode = CAMERA_GRAB_LATEST;
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Falha ao inicializar a câmera: 0x%x", err);
        while (true)
        {
        }; // trava programa aqui em caso de erro
    }
}

void loop()
{
    reconectarWiFi();
    servidor.handleClient();
}
