#include "mbed.h"
#include "L3_FSMevent.h"
#include "L3_timer.h"
#include "protocol_parameters.h"

// 다중 타이머 배열
static Timeout timers[L3_TIMER_MAX];
static uint8_t timerStatus[L3_TIMER_MAX] = {0};

// 타이머 핸들러들
void L3_timer_broadcastHandler(void) {
    timerStatus[L3_TIMER_BROADCAST] = 0;
    L3_event_setEventFlag(L3_event_broadcastTimer);
}

void L3_timer_proximityHandler(void) {
    timerStatus[L3_TIMER_PROXIMITY] = 0;
    L3_event_setEventFlag(L3_event_proximityTimer);
}

void L3_timer_waitResponseHandler(void) {
    timerStatus[L3_TIMER_WAIT_RESPONSE] = 0;
    L3_event_setEventFlag(L3_event_waitResponseTimer);
}

void L3_timer_userResponseHandler(void) {
    timerStatus[L3_TIMER_USER_RESPONSE] = 0;
    // 사용자 응답 타임아웃은 별도 처리 (자동 거절)
}

// 타이머 핸들러 함수 포인터 배열
static void (*timer_handlers[L3_TIMER_MAX])(void) = {
    L3_timer_broadcastHandler,
    L3_timer_proximityHandler,
    L3_timer_waitResponseHandler,
    L3_timer_userResponseHandler
};

void L3_timer_init(void) {
    for (int i = 0; i < L3_TIMER_MAX; i++) {
        timerStatus[i] = 0;
    }
}

void L3_timer_startTimer(L3_timer_type_e timer_type, float timeout_sec) {
    if (timer_type >= L3_TIMER_MAX) return;
    
    timers[timer_type].attach(timer_handlers[timer_type], timeout_sec);
    timerStatus[timer_type] = 1;
}

void L3_timer_stopTimer(L3_timer_type_e timer_type) {
    if (timer_type >= L3_TIMER_MAX) return;
    
    timers[timer_type].detach();
    timerStatus[timer_type] = 0;
}

uint8_t L3_timer_getTimerStatus(L3_timer_type_e timer_type) {
    if (timer_type >= L3_TIMER_MAX) return 0;
    return timerStatus[timer_type];
}

// 기존 함수들 (호환성 유지)
static Timeout legacy_timer;
static uint8_t legacy_timerStatus = 0;

void L3_timer_timeoutHandler(void) {
    legacy_timerStatus = 0;
}

void L3_timer_startTimer() {
    uint8_t waitTime = 1;
    legacy_timer.attach(L3_timer_timeoutHandler, waitTime);
    legacy_timerStatus = 1;
}

void L3_timer_stopTimer() {
    legacy_timer.detach();
    legacy_timerStatus = 0;
}

uint8_t L3_timer_getTimerStatus() {
    return legacy_timerStatus;
}
