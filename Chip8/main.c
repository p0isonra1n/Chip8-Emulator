#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "main.h"

#define MAX_RAM 409600
#define PROGRAM_OFFSET 512
#define FONT_OFFSET 0x180

#define DEBUG 1

unsigned char RAM[MAX_RAM];
unsigned char V[15]; //Registers V0-VE

unsigned short I;  //Index Register
unsigned short PC; //Program Counter

unsigned short STACK[16];
unsigned short SP;

unsigned char GFX[64 * 32]; //64 Pixels by 32 Pixels
int GFX_UPDATED;

unsigned char Delay_Counter;
unsigned char Sound_Counter;

/* Standard 4x5 font */
static unsigned char font[] = {
    /* '0' */ 0xF0,
    0x90,
    0x90,
    0x90,
    0xF0,
    /* '1' */ 0x20,
    0x60,
    0x20,
    0x20,
    0x70,
    /* '2' */ 0xF0,
    0x10,
    0xF0,
    0x80,
    0xF0,
    /* '3' */ 0xF0,
    0x10,
    0xF0,
    0x10,
    0xF0,
    /* '4' */ 0x90,
    0x90,
    0xF0,
    0x10,
    0x10,
    /* '5' */ 0xF0,
    0x80,
    0xF0,
    0x10,
    0xF0,
    /* '6' */ 0xF0,
    0x80,
    0xF0,
    0x90,
    0xF0,
    /* '7' */ 0xF0,
    0x10,
    0x20,
    0x40,
    0x40,
    /* '8' */ 0xF0,
    0x90,
    0xF0,
    0x90,
    0xF0,
    /* '9' */ 0xF0,
    0x90,
    0xF0,
    0x10,
    0xF0,
    /* 'A' */ 0xF0,
    0x90,
    0xF0,
    0x90,
    0x90,
    /* 'B' */ 0xE0,
    0x90,
    0xE0,
    0x90,
    0xE0,
    /* 'C' */ 0xF0,
    0x80,
    0x80,
    0x80,
    0xF0,
    /* 'D' */ 0xE0,
    0x80,
    0x80,
    0x80,
    0xE0,
    /* 'E' */ 0xF0,
    0x80,
    0xF0,
    0x80,
    0xF0,
    /* 'F' */ 0xF0,
    0x80,
    0xF0,
    0x80,
    0x80,
};

int main(int argc, char **argv)
{
    for (int i = 0; i < argc; ++i)
    {
        printf("argv[%d]: %s\n", i, argv[i]);
    }
    printf("TEST\n");

    memset(RAM, 0, sizeof RAM);

    FILE *rom_ptr;

    rom_ptr = fopen("pong.rom", "rb");

    if (rom_ptr == NULL)
    {
        printf("Unable to open ROM\n");
        exit(1);
    }

    unsigned int FILE_LEN;

    //Get length of file
    fseek(rom_ptr, 0, SEEK_END);
    FILE_LEN = ftell(rom_ptr);

    if (DEBUG)
        printf("File is %d long\n", FILE_LEN);

    if (FILE_LEN == 0 || FILE_LEN + PROGRAM_OFFSET > MAX_RAM)
    {
        fclose(rom_ptr);
        if (DEBUG)
            printf("Need RAM size of %d while MAX RAM is %d \n", FILE_LEN + PROGRAM_OFFSET, MAX_RAM);
        printf("File is too large\n");
        exit(1);
    }

    rewind(rom_ptr);

    unsigned int read_len = fread(RAM + PROGRAM_OFFSET, 1, FILE_LEN, rom_ptr);
    fclose(rom_ptr);
    if (read_len != FILE_LEN)
    {
        printf("Error loading file, lengths do not match\n");
        exit(1);
    }

    printf("ROM loaded into memory\n");

    PC = PROGRAM_OFFSET;
    I = 0;
    Delay_Counter = 0;
    Sound_Counter = 0;
    SP = 0;
    GFX_UPDATED = 0;
    memset(V, 0, sizeof V);
    memset(STACK, 0, sizeof STACK);
    memcpy(RAM + FONT_OFFSET, font, sizeof font);

    unsigned int RUNNING = 1;
    /*
    for(int i = 0; i <= RAM; i++){
        printf("%x", RAM[i]);
    }*/

    int failed_runs = 0;

    while (RUNNING)
    {
        step();
        tick(); //This is way too fast need to run at 60hz
    }
}

