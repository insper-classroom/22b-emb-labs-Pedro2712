#include "conf_board.h"
#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"

#include "gfx_mono_text.h"
#include "sysfont.h"

#include <string.h>
#include "ili9341.h"
// #include "lvgl.h"

/************************************************************************/
/* BOARD CONFIG                                                         */
/************************************************************************/

#define TRIGGER_PIO PIOA
#define TRIGGER_PIO_ID ID_PIOA
#define TRIGGER_PIO_IDX 27
#define TRIGGER_PIO_IDX_MASK (1 << TRIGGER_PIO_IDX)

#define ECHO_PIO PIOD
#define ECHO_PIO_ID ID_PIOD
#define ECHO_PIO_PIN 31
#define ECHO_PIO_PIN_MASK (1 << ECHO_PIO_PIN)

#define BUT_PIO_OLED1 PIOD
#define BUT_PIO_OLED1_ID ID_PIOD
#define BUT_PIO_OLED1_IDX 28
#define BUT_PIO_OLED1_IDX_MASK (1 << BUT_PIO_OLED1_IDX)

#define USART_COM_ID ID_USART1
#define USART_COM USART1

/************************************************************************/
/* RTOS                                                                */
/************************************************************************/

#define TASK_TRIGGER_STACK_SIZE (4096 / sizeof(portSTACK_TYPE))
#define TASK_TRIGGER_STACK_PRIORITY (tskIDLE_PRIORITY)

#define TASK_ECHO_STACK_SIZE (4096 / sizeof(portSTACK_TYPE))
#define TASK_ECHO_STACK_PRIORITY (tskIDLE_PRIORITY)

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
                                          signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

/************************************************************************/
/* recursos RTOS                                                        */
/************************************************************************/

/** Semaforo a ser usado pela task echo */
SemaphoreHandle_t xSemaphoreEcho;

/** Queue for msg log send data */
QueueHandle_t xQueueLedFreq;

/************************************************************************/
/* prototypes local                                                     */
/************************************************************************/
void echo_callback();
static void USART1_init(void);
static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource);
void trigger_init();
static void ECHO_init(void);

/************************************************************************/
/* RTOS application funcs                                               */
/************************************************************************/

/**
 * \brief Called if stack overflow during execution
 */
extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
                                          signed char *pcTaskName) {
    printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
    /* If the parameters have been corrupted then inspect pxCurrentTCB to
     * identify which task has overflowed its stack.
     */
    for (;;) {
    }
}

/**
 * \brief This function is called by FreeRTOS idle task
 */
extern void vApplicationIdleHook(void) { pmc_sleep(SAM_PM_SMODE_SLEEP_WFI); }

/**
 * \brief This function is called by FreeRTOS each tick
 */
extern void vApplicationTickHook(void) {}

extern void vApplicationMallocFailedHook(void) {
    /* Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    internally by FreeRTOS API functions that create tasks, queues, software
    timers, and semaphores.  The size of the FreeRTOS heap is set by the
    configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

    /* Force an assert. */
    configASSERT((volatile void *)NULL);
}

/************************************************************************/
/* handlers / callbacks                                                 */
/************************************************************************/

void echo_callback() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphoreEcho, &xHigherPriorityTaskWoken);
}

void atualiza_tela(int dist) {
    gfx_mono_draw_filled_rect(0, 0, 128, 32, GFX_PIXEL_CLR);
    if (dist < 500) {
        char dist_str[10];
        sprintf(dist_str, "Distancia:  %d cm", dist);
        gfx_mono_draw_string(dist_str, 15, 15, &sysfont);
    } else {
        gfx_mono_draw_filled_rect(0, 0, 128, 32, GFX_PIXEL_CLR);
        gfx_mono_draw_string("Espaco aberto", 15, 15, &sysfont);
    }
    
}

// void lv_ex_chart_1(void)
// {
//     /*Create a chart*/
//     lv_obj_t * chart;
//     chart = lv_chart_create(lv_scr_act());
//     lv_obj_set_size(chart, 200, 150);
//     lv_obj_center(chart);
//     lv_chart_set_type(chart, LV_CHART_TYPE_LINE);   /*Show lines and points too*/

//     /*Add two data series*/
//     lv_chart_series_t * ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

