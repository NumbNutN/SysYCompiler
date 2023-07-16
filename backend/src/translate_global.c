#include "instruction.h"
//struct _operand
#include "operand.h"
//data bss
#include "arm.h"
//ins_get
#include "interface_zzq.h"
//opType2Size
#include "memory_manager.h"

/**
@brief:翻译全局变量存储
@update:2023-5-29
*/
void translate_global_store_instruction(Instruction* this)
{
    char* name = ins_get_operand(this,FIRST_OPERAND)->name;
    struct _operand stored_elem = toOperand(this,SECOND_OPERAND);
    
    dot_long_expression(name,stored_elem,true);
}

/**
 * @brief 翻译allocate语句
 * @birth: Created by LGD on 2023-7-16
**/
void translate_global_allocate_instruction(Instruction* this)
{
    char* name = ins_get_operand(this, TARGET_OPERAND)->name;
    size_t size = opTye2size(ins_get_operand(this,TARGET_OPERAND)->VTy->TID);

    dot_zero_expression(name);
}

/**
 * @brief 由于翻译store永远在allocate后面，这里旨在检查是否有已经翻译的allocate语句
 */