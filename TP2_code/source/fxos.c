#include "fxos.h"
#include "gpio.h"
#include "i2c.h"
#include "hardware.h"
#include <math.h>

#define I2C_BAUDRATE 			  (uint32_t)100000

#define FXOS_SLAVE_ADDRESS 		  0x1D

#define FXOS8700CQ_STATUS         0x00

#define FXOS8700CQ_OUT_X_MSB      0x01
#define FXOS8700CQ_OUT_X_LSB      0x02
#define FXOS8700CQ_OUT_Y_MSB      0x03
#define FXOS8700CQ_OUT_Y_LSB      0x04
#define FXOS8700CQ_OUT_Z_MSB      0x05
#define FXOS8700CQ_OUT_Z_LSB      0x06

#define FXOS8700CQ_WHO_AM_I       0x0D
#define FXOS8700CQ_XYZ_DATA_CFG   0x0E

#define FXOS8700CQ_CTRL_REG1      0x2A
#define FXOS8700CQ_CTRL_REG2      0x2B
#define FXOS8700CQ_CTRL_REG3      0x2C
#define FXOS8700CQ_CTRL_REG4	  0x2D
#define FXOS8700CQ_CTRL_REG5	  0x2E

#define FXOS8700CQ_M_OUT_X_MSB    0x33
#define FXOS8700CQ_M_OUT_X_LSB    0x34
#define FXOS8700CQ_M_OUT_Y_MSB    0x35
#define FXOS8700CQ_M_OUT_Y_LSB    0x36
#define FXOS8700CQ_M_OUT_Z_MSB    0x37
#define FXOS8700CQ_M_OUT_Z_LSB    0x38

#define FXOS8700CQ_M_CTRL_REG1    0x5B
#define FXOS8700CQ_M_CTRL_REG2    0x5C

#define FXOS8700CQ_WHO_AM_I_VALUE 0xC7

typedef struct {
    int16_t ax, ay, az;
    int16_t mx, my, mz;
} FXOS_raw_t;

static volatile FXOS_raw_t current_sample;
static volatile uint8_t raw_data_buffer[12];
static volatile bool read_pending;
static uint8_t raw_data_address = FXOS8700CQ_OUT_X_MSB;

static void dataReady_IRQ(void);

bool FXOS_INIT(void)
{
	hw_EnableInterrupts();
	static I2C_config_t i2c_conf = {I2C_0, I2C_BAUDRATE, NON_BLOCKING};

	if(!I2C_INIT(i2c_conf))
	{
		hw_DisableInterrupts();
		return false;
	}

	uint8_t who_am_i_address = FXOS8700CQ_WHO_AM_I;
	uint8_t who_am_i_value;
	if(!I2C_write_read(I2C_0, FXOS_SLAVE_ADDRESS, &who_am_i_address, 1, &who_am_i_value, 1))
	{
		hw_DisableInterrupts();
		return false;
	}

	while(I2C_busy(I2C_0));

	if(who_am_i_value != FXOS8700CQ_WHO_AM_I_VALUE)
	{
		return false;
	}

	uint8_t active[2] = {FXOS8700CQ_CTRL_REG1, 0x00};

	if(!I2C_write(I2C_0, FXOS_SLAVE_ADDRESS, active, 2))
	{
		hw_DisableInterrupts();
		return false;
	}
	while(I2C_busy(I2C_0));

	uint8_t fxos_config[3] = {FXOS8700CQ_CTRL_REG4, 0x01, 0x01};
	if(!I2C_write(I2C_0, FXOS_SLAVE_ADDRESS, fxos_config, 3))
	{
		hw_DisableInterrupts();
		return false;
	}

	while(I2C_busy(I2C_0));

	fxos_config[0] = FXOS8700CQ_M_CTRL_REG1;
	fxos_config[1] = 0x03;
	fxos_config[2] = 0x20;
	if(!I2C_write(I2C_0, FXOS_SLAVE_ADDRESS, fxos_config, 3))
	{
		hw_DisableInterrupts();
		return false;
	}

	while(I2C_busy(I2C_0));

	gpioMode(PORTNUM2PIN(PC, 6), INPUT);

	uint8_t int_source_address = 0x0C;
	uint8_t int_source_value;
	if(!I2C_write_read(I2C_0, FXOS_SLAVE_ADDRESS, &int_source_address, 1, &int_source_value, 1))
	{
		hw_DisableInterrupts();
		return false;
	}

	while(I2C_busy(I2C_0));

	if(int_source_value & 0x01)
	{
		if(!I2C_write_read(I2C_0, FXOS_SLAVE_ADDRESS, &raw_data_address, 1, (uint8_t *)raw_data_buffer, 12))
		{
			hw_DisableInterrupts();
			return false;
		}

		while(I2C_busy(I2C_0));
	}

	if(!gpioIRQ (PORTNUM2PIN(PC, 6), GPIO_IRQ_MODE_FALLING_EDGE, dataReady_IRQ))
	{
		hw_DisableInterrupts();
		return false;
	}

	active[1] = 0x01;
	if(!I2C_write(I2C_0, FXOS_SLAVE_ADDRESS, active, 2))
	{
		hw_DisableInterrupts();
		return false;
	}
	while(I2C_busy(I2C_0));

	hw_DisableInterrupts();
	return true;
}

