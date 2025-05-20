#include "mbed.h"
#include "string.h"
#include "L2_FSMmain.h"
#include "L3_FSMmain.h"

//serial port interface
Serial pc(USBTX, USBRX);

//GLOBAL variables (DO NOT TOUCH!) ------------------------------------------

//source/destination ID
uint8_t input_thisId=1;
uint8_t input_destId=0;

int main(void){

    pc.printf("------------------ protocol stack starts! --------------------------\n");
    
    pc.printf(":: ID for this node : ");
    pc.scanf("%d", &input_thisId);

    pc.printf(":: ID for the destination : ");
    pc.scanf("%d", &input_destId);

    uint8_t input_likedId;
    pc.printf(":: ID you like (liked_id) : ");
    pc.scanf("%d", &input_likedId);
    pc.getc();

    pc.printf("endnode : %i, dest : %i, liked : %i\n", input_thisId, input_destId, input_likedId);
    
    L2_initFSM(input_thisId);
    L2_setLikedId(input_likedId);  // ✅ 추가
    L3_initFSM(input_destId);
    
    while(1)
    {
        L2_FSMrun();
        L3_FSMrun();
    }
}
