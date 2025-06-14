#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include "L2_FSMmain.h"

extern void disconnect_chat();
extern void show_liked_me_list_and_choose();
extern void L2_initFSM(uint8_t myId);
extern std::vector<int> liked_me_list;
extern void L2_LLI_handleDataReq(uint8_t* sdu, uint8_t len, uint8_t destId);

enum ChatState { IDLE, WAITING_FOR_ANSWER, CONNECTED }; // enum 정의
extern ChatState chat_state;
extern int connected_node;
extern uint8_t liked_id; // 전역 변수는 L2_FSMmain.cpp에 존재

void show_who_likes_me() {
    if (liked_me_list.empty()) {
        printf("당신에게 채팅을 요청한 사람이 없습니다.\n");
    } else {
        printf("당신에게 채팅 요청한 사람 목록:\n");
        for (size_t i = 0; i < liked_me_list.size(); ++i) {
            printf("%d. Node %d\n", (int)i+1, liked_me_list[i]);
        }
    }
}

int main() {
    while (true) {
        int cmd;
        printf("\n명령을 선택하세요:\n");
        printf("1. 내 ID 설정하기\n");
        printf("2. 좋아하는 사람 설정 (한 번만 입력 가능)\n");
        printf("3. 좋아하는 사람에게 채팅 요청\n");
        printf("4. 좋아하는 사람 보기\n");
        printf("5. 채팅 종료\n");
        printf("6. 프로그램 종료\n");
        printf("7. 나에게 채팅 요청한 사람 보기\n");
        scanf("%d", &cmd);

        switch (cmd) {
            case 1: {
                int myId;
                printf("당신의 L2 ID를 입력하세요 (1~254): ");
                scanf("%d", &myId);
                if (myId < 1 || myId > 254) {
                    printf("잘못된 ID입니다. 1로 초기화합니다.\n");
                    myId = 1;
                }
                L2_initFSM((uint8_t)myId);
                break;
            }
            case 2: {
                if (liked_id != 0) {
                    printf("이미 좋아하는 사람을 설정했습니다: Node %d\n", liked_id);
                    break;
                }
                int id;
                printf("좋아하는 사람의 Node ID를 입력하세요 (1~254): ");
                scanf("%d", &id);
                if (id < 1 || id > 254) {
                    printf("잘못된 ID입니다. 좋아하는 사람 설정 실패.\n");
                } else {
                    liked_id = (uint8_t)id;
                    printf("좋아하는 사람으로 Node %d를 설정했습니다.\n", liked_id);
                }
                break;
            }
            case 3: {
                if (liked_id == 0) {
                    printf("좋아하는 사람을 먼저 설정하세요.\n");
                    break;
                }
                if (chat_state != IDLE) {
    printf("현재 채팅 중이므로 요청할 수 없습니다.");
    break;
}
                printf("좋아하는 사람(Node %d)에게 채팅 요청을 보냅니다.\n", liked_id);
                chat_state = WAITING_FOR_ANSWER;
                connected_node = liked_id;

                const char* msg = "QUERY_LIKE";
                L2_LLI_handleDataReq((uint8_t*)msg, strlen(msg), liked_id);
                break;
            }
            case 4:
                show_liked_me_list_and_choose();
                break;
            case 5: {
                if (chat_state == IDLE) {
                    printf("현재 채팅 중이 아닙니다.\n");
                } else {
                    disconnect_chat();
                    printf("채팅이 종료되었습니다.\n");
                }
                break;
            }
            case 6:
                printf("프로그램을 종료합니다.\n");
                return 0;
            case 7:
                show_who_likes_me();
                break;
            default:
                printf("잘못된 명령입니다.\n");
        }
    }
}
