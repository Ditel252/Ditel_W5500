#include "Ditel_W5500.h"

uint8_t _DW5500_dhcp_buffer[1024];
uint8_t _DW5500_dns_buffer[1024];

static bool _DW5500_IP_Assigned = false;
static bool _DW5500_IP_Update = false;
static bool _DW5500_IP_Conflict = false;

static SPI_HandleTypeDef *_DW5500_SPI;
static GPIO_TypeDef *_DW5500_SPI_CS_Port;
static uint16_t _DW5500_CS_Pin;

static uint16_t _sendDataMaxSize;

uint8_t DW5500_init(SPI_HandleTypeDef *_hspi, GPIO_TypeDef *_cs_port, uint16_t _cs_pin, DW5500_Network_Info *_info, bool _setStaticIPAddress){
    _DW5500_SPI = _hspi;
    _DW5500_SPI_CS_Port = _cs_port;
    _DW5500_CS_Pin = _cs_pin;

    reg_wizchip_cs_cbfunc(_DW5500_Select, _DW5500_Unselect);
    reg_wizchip_spi_cbfunc(_DW5500_ReadByte, _DW5500_WriteByte);
    reg_wizchip_spiburst_cbfunc(_DW5500_ReadBuff, _DW5500_WriteBuff);

    wiz_NetInfo _Network_Info;

    _Network_Info.dhcp = NETINFO_DHCP;
    
    for (int _i = 0; _i < 6; _i++)
        *(_Network_Info.mac + _i) = *(_info->macAddress + _i);

    setSHAR(_Network_Info.mac);

    if(_setStaticIPAddress){
        for(int _i = 0; _i < 4; _i++)
            _Network_Info.ip[_i] = *((_info->ipAddress) + _i);
            
        for(int _i = 0; _i < 4; _i++)
            _Network_Info.gw[_i] = *((_info->gateWayAddress) + _i);
            
        for(int _i = 0; _i < 4; _i++)
            _Network_Info.sn[_i] = *((_info->subNetMaskAddress) + _i);

        for(int _i = 0; _i < 4; _i++)
            _Network_Info.dns[_i] = *((_info->DNSServerAddress) + _i);
    }else{
        DHCP_init(_DW5500_DHCP_SOCKET, _DW5500_dhcp_buffer);

        reg_dhcp_cbfunc(_DW5500_Callback_IPAssigned, _DW5500_Callback_IPUpddated, _DW5500_Callback_IPConflict);

        uint32_t _reTryCounter = 10000;
        while((!_DW5500_IP_Assigned) && (_reTryCounter > 0)) {
            DHCP_run();
            _reTryCounter--;
        }
        if(!_DW5500_IP_Assigned) {
            return DW5500_ERR_IP_NOT_ASSIGED;
        }

        getIPfromDHCP(_Network_Info.ip);
        getGWfromDHCP(_Network_Info.gw);
        getSNfromDHCP(_Network_Info.sn);

        uint8_t _Network_Info_dns[4];
        getDNSfromDHCP(_Network_Info_dns);
    }

    wizchip_setnetinfo(&_Network_Info);

    wizchip_getnetinfo(&_Network_Info);
    
    for(int _i = 0; _i < 6; _i++)
        *((_info->macAddress) + _i) = _Network_Info.mac[_i];

    for(int _i = 0; _i < 4; _i++)
        *((_info->ipAddress) + _i) = _Network_Info.ip[_i];
        
    for(int _i = 0; _i < 4; _i++)
        *((_info->gateWayAddress) + _i) = _Network_Info.gw[_i];
        
    for(int _i = 0; _i < 4; _i++)
        *((_info->subNetMaskAddress) + _i) = _Network_Info.sn[_i];

    for(int _i = 0; _i < 4; _i++)
        *((_info->DNSServerAddress) + _i) = _Network_Info.dns[_i];

    return DW5500_OK;
}

void _DW5500_Select(void) {
    HAL_GPIO_WritePin(_DW5500_SPI_CS_Port, _DW5500_CS_Pin, GPIO_PIN_RESET);
}

void _DW5500_Unselect(void) {
    HAL_GPIO_WritePin(_DW5500_SPI_CS_Port, _DW5500_CS_Pin, GPIO_PIN_SET);
}

void _DW5500_ReadBuff(uint8_t *_buff, uint16_t _len) {
    HAL_SPI_Receive(_DW5500_SPI, _buff, _len, HAL_MAX_DELAY);
}

