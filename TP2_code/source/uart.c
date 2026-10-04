#include "uart.h"
#include "hardware.h"

#define UART_HAL_DEFAULT_BAUDRATE 9600

static UART_Type * const uarts[] = UART_BASE_PTRS;

static uint8_t uarts_RX_TX_IRQn[] = UART_RX_TX_IRQS;

static bool UART_yainit[6];

static bool receivingInterrupt[6];
static bool transmitingInterrupt[6];

static bool usingFIFO;

static void clockEnable(uint8_t UART_num);
static void UART_setBaudrate(UART_Type * uart, uint32_t baudrate);


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

	clockEnable(config.UART_num);

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

	return true;
}

static void clockEnable(uint8_t UART_num)
{
	switch(UART_num)
	{
	case 0:
		SIM->SCGC4 |= SIM_SCGC4_UART0_MASK;
		break;
	case 1:
		SIM->SCGC4 |= SIM_SCGC4_UART1_MASK;
		break;
	case 2:
		SIM->SCGC4 |= SIM_SCGC4_UART2_MASK;
		break;
	case 3:
		SIM->SCGC4 |= SIM_SCGC4_UART3_MASK;
		break;
	case 4:
		SIM->SCGC1 |= SIM_SCGC1_UART4_MASK;
		break;
	case 5:
		SIM->SCGC1 |= SIM_SCGC1_UART5_MASK;
		break;
	default:
		break;
	}
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
