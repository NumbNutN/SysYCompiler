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
        }
    }while(ListNext(this,&p) && ins_get_opCode(p)!=FuncLabelOP);
}