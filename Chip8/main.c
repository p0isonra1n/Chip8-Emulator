#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define MAX_RAM 409600
#define PROGRAM_OFFSET 512
#define DEBUG 1


unsigned char RAM[MAX_RAM];
unsigned char V[15]; //Registers V0-VE

unsigned short I; //Index Register
unsigned short PC; //Program Counter

unsigned short STACK[16];
unsigned short SP;

unsigned char GFX[64 * 32]; //64 Pixels by 32 Pixels

unsigned char Delay_Counter;
unsigned char Sound_Counter;


int main(int argc, char **argv){
    for (int i = 0; i < argc; ++i)
    {
        printf("argv[%d]: %s\n", i, argv[i]);
    }
    printf("TEST\n");


    memset(RAM, 0, sizeof RAM);

    FILE *rom_ptr;

    rom_ptr = fopen("pong.rom", "rb");

    if(rom_ptr == NULL){
        printf("Unable to open ROM\n");
        exit(1);
    }

    unsigned int FILE_LEN;

    //Get length of file
    fseek(rom_ptr, 0, SEEK_END);
    FILE_LEN = ftell(rom_ptr);

    if(DEBUG) printf("File is %d long\n", FILE_LEN);

    if(FILE_LEN == 0 || FILE_LEN + PROGRAM_OFFSET > MAX_RAM){
        fclose(rom_ptr);
        if(DEBUG) printf("Need RAM size of %d while MAX RAM is %d \n", FILE_LEN + PROGRAM_OFFSET, MAX_RAM);
        printf("File is too large\n");
        exit(1);
    }

    rewind(rom_ptr);

    unsigned int read_len = fread(RAM + PROGRAM_OFFSET, 1, FILE_LEN, rom_ptr);
    fclose(rom_ptr);
    if(read_len != FILE_LEN){
        printf("Error loading file, lengths do not match\n");
        exit(1);
    }

    printf("ROM loaded into memory\n");

    PC = PROGRAM_OFFSET;
    I = 0;
    Delay_Counter = 0;
    Sound_Counter = 0;
    SP = 0;
    memset(V, 0, sizeof V);
    memset(STACK, 0, sizeof STACK);

    unsigned int RUNNING = 1;
    /*
    for(int i = 0; i <= RAM; i++){
        printf("%x", RAM[i]);
    }*/

    int failed_runs = 0;

    while(RUNNING){
        unsigned short OPCODE = RAM[PC] << 8 | RAM[PC + 1];
        unsigned char x = (OPCODE >> 8) & 0x0F;
	    unsigned char y = (OPCODE >> 4) & 0x0F;
	    unsigned char nibble = OPCODE & 0x0F;
	    unsigned short nnn = OPCODE & 0x0FFF;
	    unsigned char kk = OPCODE & 0xFF;

        for(int gfxX = 0; gfxX < 64; gfxX++){
            for(int gfxY = 0; gfxY < 32; gfxY++){
                printf("%x", GFX[gfxX + gfxY]);
            }
            printf("\n");
        }


        printf("PC: %x \n", PC);
        printf("OPCODE: %04x \n", OPCODE);
        printf("nnn: %02x \n", nnn);
        printf("V0: %02x V1: %02x V2: %02x V3: %02x V4: %02x V5: %02x V6: %02x V7: %02x V8: %02x V9: %02x VA: %02x VB: %02x VC: %02x VD: %02x VE: %02x \n", V[0], V[1],V[2],V[3],V[4],V[5],V[6],V[7],V[8],V[9],V[10],V[11],V[12],V[13],V[14],V[15]);
        
        switch(OPCODE & 0xF000){
            case 0x0000:
                printf("Unused OPCODE");
                break;
            case 0x2000: //CALL nnn
                //TODO: Check stack is not full
                STACK[SP++] = PC;
                PC = nnn;
                break;
            case 0x6000: //6xkk LD Vx, byte
                V[x] = kk;
                break;
            case 0x7000: // ADD Vx, byte
                V[x] = V[x] + kk;
                break;
            case 0xA000:
                I = OPCODE & 0x0FFF;
                break;
            case 0xD000:
                printf("Draw to screen not implemented yet \n");
                break;
            case 0xF000:
                switch(kk){
                    case 0x33: // LD B, Vx
                        RAM[I] = (V[x] / 100) % 10;
					    RAM[I + 1] = (V[x] / 10) % 10;
					    RAM[I + 2] = V[x] % 10;
                        break;
                }
                break;
            default:
                printf("Unknown OPCODE \n");
                exit(1);
                break;
        } 
        PC += 2;
        printf("\n\n");
        //if(PC > 0x220) exit(1);
    }

}