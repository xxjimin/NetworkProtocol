#include "mbed.h"
#include "L3_chatProtocol.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"
#include <string.h>

extern Serial pc;


// 상태 변수
static uint8_t myDeviceId;
static uint8_t likedDeviceId;
static uint8_t chatState = L3_CHAT_IDLE;
static uint8_t currentChatPartner = 0;
// 상단에 추가 (전역 변수)
static uint8_t pendingChatRequestId = 0;


// 매칭 정보 저장 (정적 배열)
static MatchInfo_t matchedDevices[L3_MAX_MATCHES];
static int matchedDeviceCount = 0;

// 비콘 메시지 구조: [TYPE(1)] [MY_ID(1)] [LIKED_ID(1)]
// 채팅 요청: [TYPE(1)] [SRC_ID(1)] [DST_ID(1)]
// 채팅 메시지: [TYPE(1)] [SRC_ID(1)] [DST_ID(1)] [MSG(...)]

// 타이머
static Ticker beaconTimer;
static Timer systemTimer;

// 내부 함수 선언
static void handleBeacon(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi);
static void handleChatRequest(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi);
static void handleChatAck(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi);
static void handleChatNack(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi);
static void handleChatMessage(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi);
static void handleChatEnd(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi);
static void sendBeaconTask(void);
static void cleanupOldMatches(void);

// L3 데이터 수신 콜백
void L3_dataInd(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi) {
    if (size < 1) return;
    
    uint8_t msgType = data[0];
    
    switch(msgType) {
        case L3_MSG_TYPE_BEACON:
            handleBeacon(data, srcId, size, snr, rssi);
            break;
        case L3_MSG_TYPE_CHAT_REQ:
            handleChatRequest(data, srcId, size, snr, rssi);
            break;
        case L3_MSG_TYPE_CHAT_ACK:
            handleChatAck(data, srcId, size, snr, rssi);
            break;
        case L3_MSG_TYPE_CHAT_NACK:
            handleChatNack(data, srcId, size, snr, rssi);
            break;
        case L3_MSG_TYPE_CHAT_MSG:
            handleChatMessage(data, srcId, size, snr, rssi);
            break;
        case L3_MSG_TYPE_CHAT_END:
            handleChatEnd(data, srcId, size, snr, rssi);
            break;
    }
}

// L3 데이터 전송 확인 콜백
void L3_dataCnf(int err) {
    // 필요시 구현
}

// L3 재설정 확인 콜백
void L3_reconfigSrcIdCnf(int err) {
    if (err == 0) {
        pc.printf("[L3] Device ID successfully changed to %d\n", myDeviceId);
    } else {
        pc.printf("[L3] Failed to change device ID\n");
        pc.printf("[DEBUG] current chatState = %d\n", chatState);

    }
}

// 초기화
void L3_initChatProtocol(uint8_t myId, uint8_t likedId) {
    myDeviceId = myId;
    likedDeviceId = likedId;
    chatState = L3_CHAT_IDLE;
    currentChatPartner = 0;
    matchedDeviceCount = 0;
    
    // L3 인터페이스 초기화
    L3_LLI_setDataIndFunc(L3_dataInd);
    L3_LLI_setDataCnfFunc(L3_dataCnf);
    L3_LLI_setReconfigSrcIdCnfFunc(L3_reconfigSrcIdCnf);
    
    // 시스템 타이머 시작
    systemTimer.start();
    
    // 비콘 타이머 시작 (3초마다)
    beaconTimer.attach(&sendBeaconTask, 3.0);
    
    pc.printf("\n[L3] Chat Protocol Initialized\n");
    pc.printf("My ID: %d, I like ID: %d\n", myDeviceId, likedDeviceId);
    pc.printf("[DEBUG] current chatState = %d\n", chatState);

}

// 비콘 전송
static void sendBeaconTask(void) {
    L3_sendBeacon();
}

void L3_sendBeacon(void) {
    uint8_t beacon[3];
    beacon[0] = L3_MSG_TYPE_BEACON;
    beacon[1] = myDeviceId;
    beacon[2] = likedDeviceId;
    
    L3_LLI_dataReq(beacon, 3, L2_BROADCAST_ID);
}

