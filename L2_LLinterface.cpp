#include "mbed.h"
#include "PHYMAC_layer.h"
#include "L2_FSMevent.h"
#include "L2_msg.h"
#include "protocol_parameters.h"
#include "time.h"
<<<<<<< HEAD
#include <vector>
#include "L2_LLinterface.h"
// L2_LLinterface.cpp 파일 상단
static Serial pc(USBTX, USBRX);


extern uint8_t liked_id;
extern std::vector<uint8_t> tempChatCandidateList;

#define L2_LLI_MAX_PDUSIZE 50
#define L2_LLI_PKT_LOSS    0
=======

#define L2_LLI_MAX_PDUSIZE          50
#define L2_LLI_PKT_LOSS             0
>>>>>>> dgyeong

static uint8_t txType;
static uint8_t rcvdData[L2_LLI_MAX_PDUSIZE];
static uint8_t rcvdSrc;
static uint8_t rcvdSize;
static int16_t rcvdRssi;
static int8_t rcvdSnr;
static uint8_t isBroadcasted;

<<<<<<< HEAD
void L2_LLI_dataCnfFunc(int err) {
    if (txType == L2_MSG_TYPE_DATA || txType == L2_MSG_TYPE_DATA_CONT)
        L2_event_setEventFlag(L2_event_dataTxDone);
    else if (txType == L2_MSG_TYPE_ACK)
        L2_event_setEventFlag(L2_event_ackTxDone);
}

void L2_LLI_dataIndFunc(uint8_t srcId, uint8_t* dataPtr, uint8_t size, uint8_t BR) {
    if ((float)rand() / RAND_MAX > L2_LLI_PKT_LOSS) {
        memcpy(rcvdData, dataPtr, size);
=======
//interface event : DATA_CNF, TX done event
void L2_LLI_dataCnfFunc(int err) 
{
    if (txType == L2_MSG_TYPE_DATA || txType == L2_MSG_TYPE_DATA_CONT)
    {
        L2_event_setEventFlag(L2_event_dataTxDone);
    }
    else if (txType == L2_MSG_TYPE_ACK)
    {
        L2_event_setEventFlag(L2_event_ackTxDone);
    }
}

//interface event : DATA_IND, RX data has arrived
void L2_LLI_dataIndFunc(uint8_t srcId, uint8_t* dataPtr, uint8_t size, uint8_t BR)
{
    debug_if(DBGMSG_L2, "\n[L2]  --> DATA IND : src:%i, size:%i type : %i BR : %i\n", srcId, size, dataPtr[0], BR);

    if ((float)rand()/RAND_MAX > L2_LLI_PKT_LOSS)
    {
        memcpy(rcvdData, dataPtr, size*sizeof(uint8_t));
>>>>>>> dgyeong
        rcvdSrc = srcId;
        rcvdSize = size;
        rcvdSnr = phymac_getDataSnr();
        rcvdRssi = phymac_getDataRssi();
        isBroadcasted = BR;

<<<<<<< HEAD
        uint8_t msgType = dataPtr[L2_MSG_OFFSET_TYPE];

        // BCAST
        if (msgType == BCAST) {
            if (rcvdRssi >= RSSI_THRESHOLD) {
                uint8_t msg[64];
                uint8_t payload[1] = { rcvdSrc };
                uint8_t msgSize = L2_msg_encodeData(msg, payload, 0, 1, 1);
                L2_LLI_sendData(msg, msgSize, rcvdSrc);
                pc.printf("[L2] Sent QUERY_LIKE to %d (RSSI: %d)\n", rcvdSrc, rcvdRssi);
            }
        }

        // QUERY_LIKE
        else if (msgType == QUERY_LIKE) {
            uint8_t senderId = rcvdSrc;
            uint8_t response = (senderId == liked_id) ? 1 : 0;

            if (response == 1)
                pc.printf("[L2] Sent ANS_LIKE (1) to %d\n", senderId);

            uint8_t msg[64];
            uint8_t payload[1] = { response };
            uint8_t msgSize = L2_msg_encodeData(msg, payload, 0, 1, 1);
            L2_LLI_sendData(msg, msgSize, senderId);
        }

        // ANS_LIKE
        else if (msgType == ANS_LIKE) {
            uint8_t liked = rcvdData[L2_MSG_OFFSET_DATA];
            if (liked == 1) {
                tempChatCandidateList.push_back(rcvdSrc);
                pc.printf("[L2] Added to tempChatCandidateList: %d\n", rcvdSrc);
            }
        }

        if (L2_msg_checkIfData(dataPtr))
            L2_event_setEventFlag(L2_event_dataRcvd);
        else if (L2_msg_checkIfAck(dataPtr))
            L2_event_setEventFlag(L2_event_ackRcvd);
    }
}

void L2_LLI_initLowLayer(uint8_t srcId) {
=======
        //ready for ACK TX
        if (L2_msg_checkIfData(dataPtr))
        {
            L2_event_setEventFlag(L2_event_dataRcvd);
        }
        else if (L2_msg_checkIfAck(dataPtr))
        {
            L2_event_setEventFlag(L2_event_ackRcvd);
        }
    }
    else
    {
        debug_if(DBGMSG_L2, "\n\n PDU error!\n");
    }
}


void L2_LLI_initLowLayer(uint8_t srcId)
{
>>>>>>> dgyeong
    srand(time(NULL));
    phymac_init(srcId, L2_LLI_dataCnfFunc, L2_LLI_dataIndFunc);
}

<<<<<<< HEAD
void L2_LLI_sendData(uint8_t* msg, uint8_t size, uint8_t dest) {
=======



//TX function
void L2_LLI_sendData(uint8_t* msg, uint8_t size, uint8_t dest)
{
>>>>>>> dgyeong
    phymac_dataReq(msg, size, dest);
    txType = msg[L2_MSG_OFFSET_TYPE];
}

<<<<<<< HEAD
int L2_LLI_configSrcId(uint8_t srcId) {
    int res = phymac_configSrcId(srcId);
    if (res != PHYMAC_ERR_NONE)
        pc.printf("[L2] Failed to config Src ID at PHY (cause : %i)\n", res);
    return res;
}

uint8_t L2_LLI_getSrcId() { return rcvdSrc; }
uint8_t* L2_LLI_getRcvdDataPtr() { return rcvdData; }
uint8_t L2_LLI_getSize() { return rcvdSize; }
int16_t L2_LLI_getRssi(void) { return rcvdRssi; }
int8_t L2_LLI_getSnr(void) { return rcvdSnr; }
uint8_t L2_LLI_getIsBroadcasted(void) { return isBroadcasted; }
=======

int L2_LLI_configSrcId(uint8_t srcId)
{
    int res;
    if ( (res=phymac_configSrcId(srcId)) != PHYMAC_ERR_NONE)
        debug("[L2] Failed to config Src ID at PHY (cause : %i)\n", res);

    return res;
}

//GET functions
uint8_t L2_LLI_getSrcId()
{
    return rcvdSrc;
}

uint8_t* L2_LLI_getRcvdDataPtr()
{
    return rcvdData;
}

uint8_t L2_LLI_getSize()
{
    return rcvdSize;
}


int16_t L2_LLI_getRssi(void)
{
    return rcvdRssi;
}

int8_t L2_LLI_getSnr(void)
{
    return rcvdSnr;
}

uint8_t L2_LLI_getIsBroadcasted(void)
{
    return isBroadcasted;
}
>>>>>>> dgyeong
