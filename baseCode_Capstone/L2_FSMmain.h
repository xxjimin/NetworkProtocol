void L2_initFSM(uint8_t myId);
void L2_FSMrun(void);
void L2_setLikedId(uint8_t id);

#include <ctime>  // clock() 함수 사용

static uint32_t last_broadcast_time = 0;
const uint32_t broadcast_interval = 5000; // 5초 주기 (단위: ms)