//     // /*Set the next points on 'ser1'*/
//     // lv_chart_set_next_value(chart, ser1, 10);
//     // lv_chart_set_next_value(chart, ser1, 10);
//     // lv_chart_set_next_value(chart, ser1, 10);
//     // lv_chart_set_next_value(chart, ser1, 10);
//     // lv_chart_set_next_value(chart, ser1, 10);
//     // lv_chart_set_next_value(chart, ser1, 10);
//     // lv_chart_set_next_value(chart, ser1, 10);
//     // lv_chart_set_next_value(chart, ser1, 30);
//     // lv_chart_set_next_value(chart, ser1, 70);
//     // lv_chart_set_next_value(chart, ser1, 90);

//     /*Directly set points on 'ser2'*/
//     ser1->y_points[0] = 90;
//     ser1->y_points[1] = 70;
//     ser1->y_points[2] = 65;
//     ser1->y_points[3] = 65;
//     ser1->y_points[4] = 65;
//     ser1->y_points[5] = 65;
//     ser1->y_points[6] = 65;
//     ser1->y_points[7] = 65;
//     ser1->y_points[8] = 65;
//     ser1->y_points[9] = 65;

//     lv_chart_refresh(chart); /*Required after direct set*/
// }

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/
static void task_echo(void *pvParameters) {
	gfx_mono_ssd1306_init();
    /* inicializa ECHO */
    ECHO_init();
    
    for (;;) {
        /* aguarda por tempo inderteminado at� a liberacao do semaforo */
        if (xSemaphoreTake(xSemaphoreEcho, 100)) {
            if (pio_get(ECHO_PIO, PIO_INPUT, ECHO_PIO_PIN_MASK)) {
                RTT_init(10000, 0, 0);
            } else {
                uint32_t time_read = rtt_read_timer_value(RTT);
                int dist = 1.0 / 10000 * time_read * 170 * 100;
				atualiza_tela(dist);
            }
        } else {
            gfx_mono_draw_filled_rect(0, 0, 128, 32, GFX_PIXEL_CLR);
            gfx_mono_draw_string("Desconectado!!", 15, 15, &sysfont);
        }
    }
}

static void task_trigger(void *pvParameters) {
    trigger_init();
    for (;;) {
        if (!pio_get(ECHO_PIO, PIO_INPUT, ECHO_PIO_PIN_MASK)) {
            vTaskDelay(60);
            pio_set(TRIGGER_PIO, TRIGGER_PIO_IDX_MASK);
            delay_us(10);
            pio_clear(TRIGGER_PIO, TRIGGER_PIO_IDX_MASK);
        }
    }
}

/************************************************************************/
/* funcoes                                                              */
/************************************************************************/

/**
 * \brief Configure the console UART.
 */

static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource) {

    uint16_t pllPreScale = (int)(((float)32768) / freqPrescale);

    rtt_sel_source(RTT, false);
    rtt_init(RTT, pllPreScale);

    if (rttIRQSource & RTT_MR_ALMIEN) {
        uint32_t ul_previous_time;
        ul_previous_time = rtt_read_timer_value(RTT);
        while (ul_previous_time == rtt_read_timer_value(RTT))
            ;
        rtt_write_alarm_time(RTT, IrqNPulses + ul_previous_time);
    }

    /* config NVIC */
    NVIC_DisableIRQ(RTT_IRQn);
    NVIC_ClearPendingIRQ(RTT_IRQn);
    NVIC_SetPriority(RTT_IRQn, 4);
    NVIC_EnableIRQ(RTT_IRQn);

    /* Enable RTT interrupt */
    if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN))
        rtt_enable_interrupt(RTT, rttIRQSource);
    else
        rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);
}

static void configure_console(void) {
    const usart_serial_options_t uart_serial_options = {
        .baudrate = CONF_UART_BAUDRATE,
        .charlength = CONF_UART_CHAR_LENGTH,
        .paritytype = CONF_UART_PARITY,
        .stopbits = CONF_UART_STOP_BITS,
    };

    /* Configure console UART. */
    stdio_serial_init(CONF_UART, &uart_serial_options);

    /* Specify that stdout should not be buffered. */
    setbuf(stdout, NULL);
}

void trigger_init() {
    pmc_enable_periph_clk(TRIGGER_PIO_ID);
    pio_set_output(TRIGGER_PIO, TRIGGER_PIO_IDX_MASK, 1, 0, 0);
};

