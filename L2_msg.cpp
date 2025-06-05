#include "mbed.h"
#include "L2_msg.h"

int L2_msg_checkIfData(uint8_t* msg)
{
    return (msg[L2_MSG_OFFSET_TYPE] == L2_MSG_TYPE_DATA || msg[L2_MSG_OFFSET_TYPE] == L2_MSG_TYPE_DATA_CONT);
}

int L2_msg_checkIfEndData(uint8_t* msg)
{
    return (msg[L2_MSG_OFFSET_TYPE] == L2_MSG_TYPE_DATA);
}


int L2_msg_checkIfAck(uint8_t* msg)
{
    return (msg[L2_MSG_OFFSET_TYPE] == L2_MSG_TYPE_ACK);
}

uint8_t L2_msg_encodeAck(uint8_t* msg_ack, uint8_t seq)
{
    msg_ack[L2_MSG_OFFSET_TYPE] = L2_MSG_TYPE_ACK;
    msg_ack[L2_MSG_OFFSET_SEQ] = seq;
    msg_ack[L2_MSG_OFFSET_DATA] = 1;

    return L2_MSG_ACKSIZE;
}

uint8_t L2_msg_encodeData(uint8_t* msg_data, uint8_t* data, int seq, int len, uint8_t flag_end)
{
    if (flag_end == 1)
        msg_data[L2_MSG_OFFSET_TYPE] = L2_MSG_TYPE_DATA;
    else
        msg_data[L2_MSG_OFFSET_TYPE] = L2_MSG_TYPE_DATA_CONT;
    msg_data[L2_MSG_OFFSET_SEQ] = seq;
    memcpy(&msg_data[L2_MSG_OFFSET_DATA], data, len*sizeof(uint8_t));

    return len+L2_MSG_OFFSET_DATA;
}
                    

uint8_t L2_msg_getSeq(uint8_t* msg)
{
    return msg[L2_MSG_OFFSET_SEQ];
}

uint8_t* L2_msg_getWord(uint8_t* msg)
{
    return &msg[L2_MSG_OFFSET_DATA];
}

// L2_msg.cpp
uint8_t L2_msg_encodeChat(uint8_t* msg, uint8_t type, const char* payload, uint8_t len) {
    msg[L2_MSG_OFFSET_TYPE] = type;
    msg[L2_MSG_OFFSET_SEQ] = 0;
    memcpy(&msg[L2_MSG_OFFSET_DATA], payload, len);
    return L2_MSG_OFFSET_DATA + len;
}
