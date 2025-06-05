#include "L3_FSMevent.h"
#include "L3_msg.h"
#include "L3_timer.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"
#include "mbed.h"
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
        }
    }
}

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
