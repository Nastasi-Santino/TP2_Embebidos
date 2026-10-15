#include "spi.h"
#include "hardware.h"

#define SPI_MODULE_CLOCK (__CORE_CLOCK__ >> 1)

static SPI_Type * const spis[] = SPI_BASE_PTRS;

static IRQn_Type spi_IRQn[] = SPI_IRQS;
static uint16_t dummy_read[256];
static uint16_t dummy_write[256];

volatile static uint16_t * write_buffer[3];
volatile static uint16_t * read_buffer[3];
volatile static uint8_t buffer_length[3];
volatile static uint8_t counter_tx[3];
volatile static uint8_t counter_rx[3];
volatile static uint8_t active_slave[3];
volatile static bool busy[3];
volatile static bool EOQF_received[3];
volatile static bool last_rx[3];

static bool active_interrupt[3];

static void clock_PIN_Enable(uint8_t spi_num, uint8_t num_of_slaves);
static void set_PIN(PORT_Type * port, uint8_t pin, uint8_t alt);
static void SPI_setBaudrate(SPI_Type * spi, uint32_t baudrate);
static void SPI_handler(uint8_t spi_num);

bool SPI_INIT(SPI_config_t config)
{
	if(config.spi_num > SPI_2 ||
	   config.baudrate == 0 ||
	   config.frame_size < 4 ||
	   config.frame_size > 16 ||
	   config.first_bit > MSB_FIRST_SPI ||
	   (config.spi_num == SPI_0 && config.num_of_slaves > 6) ||
	   (config.spi_num == SPI_1 && config.num_of_slaves > 4) ||
	   (config.spi_num == SPI_2 && config.num_of_slaves > 2))
	{
	    return false;
	}

	clock_PIN_Enable(config.spi_num, config.num_of_slaves);

	SPI_Type * spi = spis[config.spi_num];

	spi->MCR |= SPI_MCR_HALT_MASK;

	SPI_setBaudrate(spi, config.baudrate);

	spi->MCR |= SPI_MCR_MSTR_MASK;

	if(config.active_low)
	{
		spi->MCR |= SPI_MCR_PCSIS_MASK;
	}

	spi->MCR |= SPI_MCR_CLR_TXF_MASK | SPI_MCR_CLR_RXF_MASK;

	spi->CTAR[0] &= ~SPI_CTAR_FMSZ_MASK;
	spi->CTAR[0] |= (config.frame_size - 1) << SPI_CTAR_FMSZ_SHIFT;


	spi->CTAR[0] &= ~(SPI_CTAR_LSBFE_MASK |
	                  SPI_CTAR_CPOL_MASK |
	                  SPI_CTAR_CPHA_MASK);
	if(config.first_bit == LSB_FIRST_SPI)
	{
		spi->CTAR[0] |= SPI_CTAR_LSBFE_MASK;
	}

	if(config.sck_active == SCK_ACTIVE_LOW)
	{
		spi->CTAR[0] |= SPI_CTAR_CPOL_MASK;
	}

	if(config.sck_edge_sampling == FOLLOWING_EDGE_SAMPLING)
	{
		spi->CTAR[0] |= SPI_CTAR_CPHA_MASK;
	}

	if(config.blocking == NON_BLOCKING_SPI)
	{
	    NVIC_ClearPendingIRQ(spi_IRQn[config.spi_num]);
	    NVIC_EnableIRQ(spi_IRQn[config.spi_num]);

	    active_interrupt[config.spi_num] = true;

	    spi->RSER |= SPI_RSER_RFDF_RE_MASK | SPI_RSER_EOQF_RE_MASK;
	}

	for(int i = 0; i < 256; i++)
	{
		dummy_write[i] = SPI_DUMMY_DATA;
	}

	spi->MCR &= ~SPI_MCR_MDIS_MASK;
	spi->MCR &= ~SPI_MCR_HALT_MASK;

	return true;
}


