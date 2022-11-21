/************************************************************************
 * 5 semestre - Eng. da Computao - Insper
 * Rafael Corsi - rafael.corsi@insper.edu.br
 *
 * Material:
 *  - Kit: ATMEL SAME70-XPLD - ARM CORTEX M7
 *
 * Objetivo:
 *  - Demonstrar interrupção do PIO
 *
 * Periféricos:
 *  - PIO
 *  - PMC
 *
 * Log:
 *  - 10/2018: Criação
 ************************************************************************/

/************************************************************************/
/* includes                                                             */
/************************************************************************/

#include "asf.h"

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

/************************************************************************/
/* defines                                                              */
/************************************************************************/

// LED
#define LED_PIO PIOC
#define LED_PIO_ID ID_PIOC
#define LED_IDX 8
#define LED_IDX_MASK (1 << LED_IDX)

// LED 2
#define LED2_PIO PIOC                         // periferico que controla o LED
#define LED2_PIO_ID ID_PIOC                   // ID do periférico PIOC (controla LED)
#define LED2_PIO_IDX 30                       // ID do LED no PIO
#define LED2_PIO_IDX_MASK (1 << LED2_PIO_IDX) // Mascara para CONTROLARMOS o LED

// Botão
#define BUT_PIO PIOA
#define BUT_PIO_ID ID_PIOA
#define BUT_IDX 11
#define BUT_IDX_MASK (1 << BUT_IDX)

// BOTÃO 1
#define BUT1_PIO PIOD
#define BUT1_PIO_ID ID_PIOD
#define BUT1_PIO_IDX 28
#define BUT1_IDX_MASK (1u << BUT1_PIO_IDX) // esse já está pronto.

// BOTÃO 2
#define BUT2_PIO PIOC
#define BUT2_PIO_ID ID_PIOC
#define BUT2_PIO_IDX 31
#define BUT2_IDX_MASK (1u << BUT2_PIO_IDX) // esse já está pronto.

// BOTÃO 3
#define BUT3_PIO PIOA
#define BUT3_PIO_ID ID_PIOA
#define BUT3_PIO_IDX 19
#define BUT3_IDX_MASK (1u << BUT3_PIO_IDX) // esse já está pronto.

/************************************************************************/
/* constants                                                            */
/************************************************************************/

/************************************************************************/
/* variaveis globais                                                    */
/************************************************************************/

/************************************************************************/
/* prototype                                                            */
/************************************************************************/
void io_init(void);
void pisca_led(int n, int t);
volatile char but_flag = 0;
int freq = 100;
char str[128];

/************************************************************************/
/* handler / callbacks                                                  */
/************************************************************************/

/*
 * Exemplo de callback para o botao, sempre que acontecer
 * ira piscar o led por 5 vezes
 *
 * !! Isso é um exemplo ruim, nao deve ser feito na pratica, !!
 * !! pois nao se deve usar delays dentro de interrupcoes    !!
 */

/************************************************************************/
/* funções                                                              */
/************************************************************************/

void add_freq(void) {
	but_flag = 1;
    if (pio_get(BUT1_PIO, PIO_INPUT, BUT1_IDX_MASK)) {
	    freq += 100;
    }
}

void sub_freq(void) {
	but_flag = 1;
    if (pio_get(BUT3_PIO, PIO_INPUT, BUT3_IDX_MASK)) {
        if (freq != 100) {
            freq -= 100;
        }
    }
    
}

void stop_led(void) {
    but_flag = 0;
}

