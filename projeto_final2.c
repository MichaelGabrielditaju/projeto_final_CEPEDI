    #include <stdio.h>
    #include <stdlib.h>
    #include "pico/stdlib.h"
    #include "hardware/adc.h"
    #include "hardware/gpio.h"
    #include "hardware/i2c.h"
    #include "lib/ssd1306.h"

    #define I2C_PORT i2c1
    #define I2C_SDA 14
    #define I2C_SCL 15
    #define ENDERECO 0x3C
    #define JOYSTICK_Y_PIN 26

    #define LED_VERDE 12
    #define LED_AZUL 11
    #define LED_VERMELHO 13

    #define MM_POR_VARIACAO 14
    #define MAX_AGUA_MM 300

    ssd1306_t ssd;
    int recorde_nivel = 0;

    void atualizar_display(int nivel_atual, int recorde) {
        char buffer[20];
        ssd1306_fill(&ssd, false);
        sprintf(buffer, "Nivel: %d mm", nivel_atual);
        ssd1306_draw_string(&ssd, buffer, 10, 10);
        sprintf(buffer, "Recorde: %d mm", recorde);
        ssd1306_draw_string(&ssd, buffer, 10, 30);
        ssd1306_send_data(&ssd);
    }

    void atualizar_leds(int nivel) {
        bool verde = (nivel < MAX_AGUA_MM / 3);
        bool azul = (nivel >= MAX_AGUA_MM / 3) && (nivel < 2 * MAX_AGUA_MM / 3);
        bool vermelho = (nivel >= 2 * MAX_AGUA_MM / 3);
    
        gpio_put(LED_VERDE, verde ? 1 : 0);
        gpio_put(LED_AZUL, azul ? 1 : 0);
        gpio_put(LED_VERMELHO, vermelho ? 1 : 0);
    
        printf("Nivel: %d mm -> LED Verde: %d, LED Azul: %d, LED Vermelho: %d\n",
               nivel, verde, azul, vermelho);
    }

    int ler_nivel_agua() {
        adc_select_input(0);
        int adc_val = adc_read();
        return adc_val / MM_POR_VARIACAO;
    }

    void inicializa_leds()
    {
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

    int main() {
        stdio_init_all();
        
        adc_init();
        adc_gpio_init(JOYSTICK_Y_PIN);
        
        i2c_init(I2C_PORT, 400 * 1000);
        gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
        gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
        ssd1306_init(&ssd, 128, 64, false, ENDERECO, I2C_PORT);
        ssd1306_config(&ssd);

        inicializa_leds();
        
        while (1) {
            int nivel_atual = ler_nivel_agua();
            if (nivel_atual > recorde_nivel) {
                recorde_nivel = nivel_atual;
            }
            
            atualizar_display(nivel_atual, recorde_nivel);
            atualizar_leds(nivel_atual);
            
            sleep_ms(500);
        }
    }