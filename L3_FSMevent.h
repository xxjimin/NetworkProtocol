typedef enum L3_event
{
    L3_event_msgRcvd = 2,           // 기존: 메시지 수신
    L3_event_dataToSend = 4,        // 기존: 데이터 전송 요청
    L3_event_dataSendCnf = 5,       // 기존: 데이터 전송 확인
    L3_event_recfgSrcIdCnf = 6,     // 기존: 소스 ID 재설정 확인
    
    // 새로 추가: 채팅 프로토콜용 이벤트들
    L3_event_broadcastTimer = 7,    // 브로드캐스트 타이머 만료
    L3_event_proximityTimer = 8,    // 근접성 체크 타이머 만료
    L3_event_waitResponseTimer = 9, // 채팅 응답 대기 타이머 만료
    L3_event_userResponse = 10,     // 사용자 응답 (버튼/키보드)
    L3_event_userInput = 11,        // 사용자 입력 (채팅 메시지 등)
    L3_event_chatRequest = 12       // 채팅 요청 수신
} L3_event_e;

void L3_event_setEventFlag(L3_event_e event);
void L3_event_clearEventFlag(L3_event_e event);
void L3_event_clearAllEventFlag(void);
int L3_event_checkEventFlag(L3_event_e event);