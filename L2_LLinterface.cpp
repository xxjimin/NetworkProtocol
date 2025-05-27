#include "mbed.h"
#include "PHYMAC_layer.h"
#include "L2_FSMevent.h"
#include "L2_msg.h"
#include "protocol_parameters.h"
#include "time.h"
#include "protocol_parameters.h"  // ← 여기 넣어야 BCAST, QUERY_LIKE 등 인식
#include <vector>  // 🔥 std::vector 쓰려면 이게 필요합니다
#include "L2_LLinterface.h" // 🔥 L2_LLI_sendData 선언 포함돼야 함
#include "L2_msg.h"         // 🔥 L2_msg_encodeData 등 사용 가능하게 함
#include "protocol_parameters.h"  // 🔥 BCAST, QUERY_LIKE, ANS_LIKE 정의된 곳
extern uint8_t liked_id;
extern std::vector<uint8_t> tempChatCandidateList;

#define L2_LLI_MAX_PDUSIZE          50
#define L2_LLI_PKT_LOSS             0

static uint8_t txType;
static uint8_t rcvdData[L2_LLI_MAX_PDUSIZE];
static uint8_t rcvdSrc;
static uint8_t rcvdSize;
static int16_t rcvdRssi;
static int8_t rcvdSnr;
static uint8_t isBroadcasted;

extern uint8_t liked_id;
extern std::vector<uint8_t> tempChatCandidateList;


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

void L2_LLI_dataIndFunc(uint8_t srcId, uint8_t* dataPtr, uint8_t size, uint8_t BR)
{
    debug_if(DBGMSG_L2, "\n[L2]  --> DATA IND : src:%i, size:%i type : %i BR : %i\n", srcId, size, dataPtr[0], BR);

    if ((float)rand() / RAND_MAX > L2_LLI_PKT_LOSS)
    {
        // 수신 데이터 저장
        memcpy(rcvdData, dataPtr, size * sizeof(uint8_t));
        rcvdSrc = srcId;
        rcvdSize = size;
        rcvdSnr = phymac_getDataSnr();
        rcvdRssi = phymac_getDataRssi();
        isBroadcasted = BR;

        uint8_t msgType = dataPtr[L2_MSG_OFFSET_TYPE];

        // ====== BCAST 수신 시 RSSI 비교 및 QUERY_LIKE 전송 ======
        if (msgType == BCAST)
        {
            debug("[L2] Received BCAST, RSSI: %d dBm\n", rcvdRssi);

            if (rcvdRssi >= RSSI_THRESHOLD)
            {
                uint8_t msg[64];
                uint8_t payload[1] = {rcvdSrc};  // 수신자가 보낸 ID로 응답
                uint8_t msgSize = L2_msg_encodeData(msg, payload, 0, 1, 1);
                L2_LLI_sendData(msg, msgSize, rcvdSrc);  // QUERY_LIKE 전송
                debug("[L2] Sent QUERY_LIKE to %i\n", rcvdSrc);
            }
        }

        // ====== QUERY_LIKE 수신 시 liked_id 비교 후 ANS_LIKE 전송 ======
        else if (msgType == QUERY_LIKE)
        {
            uint8_t senderId = rcvdSrc;
            uint8_t response = (senderId == liked_id) ? 1 : 0;

            uint8_t msg[64];
            uint8_t payload[1] = {response};
            uint8_t size = L2_msg_encodeData(msg, payload, 0, 1, 1);
            L2_LLI_sendData(msg, size, senderId);
            debug("[L2] Sent ANS_LIKE (%i) to %i\n", response, senderId);
        }

        // ====== ANS_LIKE 수신 시 후보 리스트에 추가 ======
        else if (msgType == ANS_LIKE)
        {
            uint8_t liked = rcvdData[L2_MSG_OFFSET_DATA];
            if (liked == 1)
            {
                tempChatCandidateList.push_back(rcvdSrc);
                debug("[L2] Added to tempChatCandidateList: %i\n", rcvdSrc);
            }
        }

        // ====== 기본 데이터 or ACK 수신 이벤트 처리 ======
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
    srand(time(NULL));
    phymac_init(srcId, L2_LLI_dataCnfFunc, L2_LLI_dataIndFunc);
}




//TX function
void L2_LLI_sendData(uint8_t* msg, uint8_t size, uint8_t dest)
{
    phymac_dataReq(msg, size, dest);
    txType = msg[L2_MSG_OFFSET_TYPE];
}


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