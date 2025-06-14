#include "mbed.h"
#include "L3_msg.h"
#include <cstring>

uint8_t L3_msg_encodePDU(uint8_t* pdu_buffer, uint8_t type, uint32_t sender_id, uint32_t target_id, const void* data, uint8_t data_len) {
    if (!pdu_buffer) return 0;
    
    // PDU 헤더 설정
    pdu_buffer[L3_PDU_OFFSET_TYPE] = type;
    
    // sender_id (4바이트, 리틀 엔디안)
    pdu_buffer[L3_PDU_OFFSET_SENDER + 0] = (sender_id >> 0) & 0xFF;
    pdu_buffer[L3_PDU_OFFSET_SENDER + 1] = (sender_id >> 8) & 0xFF;
    pdu_buffer[L3_PDU_OFFSET_SENDER + 2] = (sender_id >> 16) & 0xFF;
    pdu_buffer[L3_PDU_OFFSET_SENDER + 3] = (sender_id >> 24) & 0xFF;
    
    // target_id (4바이트, 리틀 엔디안)
    pdu_buffer[L3_PDU_OFFSET_TARGET + 0] = (target_id >> 0) & 0xFF;
    pdu_buffer[L3_PDU_OFFSET_TARGET + 1] = (target_id >> 8) & 0xFF;
    pdu_buffer[L3_PDU_OFFSET_TARGET + 2] = (target_id >> 16) & 0xFF;
    pdu_buffer[L3_PDU_OFFSET_TARGET + 3] = (target_id >> 24) & 0xFF;
    
    // 데이터 부분
    uint8_t total_size = L3_PDU_OFFSET_DATA;
    if (data && data_len > 0) {
        uint8_t actual_data_len = (data_len > L3_MAXDATASIZE) ? L3_MAXDATASIZE : data_len;
        memcpy(&pdu_buffer[L3_PDU_OFFSET_DATA], data, actual_data_len);
        total_size += actual_data_len;
    }
    
    return total_size;
}

uint8_t L3_msg_decodePDU(const uint8_t* pdu_buffer, L3_PDU_t* pdu) {
    if (!pdu_buffer || !pdu) return 0;
    
    // 헤더 디코딩
    pdu->type = pdu_buffer[L3_PDU_OFFSET_TYPE];
    
    // sender_id 디코딩
    pdu->sender_id = ((uint32_t)pdu_buffer[L3_PDU_OFFSET_SENDER + 0] << 0) |
                     ((uint32_t)pdu_buffer[L3_PDU_OFFSET_SENDER + 1] << 8) |
                     ((uint32_t)pdu_buffer[L3_PDU_OFFSET_SENDER + 2] << 16) |
                     ((uint32_t)pdu_buffer[L3_PDU_OFFSET_SENDER + 3] << 24);
    
    // target_id 디코딩
    pdu->target_id = ((uint32_t)pdu_buffer[L3_PDU_OFFSET_TARGET + 0] << 0) |
                     ((uint32_t)pdu_buffer[L3_PDU_OFFSET_TARGET + 1] << 8) |
                     ((uint32_t)pdu_buffer[L3_PDU_OFFSET_TARGET + 2] << 16) |
                     ((uint32_t)pdu_buffer[L3_PDU_OFFSET_TARGET + 3] << 24);
    
    return 1; // 성공
}

uint8_t L3_msg_getPDUType(const uint8_t* pdu_buffer) {
    return pdu_buffer ? pdu_buffer[L3_PDU_OFFSET_TYPE] : 0;
}

uint32_t L3_msg_getSenderId(const uint8_t* pdu_buffer) {
    if (!pdu_buffer) return 0;
    
    return ((uint32_t)pdu_buffer[L3_PDU_OFFSET_SENDER + 0] << 0) |
           ((uint32_t)pdu_buffer[L3_PDU_OFFSET_SENDER + 1] << 8) |
           ((uint32_t)pdu_buffer[L3_PDU_OFFSET_SENDER + 2] << 16) |
           ((uint32_t)pdu_buffer[L3_PDU_OFFSET_SENDER + 3] << 24);
}

uint32_t L3_msg_getTargetId(const uint8_t* pdu_buffer) {
    if (!pdu_buffer) return 0;
    
    return ((uint32_t)pdu_buffer[L3_PDU_OFFSET_TARGET + 0] << 0) |
           ((uint32_t)pdu_buffer[L3_PDU_OFFSET_TARGET + 1] << 8) |
           ((uint32_t)pdu_buffer[L3_PDU_OFFSET_TARGET + 2] << 16) |
           ((uint32_t)pdu_buffer[L3_PDU_OFFSET_TARGET + 3] << 24);
}

uint8_t* L3_msg_getDataPtr(const uint8_t* pdu_buffer) {
    return pdu_buffer ? (uint8_t*)&pdu_buffer[L3_PDU_OFFSET_DATA] : NULL;
}
