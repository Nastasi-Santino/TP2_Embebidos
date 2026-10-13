#include "i2c.h"
#include "hardware.h"

#define I2C_MODULE_CLOCK (__CORE_CLOCK__ >> 1)

static I2C_Type * const i2cs[] = I2C_BASE_PTRS;

static uint8_t i2c_IRQn[] = I2C_IRQS;

static bool active_interrupt[3];
static volatile bool rx_busy[3];
static volatile bool tx_busy[3];

static volatile bool tx_rx[3];
static uint8_t slave_addresses[3];

static uint8_t * read_pointers[3];
static uint8_t read_lengths[3];
static volatile uint8_t read_counter[3];

static uint8_t * write_pointers[3];
static uint8_t write_lengths[3];
static uint8_t write_counter[3];

static void I2C_handler(uint8_t I2C_num);

static void clock_PIN_Enable(uint8_t I2C_num);
static void set_PIN(PORT_Type * port, uint8_t pin, uint8_t alt);
static void I2C_setBaudrate(I2C_Type * i2c, uint32_t baudrate);
static bool check_ack(uint8_t I2C_num);

bool I2C_INIT(I2C_config_t config)
{
	if(config.i2c_num > 2)
	{
		return false;
	}

	clock_PIN_Enable(config.i2c_num);

	I2C_Type * i2c = i2cs[config.i2c_num];

	i2c->A1 = MY_SLAVE_ADDRESS << 1;

	I2C_setBaudrate(i2c, config.baudrate);

	i2c->C1 |= I2C_C1_IICEN_MASK;

	if(config.blocking == NON_BLOCKING)
	{
		NVIC_EnableIRQ(i2c_IRQn[config.i2c_num]);
		active_interrupt[config.i2c_num] = true;
	}

	return true;
}

static void clock_PIN_Enable(uint8_t I2C_num)
{
	switch(I2C_num)
	{
	case 0:
		SIM->SCGC4 |= SIM_SCGC4_I2C0_MASK;
		set_PIN(PORTE, 24, 5);
		set_PIN(PORTE, 25, 5);
		break;
	case 1:
		SIM->SCGC4 |= SIM_SCGC4_I2C1_MASK;
		set_PIN(PORTE, 0, 6);
		set_PIN(PORTE, 1, 6);
		break;
	case 2:
		SIM->SCGC1 |= SIM_SCGC1_I2C2_MASK;
		set_PIN(PORTA, 11, 5);
		set_PIN(PORTA, 12, 5);
		break;
	default:
		break;
	}
}

static void set_PIN(PORT_Type * port, uint8_t pin, uint8_t alt)
{
	port->PCR[pin] &= ~(7 << PORT_PCR_MUX_SHIFT);
	port->PCR[pin] |= (alt << PORT_PCR_MUX_SHIFT);

	port->PCR[pin] |= PORT_PCR_ODE_MASK;
}


static void I2C_setBaudrate(I2C_Type * i2c, uint32_t baudrate)
{
    static const uint16_t dividers[64] = {
         20,   22,   24,   26,   28,   30,   34,   40,
         28,   32,   36,   40,   44,   48,   56,   68,
         48,   56,   64,   72,   80,   88,  104,  128,
         80,   96,  112,  128,  144,  160,  192,  240,
        160,  192,  224,  256,  288,  320,  384,  480,
        320,  384,  448,  512,  576,  640,  768,  960,
        640,  768,  896, 1024, 1152, 1280, 1536, 1920,
       1280, 1536, 1792, 2048, 2304, 2560, 3072, 3840
    };

    static const uint8_t mult_values[3] = {1, 2, 4};

    uint32_t clock = I2C_MODULE_CLOCK;

    uint32_t best_baud = 0;
    uint8_t best_icr = 0;
    uint8_t best_mult = 0;

    for(uint8_t mult = 0; mult < 3; mult++)
    {
        for(uint8_t icr = 0; icr < 64; icr++)
        {
            uint32_t real_baud =
                clock / (mult_values[mult] * dividers[icr]);

            if((real_baud <= baudrate) && (real_baud > best_baud))
            {
                best_baud = real_baud;
                best_icr = icr;
                best_mult = mult;
            }
        }
    }

    if(best_baud == 0)
    {
        best_mult = 2;
        best_icr = 0x3F;
    }

    i2c->F = I2C_F_MULT(best_mult) |
             I2C_F_ICR(best_icr);
}

