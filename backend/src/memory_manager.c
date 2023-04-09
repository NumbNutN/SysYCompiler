/**************************************************************/
/*                           内存管理                          */
/***************************************************************/

#include <stdbool.h>
#include "cds.h"
#include <assert.h>
#include "memory_manager.h"
#include "arm.h"


//栈帧指针
#define FP R7
//栈顶指针
#define PTOS SP

//堆栈边界
#define PARAMETER_ON_THE_STACK_BOUNDARY 4

//物理内存地址基址
//这是为了克服 0 偏移 和 R0 寄存器作为值在HashMap中无法判断是否有返回的问题，加上偏移值作为存储内容，
//相应的，每次取出要减去基址

#define BASE_VALUE 32

struct _Produce_Frame{

    //栈顶偏移量
    int SPOffset;
    //栈帧偏移量
    int FPOffset;
    //定义了当前已被使用的局部变量栈内存单元偏移值，其初始值为栈帧至栈底之差，并在每次分配时自增
    int cur_use_variable_offset;
    //定义了当前已被使用的参数传递用栈内存单元偏移值，其初始值为栈顶至栈帧之差，并在每次分配时自增
    int cur_use_parameter_offset;

} currentPF;


//当前函数的寄存器映射表
HashMap* cur_register_mapping_map;
//当前函数的内存映射表
HashMap* cur_memory_mapping_map;


Stack* MemoryUnit;
//原栈帧指针值 Frame pointer指向位置
size_t oriFP = 0;
//当前栈帧指针值 Frame pointer指向位置
size_t curFP = 0;


//原栈顶指针值 Top of Stack Pointer
size_t oriST = 0;
//当前栈顶指针值 Top of Stack Pointer
size_t curST = 0;


//定义了参数的个数
int cur_param_number;
//定义了局部变量的个数
int cur_local_variable_number;

//定义了当前已被处理的参数个数
size_t passed_param_number = 0;

//定义了当前已被使用的局部变量栈内存单元偏移值，其初始值为栈帧至栈底之差，并在每次分配时自增
int cur_use_variable_offset;

//定义了当前已被使用的参数传递用栈内存单元偏移值，其初始值为栈顶至栈帧之差，并在每次分配时自增
int cur_use_parameter_offset;

RegisterOrder param_passing_register[PARAMETER_ON_THE_STACK_BOUNDARY];

/**************************************************************/
/*                           函数初始化                          */
/***************************************************************/


/**
 * @brief 定义参数传递候选寄存器列表
*/
void init_parameter_register_list()
{
    size_t cnt = 0;
    for(RegisterOrder i = R0;i<PARAMETER_ON_THE_STACK_BOUNDARY;i++,cnt++)
        param_passing_register[cnt] = i;
}

/**
 * @brief 设置栈帧指针的相对位移
 * @birth: Created by LGD on 20230312
*/
void set_fp_relative(int offset)
{
    curFP += offset;
}

/**
 * @brief 设置栈顶指针的相对偏移
 * @birth: Created by LGD on 20230312
*/
void set_pToS_relative(int offset)
{
    curST += offset;
}

/**
 * @brief 设置局部变量的传递个数
*/
void set_local_variable(size_t num)
{
    set_pToS_relative(-num*4);
    set_fp_relative(-num*4);

    //设置局部变量初始堆栈偏移值
    cur_use_variable_offset = num*4;

    //设置局部变量变量
    cur_local_variable_number = num;
}

/**
 * @brief 设置参数传递的个数
*/
void set_parameter_passing_number(size_t num)
{
    if(num <= 4)
        return;
    set_pToS_relative(-(num - 4)*4);

    //设置参数传递初始堆栈偏移值
    cur_use_parameter_offset = (num - 1)*4;    

    //设置参数个数
    cur_param_number = num-4;
}

/**
 * @brief 使对栈帧指针的更改生效
 * @update: 2023-4-4 将栈顶相对偏移改为栈帧
*/
void update_fp_value()
{
    if(!abs(curFP - oriFP))
        return;

    AssembleOperand offset;
    
    offset.addrMode = IMMEDIATE;
    offset.oprendVal = abs(curFP - oriFP);
    if(curFP-oriFP > 0)
        general_data_processing_instructions("ADD",fp,fp,offset," ",false,"\t");
    else
        general_data_processing_instructions("SUB",fp,fp,offset," ",false,"\t");
}

/**
 * @biref 使对栈顶指针的更改生效
*/
void update_st_value()
{
    if(!abs(curST - oriST))
        return;

    AssembleOperand sp,offset;
    sp.addrMode = REGISTER_DIRECT;
    sp.oprendVal = SP;
    
    offset.addrMode = IMMEDIATE;
    offset.oprendVal = abs(curST - oriST);
    if(curFP-oriFP > 0)
        general_data_processing_instructions("ADD",sp,sp,offset," ",false,"\t");
    else
        general_data_processing_instructions("SUB",sp,sp,offset," ",false,"\t");    
}

