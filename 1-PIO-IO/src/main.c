#include "asf.h"

// LED DA PLACA
#define LED_PIO PIOC                        // periferico que controla o LED
#define LED_PIO_ID ID_PIOC                  // ID do periférico PIOC (controla LED)
#define LED_PIO_IDX 8                       // ID do LED no PIO
#define LED_PIO_IDX_MASK (1 << LED_PIO_IDX) // Mascara para CONTROLARMOS o LED

// LED 1
#define LED1_PIO PIOA                         // periferico que controla o LED
#define LED1_PIO_ID ID_PIOA                   // ID do periférico PIOC (controla LED)
#define LED1_PIO_IDX 0                        // ID do LED no PIO
#define LED1_PIO_IDX_MASK (1 << LED1_PIO_IDX) // Mascara para CONTROLARMOS o LED

// LED 2
#define LED2_PIO PIOC                         // periferico que controla o LED
#define LED2_PIO_ID ID_PIOC                   // ID do periférico PIOC (controla LED)
#define LED2_PIO_IDX 30                       // ID do LED no PIO
#define LED2_PIO_IDX_MASK (1 << LED2_PIO_IDX) // Mascara para CONTROLARMOS o LED

// LED 3
#define LED3_PIO PIOB                         // periferico que controla o LED
#define LED3_PIO_ID ID_PIOB                   // ID do periférico PIOC (controla LED)
#define LED3_PIO_IDX 2                        // ID do LED no PIO
#define LED3_PIO_IDX_MASK (1 << LED3_PIO_IDX) // Mascara para CONTROLARMOS o LED

// BOTÃO DA PLACA
#define BUT_PIO PIOA
#define BUT_PIO_ID ID_PIOA
#define BUT_PIO_IDX 11
#define BUT_PIO_IDX_MASK (1u << BUT_PIO_IDX) // esse já está pronto.

// BOTÃO 1
#define BUT1_PIO PIOD
#define BUT1_PIO_ID ID_PIOD
#define BUT1_PIO_IDX 28
#define BUT1_PIO_IDX_MASK (1u << BUT1_PIO_IDX) // esse já está pronto.

// BOTÃO 2
#define BUT2_PIO PIOC
#define BUT2_PIO_ID ID_PIOC
#define BUT2_PIO_IDX 31
#define BUT2_PIO_IDX_MASK (1u << BUT2_PIO_IDX) // esse já está pronto.

// BOTÃO 3
#define BUT3_PIO PIOA
#define BUT3_PIO_ID ID_PIOA
#define BUT3_PIO_IDX 19
#define BUT3_PIO_IDX_MASK (1u << BUT3_PIO_IDX) // esse já está pronto.

// CÓDIGO OMITIDO

// Função de inicialização do uC
void init(void) {
    // responsável por aplicar as configurações do arquivo config/conf_clock.h configurado para operar em 300 MHz
    sysclk_init();

    // Desativa WatchDog Timer
    WDT->WDT_MR = WDT_MR_WDDIS;

    // Ativa o PIO na qual o LED foi conectado, para que possamos controlar o LED.
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOB);
    pmc_enable_periph_clk(ID_PIOC);
    pmc_enable_periph_clk(ID_PIOD);

    // Inicializa PC8 como saída
    pio_set_output(LED_PIO, LED_PIO_IDX_MASK, 0, 0, 0);
    pio_set_output(LED1_PIO, LED1_PIO_IDX_MASK, 0, 0, 0);
    pio_set_output(LED2_PIO, LED2_PIO_IDX_MASK, 0, 0, 0);
    pio_set_output(LED3_PIO, LED3_PIO_IDX_MASK, 0, 0, 0);

    // configura pino ligado ao botão como entrada com um pull-up.
    pio_set_input(BUT_PIO, BUT_PIO_IDX_MASK, PIO_DEFAULT);
    pio_pull_up(BUT_PIO, BUT_PIO_IDX_MASK, 1);
	pio_set_input(BUT1_PIO, BUT1_PIO_IDX_MASK, PIO_DEFAULT);
    pio_pull_up(BUT1_PIO, BUT1_PIO_IDX_MASK, 1);
	pio_set_input(BUT2_PIO, BUT2_PIO_IDX_MASK, PIO_DEFAULT);
    pio_pull_up(BUT2_PIO, BUT2_PIO_IDX_MASK, 1);
	pio_set_input(BUT3_PIO, BUT3_PIO_IDX_MASK, PIO_DEFAULT);
    pio_pull_up(BUT3_PIO, BUT3_PIO_IDX_MASK, 1);
}

void pisca_led(Pio *p_pio, const uint32_t ul_mask) {
    for (int i = 0; i < 10; i++) {
        pio_clear(p_pio, ul_mask);
        delay_ms(200);
        pio_set(p_pio, ul_mask);
        delay_ms(200);
    }
}

/************************************************************************/
/* Main                                                                 */
/************************************************************************/

// Funcao principal chamada na inicalizacao do uC.
int main(void) {
    init();
    sysclk_init();
    delay_init();

    // super loop
    // aplicacoes embarcadas não devem sair do while(1).
    while (1) {
        if (!pio_get(BUT_PIO, PIO_INPUT, BUT_PIO_IDX_MASK)) {
            pisca_led(LED_PIO, LED_PIO_IDX_MASK);
        } 
		if (!pio_get(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK)) {
            pisca_led(LED1_PIO, LED1_PIO_IDX_MASK);
        }
		if (!pio_get(BUT2_PIO, PIO_INPUT, BUT2_PIO_IDX_MASK)) {
            pisca_led(LED2_PIO, LED2_PIO_IDX_MASK);
        }
		if (!pio_get(BUT3_PIO, PIO_INPUT, BUT3_PIO_IDX_MASK)) {
            pisca_led(LED3_PIO, LED3_PIO_IDX_MASK);
        } else {
            pio_set(LED_PIO, LED_PIO_IDX_MASK);
            pio_set(LED1_PIO, LED1_PIO_IDX_MASK);
            pio_set(LED2_PIO, LED2_PIO_IDX_MASK);
            pio_set(LED3_PIO, LED3_PIO_IDX_MASK);
        }
    }

   return 0;
}
