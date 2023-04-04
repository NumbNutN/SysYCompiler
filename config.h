#ifndef CONFIG_H
#define CONFIG_H

//开启二元算数运算指令的翻译
#define OPEN_TRANSLATE_BINARY
//开启逻辑运算指令的翻译
#define OPEN_TRANSLATE_LOGICAL

/*开启函数带返回值调用*/
//#define OPEN_FUNCTION_WITH_RETURN_VALUE


#ifdef OPEN_TRANSLATE_BINARY

    /*使用最开始的Load Store通用方法处理binary运算*/
    //#define OPEN_LOAD_AND_STORE_BINARY_OPERATE

#endif

/* 赋值号左侧变量存储位置 */
#define LEFT_VALUE_IS_IN_INSTRUCTION
//#define LEFT_VALUE_IS_OUTSIDE_INSTRUCTION



/*基于名字的变量信息表*/
#define VARIABLE_MAP_BASE_ON_NAME

/*基于Value地址的变量信息表*/
//#define VARIABLE_MAP_BASE_ON_VALUE_ADDRESS



/* 基于名字判断立即数Value*/
//#define JUDGE_IMMEDIATE_BY_NAME
/*基于类型判断立即数Value*/
#define JUDGE_IMMEDIATE_BY_TYPE


/*zzq的寄存器分配与load and store 指令已经安插*/
//#define LLVM_LOAD_AND_STORE_INSERTED


//TEST
/*开启寄存器分配*/
//#define OPEN_REGISTER_ALLOCATION
#endif