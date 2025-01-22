#include <NeoPixelBus.h>
#include "defines.h"

extern NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1Ws2812xMethod> strip;

void sparkle() {
  int sparkleRounds = 75;
  int sparklesPerRound = 4;
  int sparkleDelay = 50;

  for (int sparkleRound = 0; sparkleRound < sparkleRounds; sparkleRound++) {
    strip.ClearTo(RgbColor(0, 0, 0));
    for (int sparkle = 0; sparkle < sparklesPerRound; sparkle++) {
      int ledNumber = random(totalLEDs);
      RgbColor color = RgbColor(random(brightness), random(brightness), random(brightness));
      setPixel(ledNumber, color);
    }
    strip.Show();
    handlingDelay(sparkleDelay);
  }
}

void pacman() {
  RgbColor color = RgbColor(brightness / 2 + 1, brightness / 2 + 1, 0);
  int bites = 4;
  int biteDelay = 1;
  int stepSize = 3;

  strip.ClearTo(RgbColor(0, 0, 0));

  for (int ring = 1; ring < RINGS; ring++) {
    int startLED = startLEDs[ring] + 315 * ringSizes[ring] / 360;
    int endLED = startLEDs[ring] + ringSizes[ring] - 1;
    for (int led = startLED; led <= endLED; led++) {
      setPixel(led, color);
    }

    startLED = startLEDs[ring];
    endLED = startLEDs[ring] + 225 * ringSizes[ring] / 360;
    for (int led = startLED; led <= endLED; led++) {
      setPixel(led, color);
    }
  }

  while (bites-- > 0) {
    for (int angle = 80; angle >= stepSize; angle -= stepSize) {
      drawAngle(270 + angle, RINGS, color);
      drawAngle(270 - angle, RINGS, color);
      strip.Show();
      handlingDelay(biteDelay);
    }

    handlingDelay(200);

    for (int angle = 0; angle <= 60; angle += stepSize) {
      drawAngle(270 + angle, RINGS, RgbColor(0, 0, 0), true, 0.2);
      drawAngle(270 - angle, RINGS, RgbColor(0, 0, 0), true, 0.2);
      strip.Show();
      handlingDelay(biteDelay);
    }

    handlingDelay(200);
  }
}

void scan() {
  int stepSize = 5;
  double intensity = 0.0;
  double intensityStep = 0.001;
  double fadeOutSpeedup = 5.0;
  int maxAngle = 5 * 360;

  int red = random(brightness);
  int green = random(brightness);
  int blue = random(brightness);
  int offsetAngle = random(360);

  strip.ClearTo(RgbColor(0, 0, 0));
  for (int angle = 0; angle < maxAngle; angle += stepSize) {
    if (intensityStep > 0.0 && intensityStep * fadeOutSpeedup * (maxAngle - angle - 45) <= 1.0) {
      intensityStep = -intensityStep * fadeOutSpeedup;
    }
    intensity += intensityStep * stepSize;
    if (intensity > 1.0) intensity = 1.0;
    if (intensity < 0.0) intensity = 0.0;

    RgbColor color = RgbColor(red * intensity, green * intensity, blue * intensity);

    drawAngle((angle + offsetAngle) % 360, RINGS, color, true);
    drawAngle((angle - 90 + offsetAngle) % 360, RINGS, RgbColor(0, 0, 0), true);
    strip.Show();

    handlingDelay(0);
  }

  handlingDelay(100);
}

void fire() {
  int rounds = 150;

  int fireWidth = ringSizes[RINGS - 1];
  int fireHeight = RINGS;
  uint8_t heat[fireWidth][fireHeight];

  for(int i = 0; i < fireWidth; i++) {
    for(int j = 0; j < fireHeight; j++) {
      heat[i][j] = 0;
    }
  }

  for (int round = 0; round < rounds; round++) {
    strip.ClearTo(RgbColor(0, 0, 0));
    int activity = random(100) + 156;
    for (uint8_t h = 0; h < fireWidth; h++) {
      for (uint8_t i = 0; i < fireHeight; i++) {
        int newHeat = heat[h][i] - random(80);
        heat[h][i] = newHeat > 0 ? newHeat : 0;
      }

      for (uint8_t k = fireHeight - 1; k >= 1; k--) {
        uint8_t hleft = (h + fireWidth - 1) % fireWidth;
        uint8_t hright = (h + 1) % fireWidth;
        heat[h][k] = (heat[h][k] + heat[hleft][k - 1] + heat[h][k - 1] + heat[hright][k - 1]) / 4;
      }

      if (random(256) < activity) {
        int newHeat = heat[h][0] + random(100);
        heat[h][0] = newHeat < 255 ? newHeat : 255;
      }
    }

    uint8_t ring = 0;
    do {
      int led = startLEDs[RINGS - ring - 1];
      uint8_t count = ringSizes[RINGS - ring - 1];
      uint16_t td = fireWidth * 255 / count;
      uint16_t t = 0;
      for (uint8_t i = 0; i < count; i++) {
        uint8_t h = heat[t >> 8][fireHeight - 1 - ring];
        if (ring >= RINGS - 2) h = h | 128;

        RgbColor color;
        int t192 = h * 191 / 255;
        int heatramp = t192 & 0x3F;
        heatramp <<= 2;
        if(t192 & 0x80) {
          color = RgbColor(brightness, brightness, heatramp * brightness / 255);
        } else if(t192 & 0x40) {
          color = RgbColor(brightness, heatramp * brightness / 255, 0);
        } else {
          color = RgbColor(heatramp * brightness / 255, 0, 0);
        }

        setPixel(led++, color);
        t += td;
      }
    } while (++ring < RINGS);
    strip.Show();

    handlingDelay(100);
  }
}