/**
 * @brief 依据局部变量个数和传递参数个数确定两个指针的跳跃位置
 * @birth: Created by LGD on 2023-3-12
*/
void set_stack_frame_status(size_t param_num,size_t local_var_num)
{

    //设置栈帧和栈顶指针的相对位置
    set_local_variable(local_var_num);
    set_parameter_passing_number(param_num);

    //执行期间使指针变动生效
    update_fp_value();
    update_st_value();

#ifdef LLVM_LOAD_AND_STORE_INSERTED
    //初始化寄存器映射表
    vitual_register_map_init(&cur_register_mapping_map);
    //初始化内存映射表
    vitual_Stack_Memory_map_init(&cur_memory_mapping_map);
#endif
}

/**
 * @brief 回复函数的栈帧和栈顶
 * @birth: Created by LGD on 2023-4-4
*/
void reset_stack_frame_status()
{

}


/**
 * @brief 恢复当前函数的堆栈状态
*/


struct _operand r027[8] = {{REGISTER_DIRECT,R0,0},
                            {REGISTER_DIRECT,R1,0},
                            {REGISTER_DIRECT,R2,0},
                            {REGISTER_DIRECT,R3,0}};
struct _operand immedOp = {IMMEDIATE,0,0};
struct _operand sp = {REGISTER_DIRECT,SP,0};
struct _operand lr = {REGISTER_DIRECT,LR,0};
struct _operand fp = {REGISTER_DIRECT,R7,0};
struct _operand pc = {REGISTER_DIRECT,PC,0};
struct _operand sp_indicate_offset = {
                REGISTER_INDIRECT_WITH_OFFSET,
                SP,
                0
};

struct _operand trueOp = {IMMEDIATE,1,0};
struct _operand falseOp = {IMMEDIATE,0,0};






/**********************************************/
/*                 虚拟寄存器映射表             */
/**********************************************/

//设置可供变量暂时存放的寄存器池的个数
#define ALLOCABLE_REGISTER_NUM 7

//设置可供变量暂时存放的寄存器池全集
#define ALLOCABLE_REGISTER R4,R5,R6,R7,R8
RegisterOrder allocable_register_pool[ALLOCABLE_REGISTER_NUM] = {ALLOCABLE_REGISTER};

/**
 * @brief 申请一个新的可分配的寄存器
 * @birth: Created by LGD on 2023-3-12
 * @update: 2023-4-4 分配后标注为已分配
*/
RegisterOrder request_new_allocable_register()
{
    RegisterOrder reg;
    for(size_t i=0;i<ALLOCABLE_REGISTER_NUM;i++)
        if((reg = allocable_register_pool[i])!=-1){
            allocable_register_pool[i] = -1;
            return reg;
        }
            
    assert(false && "No enough allocable when register allocate");
}

/**
 * @brief 回收新的可分配的寄存器
 * @birth: Created by LGD on 2023-3-12
*/
void return_allocable_register(RegisterOrder reg)
{
    for(size_t i=0;i<ALLOCABLE_REGISTER_NUM;i++)
        if(allocable_register_pool[i]==-1)
        {
            allocable_register_pool[i] = reg;
            return;
        }
    assert(false && "allocable registers request to return are more than the max number");
}

/**
 * @brief 重设了键的判等依据等
*/
int Virtual_Register_CompareKey_Value(void* lhs,void* rhs)          {return lhs != rhs;}
int Virtual_Register_CleanKey(void* key)                            {}
int Virtual_Register_CleanValue(void* value)                        {}


/**
 * @brief   对一个变量信息表进行初始化
 * @author  Created by LGD on 20230110
*/
void vitual_register_map_init(HashMap** map)
{
    *map = HashMapInit();
    HashMapSetHash(*map,HashDjb2);
    HashMapSetCompare(*map,Virtual_Register_CompareKey_Value);

    HashMapSetCleanKey(*map,Virtual_Register_CleanKey);
    HashMapSetCleanValue(*map,Virtual_Register_CleanValue);
}


/**
 * @brief   为变量状态表插入新的键值对，若键已存在则覆盖值
 * @author  Created by LGD on 2023-3-12
*/
void vitual_register_map_insert_pair(HashMap* map,size_t ViOrder,RegisterOrder RealOrder)
{
    HashMapPut(map,ViOrder,RealOrder + BASE_VALUE);
}


/**
 * @brief   返回当前虚拟寄存器映射值，若为正数则为寄存器编号，若为-1则未作映射
 * @author  Created by LGD on 2023-3-12
*/
RegisterOrder vitual_register_map_get_value(HashMap* map,size_t ViOrder)
{
    return HashMapGet(map,ViOrder) != NULL? HashMapGet(map,ViOrder) - BASE_VALUE : -1;
}

