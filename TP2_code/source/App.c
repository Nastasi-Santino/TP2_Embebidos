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
	c.UART_num = 1;
	UART_init(c);
}

/* Función que se llama constantemente en un ciclo infinito */
void App_Run (void)
{



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
