#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define COND 32
#define LINK 33
#define COUNT 34
#define INT_EXCEPTION 35
#define SP 1

#define MEMORY_SIZE 0x40000

typedef struct{
    uint32_t registers[36];
    uint32_t pc;
    uint8_t *memory;
} Emulator;

Emulator* create_emulator(size_t size, uint32_t pc, uint32_t sp) {
    Emulator* emu = malloc(sizeof(Emulator));
    emu->memory = malloc(size);

    memset(emu->registers, 0, sizeof(emu->registers));

    emu->pc = pc;
    emu->registers[SP] = sp;

    return emu;
}


void destroy_emulator(Emulator* emu) {
    free(emu->memory);
    free(emu);
}

uint32_t get_code(Emulator* emu, int index) {
    uint32_t ret = 0;
    for(int i = 0; i < 4; i++) {
        ret += emu->memory[emu->pc+index+i] << ((3-i)*8);
    }
    return ret;
}

void load_immidiate_shifted(Emulator* emu) {
    uint32_t code = get_code(emu,0);
    uint8_t opcode = (code >> 24) & 0xfc;
    uint8_t src = (code >> 21) & 0x1f;
    uint8_t dst = (code >> 16) & 0x1f;
    int16_t immidiate = code & 0xffff;

    printf("lis r%d, %d\n", src, immidiate);
    emu->registers[src] = (immidiate << 16);
    emu->pc += 4;
}

void load_word_and_zero(Emulator* emu) {
    uint32_t code = get_code(emu,0);
    uint8_t opcode = (code >> 24) & 0xfc;
    uint8_t dst = (code >> 21) & 0x1f;
    uint8_t src = (code >> 16) & 0x1f;
    uint16_t immidiate = code & 0xffff;

    printf("lwz r%d, %d(r%d)\n", dst, immidiate, src);
    uint32_t address = emu->registers[src] & 0x0fffffff;
    emu->registers[dst] = emu->memory[address + immidiate];
    emu->pc += 4;
}

void add_immidiate_or_load_immidiate(Emulator *emu) {
    uint32_t code = get_code(emu,0);
    uint8_t opcode = (code >> 24) & 0xfc;
    uint8_t src = (code >> 21) & 0x1f;
    uint8_t dst = (code >> 16) & 0x1f;
    uint16_t immidiate = code & 0xffff;

    if(dst == 0) {
        printf("li r%d, %d\n", src, immidiate);
        emu->registers[src] = immidiate;
    } else {
        printf("addi r%d, r%d, %d\n", src, dst, immidiate);
        emu->registers[src] = emu->registers[dst] + immidiate;
    }
    emu->pc += 4;
}

void move_to_register(Emulator *emu) {
    uint32_t code = get_code(emu,0);
    uint8_t opcode = (code >> 24) & 0xfc;
    uint8_t src = (code >> 21) & 0x1f;
    uint8_t dst = (code >> 16) & 0x1f;
    uint16_t immidiate = code & 0xffff;

    if(dst == 0x1001) {
        printf("mtctr r%d\n", src);
        emu->registers[COUNT] = emu->registers[src];
    } else if (dst == 0x1000) {
        printf("mtlr r%d\n", src);
        emu->registers[LINK] = emu->registers[src];
    } else if (dst == 0x0001) {
        printf("mtxer r%d\n", src);
        emu->registers[INT_EXCEPTION] = emu->registers[src];
    }
    emu->pc += 4;
}

void system_call(Emulator *emu) {
    if(emu->registers[0] == 4) {
        printf("write %d, 0x%x, %d\n", emu->registers[3], emu->registers[4] & 0x0ff0ffff, emu->registers[5]);
        write((int)emu->registers[3], emu->memory+(emu->registers[4] & 0x0ff0ffff), emu->registers[5]);
    } else if(emu->registers[0] == 1) {
        printf("exit %d\n", emu->registers[3]);
        exit(emu->registers[3]);
    }
    emu->pc += 4;
}

void branch_decrement_non_zero(Emulator* emu) {
}

typedef void instruction_func_t(Emulator*);
instruction_func_t* instructions[256];
void init_instructions(void) {
    instructions[0x3c] = load_immidiate_shifted;
    instructions[0x38] = add_immidiate_or_load_immidiate;
    instructions[0x80] = load_word_and_zero;
    instructions[0x7c] = move_to_register;
    instructions[0x44] = system_call;
}

int main (int argc, char* argv[]) {
    FILE *binary;
    Emulator *emu;

    if(argc != 2) {
        printf("usage: ./ppc_emu filename\n");
        return 1;
    }

    emu = create_emulator(MEMORY_SIZE, 0xb0, 0x500); 

    printf("argv[1] = %s\n", argv[1]);
    binary = fopen(argv[1], "rb");

    if(binary == NULL) {
        printf("cannot open file.\n");
        return 1;
    }

    fread(emu->memory, 0x200, 1, binary);

    fclose(binary);

    init_instructions();

    while(emu->pc < MEMORY_SIZE) {
        uint32_t code = get_code(emu,0);
//        if(code != 0)
//            printf("0x%08x \n", code);

        uint8_t opcode = (code >> 24) & 0xfc;

        if(opcode == 0x00) {
            printf("\nend of program\n\n");
            break;
        }

        if(instructions[opcode] == NULL) {
            printf("%02x is not registered opcode\n", opcode);
            break;
        }

        instructions[opcode](emu);
    }


    destroy_emulator(emu);
    return 0;
}

