#include "mbed.h"
#include "L2_FSMmain.h"
#include "L3_chatProtocol.h"
#include "protocol_parameters.h"
#include <string.h>
#include <stdio.h>

Serial pc(USBTX, USBRX);

static uint8_t myId = 0;
static uint8_t likedId = 0;
static bool isConfigured = false;

static char inputBuffer[100];
static int inputIndex = 0;
static bool inputReady = false;

static Timer periodicTimer;
static Timer inputTimer;

void processInput(void);
void handleIdleInput(const char* input);
void handleChatInput(const char* input);
void displayMatchedDevices(void);

int main() {
    pc.baud(115200);

    pc.printf("\n========================================\n");
    pc.printf("LoRa Dating Chat System\n");
    pc.printf("========================================\n");

    while (!isConfigured) {
        pc.printf("\nEnter your device ID (1-254): ");
        int id1;
        pc.scanf("%d", &id1);
        myId = (uint8_t)id1;

        if (myId < 1 || myId > 254) {
            pc.printf("Invalid ID! Please enter 1-254.\n");
            continue;
        }

        pc.printf("Enter the ID of device you like (1-254): ");
        int id2;
        pc.scanf("%d", &id2);
        likedId = (uint8_t)id2;

        if (likedId < 1 || likedId > 254) {
            pc.printf("Invalid ID! Please enter 1-254.\n");
            continue;
        }

        if (myId == likedId) {
            pc.printf("You cannot like yourself! Try again.\n");
            continue;
        }

        isConfigured = true;
    }

    while (pc.readable()) pc.getc();

    L2_initFSM(myId);
    L3_initChatProtocol(myId, likedId);

    pc.printf("\n========================================\n");
    pc.printf("Configuration complete!\n");
    pc.printf("Waiting for matches...\n");
    pc.printf("Commands:\n");
    pc.printf("  'list' - Show matched devices\n");
    pc.printf("  'chat <ID>' - Start chat\n");
    pc.printf("  'quit' - End current chat\n");
    pc.printf("========================================\n\n");

    periodicTimer.start();
    inputTimer.start();

    while (1) {
        L2_FSMrun();

        if (periodicTimer.read() > 1.0f) {
            L3_periodicTask();
            periodicTimer.reset();
        }

        processInput();
        wait(0.01);
    }
}

void processInput(void) {
    while (pc.readable() && !inputReady) {
        char c = pc.getc();

        if (c == '\n' || c == '\r') {
            inputBuffer[inputIndex] = '\0';
            if (inputIndex > 0) inputReady = true;
            inputIndex = 0;
        } else if (inputIndex < 99) {
            inputBuffer[inputIndex++] = c;
        }
    }

    if (inputReady) {
        if (L3_hasPendingRequest()) {
            if (strlen(inputBuffer) == 1 && (inputBuffer[0] == '1' || inputBuffer[0] == '0')) {
                L3_processPendingResponse(inputBuffer[0]);
            } else {
                pc.printf("[ERROR] Invalid input. Enter 1 (accept) or 0 (decline).\n");
            }
        } else if (L3_getChatState() == L3_CHAT_ACTIVE) {
            handleChatInput(inputBuffer);
        } else {
            handleIdleInput(inputBuffer);
        }
        inputReady = false;
    }
}

void handleIdleInput(const char* input) {
    if (strcmp(input, "list") == 0) {
        displayMatchedDevices();
    } else if (strncmp(input, "chat ", 5) == 0) {
        int targetId = 0;
        if (sscanf(input + 5, "%d", &targetId) == 1) {
            MatchInfo_t* matches = NULL;
            int matchCount = 0;
            L3_getMatchedDevicesList(&matches, &matchCount);
            bool found = false;
            for (int i = 0; i < matchCount; i++) {
                if (matches[i].deviceId == targetId) { found = true; break; }
            }
            if (found) L3_requestChat(targetId);
            else pc.printf("[ERROR] Device %d not in match list!\n", targetId);
        } else {
            pc.printf("[ERROR] Invalid command. Use: chat <ID>\n");
        }
    } else {
        pc.printf("[ERROR] Unknown command. Available: list, chat <ID>\n");
    }
}

void handleChatInput(const char* input) {
    if (strcmp(input, "quit") == 0) {
        L3_endChat();
    } else if (strlen(input) > 0) {
        L3_sendChatMessage(input);
    }
}

void displayMatchedDevices(void) {
    MatchInfo_t* matches = NULL;
    int matchCount = 0;
    L3_getMatchedDevicesList(&matches, &matchCount);

    pc.printf("\n=========== Matches ===========\n");
    if (matchCount == 0) {
        pc.printf("No matches found.\n");
    } else {
        for (int i = 0; i < matchCount; i++) {
            pc.printf("Device ID: %d, RSSI: %d dBm\n", matches[i].deviceId, matches[i].rssi);
        }
    }
    pc.printf("================================\n\n");
}