void step()
{
    unsigned short OPCODE = RAM[PC] << 8 | RAM[PC + 1];
    unsigned char x = (OPCODE >> 8) & 0x0F;
    unsigned char y = (OPCODE >> 4) & 0x0F;
    unsigned char nibble = OPCODE & 0x0F;
    unsigned short nnn = OPCODE & 0x0FFF;
    unsigned char kk = OPCODE & 0xFF;

    printf("PC: %x \n", PC);
    printf("OPCODE: %04x \n", OPCODE);
    printf("nnn: %02x \n", nnn);
    printf("V0: %02x V1: %02x V2: %02x V3: %02x V4: %02x V5: %02x V6: %02x V7: %02x V8: %02x V9: %02x VA: %02x VB: %02x VC: %02x VD: %02x VE: %02x VF: %02x \n", V[0], V[1], V[2], V[3], V[4], V[5], V[6], V[7], V[8], V[9], V[10], V[11], V[12], V[13], V[14], V[15]);

    PC += 2;
    GFX_UPDATED = 0;

    switch (OPCODE & 0xF000)
    {
    case 0x0000:
        if (OPCODE == 0x00EE)
        {
            if (SP == 0)
            {
                printf("Stack Pointer is already 0 can not return any more!");
                exit(1);
            }
            PC = STACK[--SP];
        }
        else
        {
            printf("Not implemented");
        }
        break;
    case 0x1000: // JP addr
        PC = nnn;
        break;
    case 0x2000: //CALL nnn
        if (SP >= 16)
        {
            printf("Stack Pointer is full, can not jump again!");
            exit(1);
        }
        STACK[SP++] = PC;
        PC = nnn;
        break;
    case 0x3000:
        if (V[x] == kk)
        {
            PC += 2;
        }
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
    case 0xC000: // RND Vx, byte
        V[x] = (rand() % 256) & kk;
        break;
    case 0xD000:
        x = V[x]; y = V[y];
		for(int q = 0; q < nibble; q++) {
			for(int p = 0; p < 8; p++) {
				int pix = (RAM[I + q] & (0x80 >> p)) != 0;
				if(pix) {
                    int tx = (x + p) & 0x3F, ty = (y + q) & 0x1F;
					int byte = ty * 64 + tx;
					int bit = 1 << (byte & 0x07);
                    byte >>= 3;
					if(GFX[byte] & bit)
					    V[0x0F] = 1;
					GFX[byte] ^= bit;				
				}				
			}		
		}		
        GFX_UPDATED = 1;
        draw_to_screen();
        break;
    case 0xF000:
        switch (kk)
        {
        case 0x33: // LD B, Vx
            RAM[I] = (V[x] / 100) % 10;
            RAM[I + 1] = (V[x] / 10) % 10;
            RAM[I + 2] = V[x] % 10;
            break;
        case 0x07:
            V[x] = Delay_Counter;
            break;
        case 0x15: // LD DT, Vx
            Delay_Counter = V[x];
            break;
        case 0x29: //LD F, Vx
            I = FONT_OFFSET + (V[x] & 0x0F) * 5;
            break;
        case 0x65:
            if (I + x > MAX_RAM)
            {
                x = MAX_RAM - I;
            }
            assert(I + x <= MAX_RAM);
            if (x >= 0)
            {
                memcpy(V, RAM + I, x + 1);
            }
            break;
        default:
            printf("0xF0%02x is not implemented\n", kk);
            exit(1);
            break;
        }
        break;
    default:
        printf("Unknown OPCODE \n");
        exit(1);
        break;
    }
    printf("\n\n");
    //if(PC > 0x220) exit(1);
}

void tick(){
    if(Delay_Counter > 0) Delay_Counter--;
    if(Sound_Counter > 0) Sound_Counter--;
}

int get_pixel(int x, int y){
    int byte, bit;
	int w = 64; 
    int h = 32;
	if(x < 0 || x >= w || y < 0 || y >= h) return 0;
	byte = y * w + x;
	bit = byte & 0x07;
	byte >>= 3;
	assert(byte < sizeof GFX);
	assert(bit < 8);
	return (GFX[byte] & (1 << bit)) != 0;
}

int draw_to_screen()
{
    if(GFX_UPDATED){
        for (int gfxY = 0; gfxY < 32; gfxY++)
        {
            for (int gfxX = 0; gfxX < 64; gfxX++)
            {
                printf("%d", get_pixel(gfxX, gfxY));
            }
            printf("\n");
        }
    }
}