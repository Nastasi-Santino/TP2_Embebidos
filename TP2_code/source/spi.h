#ifndef SPI_H_
#define SPI_H_

#include <stdbool.h>
#include <stdint.h>

#define SPI_DUMMY_DATA 0xFFFF

enum{
	SPI_0,
	SPI_1,
	SPI_2
};

enum{
	BLOCKING_SPI,
	NON_BLOCKING_SPI
};

enum{
	LSB_FIRST_SPI,
	MSB_FIRST_SPI
};

enum{
	SCK_ACTIVE_LOW,
	SCK_ACTIVE_HIGH
};

enum{
	LEADING_EDGE_SAMPLING,
	FOLLOWING_EDGE_SAMPLING
};

typedef struct{
	uint8_t spi_num;
	uint32_t baudrate;
	uint8_t num_of_slaves;
	bool active_low;
	uint8_t frame_size;
	bool blocking;
	uint8_t first_bit;
	bool sck_active;
	bool sck_edge_sampling;
}SPI_config_t;

bool SPI_INIT(SPI_config_t config);

bool SPI_write(uint8_t spi_num, uint8_t slave, uint16_t * data, uint8_t length);
bool SPI_read(uint8_t spi_num, uint8_t slave, uint16_t * data, uint8_t length);
bool SPI_transfer(uint8_t spi_num, uint8_t slave, uint16_t * data_write,
		uint16_t * data_read, uint8_t length);

bool SPI_busy(uint8_t spi_num);

#endif /* SPI_H_ */
