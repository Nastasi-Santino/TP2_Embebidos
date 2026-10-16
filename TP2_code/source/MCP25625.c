#include "spi.h"
#include "MCP25625.h"




void MCP_INIT(void)
{
	SPI_config_t spi_config ;
	spi_config.spi_num = SPI_0;
	spi_config.baudrate = 100000U;
	spi_config.num_of_slaves = 1;
	spi_config.active_low = true;
	spi_config.frame_size = 8;
	spi_config.blocking = NON_BLOCKING_SPI;
	spi_config.first_bit = MSB_FIRST_SPI;
	spi_config.sck_active = SCK_ACTIVE_LOW;
	spi_config.sck_edge_sampling = LEADING_EDGE_SAMPLING;

	SPI_INIT(spi_config);

	//CHEQUEAR EL SLAVE CORRECTO
	//Reseteamos para configurar
	SPI_write(spi_config->spi_num, 0, MCP_INST_RESET, 1);

}

bool MCP_write_register(uint8_t spi_num, uint8_t slave, uint8_t data, uint8_t address )
{
	uint8_t buffer_data[3];
	buffer_data[0] = MCP_INST_WRITE;
	buffer_data[1] = address;
	buffer_data[2] = data;

	SPI_write(spi_num, slave, buffer_data, 3);
}
