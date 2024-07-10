#include "stdlib.h"
#include "socket.h"
#include "socket.h"
#include "dhcp.h"
#include "stm32f4xx_hal.h"
#include "wizchip_conf.h"
#include "stdbool.h"

#define DW5500_OK 0x01
#define DW5500_ERR_IP_NOT_ASSIGED 0x10
#define DW5500_ERR_SET_SIMR 0x20
#define DW5500_ERR_SET_SN_IMR 0x21
#define DW5500_ERR_SOCKET_OPEN 0x22
#define DW5500_ERR_UDP_SEND 0x23
#define DW5500_ERR_UDP_READ 0x24

#define _DW5500_DHCP_SOCKET 0
#define _DW5500_SOCKET      1

#define _DW5500_RWB_READ     0b0
#define _DW5500_RWB_WRITE    0b1

#define _DW5500_OM_NBYTES    0b00
#define _DW5500_OM_1BYTE     0b01
#define _DW5500_OM_2BYTE     0b10
#define _DW5500_OM_3BYTE     0b11

#define _DW5500_ADD_COMMON_REG          0b00000
#define _DW5500_ADD_UDP_SOCKET_REG      0b00001
#define _DW5500_ADD_UDP_SOCKET_TX_BUF   0b00010

#define _DW5500_ADD_PHYCFGR 0x002E

#define _DW5500_ADD_SN_MR_ADDRESS       0x0000
#define _DW5500_ADD_SN_CR_ADDRESS       0x0001
#define _DW5500_ADD_SR_CR_ADDRESS       0x0003
#define _DW5500_ADD_SR_PORT_ADDRESS     0x0004
#define _DW5500_ADD_SR_DIPR_ADDRESS     0x000C
#define _DW5500_ADD_SR_DPORT_ADDRESS    0x0010
#define _DW5500_ADD_SN_TX_WR_ADDRESS    0x0024

#define _DW5500_SOCKET_FLAG_SETTING 0x00

typedef struct _DW5500_Network_Info{
    uint8_t macAddress[6];
    uint8_t ipAddress[4];
    uint8_t gateWayAddress[4];
    uint8_t subNetMaskAddress[4];
    uint8_t DNSServerAddress[4];
}DW5500_Network_Info;

void _DW5500_Select(void);
void _DW5500_Unselect(void);
void _DW5500_ReadBuff(uint8_t *_buff, uint16_t _len);
void _DW5500_WriteBuff(uint8_t *_buff, uint16_t _len);
uint8_t _DW5500_ReadByte(void);
void _DW5500_WriteByte(uint8_t _byte);
void _DW5500_Callback_IPAssigned(void);
void _DW5500_Callback_IPUpddated(void); 
void _DW5500_Callback_IPConflict(void);
uint8_t _DW5500_Control_Phase(uint8_t _bsb, uint8_t _rwb, uint8_t _om);

uint8_t DW5500_init(SPI_HandleTypeDef *_hspi, GPIO_TypeDef *_cs_port, uint16_t _cs_pin, DW5500_Network_Info *_info, bool _setStaticIPAddress);
uint8_t DW5500_UDP_Init(const uint8_t _socket);
uint8_t DW5500_UDP_Open(const uint8_t _socket, const uint16_t _openPort);
void DW5500_UDP_Close(const uint8_t _socket);
uint8_t DW5500_UDP_Send(const uint8_t _socket, uint8_t *_toIp, uint16_t _toPort,uint8_t *_sendData, const uint16_t _sendDataSize);
uint16_t DW5500_UDP_Avaiable(const uint8_t _socket);
uint8_t DW5500_UDP_Read(const uint8_t _socket, uint8_t *_readData, uint16_t _readDataSize, uint8_t *_fromIp, uint16_t *_fromPort);