// L2_FSMmain.cpp (SN check 완전 무시 적용 최종본)

#include "L2_FSMevent.h"
#include "L2_msg.h"
#include "L2_timer.h"
#include "L2_LLinterface.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"

#define L2STATE_IDLE 0
#define L2STATE_TX 1
#define SDUBUFFER_SIZE 1024

static uint8_t main_state = L2STATE_IDLE;
static uint8_t prev_state = L2STATE_IDLE;

static uint8_t myL2ID = 1;
static uint8_t destL2ID = 0;

static uint8_t sduBuffer[SDUBUFFER_SIZE];
static uint8_t sduBufferSize;

static uint8_t arqPdu[200];
static uint8_t sduIn[200];
static uint8_t pduSize;
static uint8_t sduLen;

static uint8_t pduBuffer[SDUBUFFER_SIZE];
static uint8_t pduBufferSize;

static uint8_t seqNum = 0;
static uint8_t reqestedId = 0;

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
    if (size > sduBufferSize) size = sduBufferSize;
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
    if (L2_configDestId(destId) == 1 && L2_event_checkEventFlag(L2_event_dataToSendBuffer)) return;
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
    myL2ID = myId;
    destL2ID = 0;
    L2_event_clearAllEventFlag();
    L2_validityCheck_ID();
    L2_LLI_initLowLayer(myL2ID);
    L3_LLI_setDataReqFunc(L2_LLI_handleDataReq);
    L3_LLI_setReconfigSrcIdReqFunc(L2_LLI_reconfigSrcId);
}

int L2_aggregateData(uint8_t* dataPtr, uint8_t srcId, uint8_t size, uint8_t brflag, uint8_t flag_end) {
    memcpy(pduBuffer+pduBufferSize, L2_msg_getWord(dataPtr), size);
    pduBufferSize += size - L2_MSG_OFFSET_DATA;
    if (brflag == 1 || flag_end == 1) {
        L3_LLI_dataInd(pduBuffer, srcId, pduBufferSize, L2_LLI_getSnr(), L2_LLI_getRssi());
        pduBufferSize = 0;
        return 0;
    }
    return 1;
}

void L2_FSMrun(void) {
    if (prev_state != main_state) {
        debug_if(DBGMSG_L2, "[L2] State transition from %i to %i\n", prev_state, main_state);
        prev_state = main_state;
    }

    switch (main_state) {
        case L2STATE_IDLE:
            if (L2_event_checkEventFlag(L2_event_reconfigSrcId)) {
                int res = L2_LLI_configSrcId(reqestedId);
                L3_LLI_reconfigSrcIdCnf(res==0);
                L2_event_clearEventFlag(L2_event_reconfigSrcId);
            }
            else if (L2_event_checkEventFlag(L2_event_dataRcvd)) {
                uint8_t srcId = L2_LLI_getSrcId();
                uint8_t* dataPtr = L2_LLI_getRcvdDataPtr();
                uint8_t size = L2_LLI_getSize();
                uint8_t brflag = L2_LLI_getIsBroadcasted();
                uint8_t flag_end = L2_msg_checkIfEndData(dataPtr);

                L2_aggregateData(dataPtr, srcId, size, brflag, flag_end);
                L2_event_clearEventFlag(L2_event_dataRcvd);
            }
            else if (L2_event_checkEventFlag(L2_event_dataToSend)) {
                pduSize = L2_msg_encodeData(arqPdu, sduIn, seqNum, sduLen, L2_event_checkEventFlag(L2_event_dataToSendBuffer)==0);
                L2_LLI_sendData(arqPdu, pduSize, destL2ID);
                seqNum = (seqNum + 1) % L2_MSSG_MAX_SEQNUM;
                L2_event_clearEventFlag(L2_event_dataToSend);
                main_state = L2STATE_TX;
            }
            break;

        case L2STATE_TX:
            if (L2_event_checkEventFlag(L2_event_dataTxDone)) {
                main_state = L2STATE_IDLE;
                L3_LLI_dataCnf(1);
                L2_event_clearEventFlag(L2_event_dataTxDone);
            }
            break;

        default:
            break;
    }
}

void L2_resetFSM(void) {
    destL2ID = 0;
    seqNum = 0;
    main_state = L2STATE_IDLE;
    pduBufferSize = 0;
    sduBufferSize = 0;
    printf("[L2] FSM Reset Completed.\n");
}
