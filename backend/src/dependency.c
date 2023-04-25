#include "dependency.h"
#include "arm_assembly.h"
#include "variable_map.h"
#include "interface_zzq.h"
#include <math.h>

// Value* tempReg; //算数运算暂存寄存器
// Value* tempReg_forTar; //目标寄存器


// PLACE value_get_type(Value* val)
// {
//     srand((unsigned)time(NULL));
//     return rand()/2;
// }

unsigned value_getConstant(Value* val)
{
    /*
    通过Value类型的指针找出Constant
    目前只支持整型
    */
   //return ((ConstantNum*)(val))->num.num_int;
   return *((int*)val->pdata);
}



unsigned value_get_reg(Value* val)
{
    srand((unsigned)time(NULL));
    return rand()/12;
}

unsigned value_get_addr(Value* val)
{
    srand((unsigned)time(NULL));
    return rand()/1000;
}


/**
 * @brief 判断一个操作数是不是在指令中的常量
 * @update 20230122 添加了对浮点立即数的支持
 *         2023-3-28 更改了实现对Immediate判断的依据
*/
bool op_is_in_instruction(Value* op)
{
    
#ifdef JUDGE_IMMEDIATE_BY_NAME
    if(strcmp(op->name,"immediateInt")==0)
        return 1;
    else if(strcmp(op->name,"immediateFloat") == 0)
        return 1;
    else
        return 0;
#elif defined JUDGE_IMMEDIATE_BY_TYPE
    switch(op->VTy->TID)
    {
        case ImmediateIntTyID:
        case ImmediateFloatTyID:
        return 1;
    }
    return 0;
#endif
}



/**
 * @brief 判断一个变量是否在内存中
 * @birth: Created by LGD on 20230123
*/
bool variable_is_in_memory(Instruction* this,Value* var)
{
    return get_variable_place(this,var) == IN_MEMORY;
}

/**
 * @brief 判断一个变量是否是立即数
 * @birth: Created by LGD on 20230307
*/
bool variable_is_in_instruction(Instruction* this,Value* var)
{
    return get_variable_place(this,var) == IN_INSTRUCTION;
}

/**
 * @brief 返回指令的操作数个数
 * @birth: Created by LGD on 20230123
 * @update: 2023-3-28 采用ins_get_left_assign基本方法
*/
ins_get_operand_num(Instruction* this)
{
    return ins_get_assign_left_value(this)->NumUserOperands;
}

/**
 * @brief 判断一个操作数是否是变量
 * @author Created by LGD on 20220106
 * @update: 20230123 添加了对浮点立即数的判定
*/
bool op_is_a_variable(Value* op)
{
    if(strcmp(op->name,"immediateInt")==0)
        return 0;
    else if(strcmp(op->name,"immediateFloat") == 0)
        return 0;
    else
        return 1;
}

TypeID value_get_type(Value* val)
{
    return val->VTy->TID;
}


/**
 * @brief   浮点数转换为01串并存储在32/64位空间中返回
 * @birth: Created by LGD on 20230122
*/
uint64 float_754_binary_code(double num,int bitType){
    /*
        定义一个uint64的变量(unsigned long long Windows11 x64系统 编译器gcc 版本8.1.0)用来存储最多64位的IEEE754定义的浮点数二进制编码，变量名为code

        以下以32位浮点数二进制编码的转换为例
        当用户指定一个小数并要求转化为32位浮点数二进制编码时，

        1.小数交给c的double类型浮点数对数值进行储存，在后面的处理中保证精度的损失在IEEE754 64位的要求之内
        2.对小数不断/2，记录余数存到小数点右边，转化为N = (1.xxxxx)(二进制)  x 2^E 的形式
        3.尾数x2^23位变成一个整数部分有23位的浮点数并+入code ,如果该数仍有小数部分，会因为强制类型转换抹去，这部分小数以IEEE754 32位的精度也表示不出来
        4.阶数E+127(8位，如果用户定义的整数过大使阶数最后大于8位，这个转化程序只会存储阶数的低9位，且只会打印出低8位，还没有分析IEEE754怎么处理超过范围的整数部分，不确定这个处理对不对)，左移23位+入code
        5.依据小数的正负打印0或1
        5.将code以二进制形式打印前31位输出到命令行窗口
    */
    uint64 e = 0;  //定义阶码
    double m;  //定义尾数
    uint8 sign=0;  //定义符号
    uint8 remainder; //整数/2的余数
    long long integer_part = (int)num;  //获取整数部分
    double decimal_part = num - integer_part;  //获取小数部分

    //理论上integer_part 应该用uint64声明，但是我在强制类型转化会出现乱码，因为定义了uint32为unsigned long long的原因，这里使用long long (-2^63 ~ 2^63-1)来定义
    //但 事实上IEEE 754 定义的64位浮点数的最大真值应该为 2^(2^11 - 1 - 2^10 + 1)x(1-2^(-52)) 约为2^1024，这个量级是long long远远不能表示的
    //如果用户传入的小数 整数部分过大，我暂时没有做处理，只能保留用户的整数的低63位
    //而e在64位浮点数中最多只有11位，用uint16表示即可,但是因为在拼凑代码中会最多左移52位，需要有63位容量


    uint64 code=0; //浮点数码

    //我们在确定底数以后再将小数部分转化为整型
    //整数>>1后一定会把0位的1给噶掉
    //小数的精度较高不会被噶

    //正负号判断
    if(integer_part<0){
        integer_part = 0- integer_part;
        decimal_part = 0- decimal_part;
        sign=1;
    }

    while(integer_part/2){
        //整数每右移一位，阶码+1，浮点数除2，相当于尾数部分代码右移1位，给尾数添入整数/2的余数
        remainder = integer_part % 2;
        integer_part >>= 1;
        e++;

        decimal_part /= 2;
        decimal_part += (double)remainder/2;
    }

    m = decimal_part;


    switch(bitType){
        case BITS_32:
            //阶码补127
            e+=(pow(2,7)-1);
            //尾数变为整数
            m *= pow(2,IEEE_754_32BIT_MANTISSA);

            //IEEE 754 32位 浮点数代码
            code = (e << IEEE_754_32BIT_MANTISSA) + m;
            return code;
        case BITS_64:
            //阶码补2^10-1
            e+=(pow(2,10)-1);
            //尾数变整数
            m *= pow(2,IEEE_754_64BIT_MANTISSA);

            //IEEE 754 64位 浮点数代码
            code =(e << IEEE_754_64BIT_MANTISSA) + m;
            return code;
    }
}

/**
 * @brief 判断一个数是否是2的指数
*/
bool number_is_power_of_2(int num)
{
    size_t cnt_bit1 = 0;
    do
    {
        cnt_bit1 += num & 0x1;
        num = num >> 1;
    } while (num != 0);
    return cnt_bit1 == 1;
}