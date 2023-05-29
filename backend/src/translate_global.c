#include "instruction.h"
//struct _operand
#include "operand.h"
//data bss
#include "arm.h"
//ins_get
#include "interface_zzq.h"

/**
@brief:翻译全局变量存储
@update:2023-5-29
*/
void translate_global_store_instruction(Instruction* this)
{
    char* name = ins_get_assign_left_value(this)->name;
    struct _operand stored_elem = toOperand(this,SECOND_OPERAND);
    
    dot_long_expression(name,stored_elem);
}
