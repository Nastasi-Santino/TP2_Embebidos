#include "spi.h"
#include "MCP25625.h"

// Instrucciones básicas sin parámetros
#define MCP_INST_RESET         	0xC0
#define MCP_INST_READ          	0x03
#define MCP_INST_WRITE         	0x02
#define MCP_INST_READ_STATUS   	0xA0
#define MCP_INST_RX_STATUS     	0xB0
#define MCP_INST_BIT_MODIFY    	0x05

#define MCP_CANCTRL_ADDRESS		0x0F

#define MCP_CANCTRL_CONFIG_MODE	0x93

// Instrucciones con parámetros de ubicación/búfer
#define MCP_INST_READ_RX_BUFFER(nm)  (0x90 | (((nm) & 0x03) << 1))
#define MCP_INST_LOAD_TX_BUFFER(abc) (0x40 | ((abc) & 0x07))
#define MCP_INST_RTS(nnn)            (0x80 | ((nnn) & 0x07))

// Opciones individuales para el comando RTS
#define MCP_RTS_TXB0 0x01
#define MCP_RTS_TXB1 0x02
#define MCP_RTS_TXB2 0x04

static bool MCP_write_register(uint8_t spi_num, uint8_t data, uint8_t address);


void MCP_INIT(uint8_t spi_num)
{
	SPI_config_t spi_config = {spi_num, 100000U, 1, true, 8,
			NON_BLOCKING_SPI, MSB_FIRST_SPI, SCK_ACTIVE_LOW, LEADING_EDGE_SAMPLING};
	SPI_INIT(spi_config);

	uint16_t reset = MCP_INST_RESET;
	SPI_write(spi_num, 0, &reset, 1);
	while(SPI_busy(spi_num));

	MCP_write_register(spi_num, MCP_CANCTRL_CONFIG_MODE, MCP_CANCTRL_ADDRESS);


}

static bool MCP_write_register(uint8_t spi_num, uint8_t data, uint8_t address)
{
	uint16_t buffer_data[3];
	buffer_data[0] = MCP_INST_WRITE;
	buffer_data[1] = (uint16_t)address;
	buffer_data[2] = (uint16_t)data;

	if(!SPI_write(spi_num, 0, buffer_data, 3))
	{
		return false;
	}
	while(SPI_busy(spi_num));

	return true;
}
