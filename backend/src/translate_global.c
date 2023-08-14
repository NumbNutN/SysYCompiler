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
 * @update: 2023-8-14 现在未定义字面量的全局数组会放在bss段
**/
void translate_global_allocate_instruction(Instruction* this)
{
    char* name = ins_get_operand(this,TARGET_OPERAND)->name;
    Value* var = ins_get_operand(this,TARGET_OPERAND);
    //翻译全局或局部数组
    if(value_get_type(ins_get_operand(this, TARGET_OPERAND)) == ArrayTyID)
    {
        List* list;
        size_t totalSpace = var->pdata->array_pdata.total_member * 4;
        if(!array_init_literal(var,totalSpace,(List*)HashMapGet(global_array_init_hashmap, (void*)name)))
        {
            //如果当前全局数组不存在字面量，则声明到bss段
            //即使全局数组不存在字面量，其也应当被声明空间
            dot_zero_expression(BSS,name, var->pdata->array_pdata.total_member * 4);
        }
    }
    //翻译全局变量
    else{
        size_t size = opTye2size(ins_get_operand(this,TARGET_OPERAND)->VTy->TID);
        dot_zero_expression(DATA,name,4);
    }

}

/**
 * @brief:翻译对全局变量的赋值
 * @birth:Created by LGD on 2023-5-29
*/
void translate_global_store_instruction(Instruction* this)
{
    char* name = ins_get_operand(this,FIRST_OPERAND)->name;
    struct _operand stored_elem = toOperand(this,SECOND_OPERAND);
    
    dot_long_expression(name,stored_elem,true);
}