#include "mbed.h"
#include "L3_LLinterface.h"

// L3 콜백 함수 포인터
static L3_dataIndFunc_t dataIndFunc = NULL;
static L3_dataCnfFunc_t dataCnfFunc = NULL;
static L3_reconfigSrcIdCnfFunc_t reconfigSrcIdCnfFunc = NULL;

// L2 콜백 함수 포인터
static void (*L2_dataReqFunc)(uint8_t*, uint8_t, uint8_t) = NULL;
static void (*L2_reconfigSrcIdReqFunc)(uint8_t) = NULL;

// L3 초기화
void L3_LLI_initLowLayer(void) {
    // 초기화 작업
}

// L3 콜백 함수 설정
void L3_LLI_setDataIndFunc(L3_dataIndFunc_t func) {
    dataIndFunc = func;
}

void L3_LLI_setDataCnfFunc(L3_dataCnfFunc_t func) {
    dataCnfFunc = func;
}

void L3_LLI_setReconfigSrcIdCnfFunc(L3_reconfigSrcIdCnfFunc_t func) {
    reconfigSrcIdCnfFunc = func;
}

// L2 콜백 함수 설정 (L2에서 호출)
void L3_LLI_setDataReqFunc(void (*func)(uint8_t*, uint8_t, uint8_t)) {
    L2_dataReqFunc = func;
}

void L3_LLI_setReconfigSrcIdReqFunc(void (*func)(uint8_t)) {
    L2_reconfigSrcIdReqFunc = func;
}

// L2로 데이터 전송 요청
void L3_LLI_dataReq(uint8_t* data, uint8_t size, uint8_t destId) {
    if (L2_dataReqFunc != NULL) {
        L2_dataReqFunc(data, size, destId);
    }
}

// L2로 재설정 요청
void L3_LLI_reconfigSrcIdReq(uint8_t myId) {
    if (L2_reconfigSrcIdReqFunc != NULL) {
        L2_reconfigSrcIdReqFunc(myId);
    }
}

// L2에서 호출되는 함수들
void L3_LLI_dataInd(uint8_t* data, uint8_t srcId, uint8_t size, int8_t snr, int16_t rssi) {
    if (dataIndFunc != NULL) {
        dataIndFunc(data, srcId, size, snr, rssi);
    }
}

void L3_LLI_dataCnf(int err) {
    if (dataCnfFunc != NULL) {
        dataCnfFunc(err);
    }
}

void L3_LLI_reconfigSrcIdCnf(int err) {
    if (reconfigSrcIdCnfFunc != NULL) {
        reconfigSrcIdCnfFunc(err);
    }
}