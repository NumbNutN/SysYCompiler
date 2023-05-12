#ifndef _DEPENDENCY_H
#define _DEPENDENCY_H


#include <assert.h>
#include <stdbool.h>
#include "type.h"
#include "value.h"
#include "instruction.h"

#define TEMP_REG_NUM 4

#define IEEE_754_32BIT_EXPONENT 8
#define UINT_32 32
#define IEEE_754_32BIT_MANTISSA 23
#define IEEE_754_64BIT_MANTISSA 52
#define uint32 unsigned int
#define uint8 unsigned char
#define uint16 unsigned short
#define uint64 unsigned long long

enum{
    BITS_32 = 32,
    BITS_64 = 64
};

/**
 * @brief   浮点数转换为01串并存储在32/64位空间中返回
 * @birth: Created by LGD on 20230122
*/
uint64 float_754_binary_code(double num,int bitType);

typedef struct _ins_info
{
    union {
        //赋值运算
        struct {
            char* var_name;         //目标操作数的变量名
        }assign;
        //跳转指令
        struct {
            char* tarLabel;         //目标跳转标签
            bool isConditional;     //是否有条件
            int condition_opCode;   //条件的双目运算的符号
        }branch;
        //函数开始/结束指令
        struct {
            int iValue;             //函数的栈帧大小
            char* sValue;           //函数的返回值
        }FunBegin;
    }option;
    //common member
    int opCode;                     //操作符  Add Sub Goto Call Return
    char* label;                    //当前语句的标签
    
} ins_info;


enum AdditionalOpCode{
    Goto_Equal = 11,
    Goto_NotEqual,
    Goto_GreatEqual,
    Goto_GreatThan,
    Goto_LessEqual,
    Goto_LessThan
};


typedef struct _TempReg
{
    int reg;
    bool isAviliable;
} TempReg;


void Init_tempValueReg();


/**
 * @brief 返回指令的操作数个数
 * @birth: Created by LGD on 20230123
*/
ins_get_operand_num(Instruction* this);
/**
 * @brief 判断一个变量是否在内存中
 * @birth: Created by LGD on 20230123
*/
bool variable_is_in_memory(Instruction* this,Value* var);
/**
 * @brief 判断一个变量是否是立即数
 * @birth: Created by LGD on 20230307
*/
bool variable_is_in_instruction(Instruction* this,Value* var);
/**
 * @brief 判断一个变量是否在寄存器中
 * @birth: Created by LGD on 2023-5-3
*/
bool variable_is_in_register(Instruction* this,Value* var);

void recycle_temp_value_register(Value* reg);

Value* value_create_temp_register(unsigned order);
Value* random_operand();
unsigned value_getConstant(Value* val);

// @brief:当操作数为常数时，获取指令的操作数的常数值
// @param: [int i]: 0：第一操作数  1：第二操作数
// @notice: 不包括目标操作数




bool op_is_in_instruction(Value* val);
/**
 * @brief 判断一个操作数是否是变量
 * @author Created by LGD on 20220106
*/
bool op_is_a_variable(Value* op);

TypeID value_get_type(Value* val);

/**
 * @brief 判断一个数是否是2的指数
*/
bool number_is_power_of_2(int num);


/**
 * @brief 翻译前执行的初始化
 * @birth: Created by LGD on 2023-3-28
*/
void TranslateInit();

/**
 * @brief 每次翻译新函数前要执行的初始化
 * @birth:Created by LGD on 2023-5-9
*/
void InitBeforeFunction();


#endif