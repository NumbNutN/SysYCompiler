/**************************************************************/
/*                           内存管理                          */
/***************************************************************/

#include <stdbool.h>
#include "cds.h"
#include <assert.h>
#include "memory_manager.h"
#include "arm.h"

#include "type.h"
#include <assert.h>

#include "config.h"
#include "operand.h"


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

struct _Produce_Frame  currentPF = 
{
    .symbol_map = NULL
};


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

    //定义了当前已被使用的局部变量栈内存单元偏移值，其初始值为栈帧至栈底之差，并在每次分配时自增
    int cur_use_variable_offset;
    //定义了当前已被使用的参数传递用栈内存单元偏移值，其初始值为栈顶至栈帧之差，并在每次分配时自增
    int cur_use_parameter_offset;

//定义了参数的个数
int cur_param_number;
//定义了局部变量的个数
int cur_local_variable_number;

//定义了当前已被处理的参数个数
size_t passed_param_number = 0;


RegisterOrder param_passing_register[PARAMETER_ON_THE_STACK_BOUNDARY];

/**************************************************************/
/*                           函数初始化                          */
/***************************************************************/
                                                                                       

/**
 * @brief 依据局部变量个数和传递参数个数确定两个指针的跳跃位置
 * @birth: Created by LGD on 2023-3-12
*/
void set_stack_frame_status(size_t param_num,size_t local_var_num)
{   

    //堆栈LR寄存器和R7
    bash_push_pop_instruction("PUSH",&fp,&lr,END);

    //设置栈帧和栈顶指针的相对位置
    currentPF.FPOffset -= local_var_num*4;
    currentPF.SPOffset -= (local_var_num + param_num)*4;

    //设置当前用户使用的栈帧偏移
    currentPF.cur_use_variable_offset = local_var_num*4;
    currentPF.cur_use_parameter_offset = (param_num - 1)*4;

#ifdef LLVM_LOAD_AND_STORE_INSERTED
    //初始化寄存器映射表
    vitual_register_map_init(&cur_register_mapping_map);
    //初始化内存映射表
    vitual_Stack_Memory_map_init(&cur_memory_mapping_map);
#endif
}

/**
 * @brief 恢复函数的栈帧和栈顶
 * @birth: Created by LGD on 2023-4-4
 * @update:2023-5-13 不再需要恢复FP的值，因为它妥善保管在栈中
*/
void reset_stack_frame_status()
{
    currentPF.FPOffset = - currentPF.FPOffset;
    currentPF.SPOffset = - currentPF.SPOffset;

    //执行期间使指针变动生效
    // update_fp_value();
    update_sp_value();
}


/**
 * @brief 使对栈帧指针的更改生效
 * @update: 2023-4-4 将栈顶相对偏移改为栈帧
*/
void update_fp_value()
{
    if(!abs(currentPF.FPOffset))
        return;

    // AssembleOperand offset;
    // offset.addrMode = IMMEDIATE;
    // offset.oprendVal = abs(currentPF.FPOffset);
    // if(currentPF.FPOffset > 0)
    //     general_data_processing_instructions(ADD,fp,fp,offset," ",false);
    // else
    //     general_data_processing_instructions(SUB,fp,fp,offset," ",false);
    struct _operand immd = operand_create_immediate_op(currentPF.FPOffset);
    struct _operand reg_off = operand_load_immediate(immd,ARM);
    general_data_processing_instructions(ADD,fp,fp,reg_off," ",false);
    operand_recycle_temp_register(reg_off);
}

/**
 * @brief 使对栈顶指针的更改生效
 * @birth:???
 * @update:2023-5-13 多数情况下，对SP的改动借助临时寄存器是不可取的
*/
void update_sp_value()
{
    if(!abs(currentPF.SPOffset))
        return;
    struct _operand offset = operand_create_immediate_op(abs(currentPF.SPOffset));

    if(currentPF.SPOffset > 0)
        general_data_processing_instructions(ADD,sp,sp,offset," ",false);
    else
        general_data_processing_instructions(SUB,sp,sp,offset," ",false);
    //struct _operand immd = operand_create_immediate_op(currentPF.SPOffset);
    //struct _operand reg_off = operand_load_immediate(immd,ARM);
    //general_data_processing_instructions(ADD,sp,sp,reg_off," ",false);
    //operand_recycle_temp_register(reg_off);
}

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
 * @brief 恢复当前函数的堆栈状态
*/



/**********************************************/
/*                 虚拟寄存器映射表             */
/**********************************************/

//设置可供变量暂时存放的寄存器池的个数
#define ALLOCABLE_REGISTER_NUM 8

//设置可供变量暂时存放的寄存器池全集
#define ALLOCABLE_REGISTER R4,R5,R6,R8,R9,R10,R11,R12
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
 * @brief 依据指定输入申请可分配的寄存器
 * @birth: Created by LGD on 2023-5-14