static bool check_ack(uint8_t i2c_num)
{
	I2C_Type * i2c = i2cs[i2c_num];

	if(i2c->S & I2C_S_RXAK_MASK)
	{

		i2c->C1 &= ~I2C_C1_MST_MASK;
		if(active_interrupt[i2c_num])
		{
			i2c->C1 &= ~I2C_C1_IICIE_MASK;
			rx_busy[i2c_num] = false;
			tx_busy[i2c_num] = false;
		}
		tx_rx[i2c_num] = false;
		return false;
	}

	return true;
}

bool I2C_read(uint8_t i2c_num, uint8_t slave_address , uint8_t * data_byte, uint8_t length)
{
	I2C_Type * i2c = i2cs[i2c_num];

	read_counter[i2c_num] = 0;

	if((rx_busy[i2c_num] || tx_busy[i2c_num]) && !tx_rx[i2c_num])
	{
		return false;
	}

	i2c->C1 |= I2C_C1_TX_MASK;
	i2c->C1 |= I2C_C1_MST_MASK;

	i2c->D = (slave_address << 1) | 0x01;

	if(!active_interrupt[i2c_num])
	{
		while(!(i2c->S & I2C_S_IICIF_MASK));

		i2c->S |= I2C_S_IICIF_MASK;

		if(!check_ack(i2c_num))
		{
			return false;
		}


		i2c->C1 &= ~I2C_C1_TX_MASK;
		if(length != 1)
		{
			i2c->C1 &= ~I2C_C1_TXAK_MASK;
		} else
		{
			i2c->C1 |= I2C_C1_TXAK_MASK;
		}
		(void)i2c->D;

		for(int i = 0; i < length; i++)
		{
			while(!(i2c->S & I2C_S_IICIF_MASK));
			i2c->S |= I2C_S_IICIF_MASK;

			if(i == length - 2)
			{
				i2c->C1 |= I2C_C1_TXAK_MASK;
			}

			if(i == length - 1)
			{
				i2c->C1 &= ~I2C_C1_MST_MASK;
				tx_rx[i2c_num] = false;
			}

			data_byte[i] = i2c->D;
		}

	} else {
		read_pointers[i2c_num] = data_byte;
		read_lengths[i2c_num] = length;
		rx_busy[i2c_num] = true;
		i2c->C1 |= I2C_C1_IICIE_MASK;
	}

	return true;
}

bool I2C_write(uint8_t i2c_num, uint8_t slave_address, uint8_t * data_byte, uint8_t length)
{
	if(rx_busy[i2c_num] || tx_busy[i2c_num])
	{
		return false;
	}

	I2C_Type * i2c = i2cs[i2c_num];

	write_counter[i2c_num] = 0;

	i2c->C1 |= I2C_C1_TX_MASK;
	i2c->C1 |= I2C_C1_MST_MASK;

	i2c->D = (slave_address << 1) & 0xFE;

	if(!active_interrupt[i2c_num])
	{
		while(!(i2c->S & I2C_S_IICIF_MASK));
		i2c->S |= I2C_S_IICIF_MASK;

		if(!check_ack(i2c_num))
		{
			return false;
		}

		for(int i = 0; i < length; i++)
		{
			i2c->D = data_byte[i];

			while(!(i2c->S & I2C_S_IICIF_MASK));
			i2c->S |= I2C_S_IICIF_MASK;

			if(!check_ack(i2c_num))
			{
				return false;
			}

			if(i == length - 1)
			{
				if(tx_rx[i2c_num])
				{
					i2c->C1 |= I2C_C1_RSTA_MASK;
				} else
				{
					i2c->C1 &= ~I2C_C1_MST_MASK;
				}
			}
		}
	} else
	{
		write_pointers[i2c_num] = data_byte;
		write_lengths[i2c_num] = length;
		tx_busy[i2c_num] = true;
		i2c->C1 |= I2C_C1_IICIE_MASK;
	}

	return true;
}

