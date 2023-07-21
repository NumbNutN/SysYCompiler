#include "instruction.h"
//struct _operand
#include "operand.h"
//data bss
#include "arm.h"
//ins_get
#include "interface_zzq.h"
//opType2Size
#include "memory_manager.h"
//value_get_type
#include "dependency.h"
//global_array_init_item
#include "value.h"
//hashmap_foreach
#include "variable_map.h"

extern HashMap *global_array_init_hashmap;

/**
 * @brief 翻译allocate语句
 * @birth: Created by LGD on 2023-7-16
**/
void translate_global_allocate_instruction(Instruction* this)
{
    char* name = ins_get_operand(this,TARGET_OPERAND)->name;
    //翻译全局或局部数组
    if(value_get_type(ins_get_operand(this, TARGET_OPERAND)) == ArrayTyID)
    {
        List* list;
        // HashMap_foreach(global_array_init_hashmap, name, list)
        // {
        //     printf("%s %p\n",name,list);
        // }
        array_init_literal(name,(List*)HashMapGet(global_array_init_hashmap, (void*)name));
    }
    //翻译全局变量
    else{
        size_t size = opTye2size(ins_get_operand(this,TARGET_OPERAND)->VTy->TID);
        dot_zero_expression(name,4);
    }

}

/**
 * @brief 由于翻译store永远在allocate后面，这里旨在检查是否有已经翻译的allocate语句
 */