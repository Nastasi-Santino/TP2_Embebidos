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
#include "fxos.h"
#include "spi.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define COLOR RED
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

/* Cantidad de palabras por transferencia */
#define SPI_TEST_LEN 2
/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

void blinkCallback (void);
void switchCallback (void);

uint8_t numToString(int16_t num, uint8_t *string);

/*******************************************************************************
 * STATIC VARIABLES
 ******************************************************************************/

/* Patrón alternado (10101010b y 01010101b) ideal para ver en el osciloscopio */
static uint16_t tx_data[SPI_TEST_LEN] = {0x00AA, 0x0055};
static uint16_t rx_data[SPI_TEST_LEN] = {0x0000, 0x0000};

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/


/* Función que se llama 1 vez, al comienzo del programa */
void App_Init (void)
{

	/* Inicialización del LED para diagnóstico visual */
	gpioMode(PIN_RGB, OUTPUT);
	gpioWrite(PIN_RGB, LOW); // LED apagado (activo bajo en FRDM-K64F)

	/*
	UART_config_t c ;

	FXOS_INIT();

	c.UART_num = UART_0;
	c.baudrate = 115200;
	c.data_length = EIGHT_BIT_DATA;
	c.first_bit = LSB_FIRST;
	c.mode = RECEIVE_AND_TRANSMIT;
	c.parity = NO_PARITY;
	c.stop_length = ONE_BIT_STOP;
	c.transmit_blocking = NON_BLOCKING_UART;
	c.receive_blocking = NON_BLOCKING_UART;
	c.use_hw_fifo = true;
	UART_init(c);


	uint8_t hola[4] = {'h', 'o', 'l','a'};
	UART_write(UART_0, hola, 4);


	I2C_config_t c;
	c.i2c_num = I2C_0;
	c.baudrate = 100e3;
	c.blocking = NON_BLOCKING;

	I2C_INIT(c);

	gpioMode(PIN_RGB, OUTPUT);
	gpioWrite(PIN_RGB, HIGH);
	*/

}

/* Función que se llama constantemente en un ciclo infinito */
void App_Run (void)
{
	/*
	if(UART_words_received(UART_0) >= 1)
	{
		uint8_t received;
		UART_read(UART_0, &received, 1);
		UART_write(UART_0, &received, 1);
	}



	uint8_t data;
	uint8_t reg_address = 0x0D;
	I2C_write_read(I2C_0, 0x1D, &reg_address, 1, &data, 1);
	if(data == 0xC7)
	{
		gpioToggle(PIN_RGB);
	}
	*/
	/*
	static uint8_t data;
	static uint8_t reg_address = 0x0D;
	static bool read = true;

	if((!I2C_tx_busy(I2C_0) && !I2C_rx_busy(I2C_0)) && read)
	{
		I2C_write_read(I2C_0, 0x1D, &reg_address, 1, &data, 1);
		read = false;
	}

	if(I2C_words_received(I2C_0) == 1)
	{
		gpioToggle(PIN_RGB);
		read = true;
	}

	*/
	FXOS_service();

	static uint32_t delay_cnt = 0;
	if(delay_cnt >= 500000)
	{
		/*
		uint8_t newline = '\n';
		int16_t mx, my, mz;
		int16_t roll, pitch, heading;
		FXOS_get_mag(&mx,&my,&mz);
		get_orientation(&roll, &pitch, &heading);
		*/

		//uint8_t roll_txt = 'R';
		//uint8_t roll_s[5];
		//uint8_t pitch_txt = 'P';
		//uint8_t pitch_s[5];
		//uint8_t heading_txt = 'H';
		//uint8_t heading_s[5];

		/*
		uint8_t mx_s[7];
		uint8_t my_s[7];
		uint8_t mz_s[7];
		uint8_t h_s[5];

	    uint8_t mx_txt[3] = {'M', 'X', ':'};
	    uint8_t my_txt[3] = {'M', 'Y', ':'};
	    uint8_t mz_txt[3] = {'M', 'Z', ':'};
	    uint8_t h_txt[2]  = {'H', ':'};


	    uint8_t mx_len = numToString(mx, mx_s);
	    uint8_t my_len = numToString(my, my_s);
	    uint8_t mz_len = numToString(mz, mz_s);
	    uint8_t h_len  = numToString(heading, h_s);
	    */

		//uint8_t roll_length = numToString(roll, roll_s);
		//uint8_t pitch_length = numToString(pitch, pitch_s);
		//uint8_t heading_length= numToString(heading, heading_s);

	    /*
	    UART_write(UART_0, mx_txt, 3);
	    UART_write(UART_0, mx_s, mx_len);
	    UART_write(UART_0, &newline, 1);

	    UART_write(UART_0, my_txt, 3);
	    UART_write(UART_0, my_s, my_len);
	    UART_write(UART_0, &newline, 1);

	    UART_write(UART_0, mz_txt, 3);
	    UART_write(UART_0, mz_s, mz_len);
	    UART_write(UART_0, &newline, 1);

	    UART_write(UART_0, h_txt, 2);
	    UART_write(UART_0, h_s, h_len);
	    UART_write(UART_0, &newline, 1);
	    UART_write(UART_0, &newline, 1);
		*/

		/*
		UART_write(UART_0, &roll_txt, 1);
		UART_write(UART_0, roll_s, roll_length);
		UART_write(UART_0, &newline, 1);

		UART_write(UART_0, &pitch_txt, 1);
		UART_write(UART_0, pitch_s, pitch_length);
		UART_write(UART_0, &newline, 1);

		UART_write(UART_0, &heading_txt, 1);
		UART_write(UART_0, heading_s, heading_length);
		UART_write(UART_0, &newline, 1);
		UART_write(UART_0, &newline, 1);
		*/


		//SPI TESTING
		if (!SPI_busy(SPI_0))
        {
			 /*
             * Nota: aunque num_of_slaves sea 1 en SPI_INIT, el índice 'slave'
             * en SPI_transfer / SPI_write es base 0 (0 = PCS0).
             */
            SPI_transfer(SPI_0, 0, tx_data, rx_data, SPI_TEST_LEN);

            /* Si puenteás SOUT (PTD2) con SIN (PTD3), el LED parpadea al recibir 0xAA y 0x55 */
            if (rx_data[0] == 0x00AA && rx_data[1] == 0x0055)
            {
                gpioToggle(PIN_RGB);
                rx_data[0] = 0;
                rx_data[1] = 0;
            }
        }



		delay_cnt = 0;
	} else
	{
		delay_cnt++;
	}


	if(UART_words_received(UART_0) >= 1)
	{
		uint8_t received;
		UART_read(UART_0, &received, 1);
		if(received == '1')
		{

		}
	}



}


/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static bool flag, blink;

uint8_t numToString(int16_t num, uint8_t *string)
{
    uint16_t value = (num < 0) ? -(int32_t)num : num;
    uint16_t aux = value;
    uint8_t digits = 1;

    // Contar los dígitos
    while (aux >= 10) {
        aux /= 10;
        digits++;
    }

    // Agregar signo y terminador
    string[0] = (num < 0) ? '-' : '+';
    string[digits + 1] = '\0';

    // Convertir los dígitos de derecha a izquierda
    for (uint8_t i = digits; i > 0; i--) {
        string[i] = '0' + (value % 10);
        value /= 10;
    }

    return digits + 1;
}


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
