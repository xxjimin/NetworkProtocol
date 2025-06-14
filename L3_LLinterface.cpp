#include "mbed.h"
#include "L3_FSMevent.h"
#include "L3_msg.h"
#include "protocol_parameters.h"
#include "time.h"

#include <algorithm>
#include <string>
#include <vector>

static uint8_t rcvdMsg[L3_MAXDATASIZE];
static uint8_t rcvdSize;
static int16_t rcvdRssi;
static int8_t rcvdSnr;
static uint8_t rcvdSrcId;

// Downward primitives
// TX function
void (*L3_LLI_dataReqFunc)(uint8_t* msg, uint8_t size, uint8_t destId);
void (*L3_LLI_reconfigSrcIdReqFunc)(uint8_t myId);

// Externs from main or FSM
extern enum ChatState { IDLE, WAITING_FOR_ANSWER, CONNECTED };
extern ChatState chat_state;
extern int connected_node;
extern void reset_chat_state();
extern std::vector<int> liked_me_list;

void L3_LLI_dataInd(uint8_t* dataPtr, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi)
{
    if (strncmp((char*)dataPtr, "QUERY_LIKE", size) == 0) {
        if (chat_state == CONNECTED) {
            printf("[L3] 현재 채팅 중입니다. Node %d의 요청을 거절합니다\n", srcId);
            return;
        }
        if (std::find(liked_me_list.begin(), liked_me_list.end(), srcId) == liked_me_list.end()) {
            liked_me_list.push_back(srcId);
            printf("[L3] QUERY_LIKE 수신: Node %d -> liked_me_list에 추가됨\n", srcId);
        }
    }
    else if (strncmp((char*)dataPtr, "DISCONNECT", size) == 0) {
        printf("[L3] DISCONNECT 수신: Node %d와의 채팅 종료 알림\n", srcId);
        reset_chat_state();
    }
    else if (strncmp((char*)dataPtr, "ANS_LIKE", size) == 0) {
        printf("[L3] ANS_LIKE 수신: Node %d가 채팅 수락함\n", srcId);
        chat_state = CONNECTED;
        connected_node = srcId;
    }
}

void L3_LLI_dataCnf(uint8_t res)
{
    debug_if(DBGMSG_L3, "\\n --> DATA CNF : res : %i\\n", res);
    L3_event_setEventFlag(L3_event_dataSendCnf);
}

void L3_LLI_reconfigSrcIdCnf(uint8_t res)
{
    debug_if(DBGMSG_L3, "\\n --> RECONFIG SRCID CNF : res : %i\\n", res);
    L3_event_setEventFlag(L3_event_recfgSrcIdCnf);
}

uint8_t* L3_LLI_getMsgPtr()
{
    return rcvdMsg;
}

uint8_t L3_LLI_getSize()
{
    return rcvdSize;
}

uint8_t L3_LLI_getSrcId()
{
    return rcvdSrcId;
}

void L3_LLI_setDataReqFunc(void (*funcPtr)(uint8_t*, uint8_t, uint8_t))
{
    L3_LLI_dataReqFunc = funcPtr;
}

void L3_LLI_setReconfigSrcIdReqFunc(void (*funcPtr)(uint8_t))
{
    L3_LLI_reconfigSrcIdReqFunc = funcPtr;
}