#include "uart.h"
#include "hardware.h"

#define UART_HAL_DEFAULT_BAUDRATE 9600
#define TX_SW_BUFFER_LENGTH 100

static UART_Type * const uarts[] = UART_BASE_PTRS;

static uint8_t uarts_RX_TX_IRQn[] = UART_RX_TX_IRQS;

static bool UART_yainit[6];

static bool receivingInterrupt[6];
static bool transmitingInterrupt[6];

static bool usingFIFO[6];

typedef struct{
	uint8_t buffer[TX_SW_BUFFER_LENGTH];
	uint8_t read;
	uint8_t load;
}circular_buffer;

static circular_buffer TX_buffers[6];
static circular_buffer RX_buffers[6];


static void clock_PIN_Enable(uint8_t UART_num);
static void set_PIN_alt3(PORT_Type * port, uint8_t pin);
static void UART_setBaudrate(UART_Type * uart, uint32_t baudrate);
static void circular_buffer_increase(uint8_t * num);

bool UART_init(UART_config_t config)
{
	if(config.UART_num > 5 || config.parity > 2)
	{
		return false;
	}

	if(UART_yainit[config.UART_num])
	{
		return false;
	} else
	{
		UART_yainit[config.UART_num] = true;
	}

	clock_PIN_Enable(config.UART_num);

	UART_Type * uart = uarts[config.UART_num];

	if(config.stop_length == TWO_BIT_STOP)
	{
		uart->BDH |= UART_BDH_SBNS_MASK;
	}

	UART_setBaudrate(uart, config.baudrate);

	if(config.data_length == NINE_BIT_DATA)
	{
		uart->C1 |= UART_C1_M_MASK;
	}

	if(config.parity != NO_PARITY)
	{
		uart->C1 |= UART_C1_PE_MASK;
		if(config.parity == ODD_PARITY)
		{
			uart->C1 |= UART_C1_PT_MASK;
		}
	}

	if(config.receive_blocking == NON_BLOCKING || config.transmit_blocking == NON_BLOCKING)
	{
		NVIC_EnableIRQ(uarts_RX_TX_IRQn[config.UART_num]);
		if(config.receive_blocking == NON_BLOCKING)
		{
			receivingInterrupt[config.UART_num] = true;
			uart->C2 |= UART_C2_RIE_MASK;
		}

		if(config.transmit_blocking == NON_BLOCKING)
		{
			transmitingInterrupt[config.UART_num] = true;
		}
	}

	switch(config.mode)
	{
	case RECEIVE:
		uart->C2 |= UART_C2_RE_MASK;
		break;
	case TRANSMIT:
		uart->C2 |= UART_C2_TE_MASK;
		break;
	case RECEIVE_AND_TRANSMIT:
		uart->C2 |= UART_C2_RE_MASK;
		uart->C2 |= UART_C2_TE_MASK;
		break;
	default:
		return false;
		break;
	}

	if(config.first_bit == MSB_FIRST)
	{
		uart->S2 |= UART_S2_MSBF_MASK;
	}

	if(config.use_hw_fifo)
	{
		usingFIFO[config.UART_num] = true;
	}

	return true;
}

static void clock_PIN_Enable(uint8_t UART_num)
{
	switch(UART_num)
	{
	case 0:
		SIM->SCGC4 |= SIM_SCGC4_UART0_MASK;
		set_PIN_alt3(PORTB, 16);
		set_PIN_alt3(PORTB, 17);
		break;
	case 1:
		SIM->SCGC4 |= SIM_SCGC4_UART1_MASK;
		set_PIN_alt3(PORTC, 3);
		set_PIN_alt3(PORTC, 4);
		break;
	case 2:
		SIM->SCGC4 |= SIM_SCGC4_UART2_MASK;
		set_PIN_alt3(PORTD, 2);
		set_PIN_alt3(PORTD, 3);
		break;
	case 3:
		SIM->SCGC4 |= SIM_SCGC4_UART3_MASK;
		set_PIN_alt3(PORTC, 16);
		set_PIN_alt3(PORTC, 17);
		break;
	case 4:
		SIM->SCGC1 |= SIM_SCGC1_UART4_MASK;
		set_PIN_alt3(PORTC, 14);
		set_PIN_alt3(PORTC, 15);
		break;
	case 5:
		SIM->SCGC1 |= SIM_SCGC1_UART5_MASK;
		set_PIN_alt3(PORTD, 8);
		set_PIN_alt3(PORTD, 9);
		break;
	default:
		break;
	}
}

