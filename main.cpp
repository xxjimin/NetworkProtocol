#include "mbed.h"
#include "L2_FSMmain.h"
#include "L3_chatProtocol.h"
#include "protocol_parameters.h"
#include <string.h>
#include <stdio.h>

Serial pc(USBTX, USBRX);

// 설정 변수
static uint8_t myId = 0;
static uint8_t likedId = 0;
static bool isConfigured = false;

// 입력 버퍼
static char inputBuffer[100];
static int inputIndex = 0;
static bool inputReady = false;

// 타이머
static Timer periodicTimer;
static Timer inputTimer;

// 함수 선언
void processInput(void);
void handleIdleInput(const char* input);
void handleChatInput(const char* input);
void displayMatchedDevices(void);

int main() {
    pc.baud(115200);
    
    pc.printf("\n========================================\n");
    pc.printf("LoRa Dating Chat System\n");
    pc.printf("========================================\n");
    
    // ID 설정
    while (!isConfigured) {
        pc.printf("\nEnter your device ID (1-254): ");
        int id1;
        pc.scanf("%d", &id1);
        myId = (uint8_t)id1;
        
        if (myId < 1 || myId > 254) {
            pc.printf("Invalid ID! Please enter a value between 1 and 254.\n");
            continue;
        }
        
        pc.printf("Enter the ID of device you like (1-254): ");
        int id2;
        pc.scanf("%d", &id2);
        likedId = (uint8_t)id2;
        
        if (likedId < 1 || likedId > 254) {
            pc.printf("Invalid ID! Please enter a value between 1 and 254.\n");
            continue;
        }
        
        if (myId == likedId) {
            pc.printf("You cannot like yourself! Please try again.\n");
            continue;
        }
        
        isConfigured = true;
    }
    
    // 버퍼 클리어
    while(pc.readable()) pc.getc();
    
    // L2 및 L3 초기화
    L2_initFSM(myId);
    L3_initChatProtocol(myId, likedId);
    
    pc.printf("\n========================================\n");
    pc.printf("Configuration complete!\n");
    pc.printf("Waiting for matches...\n");
    pc.printf("Commands:\n");
    pc.printf("  'list' - Show devices that like you\n");
    pc.printf("  'chat <ID>' - Start chat with device\n");
    pc.printf("  'quit' - End current chat\n");
    pc.printf("========================================\n\n");
    
    // 타이머 시작
    periodicTimer.start();
    inputTimer.start();
    
    // 메인 루프
    while (1) {
        // L2 FSM 실행
        L2_FSMrun();
        
        // 주기적 작업 (1초마다)
        if (periodicTimer.read() > 1.0f) {
            L3_periodicTask();
            periodicTimer.reset();
        }
        
        // 입력 처리
        processInput();
        
        // 짧은 대기
        wait(0.01);
    }
}

// 입력 처리
void processInput(void) {
    // 문자 입력 확인
    while (pc.readable() && !inputReady) {
        char c = pc.getc();
        
        if (c == '\n' || c == '\r') {
            inputBuffer[inputIndex] = '\0';
            if (inputIndex > 0) {
                inputReady = true;
            }
            inputIndex = 0;
        } else if (inputIndex < 99) {
            inputBuffer[inputIndex++] = c;
        }
    }
    
    // 입력 처리
    if (inputReady) {
        if (L3_getChatState() == L3_CHAT_ACTIVE) {
            handleChatInput(inputBuffer);
        } else {
            handleIdleInput(inputBuffer);
        }
        inputReady = false;
    }
}

// IDLE 상태 입력 처리
void handleIdleInput(const char* input) {
    if (strcmp(input, "list") == 0) {
        displayMatchedDevices();
    }
        else if (strcmp(input, "accept") == 0) {
        L3_acceptChatRequest();
    }
    else if (strcmp(input, "decline") == 0) {
        L3_declineChatRequest();
    }
    else if (strncmp(input, "chat ", 5) == 0) {
        int targetId = 0;
        if (sscanf(input + 5, "%d", &targetId) == 1) {
            // 매칭 목록에 있는지 확인
            MatchInfo_t* matches = NULL;
            int matchCount = 0;
            L3_getMatchedDevicesList(&matches, &matchCount);
            
            bool found = false;
            for (int i = 0; i < matchCount; i++) {
                if (matches[i].deviceId == targetId) {
                    found = true;
                    break;
                }
            }
            
            if (found) {
                L3_requestChat(targetId);
            } else {
                pc.printf("[ERROR] Device %d is not in your match list!\n", targetId);
            }
        } else {
            pc.printf("[ERROR] Invalid command. Use: chat <ID>\n");
        }
    }
    else if (strlen(input) > 0) {
        pc.printf("[ERROR] Unknown command. Available commands: list, chat <ID>\n");
    }


}

// 채팅 상태 입력 처리
void handleChatInput(const char* input) {
    if (strcmp(input, "quit") == 0) {
        L3_endChat();
    } else if (strlen(input) > 0) {
        L3_sendChatMessage(input);
    }
}

// 매칭된 기기 목록 표시
void displayMatchedDevices(void) {
    MatchInfo_t* matches = NULL;
    int matchCount = 0;
    L3_getMatchedDevicesList(&matches, &matchCount);
    
    pc.printf("\n========================================\n");
    pc.printf("Devices that like you (RSSI >= -30):\n");
    pc.printf("========================================\n");
    
    if (matchCount == 0) {
        pc.printf("No matches found.\n");
    } else {
        for (int i = 0; i < matchCount; i++) {
            pc.printf("Device ID: %d, RSSI: %d dBm\n", 
                     matches[i].deviceId, matches[i].rssi);
        }
    }
    
    pc.printf("========================================\n\n");
}