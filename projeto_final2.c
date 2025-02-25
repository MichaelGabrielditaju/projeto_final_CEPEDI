#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "hardware/irq.h"
#include "ws2818b.pio.h"
#include "matrizes.h"
#include "hardware/pio.h"
#include "neopixel_pio.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C
#define JOYSTICK_Y_PIN 26

#define LED_VERDE 12
#define LED_AZUL 11
#define LED_VERMELHO 13

#define MM_POR_VARIACAO 14
 

#define LIMITE_NIVEL 290

#define LED_COUNT 25
#define LED_PIN 7
#define BOTAO_A 5
#define BOTAO_B 6
#define DEBOUNCE_TIME 50  // Debounce para os botões em ms

ssd1306_t ssd;
int recorde_nivel = 0;
int MAX_AGUA_MM = 300; // Novo valor para armazenar o valor máximo lido

PIO np_pio;
uint sm;
npLED_t leds[LED_COUNT];

volatile bool alerta_ativo = false;

// Declarações antecipadas
void botao_a_callback(uint gpio, uint32_t events);
void botao_b_callback(uint gpio, uint32_t events);
int ler_nivel_agua();

void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, false);
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
    for (uint i = 0; i < LED_COUNT; ++i) {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
}

int getIndex(int x, int y) {
    return (y % 2 == 0) ? (24 - (y * 5 + x)) : (24 - (y * 5 + (4 - x)));
}

void desenha_fig(int matriz[5][5][3]) {
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            int index = getIndex(i, j);
            npSetLED(index, matriz[j][i][0], matriz[j][i][1], matriz[j][i][2]);
        }
    }
    npWrite();
}

void inicializar_botoes() {
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &botao_a_callback);
    
    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &botao_b_callback); 
}

void botao_a_callback(uint gpio, uint32_t events) {
    recorde_nivel = 0; // Zera o recorde
}

void botao_b_callback(uint gpio, uint32_t events) {
    MAX_AGUA_MM = ler_nivel_agua(); // Define o valor máximo
}

void atualizar_display(int nivel_atual, int recorde) {
    char buffer[20];
    ssd1306_fill(&ssd, false);
    sprintf(buffer, "Nivel: %d mm", nivel_atual);
    ssd1306_draw_string(&ssd, buffer, 10, 10);
    sprintf(buffer, "Recorde: %d mm", recorde);
    ssd1306_draw_string(&ssd, buffer, 10, 30);
    sprintf(buffer, "Max: %d mm", MAX_AGUA_MM); // Exibe o valor máximo
    ssd1306_draw_string(&ssd, buffer, 10, 50);
    ssd1306_send_data(&ssd);
}

void atualizar_leds(int nivel) {
    bool azul = (nivel > MAX_AGUA_MM / 2);
    bool verde = (nivel >= 1) && (nivel < MAX_AGUA_MM / 2);
    bool vermelho = (nivel < 1);
    
    gpio_put(LED_VERDE, verde ? 1 : 0);
    gpio_put(LED_AZUL, azul ? 1 : 0);
    gpio_put(LED_VERMELHO, vermelho ? 1 : 0);
}

int ler_nivel_agua() {
    adc_select_input(0);
    int adc_val = adc_read();
    return adc_val / MM_POR_VARIACAO;
}

void inicializa_leds() {
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_put(LED_VERDE, 0);  // Garante que inicie apagado

    gpio_init(LED_AZUL);
    gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_put(LED_AZUL, 0);

    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);
    gpio_put(LED_VERMELHO, 0);
}

void nivel_matriz(int nivel) {
    int index = 0;

    if (nivel <= 60) {
        index = 0;
    } else if (nivel <= 120) {
        index = 1;
    } else if (nivel <= 180) {
        index = 2;
    } else if (nivel <= 240) {
        index = 3;
    } else if (nivel <= 300) {
        index = 4;
    } else {
        desenha_fig(matriz_apagada);
        return;
    }

    desenha_fig(matrizes[index]); // Exibe a matriz correspondente ao nível
    npWrite(); // Garante que os LEDs são atualizados
}

void piscar_matriz() {
    // Pisca LEDs
    for (int i = 0; i < 10; i++) { // Pisca 10 vezes
        // Acende todos os LEDs
        desenha_fig(matrizes[4]);
        npWrite();
        sleep_ms(20); // Espera meio segundo

        // Apaga todos os LEDs
        for (int j = 0; j < LED_COUNT; j++) {
            npSetLED(j, 0, 0, 0); // Desliga LEDs
        }
        npWrite();
        sleep_ms(20); // Espera meio segundo
    }
}

int main() {
    stdio_init_all();
    
    adc_init();
    adc_gpio_init(JOYSTICK_Y_PIN);
    inicializar_botoes(); // Chame a função para inicializar os botões
    
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    ssd1306_init(&ssd, 128, 64, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);

    inicializa_leds();
    npInit(LED_PIN); // Inicializa os LEDs uma única vez
    
    while (1) {
        int nivel_atual = ler_nivel_agua();
        if (nivel_atual > recorde_nivel) {
            recorde_nivel = nivel_atual;
        }
        
        atualizar_display(nivel_atual, recorde_nivel);
        atualizar_leds(nivel_atual);
        nivel_matriz(nivel_atual); // Atualiza a matriz de LEDs corretamente
        if(nivel_atual > LIMITE_NIVEL) {
            piscar_matriz();
        }
        
        sleep_ms(20); // Espera antes de nova leitura
    }
}