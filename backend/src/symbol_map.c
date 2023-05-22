#include "memory_manager.h"
#include "instruction.h"
#include "cds.h"

//ins_get_left_value
#include "interface_zzq.h"

/**
 * @brief 重设了键的判等依据等
 * @birth: Created by LGD on 2023-5-22
*/
int Backend_symbolTable_CompareKey_Value(void* lhs,void* rhs){return strcmp(lhs,rhs);}
int Backend_symbolTable_CleanKey_Value(void* key)                   {}
int Backend_symbolTable_CleanValue_VarSpace(void* value)            {}

/**
 * @brief   对一个变量信息表进行初始化
 * @author  Created by LGD on 2023-05-22
*/
void back_endsymbol_table_init()
{
    if(currentPF.symbol_map != NULL)
        HashMapDeinit(currentPF.symbol_map);
    currentPF.symbol_map = HashMapInit();

    HashMapSetHash(currentPF.symbol_map,HashDjb2);
    HashMapSetCompare(currentPF.symbol_map,Backend_symbolTable_CompareKey_Value);

    HashMapSetCleanKey(currentPF.symbol_map,Backend_symbolTable_CleanKey_Value);
    HashMapSetCleanValue(currentPF.symbol_map,Backend_symbolTable_CleanValue_VarSpace);
}

/**
 * @brief 装载符号表
 * @birth: Created by LGD on 2023-5-22
*/
void traverse_list_and_load_symbol_table(List* this)
{
    Instruction* p;
    void* isFound;
    if(currentPF.symbol_map == NULL)
        back_endsymbol_table_init();
    char* name;
    ListFirst(this,false);
    ListNext(this,&p);
    Value* val;
    printf("变量信息表开始装载\n");
    do
    {
        switch(ins_get_opCode(p))
        {
            case AddOP:
            case SubOP:
            case MulOP:
            case DivOP:
            case AssignOP:
            case CallWithReturnValueOP:
            case NotEqualOP:
            case EqualOP:
            case GreatEqualOP:
            case GreatThanOP:
            case LessEqualOP:
            case LessThanOP:
            case GetelementptrOP:
            case LoadOP:
            
               //新建变量信息项
                val = ins_get_assign_left_value(p);
                isFound = variable_map_get_value(currentPF.symbol_map,val);
                if(isFound)break;
                printf("插入新的变量名：%s\n",val->name);
                HashMapPut(currentPF.symbol_map,val->name,val);
            break;
            //为数组也分配空间
            // case AllocateOP:
            //     //新建变量信息项
            //     val = ins_get_assign_left_value(p);
            //     isFound = variable_map_get_value(*myMap,val);
            //     if(isFound)break;
            //     arrayOffset = request_new_local_variable_memory_units(p->user.value.pdata->array_pdata.total_member*4);
            //     printf("数组%s分配了地址%d\n",val->name,arrayOffset);
            //     var_info = (VarInfo*)malloc(sizeof(VarInfo));
            //     memset(var_info,0,sizeof(VarInfo));
            //     printf("插入新的数组名：%s 地址%lx\n",val->name,val);
            //     variable_map_insert_pair(*myMap,val,var_info);
            //     //分配寄存器或内存单元
            //     if(*((enum _LOCATION*)HashMapGet(zzqMap,val->name)) == MEMORY)
            //     {
            //         int offset = request_new_local_variable_memory_unit(val->VTy->TID);
            //         set_variable_stack_offset_by_name(*myMap,val->name,offset);
            //         printf("%s分配了地址%d\n",val->name,offset);
            //     }
            //     else
            //     {
            //         RegisterOrder reg_order = request_new_allocable_register();
            //         //为该变量(名)创建寄存器映射
            //         set_variable_register_order_by_name(*myMap,val->name,reg_order);
            //         //打印分配结果
            //         printf("%s分配了寄存器%d\n",val->name,reg_order);                    
            //     }
            //     //使用一个指令将数组偏移值填充至对应的变量存储位置
            //     struct _operand arrOff = operand_create_immediate_op(arrayOffset);
            //     movii(var_info->ori,arrOff);

        }
    }while(ListNext(this,&p) && ins_get_opCode(p)!=FuncLabelOP);
     
}