// 비콘 처리
static void handleBeacon(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi) {
    if (size < 3) return;
    
    uint8_t senderId = data[1];
    uint8_t senderLikedId = data[2];
    
    // 상대가 나를 좋아하고 RSSI가 -30 이상인 경우
    if (senderLikedId == myDeviceId && rssi >= -30) {
        // 매칭 목록 업데이트
        bool found = false;
        int i;
        
        for (i = 0; i < matchedDeviceCount; i++) {
            if (matchedDevices[i].deviceId == senderId) {
                matchedDevices[i].rssi = rssi;
                matchedDevices[i].lastSeen = systemTimer.read_ms();
                found = true;
                break;
            }
        }
        
        if (!found && matchedDeviceCount < L3_MAX_MATCHES) {
            matchedDevices[matchedDeviceCount].deviceId = senderId;
            matchedDevices[matchedDeviceCount].rssi = rssi;
            matchedDevices[matchedDeviceCount].lastSeen = systemTimer.read_ms();
            matchedDeviceCount++;
            
            pc.printf("\n[MATCH] Device %d likes you! (RSSI: %d)\n", senderId, rssi);
        }
    }
}

// 오래된 매칭 정리
static void cleanupOldMatches(void) {
    uint32_t currentTime = systemTimer.read_ms();
    int i = 0;
    
    while (i < matchedDeviceCount) {
        if ((currentTime - matchedDevices[i].lastSeen) > 10000) {
            // 10초 타임아웃 - 항목 제거
            int j;
            for (j = i; j < matchedDeviceCount - 1; j++) {
                matchedDevices[j] = matchedDevices[j + 1];
            }
            matchedDeviceCount--;
        } else {
            i++;
        }
    }
}

// 매칭된 기기 목록 가져오기
void L3_getMatchedDevicesList(MatchInfo_t** matches, int* count) {
    cleanupOldMatches();
    *matches = matchedDevices;
    *count = matchedDeviceCount;
}

// 채팅 요청
void L3_requestChat(uint8_t targetId) {
    if (chatState != L3_CHAT_IDLE) {
        pc.printf("[CHAT] Already in chat or requesting\n");
        return;
    }
    
    uint8_t req[3];
    req[0] = L3_MSG_TYPE_CHAT_REQ;
    req[1] = myDeviceId;
    req[2] = targetId;
    
    chatState = L3_CHAT_REQUESTING;
    currentChatPartner = targetId;
    
    L3_LLI_dataReq(req, 3, targetId);
    pc.printf("[CHAT] Requesting chat with device %d...\n", targetId);
    pc.printf("[DEBUG] current chatState = %d\n", chatState);

}

// 채팅 요청 처리
// 수정된 handleChatRequest()
static void handleChatRequest(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi) {
    if (size < 3) return;
    
    uint8_t requesterId = data[1];
    uint8_t targetId = data[2];
    
    if (targetId != myDeviceId) return;

    if (chatState == L3_CHAT_IDLE) {
        chatState = L3_CHAT_PENDING;
        pendingChatRequestId = requesterId;
        
        pc.printf("\n[CHAT] Chat request from device %d\n", requesterId);
        pc.printf("Type 'accept' or 'decline'\n");
        pc.printf("[DEBUG] current chatState = %d\n", chatState);

    } else {
        // 내가 이미 채팅 중일 경우 바로 NACK 전송
        uint8_t resp[3];
        resp[0] = L3_MSG_TYPE_CHAT_NACK;
        resp[1] = myDeviceId;
        resp[2] = requesterId;

        L3_LLI_dataReq(resp, 3, requesterId);
        pc.printf("\n[CHAT] Chat request from device %d - REJECTED (busy)\n", requesterId);
        pc.printf("[DEBUG] current chatState = %d\n", chatState);

    }
}


// 채팅 수락 처리
static void handleChatAck(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi) {
    if (size < 3) return;
    if (chatState != L3_CHAT_REQUESTING) return;
    
    uint8_t ackFrom = data[1];
    
    if (ackFrom == currentChatPartner) {
        chatState = L3_CHAT_ACTIVE;
        pc.printf("\n[CHAT] Chat accepted by device %d!\n", ackFrom);
        pc.printf("You are now chatting with device %d. Type 'quit' to end.\n", ackFrom);
        pc.printf("[DEBUG] current chatState = %d\n", chatState);

    }
}

