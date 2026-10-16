/*
 * MCP25625.h
 *
 *  Created on: 16 oct. 2026
 *      Author: ramor
 */

#ifndef MCP25625_H_
#define MCP25625_H_

#include <stdbool.h>
#include <stdint.h>

// Instrucciones básicas sin parámetros
#define MCP_INST_RESET         0xC0
#define MCP_INST_READ          0x03
#define MCP_INST_WRITE         0x02
#define MCP_INST_READ_STATUS   0xA0
#define MCP_INST_RX_STATUS     0xB0
#define MCP_INST_BIT_MODIFY    0x05

// Instrucciones con parámetros de ubicación/búfer
#define MCP_INST_READ_RX_BUFFER(nm)  (0x90 | (((nm) & 0x03) << 1))
#define MCP_INST_LOAD_TX_BUFFER(abc) (0x40 | ((abc) & 0x07))
#define MCP_INST_RTS(nnn)            (0x80 | ((nnn) & 0x07))

// Opciones individuales para el comando RTS
#define MCP_RTS_TXB0 0x01
#define MCP_RTS_TXB1 0x02
#define MCP_RTS_TXB2 0x04

void MCP_INIT(void);

bool MCP_write_register(uint8_t spi_num, uint8_t slave,  )


#endif /* MCP25625_H_ */
