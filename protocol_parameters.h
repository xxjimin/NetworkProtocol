#ifndef PROTOCOL_PARAMETERS_H
#define PROTOCOL_PARAMETERS_H

// 기존 파라미터들...
#define L2_ARQ_MINWAITTIME          1
#define L2_ARQ_MAXWAITTIME          3
#define L2_ARQ_MAXRETRANSMISSION    3

// L2 브로드캐스트 ID 정의
#define L2_BROADCAST_ID             255

// 디버그 메시지 설정
#define DBGMSG_L2                   1
#define DBGMSG_L3                   1

// PC 시리얼 외부 선언
extern Serial pc;

// 디버그 매크로
#define debug(...)                  pc.printf(__VA_ARGS__)
#define debug_if(cond, ...)         if(cond) pc.printf(__VA_ARGS__)

#endif
