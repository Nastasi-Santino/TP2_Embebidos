#ifndef UART_H_
#define UART_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdbool.h>
#include <stdint.h>

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

enum{
	UART_0,
	UART_1,
	UART_2,
	UART_3,
	UART_4,
	UART_5
};

enum{
	RECEIVE,
	TRANSMIT,
	RECEIVE_AND_TRANSMIT
};

enum{
	EIGTH_BIT_DATA,
	NINE_BIT_DATA
};

enum{
	ONE_BIT_STOP,
	TWO_BIT_STOP
};

enum{
	NO_PARITY,
	ODD_PARITY,
	EVEN_PARITY
};

enum{
	LSB_FIRST,
	MSB_FIRST
};

enum{
	BLOCKING,
	NON_BLOCKING
};

typedef struct{
	uint8_t UART_num;
	uint8_t mode;
	uint8_t data_length;
	uint8_t stop_length;
	uint8_t parity;
	uint8_t first_bit;
	uint32_t baudrate;
	bool use_hw_fifo;
	bool receive_blocking;
	bool transmit_blocking;
}UART_config_t;

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

bool UART_init(UART_config_t config);

bool UART_write(uint8_t UART_num, uint16_t * words, uint8_t length);
bool UART_read(uint8_t UART_num, uint16_t * words, uint8_t length);

uint8_t UART_words_received(uint8_t UART_num);
bool UART_tx_busy(uint8_t UART_num);

#endif /* UART_H_ */
