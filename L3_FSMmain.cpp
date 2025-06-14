#include "L3_FSMevent.h"
#include "L3_msg.h"
#include "L3_timer.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"
#include "mbed.h"
<<<<<<< HEAD
#include "L2_LLinterface.h"
#include "L2_msg.h"

// FSM 상태 정의
#define L3STATE_IDLE 0

// 상태 변수
static uint8_t main_state = L3STATE_IDLE;
static uint8_t prev_state = main_state;

// 입력 버퍼
static char chat_input_buffer[1030];
static uint8_t wordLen = 0;

// 시리얼 포트
static Serial pc(USBTX, USBRX);
static Timeout bcastTimer;
static uint8_t myDestId;

// ===== BCAST 전송 함수 =====
void sendPeriodicBcast() {
    uint8_t msg[64];
    uint8_t payload[1] = { L2_LLI_getSrcId() };
    uint8_t msgSize = L2_msg_encodeData(msg, payload, 0, 1, 1);
    L2_LLI_sendData(msg, msgSize, 255);  // Broadcast
    pc.printf("\n[BCAST] Sent BCAST to all\n");

    bcastTimer.attach(&sendPeriodicBcast, 15.0f);  // 15초마다 반복
}

// ===== 사용자 입력 처리 함수 =====
static void L3service_processInputWord(void) {
    char c = pc.getc();

    if (!L3_event_checkEventFlag(L3_event_dataToSend)) {
        if (c == '\n' || c == '\r') {
            chat_input_buffer[wordLen] = '\0';
            L3_event_setEventFlag(L3_event_dataToSend);
        } else {
            if (wordLen < sizeof(chat_input_buffer) - 1) {
                chat_input_buffer[wordLen++] = c;
            } else {
                chat_input_buffer[wordLen] = '\0';
                L3_event_setEventFlag(L3_event_dataToSend);
                pc.printf("\n[WARN] 입력 초과로 자동 전송됨: %s\n", chat_input_buffer);
                wordLen = 0;
            }
=======
#include <vector>
#include <cstring>

#define L3STATE_IDLE 0
#define L3STATE_CONNECTION 1
#define L3STATE_DISCONNECTION 2
#define L3STATE_MESSAGING 3

#define L3_MAX_RETRY_COUNT 3
#define L3_RSSI_THRESHOLD -80
#define L3_PROXIMITY_CHECK_INTERVAL 5.0f
#define L3_BROADCAST_INTERVAL 3.0f
#define L3_WAIT_RESPONSE_TIMEOUT 10.0f
#define L3_USER_RESPONSE_TIMEOUT 30.0f

static uint8_t main_state = L3STATE_IDLE;
static uint8_t prev_state = L3STATE_IDLE;
static uint8_t is_chatting = 0;
static uint32_t current_chat_peer = 0;
static uint32_t liked_id = 0;
static uint32_t my_id = 1;
static uint32_t next_target = 0;
static uint32_t pending_chat_request_id = 0;
static int retry_count = 0;

static uint32_t previousChatCandidateList[10];
static uint8_t prev_candidate_count = 0;
static uint32_t currentChatCandidateList[10];
static uint8_t curr_candidate_count = 0;
static uint32_t tempChatCandidateList[10];
static uint8_t temp_candidate_count = 0;

static char chat_input_buffer[L3_MAXDATASIZE];
static uint8_t chat_input_len = 0;

static uint8_t received_msg[L3_PDU_MAXSIZE];
static uint8_t received_msg_len = 0;
static uint8_t received_src_id = 0;
static int16_t received_rssi = 0;
static uint8_t msg_available = 0;

static L3_dataReqFunc_t dataReqFunc = NULL;
static Serial pc(USBTX, USBRX);

static void L3_sendPDU(uint8_t type, uint32_t target_id, const void* data, uint8_t data_len);
static void L3_processBCAST(const uint8_t* pdu_data, uint8_t size, int16_t rssi);
static void L3_processQUERY_LIKE(const uint8_t* pdu_data, uint8_t size);
static void L3_processANS_LIKE(const uint8_t* pdu_data, uint8_t size);
static void L3_processCHAT_REQ(const uint8_t* pdu_data, uint8_t size);
static void L3_processCHAT_ACK(const uint8_t* pdu_data, uint8_t size);
static void L3_processCHAT_DEC(const uint8_t* pdu_data, uint8_t size);
static void L3_processCHAT_DATA(const uint8_t* pdu_data, uint8_t size);
static void L3_processCHAT_END(const uint8_t* pdu_data, uint8_t size);
static void L3_updateChatCandidateList(void);
static void L3_transitionToState(uint8_t new_state);
static uint8_t L3_isInCandidateList(uint32_t id);
static void L3_addToCandidateList(uint32_t id);

void L3_LLI_dataInd(uint8_t* sdu, uint8_t srcId, uint8_t len, int8_t snr, int16_t rssi) {
    if (len <= L3_PDU_MAXSIZE) {
        memcpy(received_msg, sdu, len);
        received_msg_len = len;
        received_src_id = srcId;
        received_rssi = rssi;
        msg_available = 1;
        L3_event_setEventFlag(L3_event_msgRcvd);
    }
}

void L3_LLI_dataCnf(uint8_t isSuccess) {
    if (isSuccess) {
        L3_event_setEventFlag(L3_event_dataSendCnf);
    }
}

void L3_LLI_reconfigSrcIdCnf(uint8_t isSuccess) {
    if (isSuccess) {
        L3_event_setEventFlag(L3_event_recfgSrcIdCnf);
    }
}

static void L3_dataReq(uint8_t* sdu, uint8_t len, uint8_t destId) {
    if (dataReqFunc != NULL) {
        dataReqFunc(sdu, len, destId);
    }
}

static void L3service_processInputWord(void) {
    char c = pc.getc();
    static char cmd_buffer[64];
    static uint8_t cmd_len = 0;

    if (main_state == L3STATE_MESSAGING && !L3_event_checkEventFlag(L3_event_dataToSend)) {
        if (c == '\n' || c == '\r') {
            if (chat_input_len > 0) {
                chat_input_buffer[chat_input_len] = '\0';
                L3_event_setEventFlag(L3_event_dataToSend);
            }
        } else if (c == '\b' || c == 127) {
            if (chat_input_len > 0) {
                chat_input_len--;
                pc.putc('\b'); pc.putc(' '); pc.putc('\b');
            }
        } else if (chat_input_len < L3_MAXDATASIZE - 1) {
            chat_input_buffer[chat_input_len++] = c;
            pc.putc(c);
        }
    } else {
        if (c == '\n' || c == '\r') {
            if (cmd_len > 0) {
                cmd_buffer[cmd_len] = '\0';
                if (strncmp(cmd_buffer, "like ", 5) == 0) {
                    liked_id = atoi(cmd_buffer + 5);
                    pc.printf("[L3] Set liked ID to: %d\n", liked_id);
                } else if (strncmp(cmd_buffer, "chat ", 5) == 0) {
                    uint32_t target_id = atoi(cmd_buffer + 5);
                    if (main_state == L3STATE_IDLE && L3_isInCandidateList(target_id)) {
                        L3_transitionToState(L3STATE_CONNECTION);
                        retry_count = 0;
                        L3_sendPDU(L3_PDU_TYPE_CHAT_REQ, target_id, NULL, 0);
                        L3_timer_startTimer(L3_TIMER_WAIT_RESPONSE, L3_WAIT_RESPONSE_TIMEOUT);
                        pc.printf("[L3] Sending chat request to ID: %d\n", target_id);
                    } else {
                        pc.printf("[L3] Invalid target or not in IDLE state\n");
                    }
                } else if (strcmp(cmd_buffer, "list") == 0) {
                    pc.printf("[L3] Chat candidates: ");
                    for (uint8_t i = 0; i < prev_candidate_count; i++) {
                        pc.printf("%d ", previousChatCandidateList[i]);
                    }
                    pc.printf("\n");
                } else if (pending_chat_request_id != 0) {
                    if (strcmp(cmd_buffer, "1") == 0) {
                        L3_event_setEventFlag(L3_event_userResponse);
                    } else if (strcmp(cmd_buffer, "0") == 0) {
                        L3_sendPDU(L3_PDU_TYPE_CHAT_DEC, pending_chat_request_id, NULL, 0);
                        pending_chat_request_id = 0;
                        L3_timer_stopTimer(L3_TIMER_USER_RESPONSE);
                        pc.printf("[L3] Chat request declined\n");
                    } else {
                        pc.printf("[L3] Invalid input. Type 1 (accept) or 0 (decline)\n");
                    }
                } else if (strcmp(cmd_buffer, "quit") == 0) {
                    if (main_state == L3STATE_MESSAGING) {
                        L3_transitionToState(L3STATE_DISCONNECTION);
                        L3_sendPDU(L3_PDU_TYPE_CHAT_END, current_chat_peer, NULL, 0);
                        next_target = 0;
                    }
                } else {
                    pc.printf("[L3] Unknown command.\n");
                }
                cmd_len = 0;
            }
            pc.printf("> ");
        } else if (c == '\b' || c == 127) {
            if (cmd_len > 0) { cmd_len--; pc.putc('\b'); pc.putc(' '); pc.putc('\b'); }
        } else if (cmd_len < sizeof(cmd_buffer) - 1) {
            cmd_buffer[cmd_len++] = c; pc.putc(c);
>>>>>>> dgyeong
        }
    }
}

<<<<<<< HEAD
// ===== 초기화 함수 =====
void L3_initFSM(uint8_t destId) {
    myDestId = destId;
    pc.attach(&L3service_processInputWord, Serial::RxIrq);
    sendPeriodicBcast();  // 최초 한 번만 호출, 이후 자동 반복
    pc.printf("Give a word to send: ");
}

// ===== FSM 실행 함수 =====
void L3_FSMrun(void) {
    if (prev_state != main_state) {
        prev_state = main_state;
    }

    switch (main_state) {
        case L3STATE_IDLE:

            // 메시지 수신 처리
            if (L3_event_checkEventFlag(L3_event_msgRcvd)) {
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t size = L3_LLI_getSize();

                static uint32_t lastPrint = 0;
                if (clock() - lastPrint > 1000) {  // 너무 자주 출력 방지
                    pc.printf("\n-------------------------------------------------\n");
                    pc.printf("RCVD MSG: %s (length: %i)\n", dataPtr, size);
                    pc.printf("-------------------------------------------------\n");
                    lastPrint = clock();
                }

                L3_event_clearEventFlag(L3_event_msgRcvd);
            }

            // 메시지 전송
            else if (L3_event_checkEventFlag(L3_event_dataToSend)) {
                L3_LLI_dataReqFunc((uint8_t*)chat_input_buffer, strlen(chat_input_buffer), myDestId);
                pc.printf("\n[SENT] %s\n", chat_input_buffer);
                wordLen = 0;
                pc.printf("Give a word to send: ");
                L3_event_clearEventFlag(L3_event_dataToSend);
            }
            break;

        default:
            break;
    }
}
=======
void L3_initFSM(uint8_t destId) {
    my_id = destId;
    L3_timer_init();
    L3_timer_startTimer(L3_TIMER_BROADCAST, L3_BROADCAST_INTERVAL);
    L3_timer_startTimer(L3_TIMER_PROXIMITY, L3_PROXIMITY_CHECK_INTERVAL);
    L3_LLI_setDataReqFunc(L3_dataReq);
    pc.printf("=== LoRa Chat Protocol Started ===\n");
    pc.printf("My ID: %d\n", my_id);
    pc.printf("Commands: like <ID>, chat <ID>, list, quit\n");
    pc.printf("When request arrives: Type 1 (accept) or 0 (decline)\n");
    pc.printf("> ");
}

// 나머지 FSM 루프 및 프로세스 함수는 기존과 동일하게 유지


void L3_FSMrun(void) {
    if (prev_state != main_state) {
        debug_if(DBGMSG_L3, "[L3] State transition from %i to %i\n", prev_state, main_state);
        const char* state_names[] = {"IDLE", "CONNECTION", "DISCONNECTION", "MESSAGING"};
        pc.printf("[FSM] State: %s\n", state_names[main_state]);
        prev_state = main_state;
    }

    // 타이머 이벤트 처리
    if (L3_event_checkEventFlag(L3_event_broadcastTimer)) {
        L3_sendPDU(L3_PDU_TYPE_BCAST, 255, NULL, 0); // 브로드캐스트
        L3_timer_startTimer(L3_TIMER_BROADCAST, L3_BROADCAST_INTERVAL);
        L3_event_clearEventFlag(L3_event_broadcastTimer);
    }
    
    if (L3_event_checkEventFlag(L3_event_proximityTimer)) {
        L3_updateChatCandidateList();
        L3_timer_startTimer(L3_TIMER_PROXIMITY, L3_PROXIMITY_CHECK_INTERVAL);
        L3_event_clearEventFlag(L3_event_proximityTimer);
    }

    // FSM 상태별 처리
    switch (main_state) {
        case L3STATE_IDLE:
            if (L3_event_checkEventFlag(L3_event_msgRcvd) && msg_available) {
                uint8_t* dataPtr = received_msg;
                uint8_t size = received_msg_len;
                uint8_t type = L3_msg_getPDUType(dataPtr);
                
                switch (type) {
                    case L3_PDU_TYPE_BCAST:
                        L3_processBCAST(dataPtr, size, received_rssi);
                        break;
                    case L3_PDU_TYPE_QUERY_LIKE:
                        L3_processQUERY_LIKE(dataPtr, size);
                        break;
                    case L3_PDU_TYPE_ANS_LIKE:
                        L3_processANS_LIKE(dataPtr, size);
                        break;
                    case L3_PDU_TYPE_CHAT_REQ:
                        L3_processCHAT_REQ(dataPtr, size);
                        break;
                }
                msg_available = 0;
                L3_event_clearEventFlag(L3_event_msgRcvd);
            }
            break;
            
        case L3STATE_CONNECTION:
            if (L3_event_checkEventFlag(L3_event_msgRcvd) && msg_available) {
                uint8_t* dataPtr = received_msg;
                uint8_t type = L3_msg_getPDUType(dataPtr);
                
                if (type == L3_PDU_TYPE_CHAT_ACK) {
                    L3_processCHAT_ACK(dataPtr, received_msg_len);
                } else if (type == L3_PDU_TYPE_CHAT_DEC) {
                    L3_processCHAT_DEC(dataPtr, received_msg_len);
                }
                msg_available = 0;
                L3_event_clearEventFlag(L3_event_msgRcvd);
            }
            
            if (L3_event_checkEventFlag(L3_event_waitResponseTimer)) {
                if (retry_count >= L3_MAX_RETRY_COUNT) {
                    pc.printf("[L3] Chat request failed (max retries)\n");
                    L3_transitionToState(is_chatting ? L3STATE_MESSAGING : L3STATE_IDLE);
                } else {
                    retry_count++;
                    // 재시도 대상 ID는 별도 저장 필요
                    pc.printf("[L3] Retrying chat request (%d/%d)\n", retry_count, L3_MAX_RETRY_COUNT);
                    L3_timer_startTimer(L3_TIMER_WAIT_RESPONSE, L3_WAIT_RESPONSE_TIMEOUT);
                }
                L3_event_clearEventFlag(L3_event_waitResponseTimer);
            }
            break;
            
        case L3STATE_MESSAGING:
            if (L3_event_checkEventFlag(L3_event_msgRcvd) && msg_available) {
                uint8_t* dataPtr = received_msg;
                uint8_t type = L3_msg_getPDUType(dataPtr);
                
                switch (type) {
                    case L3_PDU_TYPE_CHAT_DATA:
                        L3_processCHAT_DATA(dataPtr, received_msg_len);
                        break;
                    case L3_PDU_TYPE_CHAT_END:
                        L3_processCHAT_END(dataPtr, received_msg_len);
                        break;
                    case L3_PDU_TYPE_CHAT_REQ:
                        L3_processCHAT_REQ(dataPtr, received_msg_len);
                        break;
                }
                msg_available = 0;
                L3_event_clearEventFlag(L3_event_msgRcvd);
            }
            
            if (L3_event_checkEventFlag(L3_event_dataToSend)) {
                L3_sendPDU(L3_PDU_TYPE_CHAT_DATA, current_chat_peer, chat_input_buffer, chat_input_len);
                pc.printf("[Me]: %s\n", chat_input_buffer);
                chat_input_len = 0;
                L3_event_clearEventFlag(L3_event_dataToSend);
            }
            break;
            
        case L3STATE_DISCONNECTION:
            // DISCONNECTION 상태 처리는 L3_transitionToState에서 자동 처리
            if (next_target != 0) {
                L3_transitionToState(L3STATE_CONNECTION);
                retry_count = 0;
                L3_sendPDU(L3_PDU_TYPE_CHAT_REQ, next_target, NULL, 0);
                L3_timer_startTimer(L3_TIMER_WAIT_RESPONSE, L3_WAIT_RESPONSE_TIMEOUT);
                next_target = 0;
            } else {
                is_chatting = 0;
                current_chat_peer = 0;
                L2_resetFSM(); 
                L3_transitionToState(L3STATE_IDLE);
                pc.printf("[L3] Chat ended\n> ");
            }
            break;
    }
    
    // 사용자 응답 처리
    if (L3_event_checkEventFlag(L3_event_userResponse)) {
        if (pending_chat_request_id != 0) {
            L3_timer_stopTimer(L3_TIMER_USER_RESPONSE);
            
            if (is_chatting) {
                // 기존 채팅 종료 후 새 채팅 시작
                L3_transitionToState(L3STATE_DISCONNECTION);
                L3_sendPDU(L3_PDU_TYPE_CHAT_END, current_chat_peer, NULL, 0);
                next_target = pending_chat_request_id;
            } else {
                // 새 채팅 시작
                current_chat_peer = pending_chat_request_id;
                is_chatting = 1;
                L3_sendPDU(L3_PDU_TYPE_CHAT_ACK, pending_chat_request_id, NULL, 0);
                L3_transitionToState(L3STATE_MESSAGING);
                pc.printf("[L3] Chat connected with ID: %d\n", pending_chat_request_id);
            }
            pending_chat_request_id = 0;
        }
        L3_event_clearEventFlag(L3_event_userResponse);
    }
}

// 내부 함수 구현들
static void L3_sendPDU(uint8_t type, uint32_t target_id, const void* data, uint8_t data_len) {
    uint8_t pdu_buffer[L3_PDU_MAXSIZE];
    uint8_t pdu_size = L3_msg_encodePDU(pdu_buffer, type, my_id, target_id, data, data_len);
    
    // Layer2로 데이터 전송 요청
    L3_dataReq(pdu_buffer, pdu_size, (uint8_t)target_id);
}

static void L3_processBCAST(const uint8_t* pdu_data, uint8_t size, int16_t rssi) {
    if (rssi >= L3_RSSI_THRESHOLD) {
        uint32_t sender_id = L3_msg_getSenderId(pdu_data);
        L3_sendPDU(L3_PDU_TYPE_QUERY_LIKE, sender_id, NULL, 0);
    }
}

static void L3_processQUERY_LIKE(const uint8_t* pdu_data, uint8_t size) {
    uint32_t sender_id = L3_msg_getSenderId(pdu_data);
    uint8_t liked = (sender_id == liked_id) ? 1 : 0;
    L3_sendPDU(L3_PDU_TYPE_ANS_LIKE, sender_id, &liked, 1);
}

static void L3_processANS_LIKE(const uint8_t* pdu_data, uint8_t size) {
    uint32_t sender_id = L3_msg_getSenderId(pdu_data);
    uint8_t* data_ptr = L3_msg_getDataPtr(pdu_data);
    
    if (data_ptr && data_ptr[0] == 1) { // liked == true
        L3_addToCandidateList(sender_id);
    }
}

static void L3_processCHAT_REQ(const uint8_t* pdu_data, uint8_t size) {
    uint32_t sender_id = L3_msg_getSenderId(pdu_data);
    pending_chat_request_id = sender_id;
    
    pc.printf("[L3] Chat request from ID: %d\n", sender_id);
    pc.printf("Type 'accept' or 'decline'\n> ");
    
    L3_timer_startTimer(L3_TIMER_USER_RESPONSE, L3_USER_RESPONSE_TIMEOUT);
}

static void L3_processCHAT_ACK(const uint8_t* pdu_data, uint8_t size) {
    if (main_state == L3STATE_CONNECTION) {
        uint32_t sender_id = L3_msg_getSenderId(pdu_data);
        L3_timer_stopTimer(L3_TIMER_WAIT_RESPONSE);
        
        if (is_chatting) {
            // 기존 채팅 종료 후 새 채팅 시작
            L3_transitionToState(L3STATE_DISCONNECTION);
            L3_sendPDU(L3_PDU_TYPE_CHAT_END, current_chat_peer, NULL, 0);
            next_target = sender_id;
        } else {
            // 새 채팅 시작
            current_chat_peer = sender_id;
            is_chatting = 1;
            L3_transitionToState(L3STATE_MESSAGING);
            pc.printf("[L3] Chat connected with ID: %d\n", sender_id);
            pc.printf("Type messages (quit to end):\n");
        }
    }
}

static void L3_processCHAT_DEC(const uint8_t* pdu_data, uint8_t size) {
    if (main_state == L3STATE_CONNECTION) {
        L3_timer_stopTimer(L3_TIMER_WAIT_RESPONSE);
        L3_transitionToState(is_chatting ? L3STATE_MESSAGING : L3STATE_IDLE);
        pc.printf("[L3] Chat request declined\n> ");
    }
}

static void L3_processCHAT_DATA(const uint8_t* pdu_data, uint8_t size) {
    if (main_state == L3STATE_MESSAGING) {
        uint32_t sender_id = L3_msg_getSenderId(pdu_data);
        if (sender_id == current_chat_peer) {
            uint8_t* message = L3_msg_getDataPtr(pdu_data);
            pc.printf("[%d]: %s\n", sender_id, message);
        }
    }
}

static void L3_processCHAT_END(const uint8_t* pdu_data, uint8_t size) {
    uint32_t sender_id = L3_msg_getSenderId(pdu_data);
    if (sender_id == current_chat_peer) {
        L3_transitionToState(L3STATE_DISCONNECTION);
        next_target = 0; // 종료 목적
        L2_resetFSM();
        pc.printf("[L3] Chat ended by peer\n");
    }
}

static void L3_updateChatCandidateList(void) {
    // 임시 리스트를 현재 리스트로 복사
    for (uint8_t i = 0; i < temp_candidate_count; i++) {
        currentChatCandidateList[i] = tempChatCandidateList[i];
    }
    curr_candidate_count = temp_candidate_count;
    temp_candidate_count = 0;
    
    // 이전 리스트와 비교하여 변경사항 확인
    uint8_t changed = 0;
    if (curr_candidate_count != prev_candidate_count) {
        changed = 1;
    } else {
        for (uint8_t i = 0; i < curr_candidate_count; i++) {
            uint8_t found = 0;
            for (uint8_t j = 0; j < prev_candidate_count; j++) {
                if (currentChatCandidateList[i] == previousChatCandidateList[j]) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                changed = 1;
                break;
            }
        }
    }
    
    if (changed) {
        // 이전 리스트 업데이트
        for (uint8_t i = 0; i < curr_candidate_count; i++) {
            previousChatCandidateList[i] = currentChatCandidateList[i];
        }
        prev_candidate_count = curr_candidate_count;
        
        pc.printf("[L3] Chat candidates updated: ");
        for (uint8_t i = 0; i < prev_candidate_count; i++) {
            pc.printf("%d ", previousChatCandidateList[i]);
        }
        pc.printf("\n> ");
    }
}

static void L3_transitionToState(uint8_t new_state) {
    main_state = new_state;
}

static uint8_t L3_isInCandidateList(uint32_t id) {
    for (uint8_t i = 0; i < prev_candidate_count; i++) {
        if (previousChatCandidateList[i] == id) {
            return 1;
        }
    }
    return 0;
}

static void L3_addToCandidateList(uint32_t id) {
    // 중복 체크
    for (uint8_t i = 0; i < temp_candidate_count; i++) {
        if (tempChatCandidateList[i] == id) {
            return; // 이미 존재
        }
    }
    
    // 공간이 있으면 추가
    if (temp_candidate_count < 10) {
        tempChatCandidateList[temp_candidate_count++] = id;
    }
}
>>>>>>> dgyeong
