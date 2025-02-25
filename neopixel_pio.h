#ifndef NEOPIXEL_PIO_H
#define NEOPIXEL_PIO_H

#define LED_COUNT 25  // Número de LEDs na matriz, ajusta conforme seu caso

// Estrutura para representar um LED com cores RGB
typedef struct {
    uint8_t R;
    uint8_t G;
    uint8_t B;
} npLED_t;

#endif // NEOPIXEL_PIO_H