/**
 * @brief 给定虚拟寄存器编号，若有映射，则返回映射的实际寄存器编号；
 *          若未作映射，则尝试申请一个新的寄存器编号
 *          若无法提供更多的寄存器，说明哪里出了问题，甩出错误
 * @birth: Created by LGD on 2023-3-12
*/
RegisterOrder get_virtual_register_mapping(HashMap* map,size_t ViOrder)
{
    RegisterOrder realOrder;
    if((realOrder = vitual_register_map_get_value(map,ViOrder)) != -1)
        return realOrder;

    //若没有找到映射，新请求一个真实寄存器
    realOrder = request_new_allocable_register();
    //建立一对新的映射关系
    vitual_register_map_insert_pair(map,ViOrder,realOrder);

    return realOrder;
}


/**********************************************/
/*                 栈空间映射表                */
/**********************************************/

/**
 * @brief 申请一块新的局部变量的内存单元
*/
int request_new_local_variable_memory_unit()
{
    cur_use_variable_offset -= 4;
    if(cur_use_variable_offset >= 0)
        return cur_use_variable_offset;
    assert(false && "Request for new local variable stack unit more than expected");
}


#ifdef LLVM_LOAD_AND_STORE_INSERTED

/**
 * @brief 重设了键的判等依据等
*/
int Virtual_Stack_Memory_CompareKey_Value(void* lhs,void* rhs)          {return lhs != rhs;}
int Virtual_Stack_Memory_CleanKey(void* key)                            {}
int Virtual_Stack_Memory_CleanValue(void* value)                        {}


/**
 * @brief   对一个虚拟内存映射表进行初始化
 * @author  Created by LGD on 20230110
*/
void vitual_Stack_Memory_map_init(HashMap** map)
{
    *map = HashMapInit();
    HashMapSetHash(*map,HashDjb2);
    HashMapSetCompare(*map,Virtual_Stack_Memory_CompareKey_Value);

    HashMapSetCleanKey(*map,Virtual_Stack_Memory_CleanKey);
    HashMapSetCleanValue(*map,Virtual_Stack_Memory_CleanValue);
}


/**
 * @brief   为虚拟内存映射表插入新的键值对，若键已存在则覆盖值
 * @author  Created by LGD on 2023-3-12
*/
void vitual_Stack_Memory_map_insert_pair(HashMap* map,size_t ViMem,RegisterOrder RealMem)
{
    HashMapPut(map,ViMem,RealMem + BASE_VALUE);
}

/**
 * @brief   返回当前虚拟内存单元映射值
 * @author  Created by LGD on 2023-3-12
*/
int vitual_Stack_Memory_map_get_value(HashMap* map,size_t ViMem)
{
    return HashMapGet(map,ViMem) != NULL? HashMapGet(map,ViMem) - BASE_VALUE: -1;
}

/**
 * @brief 给定虚拟内存位置，若有映射，则返回映射的实际内存相对栈帧偏移值；
 *          若未作映射，则尝试申请一个新的内存单元
 *          若无法提供更多的内存，说明哪里出了问题，甩出错误
 * @birth: Created by LGD on 2023-3-12
*/
int get_virtual_Stack_Memory_mapping(HashMap* map,size_t viMem)
{
    int realMem;
    if((realMem = vitual_Stack_Memory_map_get_value(map,viMem)) != -1)
        return realMem;

    //若没有找到映射，新请求一个物理内存单元
    //对于对参数的访问，往往不会出现这个情况
    realMem = request_new_local_variable_memory_unit();
    //建立一对新的映射关系
    vitual_register_map_insert_pair(map,viMem,realMem);

    return realMem;
}

#endif

/**********************************************/
/*                  传递参数映射表              */
/**********************************************/



/**
 * @brief 申请一块新的参数传递用栈内存单元
 * @birth: Created by LGD on 2023-3-14
*/
int request_new_parameter_stack_memory_unit()
{
    cur_use_parameter_offset -= 4;
    if(cur_use_parameter_offset >= 0)
        return cur_use_parameter_offset;
    assert(false && "Request for new local variable stack unit more than expected");
}


/**********************************************/
/*                  使用参数映射表              */
/**********************************************/



/**
 * @brief 根据参数的序号，返回函数相对FP的偏移
*/
int return_param_offset(size_t i)
{
    return -(cur_local_variable_number*4 + (cur_param_number - i)* 4);
}




/**
 * @brief 初始化被管理的内存单元
 * @param align 对齐单位
*/
void init_memory_unit(size_t align)
{
    assert(stackSize >= align && "栈总空间不可比对齐字节数少");
    MemoryUnit = StackInit();
    size_t initialUnit = stackSize % align;
    size_t addr = initialUnit;
    do{
        StackPush(MemoryUnit,addr);
        addr += align;
    }while(addr + 4 <= stackSize);
}

/**
 * @brief 选取一块未使用的内存单元
*/
size_t pick_a_free_memoryUnit()
{
    size_t addr;
    StackTop(MemoryUnit,&addr);
    StackPop(MemoryUnit);
    return addr;
}

/**
 * @brief 回收一块内存单元
*/
void return_a_memoryUnit(size_t memAddr)
{
    StackPush(MemoryUnit,memAddr);
}