bool I2C_write_read(uint8_t i2c_num, uint8_t slave_address,
		uint8_t * write_data, uint8_t write_length, uint8_t * read_data, uint8_t read_length)
{
	if(rx_busy[i2c_num] || tx_busy[i2c_num])
	{
		return false;
	}

	write_counter[i2c_num] = 0;
	read_counter[i2c_num] = 0;

	I2C_Type * i2c = i2cs[i2c_num];

	tx_rx[i2c_num] = true;

	if(!active_interrupt[i2c_num])
	{
		if(!I2C_write(i2c_num, slave_address, write_data, write_length))
		{
			return false;
		} else
		{
			if(!I2C_read(i2c_num, slave_address, read_data, read_length))
			{
				return false;
			}
		}
	} else
	{
		i2c->C1 |= I2C_C1_TX_MASK;
		i2c->C1 |= I2C_C1_MST_MASK;

		i2c->D = (slave_address << 1) & 0xFE;

		write_pointers[i2c_num] = write_data;
		write_lengths[i2c_num] = write_length;
		read_pointers[i2c_num] = read_data;
		read_lengths[i2c_num] = read_length;
		tx_busy[i2c_num] = true;
		slave_addresses[i2c_num] = slave_address;
		i2c->C1 |= I2C_C1_IICIE_MASK;
	}

	return true;
}

bool I2C_tx_busy(uint8_t i2c_num)
{
	return tx_busy[i2c_num];
}

bool I2C_rx_busy(uint8_t i2c_num)
{
	return rx_busy[i2c_num];
}

uint8_t I2C_words_received(uint8_t i2c_num)
{
	return (read_counter[i2c_num] == 0) ? 0 : (read_counter[i2c_num]  - 1);
}

static void I2C_handler(uint8_t i2c_num)
{
	I2C_Type * i2c = i2cs[i2c_num];

	i2c->S |= I2C_S_IICIF_MASK;

	if(rx_busy[i2c_num])
	{
		if(read_counter[i2c_num] == 0)
		{
			if(check_ack(i2c_num))
			{
				i2c->C1 &= ~I2C_C1_TX_MASK;
				if(read_lengths[i2c_num] != 1)
				{
					i2c->C1 &= ~I2C_C1_TXAK_MASK;
				} else
				{
					i2c->C1 |= I2C_C1_TXAK_MASK;
				}
				(void)i2c->D;
				read_counter[i2c_num]++;
			}
		} else
		{
			if((read_counter[i2c_num] - 1) == read_lengths[i2c_num] - 2)
			{
				i2c->C1 |= I2C_C1_TXAK_MASK;
			}


			if((read_counter[i2c_num] - 1) == read_lengths[i2c_num] - 1)
			{
				i2c->C1 &= ~I2C_C1_IICIE_MASK;
				i2c->C1 &= ~I2C_C1_MST_MASK;
				rx_busy[i2c_num] = false;
				tx_rx[i2c_num] = false;
			}

			read_pointers[i2c_num][(read_counter[i2c_num] == 0) ?
					read_lengths[i2c_num] - 1 : read_counter[i2c_num]++ - 1] = i2c->D;
		}
	}

	if(tx_busy[i2c_num])
	{
		if(check_ack(i2c_num))
		{
			if(write_counter[i2c_num] == write_lengths[i2c_num])
			{
				if(tx_rx[i2c_num])
				{
					i2c->C1 |= I2C_C1_RSTA_MASK;
					rx_busy[i2c_num] = true;
					i2c->D = (slave_addresses[i2c_num] << 1) | 0x01;
				} else
				{
					i2c->C1 &= ~I2C_C1_IICIE_MASK;
					i2c->C1 &= ~I2C_C1_MST_MASK;
				}
				write_counter[i2c_num] = 0;
				tx_busy[i2c_num] = false;
			} else
			{
				i2c->D = write_pointers[i2c_num][write_counter[i2c_num]];
				write_counter[i2c_num]++;
			}
		}
	}
}

void I2C0_IRQHandler(void)
{
	I2C_handler(0);
}

void I2C1_IRQHandler(void)
{
	I2C_handler(1);
}


void I2C2_IRQHandler(void)
{
	I2C_handler(2);
}