// 채팅 거절 처리
static void handleChatNack(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi) {
    if (size < 3) return;
    if (chatState != L3_CHAT_REQUESTING) return;
    
    uint8_t nackFrom = data[1];
    
    if (nackFrom == currentChatPartner) {
        chatState = L3_CHAT_IDLE;
        currentChatPartner = 0;
        pc.printf("\n[CHAT] Device %d is busy in another chat!\n", nackFrom);
        pc.printf("[DEBUG] current chatState = %d\n", chatState);

    }
}

// 채팅 메시지 전송
void L3_sendChatMessage(const char* message) {
    if (chatState != L3_CHAT_ACTIVE) {
        pc.printf("[CHAT] Not in active chat\n");
        return;
    }
    
    uint8_t msg[100];
    msg[0] = L3_MSG_TYPE_CHAT_MSG;
    msg[1] = myDeviceId;
    msg[2] = currentChatPartner;
    
    int msgLen = strlen(message);
    if (msgLen > 96) msgLen = 96;
    
    memcpy(&msg[3], message, msgLen);
    
    L3_LLI_dataReq(msg, msgLen + 3, currentChatPartner);
}

// 채팅 메시지 처리
static void handleChatMessage(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi) {
    if (size < 4) return;
    if (chatState != L3_CHAT_ACTIVE) return;
    
    uint8_t senderId = data[1];
    uint8_t targetId = data[2];
    
    if (targetId != myDeviceId || senderId != currentChatPartner) return;
    
    char message[100] = {0};
    memcpy(message, &data[3], size - 3);
    
    pc.printf("\n[Device %d]: %s\n", senderId, message);
    pc.printf("[DEBUG] current chatState = %d\n", chatState);

}

// 채팅 종료
void L3_endChat(void) {
    if (chatState != L3_CHAT_ACTIVE) return;
    
    uint8_t end[3];
    end[0] = L3_MSG_TYPE_CHAT_END;
    end[1] = myDeviceId;
    end[2] = currentChatPartner;
    
    L3_LLI_dataReq(end, 3, currentChatPartner);
    
    pc.printf("[CHAT] Ending chat with device %d\n", currentChatPartner);
    pc.printf("[DEBUG] current chatState = %d\n", chatState);

    
    chatState = L3_CHAT_IDLE;
    currentChatPartner = 0;
}

void L3_acceptChatRequest(void) {
    if (chatState == L3_CHAT_PENDING && pendingChatRequestId != 0) {
        uint8_t ack[3] = { L3_MSG_TYPE_CHAT_ACK, myDeviceId, pendingChatRequestId };
        L3_LLI_dataReq(ack, 3, pendingChatRequestId);
        currentChatPartner = pendingChatRequestId;
        chatState = L3_CHAT_ACTIVE;
        pendingChatRequestId = 0;
        pc.printf("[CHAT] You are now chatting with device %d. Type 'quit' to end.\n", currentChatPartner);
    } else {
        pc.printf("[CHAT] No pending request to accept.\n");
        
    }
}

void L3_declineChatRequest(void) {
    if (chatState == L3_CHAT_PENDING && pendingChatRequestId != 0) {
        uint8_t nack[3] = { L3_MSG_TYPE_CHAT_NACK, myDeviceId, pendingChatRequestId };
        L3_LLI_dataReq(nack, 3, pendingChatRequestId);
        chatState = L3_CHAT_IDLE;
        pendingChatRequestId = 0;
        pc.printf("[CHAT] Chat request declined.\n");
    } else {
        pc.printf("[CHAT] No pending request to decline.\n");
    }
}

uint8_t L3_hasPendingChatRequest(void) {
    return (chatState == L3_CHAT_PENDING && pendingChatRequestId != 0);
}


// 채팅 종료 처리
static void handleChatEnd(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi) {
    if (size < 3) return;
    if (chatState != L3_CHAT_ACTIVE) return;
    
    uint8_t endFrom = data[1];
    
    if (endFrom == currentChatPartner) {
        pc.printf("\n[CHAT] Device %d has ended the chat\n", endFrom);
        chatState = L3_CHAT_IDLE;
        currentChatPartner = 0;
    }
}

// 현재 채팅 상태
uint8_t L3_getChatState(void) {
    return chatState;
}

// 현재 채팅 상대
uint8_t L3_getCurrentChatPartner(void) {
    return currentChatPartner;
}

// 주기적 작업
void L3_periodicTask(void) {
    // 주기적으로 오래된 매칭 정리
    cleanupOldMatches();
}