*/
RegisterOrder request_new_allocatble_register_by_specified_ids(int ids)
{
    switch(ids)
    {
        case 1:return R4;
        case 2:return R5;
        case 3:return R6;
        case 4:return R8;
        case 5:return R9;
        case 6:return R10;
        case 7:return R11;
        case 8:return R12;
    }
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
    return HashMapGet(map,ViOrder) != NULL? (RegisterOrder)HashMapGet(map,ViOrder) - BASE_VALUE : -1;
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
 * @brief 根据数据类型返回该类型对应的尺寸，以字节为单位
 * @birth: Created by LGD on 2023-5-2
 * @update: 2023-5-3 ArrayTyID将被视作一个绝对地址
*/
size_t opTye2size(enum _TypeID type)
{
    switch(type)
    {
        case IntegerTyID:         ///< int a;
        case FloatTyID:           ///< float b;
        case PointerTyID:         ///< Pointers
        case ReturnTyID:
        case ParamTyID:
            return 4;
        case ArrayTyID:             //< Arrays
            return 4;
            //assert(false && ArrayTyID && "Could not recognize the size of a array type");
  // PrimitiveTypes
        case HalfTyID:       ///< 16-bit floating point type
        case BFloatTyID:     ///< 16-bit floating point type (7-bit significand)
            return 2;
        case DoubleTyID:     ///< 64-bit floating point type
            return 8;
        case FP128TyID:      ///< 128-bit floating point type (112-bit significand)
        case PPC_FP128TyID:  ///< 128-bit floating point type (two 64-bits, PowerPC)
            return 16;
        case VoidTyID:
            return 0;
        default:
            assert(false && type && "Unrecognized type");
    }
}

/**
 * @brief 翻译新的函数时调用
 * @birth: Created by LGD on 2023-5-2
*/
void new_stack_frame_init(int totalsize)
{
    //assert(!currentPF.stack_frame_memory_unit_list && "上一个栈帧内存管理单元链表可能未释放");
    currentPF.stack_frame_memory_unit_list = ListInit();
    struct _MemUnitInfo* firstMemUnit = malloc(sizeof(struct _MemUnitInfo));
    memset(firstMemUnit,0,sizeof(struct _MemUnitInfo));
    firstMemUnit->baseAddr = -totalsize;
    firstMemUnit->isUsed = false;
    firstMemUnit->size = totalsize;
    ListPushBack(currentPF.stack_frame_memory_unit_list,firstMemUnit);
}

/**
 * @brief 销毁内存管理块链表
 * @birth: Created by LGD on 2023-5-2
*/
void stack_frame_deinit()
{
    ListDeinit(currentPF.stack_frame_memory_unit_list);
    currentPF.stack_frame_memory_unit_list = NULL;
}

/**
 * @brief 申请一块新的局部变量的内存空间
 * @update: Created by LGD on 2023-4-11
*/
int request_new_local_variable_memory_units(size_t req_bytes)
{
    // currentPF.cur_use_variable_offset -= 4*wordLength;
    // if(currentPF.cur_use_variable_offset >= 0)
    //     return currentPF.cur_use_variable_offset;
    // assert(false && "Request for new local variable stack units more than expected");
    
    struct _MemUnitInfo* memUnit; 
    ListFirst(currentPF.stack_frame_memory_unit_list,false);
    size_t idx=0;
    bool next_result;
    for(;(next_result = ListNext(currentPF.stack_frame_memory_unit_list,&memUnit))!= false;++idx)
    {
        if(memUnit->isUsed == true)continue;
        if(memUnit->size < req_bytes)continue;
        break;
    }
    assert(next_result && "当前栈帧没有更多的空间可供分配");
    //未分配内存空间的尺寸刚好符合申请大小，不需要改动链表，将当前单元直接分配即可
    if(memUnit->size == req_bytes)
    {
        memUnit->isUsed = true;
        return memUnit->baseAddr;
    }
    //分拆当前的内存单元
    memUnit->size -= req_bytes;
    struct _MemUnitInfo* new_allocated_memUnit=malloc(sizeof(struct _MemUnitInfo));
#ifdef SPLIT_MEMORY_UNIT_ON_HIGH_ADDRESS
    //由于我们现在无法得知栈的总大小，新的分配单元总是从当前可拆分内存单元的最高相对偏移量处划出内存块
    new_allocated_memUnit->baseAddr = memUnit->baseAddr + memUnit->size ;
#elif defined SPLIT_MEMORY_UNIT_ON_LOW_ADDRESS
    new_allocated_memUnit->baseAddr = memUnit->baseAddr;
    memUnit->baseAddr += req_bytes;
#endif
    new_allocated_memUnit->size = req_bytes;
    new_allocated_memUnit->isUsed = true;
    //由于是从高地址进行拆分的，新的分配单元总是插入原属内存单元的前一个位置
    ListInsert(currentPF.stack_frame_memory_unit_list,idx,new_allocated_memUnit);
    return new_allocated_memUnit->baseAddr;
}

/**
 * @brief 申请一块新的局部变量的内存单元
 * @birth: Created by LGD on 2023-5-2
*/
int request_new_local_variable_memory_unit(enum _TypeID type)
{
    // currentPF.cur_use_variable_offset -= 4;
    // if(currentPF.cur_use_variable_offset >= 0)
    //     return currentPF.cur_use_variable_offset;
    // assert(false && "Request for new local variable stack unit more than expected");

    return request_new_local_variable_memory_units(opTye2size(type));
}

/**
 * @brief 回收一块内存单元
 * @birth: Created by LGD on 2023-5-3
*/
void recycle_a_local_variable_memory_unit(int baseAddr)
{
    struct _MemUnitInfo* memUnit; 
    ListFirst(currentPF.stack_frame_memory_unit_list,false);
    size_t idx=0;
    bool next_result;
    for(;(next_result = ListNext(currentPF.stack_frame_memory_unit_list,memUnit))!= false;++idx)
    {
        if(memUnit->baseAddr == baseAddr){break;}
    }
    assert(next_result && "没有找到需要归还的内存单元");
    ListRemove(currentPF.stack_frame_memory_unit_list,idx);
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
    currentPF.cur_use_variable_offset -= 4;
    if(currentPF.cur_use_variable_offset >= 0)
        return currentPF.cur_use_variable_offset;
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











/**********************************************/
/*                  throw                     */
/**********************************************/



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