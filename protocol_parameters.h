#define DBGMSG_L2                       0 //debug print control
#define DBGMSG_L3                       0 //debug print control

#define L3_MAXDATASIZE                  1024


#define L2_ARQ_MAXRETRANSMISSION        10
#define L2_ARQ_MAXWAITTIME              5
#define L2_ARQ_MINWAITTIME              2

//추가
#define RSSI_THRESHOLD -90  // 단위: dBm

#define BCAST        0x01
#define QUERY_LIKE   0x02
#define ANS_LIKE     0x03
#define CHAT_REQ     0x04
#define CHAT_ACK     0x05
#define CHAT_DEC     0x06
#define CHAT_DATA    0x07
#define CHAT_END     0x08
