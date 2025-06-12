#include "mbed.h"

// Layer3 PDU 타입 정의
#define L3_PDU_TYPE_BCAST       0x10
#define L3_PDU_TYPE_QUERY_LIKE  0x11
#define L3_PDU_TYPE_ANS_LIKE    0x12
#define L3_PDU_TYPE_CHAT_REQ    0x13
#define L3_PDU_TYPE_CHAT_ACK    0x14
#define L3_PDU_TYPE_CHAT_DEC    0x15
#define L3_PDU_TYPE_CHAT_DATA   0x16
#define L3_PDU_TYPE_CHAT_END    0x17

// PDU 구조 오프셋
#define L3_PDU_OFFSET_TYPE      0
#define L3_PDU_OFFSET_SENDER    1
#define L3_PDU_OFFSET_TARGET    5
#define L3_PDU_OFFSET_DATA      9

// PDU 최대 크기 (Layer2 제한 고려)
#define L3_PDU_MAXSIZE          26
#define L3_MAXDATASIZE          (L3_PDU_MAXSIZE - L3_PDU_OFFSET_DATA)

// Layer3 PDU 구조체
typedef struct {
    uint8_t type;
    uint32_t sender_id;
    uint32_t target_id;
    union {
        bool liked;                 // ANS_LIKE용
        char message[L3_MAXDATASIZE]; // CHAT_DATA용
        uint8_t raw_data[L3_MAXDATASIZE];
    } data;
} L3_PDU_t;

// PDU 관련 함수들
uint8_t L3_msg_encodePDU(uint8_t* pdu_buffer, uint8_t type, uint32_t sender_id, uint32_t target_id, const void* data, uint8_t data_len);
uint8_t L3_msg_decodePDU(const uint8_t* pdu_buffer, L3_PDU_t* pdu);
uint8_t L3_msg_getPDUType(const uint8_t* pdu_buffer);
uint32_t L3_msg_getSenderId(const uint8_t* pdu_buffer);
uint32_t L3_msg_getTargetId(const uint8_t* pdu_buffer);
uint8_t* L3_msg_getDataPtr(const uint8_t* pdu_buffer);