static void clock_PIN_Enable(uint8_t spi_num, uint8_t num_of_slaves)
{
	switch(spi_num)
	{
	case 0:
		SIM->SCGC6 |= SIM_SCGC6_SPI0_MASK;
		set_PIN(PORTD, 1, 2); // SCLK
		set_PIN(PORTD, 2, 2); // SOUT
		set_PIN(PORTD, 3, 2); // SIN

		switch(num_of_slaves)
		{
		case 6:
		    set_PIN(PORTB, 23, 3);   // PCS5
		case 5:
		    set_PIN(PORTC, 0, 2);    // PCS4
		case 4:
		    set_PIN(PORTC, 1, 2);    // PCS3
		case 3:
		    set_PIN(PORTC, 2, 2);    // PCS2
		case 2:
		    set_PIN(PORTC, 3, 2);    // PCS1
		case 1:
		    set_PIN(PORTD, 0, 2);    // PCS0
		    break;
		}
		break;
	case 1:
		SIM->SCGC6 |= SIM_SCGC6_SPI1_MASK;
		set_PIN(PORTB, 11, 2);
		set_PIN(PORTE, 3, 7);
		set_PIN(PORTD, 7, 7);

		switch(num_of_slaves)
		{
		case 4:
		    set_PIN(PORTE, 6, 2);    // SPI1_PCS3
		case 3:
		    set_PIN(PORTE, 5, 2);    // SPI1_PCS2
		case 2:
		    set_PIN(PORTE, 0, 2);    // SPI1_PCS1
		case 1:
		    set_PIN(PORTE, 4, 2);    // SPI1_PCS0
		    break;
		}
		break;
	case 2:
		SIM->SCGC3 |= SIM_SCGC3_SPI2_MASK;
		set_PIN(PORTB, 21, 2);
		set_PIN(PORTB, 22, 2);
		set_PIN(PORTB, 23, 2);

		switch(num_of_slaves)
		{
		case 2:
		    set_PIN(PORTD, 15, 2);   // SPI2_PCS1
		case 1:
		    set_PIN(PORTB, 20, 2);   // SPI2_PCS0
		    break;
		}
		break;
	default:
		break;
	}
}

static void set_PIN(PORT_Type * port, uint8_t pin, uint8_t alt)
{
	port->PCR[pin] &= ~(7 << PORT_PCR_MUX_SHIFT);
	port->PCR[pin] |= (alt << PORT_PCR_MUX_SHIFT);
}

static void SPI_setBaudrate(SPI_Type * spi, uint32_t baudrate)
{
    static const uint8_t pbr_values[4] = {
        2, 3, 5, 7
    };

    static const uint16_t br_values[16] = {
        2, 4, 6, 8,
        16, 32, 64, 128,
        256, 512, 1024, 2048,
        4096, 8192, 16384, 32768
    };

    uint32_t clock = SPI_MODULE_CLOCK;

    uint32_t best_baud = 0;
    uint8_t best_dbr = 0;
    uint8_t best_pbr = 0;
    uint8_t best_br = 0;

    for(uint8_t dbr = 0; dbr < 2; dbr++)
    {
        for(uint8_t pbr = 0; pbr < 4; pbr++)
        {
            for(uint8_t br = 0; br < 16; br++)
            {
                uint32_t real_baud =
                    (clock * (1U + dbr)) /
                    (pbr_values[pbr] * br_values[br]);

                if((real_baud <= baudrate) &&
                   (real_baud > best_baud))
                {
                    best_baud = real_baud;
                    best_dbr = dbr;
                    best_pbr = pbr;
                    best_br = br;
                }
            }
        }
    }

    /* Baudrate pedido menor que el mínimo posible */
    if(best_baud == 0)
    {
        best_dbr = 0;
        best_pbr = 3;   // PBR = 7
        best_br = 15;   // BR = 32768
    }

    spi->CTAR[0] &= ~(SPI_CTAR_DBR_MASK |
                      SPI_CTAR_PBR_MASK |
                      SPI_CTAR_BR_MASK);

    if(best_dbr)
    {
        spi->CTAR[0] |= SPI_CTAR_DBR_MASK;
    }

    spi->CTAR[0] |= SPI_CTAR_PBR(best_pbr) |
                    SPI_CTAR_BR(best_br);
}

bool SPI_write(uint8_t spi_num, uint8_t slave, uint16_t * data, uint8_t length)
{
	return SPI_transfer(spi_num, slave, data, dummy_read, length);
}


bool SPI_read(uint8_t spi_num, uint8_t slave, uint16_t * data, uint8_t length)
{
	return SPI_transfer(spi_num, slave, dummy_write, data, length);
}

