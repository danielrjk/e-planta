#include <Adafruit_GFX.h>
#include "qrcodegen.h"

class QRCode {
private:
  uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
  uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];

public:
  Adafruit_GFX& display;
  uint16_t scale;
  uint16_t backgroundColor;
  uint16_t foregroundColor;
  qrcodegen_Ecc errorCorrectionLevel;

  QRCode(
    Adafruit_GFX& d,
    uint16_t _scale = 1,
    uint16_t c1 = 0xFFFF,
    uint16_t c2 = 0x0000,
    qrcodegen_Ecc error = qrcodegen_Ecc_LOW
  )
    : display(d),
      scale(_scale),
      backgroundColor(c1),
      foregroundColor(c2),
      errorCorrectionLevel(error) {
  }

  bool draw(String text, int16_t x, int16_t y) {
    return draw(text.c_str(), x, y);
  }

  bool draw(const char* text, int16_t x, int16_t y) {
    int x0 = x, y0 = y;

    bool ok = qrcodegen_encodeText(
      text, tempBuffer, qrcode,
      errorCorrectionLevel,
      qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX,
      qrcodegen_Mask_AUTO, true
    );

    if (!ok) {
      return false;
    }

    int tamanho = qrcodegen_getSize(qrcode);

    int padding = scale * 3;
    display.fillRect(
      x0,
      y0,
      tamanho * scale + 2 * padding,
      tamanho * scale + 2 * padding,
      backgroundColor
    );

    for (uint8_t y = 0; y < tamanho; y++) {
      for (uint8_t x = 0; x < tamanho; x++) {
        for (int i = 0; i < scale; i++) {
          for (int j = 0; j < scale; j++) {
            if (qrcodegen_getModule(qrcode, x, y) == 1) {
              display.drawPixel(
                x0 + padding + scale * x + j,
                y0 + padding + scale * y + i,
                foregroundColor
              );
            }
            else {
              display.drawPixel(
                x0 + padding + scale * x + j,
                y0 + padding + scale * y + i,
                backgroundColor
              );
            }
          }
        }
      }
    }

    return true;
  }
};