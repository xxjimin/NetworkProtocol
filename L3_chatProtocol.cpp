#include "mbed.h"
#include "L3_chatProtocol.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"
#include <string.h>

// 상태 변수
static uint8_t myDeviceId;
static uint8_t likedDeviceId;
static uint8_t chatState = L3_CHAT_IDLE;
static uint8_t currentChatPartner = 0;
static uint8_t pendingChatRequester = 0;

// 매칭 정보 저장 (정적 배열)
static MatchInfo_t matchedDevices[L3_MAX_MATCHES];
static int matchedDeviceCount = 0;

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

void L3_dataCnf(int err) {}

void L3_reconfigSrcIdCnf(int err) {
    if (err == 0) pc.printf("[L3] Device ID changed to %d\n", myDeviceId);
}

void L3_initChatProtocol(uint8_t myId, uint8_t likedId) {
    myDeviceId = myId;
    likedDeviceId = likedId;
    chatState = L3_CHAT_IDLE;
    currentChatPartner = 0;
    matchedDeviceCount = 0;
    pendingChatRequester = 0;
    
    L3_LLI_setDataIndFunc(L3_dataInd);
    L3_LLI_setDataCnfFunc(L3_dataCnf);
    L3_LLI_setReconfigSrcIdCnfFunc(L3_reconfigSrcIdCnf);
    
    systemTimer.start();
    beaconTimer.attach(&sendBeaconTask, 3.0);
    
    pc.printf("\n[L3] Chat Protocol Initialized\n");
    pc.printf("My ID: %d, I like ID: %d\n", myDeviceId, likedDeviceId);
}

static void sendBeaconTask(void) {
    L3_sendBeacon();
}

void L3_sendBeacon(void) {
    uint8_t beacon[3] = { L3_MSG_TYPE_BEACON, myDeviceId, likedDeviceId };
    L3_LLI_dataReq(beacon, 3, L2_BROADCAST_ID);
}

static void handleBeacon(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi) {
    if (size < 3) return;
    uint8_t senderId = data[1];
    uint8_t senderLikedId = data[2];

    if (senderLikedId == myDeviceId && rssi >= -30) {
        bool found = false;
        for (int i = 0; i < matchedDeviceCount; i++) {
            if (matchedDevices[i].deviceId == senderId) {
                matchedDevices[i].rssi = rssi;
                matchedDevices[i].lastSeen = systemTimer.read_ms();
                found = true; break;
            }
        }
        if (!found && matchedDeviceCount < L3_MAX_MATCHES) {
            matchedDevices[matchedDeviceCount++] = {senderId, rssi, systemTimer.read_ms()};
            pc.printf("\n[MATCH] Device %d likes you! (RSSI: %d)\n", senderId, rssi);
        }
    }
}

static void cleanupOldMatches(void) {
    uint32_t now = systemTimer.read_ms();
    for (int i = 0; i < matchedDeviceCount;) {
        if ((now - matchedDevices[i].lastSeen) > 10000) {
            for (int j = i; j < matchedDeviceCount - 1; j++) matchedDevices[j] = matchedDevices[j+1];
            matchedDeviceCount--;
        } else i++;
    }
}

void L3_getMatchedDevicesList(MatchInfo_t** matches, int* count) {
    cleanupOldMatches();
    *matches = matchedDevices;
    *count = matchedDeviceCount;
}

void L3_requestChat(uint8_t targetId) {
    if (chatState != L3_CHAT_IDLE) {
        pc.printf("[CHAT] Already in chat\n"); return;
    }
    uint8_t req[3] = {L3_MSG_TYPE_CHAT_REQ, myDeviceId, targetId};
    chatState = L3_CHAT_REQUESTING;
    currentChatPartner = targetId;
    L3_LLI_dataReq(req, 3, targetId);
    pc.printf("[CHAT] Requesting chat with device %d...\n", targetId);
}

