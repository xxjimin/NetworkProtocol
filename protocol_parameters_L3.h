#ifndef PROTOCOL_PARAMETERS_L3_H
#define PROTOCOL_PARAMETERS_L3_H

// Layer3 Chat Protocol 파라미터들
#define L3_MAX_RETRY_COUNT          3       // 최대 재시도 횟수
#define L3_RSSI_THRESHOLD           -80     // RSSI 임계값 (dBm)
#define L3_PROXIMITY_CHECK_INTERVAL 5.0f    // 근접성 체크 간격 (초)
#define L3_BROADCAST_INTERVAL       3.0f    // 브로드캐스트 간격 (초)
#define L3_WAIT_RESPONSE_TIMEOUT    10.0f   // 채팅 응답 대기 시간 (초)
#define L3_USER_RESPONSE_TIMEOUT    30.0f   // 사용자 응답 대기 시간 (초)

// PDU 크기 제한 (Layer2 최대 데이터 크기 고려)
#define L3_MAXDATASIZE              17      // Layer2 최대 26바이트 - Layer3 헤더 9바이트
#define L3_PDU_MAXSIZE              26      // Layer2 최대 크기와 동일

// 브로드캐스트 ID
#define L3_BROADCAST_ID             255

// 디버그 메시지 플래그
#ifndef DBGMSG_L3
#define DBGMSG_L3                   1
#endif