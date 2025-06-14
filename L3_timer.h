// 타이머 타입 정의
typedef enum {
    L3_TIMER_BROADCAST = 0,
    L3_TIMER_PROXIMITY = 1,
    L3_TIMER_WAIT_RESPONSE = 2,
    L3_TIMER_USER_RESPONSE = 3,
    L3_TIMER_MAX
} L3_timer_type_e;

// 타이머 관련 함수들
void L3_timer_init(void);
void L3_timer_startTimer(L3_timer_type_e timer_type, float timeout_sec);
void L3_timer_stopTimer(L3_timer_type_e timer_type);
uint8_t L3_timer_getTimerStatus(L3_timer_type_e timer_type);

// 기존 함수들 (호환성 유지)
void L3_timer_startTimer();
void L3_timer_stopTimer();
uint8_t L3_timer_getTimerStatus();