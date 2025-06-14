#ifndef L3_LLINTERFACE_H
#define L3_LLINTERFACE_H

// L3 콜백 함수 타입 정의
typedef void (*L3_dataIndFunc_t)(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi);
typedef void (*L3_dataCnfFunc_t)(int err);
typedef void (*L3_reconfigSrcIdCnfFunc_t)(int err);

// L3 인터페이스 함수
void L3_LLI_initLowLayer(void);
void L3_LLI_setDataIndFunc(L3_dataIndFunc_t func);
void L3_LLI_setDataCnfFunc(L3_dataCnfFunc_t func);
void L3_LLI_setReconfigSrcIdCnfFunc(L3_reconfigSrcIdCnfFunc_t func);

// L2로 데이터 전송
void L3_LLI_dataReq(uint8_t* data, uint8_t size, uint8_t destId);

// L2로 재설정 요청
void L3_LLI_reconfigSrcIdReq(uint8_t myId);

// L2에서 호출되는 함수들 (이미 L2 코드에 선언되어 있음)
void L3_LLI_dataInd(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi);
void L3_LLI_dataCnf(int err);
void L3_LLI_reconfigSrcIdCnf(int err);
void L3_LLI_setDataReqFunc(void (*func)(uint8_t*, uint8_t, uint8_t));
void L3_LLI_setReconfigSrcIdReqFunc(void (*func)(uint8_t));

#endif