static void set_PIN_alt3(PORT_Type * port, uint8_t pin)
{
	port->PCR[pin] &= ~(7 << PORT_PCR_MUX_SHIFT);
	port->PCR[pin] |= (3 << PORT_PCR_MUX_SHIFT);
}

static void UART_setBaudrate(UART_Type * uart, uint32_t baudrate)
{
	uint16_t sbr, brfa;
	uint32_t clock = (uart == UART0 || uart == UART1) ? (__CORE_CLOCK__) : (__CORE_CLOCK__ >> 1);

	baudrate = (baudrate == 0) ? UART_HAL_DEFAULT_BAUDRATE : baudrate;

	sbr = clock / (baudrate << 4);
	brfa = (clock << 2) / baudrate - (sbr << 5);

	uart->BDH |= UART_BDH_SBR(sbr >> 8);
	uart->BDL = UART_BDL_SBR(sbr);
	uart->C4 = (uart->C4 & ~UART_C4_BRFA_MASK) | UART_C4_BRFA(brfa);
}

static void circular_buffer_increase(uint8_t * num)
{
	(*num)++;
	if(*num >= TX_SW_BUFFER_LENGTH)
	{
		*num = 0;
	}
}

bool UART_write(uint8_t UART_num, uint8_t * words, uint8_t length)
{
	if(!UART_yainit[UART_num])
	{
		return false;
	}

	UART_Type * uart = uarts[UART_num];

	if(!transmitingInterrupt[UART_num])
	{
		for(int i=0; i < length; i++)
		{
			while(((uart->S1)&UART_S1_TDRE_MASK) == 0);
			uart->D = words[i];
		}
	} else
	{
		for(int i=0; i < length; i++)
		{
			TX_buffers[UART_num].buffer[TX_buffers[UART_num].load] = words[i];
			circular_buffer_increase(&TX_buffers[UART_num].load);
		}
		uart->C2 |= UART_C2_TIE_MASK;
	}

	return true;
}


bool UART_read(uint8_t UART_num, uint8_t * words, uint8_t length)
{
	if(!UART_yainit[UART_num])
	{
		return false;
	}

	UART_Type * uart = uarts[UART_num];

	if(!receivingInterrupt[UART_num])
	{
		for(int i = 0; i < length; i++)
		{
			while(((uart->S1)& UART_S1_RDRF_MASK) ==0);
			words[i] = uart->D;
		}
	} else
	{
		if(length > UART_words_received(UART_num))
		{
			return false;
		}

		for(int i = 0; i < length; i++)
		{
			words[i] = RX_buffers[UART_num].buffer[RX_buffers[UART_num].read];
			circular_buffer_increase(&RX_buffers[UART_num].read);
		}
	}

	return true;
}

uint8_t UART_words_received(uint8_t UART_num)
{
	if(RX_buffers[UART_num].load >= RX_buffers[UART_num].read)
	{
		return (RX_buffers[UART_num].load - RX_buffers[UART_num].read);
	} else
	{
		return TX_SW_BUFFER_LENGTH + RX_buffers[UART_num].load - RX_buffers[UART_num].read;
	}

}

static void TX_RX_handler(uint8_t UART_num)
{
	UART_Type * uart = uarts[UART_num];
	uint8_t tmp = uart->S1;

	if(uart->C2 & UART_C2_TIE_MASK && tmp & UART_S1_TDRE_MASK)
	{
		uart->D = TX_buffers[UART_num].buffer[TX_buffers[UART_num].read];
		circular_buffer_increase(&TX_buffers[UART_num].read);

		if(TX_buffers[UART_num].read == TX_buffers[UART_num].load)
		{
			uart->C2 &= ~UART_C2_TIE_MASK;
		}
	}

	if(tmp & UART_S1_RDRF_MASK)
	{
		RX_buffers[UART_num].buffer[RX_buffers[UART_num].load] = uart->D;
		circular_buffer_increase(&RX_buffers[UART_num].load);
	}
}

void UART0_RX_TX_IRQHandler (void)
{
	TX_RX_handler(0);
}

void UART1_RX_TX_IRQHandler (void)
{
	TX_RX_handler(1);
}

void UART2_RX_TX_IRQHandler (void)
{
	TX_RX_handler(2);
}

void UART3_RX_TX_IRQHandler (void)
{
	TX_RX_handler(3);
}

void UART4_RX_TX_IRQHandler (void)
{
	TX_RX_handler(4);
}

void UART5_RX_TX_IRQHandler (void)
{
	TX_RX_handler(5);
}