static void handleChatRequest(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi) {
    if (size < 3) return;
    uint8_t requesterId = data[1];
    uint8_t targetId = data[2];
    if (targetId != myDeviceId) return;

    if (chatState == L3_CHAT_IDLE && pendingChatRequester == 0) {
        pendingChatRequester = requesterId;
        pc.printf("\n[CHAT] Chat request from device %d\n", requesterId);
        pc.printf("Accept? (1: Yes / 0: No): ");
    } else {
        uint8_t nack[3] = {L3_MSG_TYPE_CHAT_NACK, myDeviceId, requesterId};
        L3_LLI_dataReq(nack, 3, requesterId);
        pc.printf("\n[CHAT] Request from %d rejected (busy)\n", requesterId);
    }
}

void L3_processPendingResponse(char c) {
    if (pendingChatRequester == 0) return;
    if (c == '1') {
        uint8_t ack[3] = {L3_MSG_TYPE_CHAT_ACK, myDeviceId, pendingChatRequester};
        chatState = L3_CHAT_ACTIVE;
        currentChatPartner = pendingChatRequester;
        L3_LLI_dataReq(ack, 3, pendingChatRequester);
        pc.printf("\n[CHAT] Chat accepted with device %d\n", pendingChatRequester);
    } else if (c == '0') {
        uint8_t nack[3] = {L3_MSG_TYPE_CHAT_NACK, myDeviceId, pendingChatRequester};
        L3_LLI_dataReq(nack, 3, pendingChatRequester);
        pc.printf("\n[CHAT] Chat declined\n");
    } else {
        pc.printf("\n[ERROR] Invalid input. Type 1 or 0\n");
        return;
    }
    pendingChatRequester = 0;
}

static void handleChatAck(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi) {
    if (size < 3) return;
    if (chatState != L3_CHAT_REQUESTING) return;
    if (data[1] == currentChatPartner) {
        chatState = L3_CHAT_ACTIVE;
        pc.printf("\n[CHAT] Chat accepted by device %d!\n", currentChatPartner);
    }
}

static void handleChatNack(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi) {
    if (size < 3) return;
    if (chatState != L3_CHAT_REQUESTING) return;
    if (data[1] == currentChatPartner) {
        chatState = L3_CHAT_IDLE;
        currentChatPartner = 0;
        pc.printf("\n[CHAT] Device %d declined\n", data[1]);
    }
}

void L3_sendChatMessage(const char* message) {
    if (chatState != L3_CHAT_ACTIVE) {
        pc.printf("[CHAT] Not in active chat\n"); return;
    }
    uint8_t msg[100] = {L3_MSG_TYPE_CHAT_MSG, myDeviceId, currentChatPartner};
    int msgLen = strlen(message);
    if (msgLen > 96) msgLen = 96;
    memcpy(&msg[3], message, msgLen);
    L3_LLI_dataReq(msg, msgLen + 3, currentChatPartner);
}

static void handleChatMessage(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi) {
    if (size < 4 || chatState != L3_CHAT_ACTIVE) return;
    if (data[1] == currentChatPartner) {
        char message[100] = {0};
        memcpy(message, &data[3], size-3);
        pc.printf("\n[Device %d]: %s\n", srcId, message);
    }
}

void L3_endChat(void) {
    if (chatState != L3_CHAT_ACTIVE) return;
    uint8_t end[3] = {L3_MSG_TYPE_CHAT_END, myDeviceId, currentChatPartner};
    L3_LLI_dataReq(end, 3, currentChatPartner);
    pc.printf("[CHAT] Ending chat with device %d\n", currentChatPartner);
    chatState = L3_CHAT_IDLE;
    currentChatPartner = 0;
}

static void handleChatEnd(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi) {
    if (size < 3 || chatState != L3_CHAT_ACTIVE) return;
    if (data[1] == currentChatPartner) {
        pc.printf("\n[CHAT] Device %d has ended the chat\n", data[1]);
        chatState = L3_CHAT_IDLE;
        currentChatPartner = 0;
    }
}

uint8_t L3_getChatState(void) { return chatState; }
uint8_t L3_getCurrentChatPartner(void) { return currentChatPartner; }
uint8_t L3_hasPendingRequest(void) { return pendingChatRequester; }
void L3_periodicTask(void) {
    cleanupOldMatches();
}