void _DW5500_WriteBuff(uint8_t *_buff, uint16_t _len) {
    HAL_SPI_Transmit(_DW5500_SPI, _buff, _len, HAL_MAX_DELAY);
}

uint8_t _DW5500_ReadByte(void) {
    uint8_t byte;
    _DW5500_ReadBuff(&byte, sizeof(byte));
    return byte;
}

void _DW5500_WriteByte(uint8_t _byte) {
    _DW5500_WriteBuff(&_byte, sizeof(_byte));
}

void _DW5500_Callback_IPAssigned(void) {
    _DW5500_IP_Assigned = true;
}

void _DW5500_Callback_IPUpddated(void) {
    _DW5500_IP_Update = true;
}
 
void _DW5500_Callback_IPConflict(void) {
    _DW5500_IP_Conflict = true;
}

uint8_t _DW5500_Control_Phase(uint8_t _bsb, uint8_t _rwb, uint8_t _om){
    return (_bsb << 3) | (_rwb << 2) | _om;
}

uint8_t DW5500_UDP_Init(const uint8_t _socket){
    setSIMR(0b1 << _socket); //Enable socket 0 interrupts
    uint8_t _eachSocketInterruptMaskStatus = getSIMR(); //Get interrupt mask status of each socket
    uint8_t _eachSocketInterruptStatus = getSIR(); //Get interrupt status of each socket

    if(!((_eachSocketInterruptMaskStatus & (0b1 << _socket))))
        return DW5500_ERR_SET_SIMR; 

    setSn_IR(_socket, Sn_IR_RECV); //Enable socket receive interrupt
    setSn_IMR(_socket, Sn_IR_RECV); //Enable socket receive interrupt mask
    uint8_t _socketInterruptStatus = getSn_IR(_socket); //Get the interrupt status of a socket
    uint8_t _socketInterruptMaskStatus = getSn_IMR(_socket); // Get interrupt mask status of a socket

    if(!(_socketInterruptMaskStatus & Sn_IR_RECV))
            return DW5500_ERR_SET_SN_IMR;
    
    return DW5500_OK;
}

uint8_t DW5500_UDP_Open(const uint8_t _socket, const uint16_t _openPort){
    int8_t resultOfSocketOpen = socket(_socket, Sn_MR_UDP, _openPort, _DW5500_SOCKET_FLAG_SETTING); //Set socket protocol to udp

    if(resultOfSocketOpen != _socket)
        return DW5500_ERR_SOCKET_OPEN;

    uint16_t _maxBufferSize = getSn_TxMAX(_socket);
    _sendDataMaxSize = _maxBufferSize - (_maxBufferSize % 1472);

    return DW5500_OK;
}

void DW5500_UDP_Close(const uint8_t _socket){
    setSn_CR(_socket, Sn_CR_CLOSE);
}

uint8_t DW5500_UDP_Send(const uint8_t _socket, uint8_t *_toIp, uint16_t _toPort,uint8_t *_sendData, const uint16_t _sendDataSize){
    uint16_t _sentDataSize = 0;
    uint16_t _tempSendDataSize;
    int32_t _resultOfSend;

    while(_sentDataSize != _sendDataSize){
        _tempSendDataSize = (_sendDataSize - _sentDataSize) > _sendDataMaxSize ? _sendDataMaxSize : (_sendDataSize - _sentDataSize);

        _resultOfSend = sendto(_socket, _sendData + _sentDataSize, _tempSendDataSize, _toIp, _toPort);

        if(_resultOfSend < 0)
            return DW5500_ERR_UDP_SEND;
        
        _sentDataSize += _resultOfSend;
    }

    return DW5500_OK;
}

uint16_t DW5500_UDP_Avaiable(const uint8_t _socket){
    uint8_t _resultOfSn_Ir = getSn_IR(_socket);
    uint16_t _readDataSize = getSn_RX_RSR(_socket);
    setSn_IR(_socket, _resultOfSn_Ir);

    return (_readDataSize > 0 ? _readDataSize - 8 : 0);
}

uint8_t DW5500_UDP_Read(const uint8_t _socket, uint8_t *_readData, uint16_t _readDataSize, uint8_t *_fromIp, uint16_t *_fromPort){
    uint8_t _resultOfRead = recvfrom(_socket, _readData, _readDataSize, _fromIp, _fromPort);

    if(_resultOfRead < 0)
        return DW5500_ERR_UDP_READ;
    else if(_resultOfRead == 0)
        return 0;
    
    return DW5500_OK;
}