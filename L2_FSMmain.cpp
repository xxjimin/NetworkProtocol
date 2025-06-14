#include "L2_FSMevent.h"
#include "L2_msg.h"
#include "L2_timer.h"
#include "L2_LLinterface.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"

#define L2STATE_IDLE              0
#define L2STATE_TX                1
#ifndef DISABLE_ARQ
#define L2STATE_ACK               2
#endif

#define SDUBUFFER_SIZE              1024
#include <vector>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

static uint8_t main_state = L2STATE_IDLE;
static uint8_t prev_state = main_state;

static uint8_t myL2ID=1;
static uint8_t destL2ID=0;

static uint8_t sduBuffer[SDUBUFFER_SIZE];
static uint8_t sduBufferSize;

static uint8_t arqPdu[200];
static uint8_t sduIn[200];
static uint8_t pduSize;
static uint8_t sduLen;

static uint8_t pduBuffer[SDUBUFFER_SIZE];
static uint8_t pduBufferSize;

static uint8_t seqNum = 0;
#ifndef DISABLE_ARQ
static uint8_t retxCnt = 0;
static uint8_t arqAck[5];
#define L2_BROADCAST_ID             255
#endif
static uint8_t reqestedId=0;
uint8_t liked_id = 0;  // 사용자가 설정할 수 있게 초기값은 0 (미설정 상태)


static uint32_t last_broadcast_time = 0;
const uint32_t broadcast_interval = 5000;
std::vector<uint8_t> tempChatCandidateList;

void L2_setLikedId(uint8_t id) { liked_id = id; }

static uint8_t L2_validityCheck_ID(void) {
    if (myL2ID == destL2ID) {
        debug("[WARNING] myID and destination ID is the same! my:%i, dest:%i\n", myL2ID, destL2ID);
        return 1;
    }
    return 0;
}

uint8_t L2_configDestId(uint8_t destId) {
    if (L2_validityCheck_ID() == 1) {
        debug("[L2] Failed to config dest to ID %i\n", destId);
        return 1;
    }
    destL2ID = destId;
    return 0;
}

int L2_pullSduBuffer(uint8_t size) {
    int res;
    if (size > sduBufferSize) {
        debug_if(DBGMSG_L2, "[L2][WARNING] sdu buffer size (%i) is less than request size (%i), truncating...\n", sduBufferSize, size);
        size = sduBufferSize;
    }
    memcpy(sduIn, sduBuffer, size);
    sduLen = size;
    sduBufferSize -= size;
    if (sduBufferSize > 0) {
        memcpy(sduBuffer, sduBuffer+size, sduBufferSize);
        res = 1;
    } else res = 0;
    return res;
}

void L2_LLI_handleDataReq(uint8_t* sdu, uint8_t len, uint8_t destId) {
    if (L2_configDestId(destId) == 1 && L2_event_checkEventFlag(L2_event_dataToSendBuffer)) {
        debug_if(DBGMSG_L2, "[L2] Failed to handle DATA_REQ (dest ID invalid or TX in progress)\n");
        return;
    }
    if (len < L2_MSG_MAXDATASIZE) {
        memcpy(sduIn, sdu, len);
        sduLen = len;
    } else {
        memcpy(sduBuffer, sdu, len);
        sduBufferSize = len;
        L2_pullSduBuffer(L2_MSG_MAXDATASIZE);
        L2_event_setEventFlag(L2_event_dataToSendBuffer);
    }
    L2_event_setEventFlag(L2_event_dataToSend);
}

void L2_LLI_reconfigSrcId(uint8_t myId) {
    reqestedId = myId;
    L2_event_setEventFlag(L2_event_reconfigSrcId);
}

void L2_initFSM(uint8_t myId) {
    if (myId == 0) {
        printf("자신의 L2 ID를 입력하세요 (1~254): ");

        int inputId;
        scanf("%d", &inputId);
        if (inputId < 1 || inputId > 254) {
            printf("잘못된 ID입니다. 기본값 1로 설정합니다.\n");
            myL2ID = 1;
        } else {
            myL2ID = (uint8_t)inputId;
        }
    } else {
        myL2ID = myId;
    }

    destL2ID = 0;
    L2_event_clearAllEventFlag();
    L2_validityCheck_ID();
    L2_LLI_initLowLayer(myL2ID);
    L3_LLI_setDataReqFunc(L2_LLI_handleDataReq);
    L3_LLI_setReconfigSrcIdReqFunc(L2_LLI_reconfigSrcId);
    printf("[L2] 내 ID는 %d 입니다.\n", myL2ID);
}


int L2_aggregateData(uint8_t* dataPtr, uint8_t srcId, uint8_t size, uint8_t brflag, uint8_t flag_end) {
    memcpy(pduBuffer+pduBufferSize,L2_msg_getWord(dataPtr), size);
    pduBufferSize+=size-L2_MSG_OFFSET_DATA;
    debug_if(DBGMSG_L2, "[L2] Aggregation PDU : size : %i end : %i\n", pduBufferSize, flag_end);
    if (brflag == 1 || flag_end == 1) {
        L3_LLI_dataInd(pduBuffer, srcId, pduBufferSize, L2_LLI_getSnr(), L2_LLI_getRssi());
        pduBufferSize = 0;
        return 0;
    }
    return 1;
}

// Chat FSM 추가 영역
enum ChatState { IDLE, WAITING_FOR_ANSWER, CONNECTED };
ChatState chat_state = IDLE;
int connected_node = -1;
std::vector<int> liked_me_list;

void send_chat_request(int dest) {
    if (chat_state != IDLE) {
        printf("현재 채팅 중이므로 요청할 수 없습니다.\n");
        return;
    }
    printf("채팅 요청을 보냅니다: Node %d\n", dest);
    chat_state = WAITING_FOR_ANSWER;
    connected_node = dest;
}

void receive_chat_response(int from, bool accepted) {
    if (accepted) {
        printf("채팅 연결됨: Node %d\n", from);
        chat_state = CONNECTED;
    } else {
        printf("상대가 채팅을 거절했습니다.\n");
        chat_state = IDLE;
        connected_node = -1;
    }
}

void disconnect_chat() {
    if (chat_state != IDLE) {
        const char* msg = "DISCONNECT";
        L2_LLI_handleDataReq((uint8_t*)msg, strlen(msg), connected_node);
    }

    // 🔥 반드시 상태 초기화
    chat_state = IDLE;
    connected_node = -1;
}


void show_liked_me_list_and_choose() {
    if (liked_me_list.empty()) {
        printf("당신을 좋아하는 사람이 없습니다.\n");
        return;
    }
    printf("당신을 좋아하는 사람 목록:\n");
    for (size_t i = 0; i < liked_me_list.size(); ++i) {
        printf("%d. Node %d\n", (int)i+1, liked_me_list[i]);
    }
    int sel;
    printf("번호를 선택하세요: ");
    scanf("%d", &sel);
    if (sel >= 1 && static_cast<size_t>(sel) <= liked_me_list.size()) {
        send_chat_request(liked_me_list[sel - 1]);
    } else {
        printf("잘못된 선택입니다.\n");
    }
}


void reset_chat_state() {
    chat_state = IDLE;
    connected_node = -1;
    printf("[FSM] 채팅 상태를 IDLE로 초기화했습니다.\n");
}
