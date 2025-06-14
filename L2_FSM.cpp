// [중략] 기존 코드 ...

// 예: L2_FSMrun 내 seqNum 검사 코드 수정
#ifndef DISABLE_ARQ
if (brflag == 0 && L2_msg_getType(dataPtr) != L2_MSG_TYPE_QUERY_LIKE && L2_msg_getType(dataPtr) != L2_MSG_TYPE_ANS_LIKE) {
    if (seqNum != L2_msg_getSeq(dataPtr)) {
        debug("[L3][WARNING] Invalid PDU SN (%i) while (%i) is required! discarding it...
",
              L2_msg_getSeq(dataPtr), seqNum);
        break;
    }
}
#endif

// [중략] 이후 코드 유지