// Inicializa botao SW0 do kit com interrupcao
void io_init(void) {
    // Configura led
    pmc_enable_periph_clk(LED2_PIO_ID);
    pio_set_output(LED2_PIO, LED2_PIO_IDX_MASK, 0, 0, 0);

    // Inicializa clock do periférico PIO responsavel pelo botao
    pmc_enable_periph_clk(BUT1_PIO_ID);
	pmc_enable_periph_clk(BUT2_PIO_ID);
	pmc_enable_periph_clk(BUT3_PIO_ID);

    // Configura PIO para lidar com o pino do botão como entrada com pull-up
    pio_configure(BUT1_PIO, PIO_INPUT, BUT1_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
    pio_set_debounce_filter(BUT1_PIO, BUT1_IDX_MASK, 60);
	pio_configure(BUT2_PIO, PIO_INPUT, BUT2_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
    pio_set_debounce_filter(BUT2_PIO, BUT2_IDX_MASK, 60);
    pio_configure(BUT3_PIO, PIO_INPUT, BUT3_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
    pio_set_debounce_filter(BUT3_PIO, BUT3_IDX_MASK, 60);

    // Configura interrupção no pino referente ao botao e associa
    // função de callback caso uma interrupção for gerada
    // a função de callback é a: but_callback()
    pio_handler_set(BUT1_PIO,
                    BUT1_PIO_ID,
                    BUT1_IDX_MASK,
                    PIO_IT_EDGE,
                    add_freq);

	pio_handler_set(BUT2_PIO,
					BUT2_PIO_ID,
					BUT2_IDX_MASK,
					PIO_IT_EDGE,
					stop_led);
    
	pio_handler_set(BUT3_PIO,
					BUT3_PIO_ID,
					BUT3_IDX_MASK,
					PIO_IT_EDGE,
					sub_freq);

    // Ativa interrupção e limpa primeira IRQ gerada na ativacao
    pio_enable_interrupt(BUT1_PIO, BUT1_IDX_MASK);
    pio_get_interrupt_status(BUT1_PIO);
	pio_enable_interrupt(BUT2_PIO, BUT2_IDX_MASK);
    pio_get_interrupt_status(BUT2_PIO);
    pio_enable_interrupt(BUT3_PIO, BUT3_IDX_MASK);
    pio_get_interrupt_status(BUT3_PIO);
	

    // Configura NVIC para receber interrupcoes do PIO do botao
    // com prioridade 0 (quanto mais próximo de 0 maior)
    NVIC_EnableIRQ(BUT1_PIO_ID);
    NVIC_SetPriority(BUT1_PIO_ID, 0); // Prioridade 0
	NVIC_EnableIRQ(BUT2_PIO_ID);
    NVIC_SetPriority(BUT2_PIO_ID, 2); // Prioridade 1
    NVIC_EnableIRQ(BUT3_PIO_ID);
    NVIC_SetPriority(BUT3_PIO_ID, 4); // Prioridade 2
}

/************************************************************************/
/* Main                                                                 */
/************************************************************************/

// Funcao principal chamada na inicalizacao do uC.
void main(void) {
    // Inicializa clock
    sysclk_init();

    // Desativa watchdog
    WDT->WDT_MR = WDT_MR_WDDIS;

    // configura botao com interrupcao
    io_init();

	// Init OLED
	gfx_mono_ssd1306_init();


	gfx_mono_draw_string("Freq: ", 0, 1, &sysfont);
	sprintf(str, "%d", freq);
	gfx_mono_draw_string(str, 50, 1, &sysfont);

    pio_set(LED2_PIO, LED2_PIO_IDX_MASK);
    // super loop aplicacoes embarcadas no devem sair do while(1).
    while (1) {
        if (but_flag) {
			delay_ms(1000);
			if (!pio_get(BUT1_PIO, PIO_INPUT, BUT1_IDX_MASK)) {
				if (freq > 100) {
					freq -= 100;
				}
			} else {
				freq += 100;
			}
            for (int i = 0; i <= 30; i++) {
                sprintf(str, "%d", freq);
                gfx_mono_draw_string(str, 50, 2, &sysfont);
                gfx_mono_draw_rect(i*4, 20, 2, 10, GFX_PIXEL_SET);
                sprintf(str, "%d", i);
                gfx_mono_draw_string(str, 82, 2, &sysfont);
                gfx_mono_draw_string("/30", 102, 2, &sysfont);

                pio_clear(LED2_PIO, LED2_PIO_IDX_MASK);
                delay_ms(freq);
                pio_set(LED2_PIO, LED2_PIO_IDX_MASK);
                delay_ms(freq);
				if (!but_flag || i == 30) {
					but_flag = 0;
					break;
				}
            }
            for(int i=30;i>=0;i-=1){
               gfx_mono_draw_rect(i*4, 20, 2, 10, GFX_PIXEL_CLR);   
            }
            add= 0;
        } 
        if (!but_flag) {
            pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
        }
    }
}
