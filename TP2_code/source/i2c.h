#ifndef I2C_H_
#define I2C_H_

#include <stdbool.h>
#include <stdint.h>

#define MY_SLAVE_ADDRESS 0x00

enum{
	I2C_0,
	I2C_1,
	I2C_2
};

enum{
	BLOCKING,
	NON_BLOCKING
};

typedef struct{
	uint8_t i2c_num;
	uint32_t baudrate;
	bool blocking;
}I2C_config_t;

bool I2C_INIT(I2C_config_t config);

bool I2C_read(uint8_t i2c_num, uint8_t slave_address , uint8_t * data_byte, uint8_t length);
bool I2C_write(uint8_t i2c_num, uint8_t slave_address, uint8_t * data_byte, uint8_t length);

bool I2C_write_read(uint8_t i2c_num, uint8_t slave_address,
		uint8_t * write_data, uint8_t write_length, uint8_t * read_data, uint8_t read_length);

bool I2C_busy(uint8_t i2c_num);
uint8_t I2C_words_received(uint8_t i2c_num);


#endif /* I2C_H_ */
