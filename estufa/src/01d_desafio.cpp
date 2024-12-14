#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <Adafruit_BME680.h>

U8G2_FOR_ADAFRUIT_GFX fontes;
GxEPD2_290_T94_V2 modeloTela(10, 14, 15, 16);
GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> tela(modeloTela);
Adafruit_BME680 sensorBME;

bool CHOVE = 0;

void setup()
{
    Serial.begin(115200);

    // EPAPER
    tela.init();
    tela.setRotation(3);
    tela.fillScreen(GxEPD_WHITE);
    tela.display(true);

    fontes.begin(tela);
    fontes.setForegroundColor(GxEPD_BLACK);

    // SENSOR
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
}

void loop()
{
    sensorBME.performReading();

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