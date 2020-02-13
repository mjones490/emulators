#include "8080_cpu.h"
#include "cpu_types.h"

union regs_t regs;
struct cpu_state_t cpu_state;

BYTE *reg_op[] = { &regs.b.B, &regs.b.C, &regs.b.D, &regs.b.E, &regs.b.H, &regs.b.L, NULL, &regs.b.A }; 

static void MOV(BYTE code)
{
    BYTE *sss = reg_op[code & 0b111];
    BYTE *ddd = reg_op[(code >> 3) & 0b111];

    if (sss == NULL && ddd == NULL)
        printf("Halt!\n");
    else if (sss == NULL)
        *ddd = get_byte(regs.w.HL);
    else if (ddd == NULL)
        put_byte(regs.w.HL, *sss);
    else
        *ddd = *sss;
}

static BYTE exec_instruction(BYTE code)
{
    if ((code & 0b11000000) == 0b01000000)
        MOV(code);

   return 0;
}

BYTE cpu_execute_instruction()
{
    return exec_instruction(get_next_byte());
}
