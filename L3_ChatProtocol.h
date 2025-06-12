#ifndef L3_CHATPROTOCOL_H
#define L3_CHATPROTOCOL_H

// 메시지 타입 정의
#define L3_MSG_TYPE_BEACON      0x10  // 주기적 브로드캐스트 (나를 좋아하는 기기 정보 포함)
#define L3_MSG_TYPE_CHAT_REQ    0x20  // 채팅 요청
#define L3_MSG_TYPE_CHAT_ACK    0x21  // 채팅 수락
#define L3_MSG_TYPE_CHAT_NACK   0x22  // 채팅 거절 (이미 채팅 중)
#define L3_MSG_TYPE_CHAT_MSG    0x30  // 채팅 메시지
#define L3_MSG_TYPE_CHAT_END    0x40  // 채팅 종료

// 채팅 상태
#define L3_CHAT_IDLE            0
#define L3_CHAT_REQUESTING      1
#define L3_CHAT_ACTIVE          2

// 최대 매칭 기기 수
#define L3_MAX_MATCHES          10

// 매칭 정보 구조체
typedef struct {
    uint8_t deviceId;
    int16_t rssi;
    uint32_t lastSeen;  // 마지막으로 비콘 받은 시간
} MatchInfo_t;

// L3 초기화
void L3_initChatProtocol(uint8_t myId, uint8_t likedId);

// 주기적 비콘 전송
void L3_sendBeacon(void);

// 매칭된 기기 목록 가져오기 (C++98 호환)
void L3_getMatchedDevicesList(MatchInfo_t** matches, int* count);

// 채팅 요청
void L3_requestChat(uint8_t targetId);

// 채팅 메시지 전송
void L3_sendChatMessage(const char* message);

// 채팅 종료
void L3_endChat(void);

// 현재 채팅 상태 확인
uint8_t L3_getChatState(void);

// 현재 채팅 상대 ID
uint8_t L3_getCurrentChatPartner(void);

// 주기적으로 호출되어야 하는 함수 (타임아웃 처리 등)
void L3_periodicTask(void);

#endif