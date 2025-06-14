#ifndef L2_FSMEVENT_H
#define L2_FSMEVENT_H

typedef enum L2_event
{
    L2_event_dataTxDone = 0,
    L2_event_ackTxDone = 1,
    L2_event_ackRcvd = 2,
    L2_event_dataRcvd = 3,
    L2_event_dataToSend = 4,
    L2_event_arqTimeout = 5,
    L2_event_reconfigSrcId = 6,
    L2_event_dataToSendBuffer = 7,
    L2_event_rssiScan = 8,           // 추가
    L2_event_beaconBroadcast = 9     // 추가 (마지막 줄에는 콤마 없음)
} L2_event_e;

void L2_event_setEventFlag(L2_event_e event);
void L2_event_clearEventFlag(L2_event_e event);
void L2_event_clearAllEventFlag(void);
int L2_event_checkEventFlag(L2_event_e event);
void L2_resetFSM(void);

#endif
