/***************************************************************************//**
  @file     App.c
  @brief    Application functions
  @author   Nicolás Magliola
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "board.h"
#include "gpio.h"
#include "pisr.h"
#include "uart.h"


/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define COLOR YELLOW
#define SWITCH 3

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

#define PIN_RGB CONCAT(PIN_LED_, COLOR)
#define PIN_SW CONCAT(PIN_SW, SWITCH)

#define LED_PERIOD_MS        500U
#define SWITCH_PERIOD_MS      50U

#define PISR_MS_TO_TICKS(ms) \
    (((ms) * 1000U) / PISR_TICK_US)

#define LED_PERIOD_TICKS     PISR_MS_TO_TICKS(LED_PERIOD_MS)
#define SWITCH_PERIOD_TICKS  PISR_MS_TO_TICKS(SWITCH_PERIOD_MS)
/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

void blinkCallback (void);
void switchCallback (void);

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/


/* Función que se llama 1 vez, al comienzo del programa */
void App_Init (void)
{
	UART_config_t c ;
	c.UART_num = 0;
	c.baudrate = 115200;
	c.data_length = EIGTH_BIT_DATA;
	c.first_bit = LSB_FIRST;
	c.mode = RECEIVE_AND_TRANSMIT;
	c.parity = NO_PARITY;
	c.stop_length = ONE_BIT_STOP;
	c.transmit_blocking = NON_BLOCKING;
	c.receive_blocking = NON_BLOCKING;
	c.use_hw_fifo = false;
	UART_init(c);

	uint8_t hola[4] = {'h', 'o', 'l','a'};
	UART_write(UART_0, hola, 4);
}

/* Función que se llama constantemente en un ciclo infinito */
void App_Run (void)
{
	if(UART_words_received(UART_0) >= 1)
	{
		uint8_t received;
		UART_read(UART_0, &received, 1);
		UART_write(UART_0, &received, 1);
	}
}


/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static bool flag, blink;

void blinkCallback (void)
{
	if (blink)
	{
		gpioToggle(PIN_RGB);
	}

}

void switchCallback (void)
{
	if(gpioRead(PIN_SW) == SW_ACTIVE)
	{
		if(flag == 0){
			flag = 1;
			if(blink == 1)
			{
				blink = 0;
				gpioWrite(PIN_RGB, HIGH);
			} else
			{
				blink = 1;
				gpioWrite(PIN_RGB, LOW);
			}
		}
	} else {
		flag = 0;
	}
}

/*******************************************************************************
 ******************************************************************************/
