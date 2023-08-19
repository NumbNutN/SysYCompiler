#ifndef CONFIG_H
#define CONFIG_H

/* 开启二元算数运算指令的翻译 */
#define OPEN_TRANSLATE_BINARY

/* 开启逻辑运算指令的翻译 */
#define OPEN_TRANSLATE_LOGICAL

/* 除法使用abi */
#define USE_DIV_ABI

/*开启函数带返回值调用 */
#define OPEN_FUNCTION_WITH_RETURN_VALUE

/* 允许二元运算 */
#ifdef OPEN_TRANSLATE_BINARY

    /* 是否允许二元的两个操作数均为立即数 2023-7-11*/
    #define ALLOW_TWO_IMMEDIATE

#endif

/* 类型不明确时缺省大小为4字节 2023-7-12 */
#define ENABLE_GIVING_DEFAULT_TYPE

/* 赋值号左侧变量存储位置 */
#define LEFT_VALUE_IS_IN_INSTRUCTION
//#define LEFT_VALUE_IS_OUTSIDE_INSTRUCTION

/*基于名字的变量信息表*/
#define VARIABLE_MAP_BASE_ON_NAME

/*基于Value地址的变量信息表*/
//#define VARIABLE_MAP_BASE_ON_VALUE_ADDRESS

/* 判断立即数Value的依据 */
//#define JUDGE_IMMEDIATE_BY_NAME
#define JUDGE_IMMEDIATE_BY_TYPE

/* 从高/低地址拆分内存单元 */
#define SPLIT_MEMORY_UNIT_ON_HIGH_ADDRESS
//#define SPLIT_MEMORY_UNIT_ON_LOW_ADDRESS

/*开启寄存器分配*/
//#define OPEN_REGISTER_ALLOCATION

/* 遇到操作数无法识别的指令生成未定义指令 */
//#define GEN_UNDEF

/* 通过何种方式计算栈帧大小 */
#define COUNT_STACK_SIZE_VIA_TRAVERSAL_INS_LIST
//#define COUNT_STACK_SIZE_VIA_TRAVERSAL_MAP

/* 定义对浮点参数传递方式 */
#define FLOAT_ABI_MODE_SOFTFP
//#define FLOAT_ABI_MODE_HARD

/* 测试模式 */
#define DEBUG_MODE

#ifdef DEBUG_MODE

    /* 调试信息打印到终端 */
    //#define PRINT_TO_TERMINAL

#endif

#endif