static void ECHO_init(void) {
    // Configura PIO para lidar com o pino do bot�o como entrada
    // com pull-up
    pio_configure(ECHO_PIO, PIO_INPUT, ECHO_PIO_PIN_MASK, 0);

    // Configura interrup��o no pino referente ao botao e associa
    // fun��o de callback caso uma interrup��o for gerada
    // a fun��o de callback � a: but_callback()
    pio_handler_set(ECHO_PIO,
                    ECHO_PIO_ID,
                    ECHO_PIO_PIN_MASK,
                    PIO_IT_EDGE,
                    echo_callback);

    // Ativa interrup��o e limpa primeira IRQ gerada na ativacao
    pio_enable_interrupt(ECHO_PIO, ECHO_PIO_PIN_MASK);
    pio_get_interrupt_status(ECHO_PIO);

    // Configura NVIC para receber interrupcoes do PIO do botao
    // com prioridade 4 (quanto mais pr�ximo de 0 maior)
    NVIC_EnableIRQ(ECHO_PIO_ID);
    NVIC_SetPriority(ECHO_PIO_ID, 4); // Prioridade 4
}

static void configure_lcd(void) {
    /**LCD pin configure on SPI*/
    pio_configure_pin(LCD_SPI_MISO_PIO, LCD_SPI_MISO_FLAGS);
    pio_configure_pin(LCD_SPI_MOSI_PIO, LCD_SPI_MOSI_FLAGS);
    pio_configure_pin(LCD_SPI_SPCK_PIO, LCD_SPI_SPCK_FLAGS);
    pio_configure_pin(LCD_SPI_NPCS_PIO, LCD_SPI_NPCS_FLAGS);
    pio_configure_pin(LCD_SPI_RESET_PIO, LCD_SPI_RESET_FLAGS);
    pio_configure_pin(LCD_SPI_CDS_PIO, LCD_SPI_CDS_FLAGS);

    ili9341_init();
	ili9341_backlight_on();
}

// void configure_lvgl(void) {
// 	lv_init();
// 	lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX);
	
// 	lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
// 	disp_drv.draw_buf = &disp_buf;          /*Set an initialized buffer*/
// 	disp_drv.flush_cb = my_flush_cb;        /*Set a flush callback to draw to the display*/
// 	disp_drv.hor_res = LV_HOR_RES_MAX;      /*Set the horizontal resolution in pixels*/
// 	disp_drv.ver_res = LV_VER_RES_MAX;      /*Set the vertical resolution in pixels*/

// 	lv_disp_t * disp;
// 	disp = lv_disp_drv_register(&disp_drv); /*Register the driver and save the created display objects*/
	
// 	/* Init input on LVGL */
// 	lv_indev_drv_init(&indev_drv);
// 	indev_drv.type = LV_INDEV_TYPE_POINTER;
// 	indev_drv.read_cb = my_input_read;
// 	lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);
// }

/************************************************************************/
/* main                                                                 */
/************************************************************************/

/**
 *  \brief FreeRTOS Real Time Kernel example entry point.
 *
 *  \return Unused (ANSI-C compatibility).
 */
int main(void) {
    /* Initialize the SAM system */
    sysclk_init();

    board_init();
    configure_console();

    /* initialize ios and the display controller */
    configure_lcd();
    // configure_lvgl();

    printf("Sys init ok \n");
    /* Attempt to create a semaphore. */
    xSemaphoreEcho = xSemaphoreCreateBinary();
    if (xSemaphoreEcho == NULL)
        printf("falha em criar o semaforo \n");

    /* cria queue com 32 "espacos" */
    /* cada espa�o possui o tamanho de um inteiro*/
    xQueueLedFreq = xQueueCreate(32, sizeof(uint32_t));
    if (xQueueLedFreq == NULL)
        printf("falha em criar a queue \n");

    /* Create task to monitor processor activity */
    if (xTaskCreate(task_echo, "ECHO", TASK_ECHO_STACK_SIZE, NULL,
                    TASK_ECHO_STACK_PRIORITY, NULL) != pdPASS) {
        printf("Failed to create UartTx task\r\n");
    } else {
        printf("task led but \r\n");
    }

    if (xTaskCreate(task_trigger, "TRIGGER", TASK_TRIGGER_STACK_SIZE, NULL,
                    TASK_TRIGGER_STACK_PRIORITY, NULL) != pdPASS) {
        printf("Failed to create UartTx task\r\n");
    }

    /* Start the scheduler. */
    vTaskStartScheduler();

    /* RTOS n�o deve chegar aqui !! */
    while (1) {
    }

    /* Will only get here if there was insufficient memory to create the idle
     * task. */
    return 0;
}