bool SPI_transfer(uint8_t spi_num, uint8_t slave, uint16_t * data_write,
		uint16_t * data_read, uint8_t length)
{
	if(spi_num > SPI_2 || data_read == (uint16_t *)0 || data_write == (uint16_t *)0 || length == 0)
	{
	    return false;
	}

	if(busy[spi_num])
	{
		return false;
	}

	SPI_Type * spi = spis[spi_num];

	if((spi_num == SPI_0 && slave > 5) ||
	   (spi_num == SPI_1 && slave > 3) ||
	   (spi_num == SPI_2 && slave > 1))
	{
		return false;
	}

	spi->SR = SPI_SR_EOQF_MASK | SPI_SR_TCF_MASK | SPI_SR_RFDF_MASK | SPI_SR_TFFF_MASK;

	if(!active_interrupt[spi_num])
	{
		for(int i = 0; i < length; i++)
		{
			while(!(spi->SR & SPI_SR_TFFF_MASK));
			if(i == length - 1)
			{
				spi->PUSHR = SPI_PUSHR_EOQ_MASK  | (1 << slave) << SPI_PUSHR_PCS_SHIFT | data_write[i];
			} else
			{
				spi->PUSHR = SPI_PUSHR_CONT_MASK | (1 << slave) << SPI_PUSHR_PCS_SHIFT | data_write[i];
			}
			while(!(spi->SR & SPI_SR_RFDF_MASK));
			data_read[i] = (uint16_t)spi->POPR;
			spi->SR = SPI_SR_RFDF_MASK | SPI_SR_TFFF_MASK;
		}

		while(!(spi->SR & SPI_SR_EOQF_MASK));

		spi->SR = SPI_SR_EOQF_MASK;
	} else
	{
		write_buffer[spi_num] = data_write;
		read_buffer[spi_num] = data_read;
		buffer_length[spi_num] = length;
		active_slave[spi_num] = slave;
		busy[spi_num] = true;
		counter_tx[spi_num] = 0;
		counter_rx[spi_num] = 0;
		EOQF_received[spi_num] = false;
		last_rx[spi_num] = false;

		uint8_t max = (length > 4) ? 4 : length - 1;

		for(int i = 0; i < max; i++)
		{
			if(spi->SR & SPI_SR_TFFF_MASK)
			{
				spi->PUSHR = SPI_PUSHR_CONT_MASK | (1 << active_slave[spi_num]) << SPI_PUSHR_PCS_SHIFT
						| write_buffer[spi_num][counter_tx[spi_num]];
				counter_tx[spi_num]++;
			} else
			{
				break;
			}
		}

		if(max != 4)
		{
			spi->PUSHR = SPI_PUSHR_EOQ_MASK  | (1 << active_slave[spi_num]) << SPI_PUSHR_PCS_SHIFT
					| write_buffer[spi_num][counter_tx[spi_num]];
			counter_tx[spi_num]++;
		}
	}

	return true;
}

bool SPI_busy(uint8_t spi_num)
{
	if(spi_num > SPI_2)
	{
	    return false;
	}

	return busy[spi_num];
}

static void SPI_handler(uint8_t spi_num)
{
	SPI_Type * spi = spis[spi_num];

	if(spi->SR & SPI_SR_RFDF_MASK)
	{
		while(((spi->SR & SPI_SR_RXCTR_MASK) >> SPI_SR_RXCTR_SHIFT) > 0)
		{
		    read_buffer[spi_num][counter_rx[spi_num]] = spi->POPR;
		    counter_rx[spi_num]++;


			if(counter_tx[spi_num] < buffer_length[spi_num] && spi->SR & SPI_SR_TFFF_MASK)
			{
				if(counter_tx[spi_num] == buffer_length[spi_num] - 1)
				{
					spi->PUSHR = SPI_PUSHR_EOQ_MASK  | (1 << active_slave[spi_num]) << SPI_PUSHR_PCS_SHIFT
							| write_buffer[spi_num][counter_tx[spi_num]];
				} else
				{
					spi->PUSHR = SPI_PUSHR_CONT_MASK | (1 << active_slave[spi_num]) << SPI_PUSHR_PCS_SHIFT
							| write_buffer[spi_num][counter_tx[spi_num]];
				}
				spi->SR = SPI_SR_TFFF_MASK;
				counter_tx[spi_num]++;
			}
		}
		spi->SR = SPI_SR_RFDF_MASK;

		if(counter_rx[spi_num] == buffer_length[spi_num])
		{
			if(EOQF_received[spi_num])
			{
				busy[spi_num] = false;
			} else
			{
				last_rx[spi_num] = true;
			}
		}
	}

	if(spi->SR & SPI_SR_EOQF_MASK)
	{
		spi->SR = SPI_SR_EOQF_MASK;

		if(last_rx[spi_num])
		{
			busy[spi_num] = false;
		} else
		{
			EOQF_received[spi_num] = true;
		}
	}
}

void SPI0_IRQHandler(void)
{
	SPI_handler(SPI_0);
}

void SPI1_IRQHandler(void)
{
	SPI_handler(SPI_1);
}

void SPI2_IRQHandler(void)
{
	SPI_handler(SPI_2);
}