void FXOS_service(void)
{
	if(read_pending && !I2C_busy(I2C_0))
	{
		read_pending = false;
		int16_t sensor_ax, sensor_ay, sensor_az;
		int16_t sensor_mx, sensor_my, sensor_mz;
		sensor_ax = (int16_t)(((raw_data_buffer[0] << 8) | raw_data_buffer[1])) >> 2;
		sensor_ay = (int16_t)(((raw_data_buffer[2] << 8) | raw_data_buffer[3])) >> 2;
		sensor_az = (int16_t)(((raw_data_buffer[4] << 8) | raw_data_buffer[5])) >> 2;
		sensor_mx = (int16_t)(((raw_data_buffer[6] << 8) | raw_data_buffer[7]));
		sensor_my = (int16_t)(((raw_data_buffer[8] << 8) | raw_data_buffer[9]));
		sensor_mz = (int16_t)(((raw_data_buffer[10] << 8) | raw_data_buffer[11]));

		current_sample.ax = sensor_ay;
		current_sample.ay = -sensor_ax;
		current_sample.az = sensor_az;
		current_sample.mx = sensor_my;
		current_sample.my = -sensor_mx;
		current_sample.mz = sensor_mz;
	}
}

int16_t get_roll(void)
{
	float_t phi = atan2f((float_t)current_sample.ay,(float_t)current_sample.az);
	return (int16_t) (phi * 180 / M_PI);
}

int16_t get_pitch(void)
{
	float_t sqrt = sqrtf((powf((float_t)current_sample.ay, (float_t)2.0)) +
			(powf((float_t)current_sample.az, (float_t)2.0)));
	float_t theta = atan2f(-(float_t)current_sample.ax, sqrt);
	return (int16_t) (theta * 180 / M_PI);
}

int16_t get_heading(void)
{
	float_t phi = atan2f((float_t)current_sample.ay,(float_t)current_sample.az);

	float_t sqrt = sqrtf((powf((float_t)current_sample.ay, (float_t)2.0)) +
			(powf((float_t)current_sample.az, (float_t)2.0)));
	float_t theta = atan2f(-(float_t)current_sample.ax, sqrt);

	float_t mx_heading = (float_t)current_sample.mx * cosf(theta) +
			(float_t)current_sample.mz * sinf(theta);

	float_t my_heading = (float_t)current_sample.mx * sinf(phi) * sinf(theta) +
			(float_t)current_sample.my * cosf(phi) -
			(float_t)current_sample.mz * sinf(phi) * cosf(theta);

	float_t heading = atan2f(my_heading, mx_heading);

	return (int16_t) (heading * 180 / M_PI);
}

void get_orientation(int16_t * roll, int16_t * pitch, int16_t * heading)
{
	float_t phi = atan2f((float_t)current_sample.ay,(float_t)current_sample.az);

	float_t sqrt = sqrtf((powf((float_t)current_sample.ay, (float_t)2.0)) +
			(powf((float_t)current_sample.az, (float_t)2.0)));
	float_t theta = atan2f(-(float_t)current_sample.ax, sqrt);

	float_t mx_heading = (float_t)current_sample.mx * cosf(theta) +
			(float_t)current_sample.mz * sinf(theta);

	float_t my_heading = (float_t)current_sample.mx * sinf(phi) * sinf(theta) +
			(float_t)current_sample.my * cosf(phi) -
			(float_t)current_sample.mz * sinf(phi) * cosf(theta);

	float_t heading_rad = atan2f(my_heading, mx_heading);

	*roll = (int16_t) (phi * 180 / M_PI);
	*pitch = (int16_t) (theta * 180 / M_PI);
	*heading =  (int16_t) (heading_rad * 180 / M_PI);
}


static void dataReady_IRQ(void)
{
	if(I2C_write_read(I2C_0, FXOS_SLAVE_ADDRESS, &raw_data_address, 1, (uint8_t *)raw_data_buffer, 12))
	{
		read_pending = true;
	}
}

