typedef enum L3_event {
    L3_event_msgRcvd = 2,
    L3_event_dataToSend = 4,
    L3_event_dataSendCnf = 5,
    L3_event_recfgSrcIdCnf = 6,
    L3_event_arqTimeout = 7,         // ✅ 누락되면 에러
    L3_event_chatReq = 10,
    L3_event_chatAck = 11,
    L3_event_chatDec = 12,
    L3_event_chatData = 13,
    L3_event_chatEnd = 14
} L3_event_e;



void L3_event_setEventFlag(L3_event_e event);
void L3_event_clearEventFlag(L3_event_e event);
void L3_event_clearAllEventFlag(void);
int L3_event_checkEventFlag(L3_event_e event);