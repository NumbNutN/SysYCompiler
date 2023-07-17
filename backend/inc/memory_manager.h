#ifndef _MEMORY_MANAGER_H
#define _MEMORY_MANAGER_H

#include "cds.h"
#include "arm.h"

#include "value.h"

#define STACK_MAX_SIZE 0xFFFF

/**
 * @brief MemUnitInfo结构体记录每一个分配的内存单元的信息，并串联成内存分配链表
 *        每一次需要分配新的内存单元时，内存分配调用定位到当前链表第一个未分配且内存足量的内存单元
 *        其将当前内存单元分离出一个大小足够的内存单元并插入节点前方作为新的内存单元
 *        在初始时，该链表只含有一个具有STACK_MAX_SIZE个字节大小且未分配的内存单元
*/
typedef struct _MemUnitInfo
{
    int baseAddr;
    size_t size;
    bool isUsed;
} MemUnitInfo;

extern struct _MemUnitInfo* stack_frame_mem_header;
struct _Produce_Frame{

    //栈顶偏移量
    int SPOffset;
    //栈帧偏移量
    int FPOffset;
    //定义了当前已被使用的局部变量栈内存单元偏移值，其初始值为栈帧至栈底之差，并在每次分配时自增
    int cur_use_variable_offset;
    //定义了当前已被使用的参数传递用栈内存单元偏移值，其初始值为栈顶至栈帧之差，并在每次分配时自增
    int cur_use_parameter_offset;
    //@brith:2023-5-2 定义了当前函数的内存单元管理链表
    List* stack_frame_memory_unit_list;
    //@birth:2023-5-9 定义了当前形参计数器
    size_t param_counter;
    //@birth:2023-5-16 维护一个变量名到Value* 的哈希表
    HashMap* symbol_map;
    //@birth:2023-5-22 定义了当前函数引起的FP递减量的计数器，其用来矫正外部函数局部变量指针的寻址方式
    int fp_offset;
    //@brief 2023-6-6 定义了当前函数使用的通用寄存器
    struct _operand used_reg[8];
};

extern struct _Produce_Frame  currentPF;
//记录当前函数的栈总容量
extern size_t stackSize;

//当前函数的寄存器映射表
extern HashMap* cur_register_mapping_map;
//当前函数的内存映射表
extern HashMap* cur_memory_mapping_map;


//定义了当前已被处理的参数个数
extern size_t passed_param_number;


/**********************************************/
/*                 内部调用                    */
/**********************************************/

/**
 * @brief   对一个变量信息表进行初始化
 * @author  Created by LGD on 20230110
*/
void vitual_register_map_init(HashMap** map);
/**
 * @brief   对一个虚拟内存映射表进行初始化
 * @author  Created by LGD on 20230110
*/
void vitual_Stack_Memory_map_init(HashMap** map);

/**
 * @brief 申请一块新的局部变量的内存单元
*/
int request_new_local_variable_memory_unit();

/**
 * @brief 使对栈帧指针的更改生效
 * @update: 2023-4-4 将栈顶相对偏移改为栈帧
*/
void update_fp_value();
/**
 * @biref 使对栈顶指针的更改生效
*/
void update_sp_value();

/**
 * @brief 使栈指针的偏移撤销
 * @param doClear 是否将偏移值回复到0
 * @birth: Created by LGD on 2023-7-17
**/
void reset_sp_value(bool doClear);

/**********************************************/
/*                 外部调用                    */
/**********************************************/

/**
 * @brief 根据数据类型返回该类型对应的尺寸，以字节为单位
 * @birth: Created by LGD on 2023-5-2
*/
size_t opTye2size(enum _TypeID type);

/**
 * @brief 翻译新的函数时调用
 * @birth: Created by LGD on 2023-5-2
*/
void new_stack_frame_init(int totalsize);
/**
 * @brief 销毁内存管理块链表
 * @birth: Created by LGD on 2023-5-2
*/
void stack_frame_deinit();
/**
 * @brief 申请一块新的参数传递用栈内存单元
 * @birth: Created by LGD on 2023-3-14
*/
int request_new_local_variable_memory_unit(enum _TypeID type);

/**
 * @brief 申请一块新的局部变量的内存空间
 * @update: Created by LGD on 2023-4-11
*/
int request_new_local_variable_memory_units(size_t req_bytes);

/**
 * @brief 给定虚拟寄存器编号，若有映射，则返回映射的实际寄存器编号；
 *          若未作映射，则尝试申请一个新的寄存器编号
 *          若无法提供更多的寄存器，说明哪里出了问题，甩出错误
 * @birth: Created by LGD on 2023-3-12
*/
RegisterOrder get_virtual_register_mapping(HashMap* map,size_t ViOrder);

/**
 * @brief 给定虚拟内存位置，若有映射，则返回映射的实际内存相对栈帧偏移值；
 *          若未作映射，则尝试申请一个新的内存单元
 *          若无法提供更多的内存，说明哪里出了问题，甩出错误
 * @birth: Created by LGD on 2023-3-12
*/
int get_virtual_Stack_Memory_mapping(HashMap* map,size_t viMem);



/**
 * @brief 依据局部变量个数和传递参数个数确定两个指针的跳跃位置
 * @birth: Created by LGD on 2023-3-12
*/
void set_stack_frame_status(size_t param_num,size_t local_var_num);

/**
 * @brief 依据指定输入申请可分配的寄存器
 * @birth: Created by LGD on 2023-5-14
*/
RegisterOrder request_new_allocatble_register_by_specified_ids(int ids);


/**********************************************/
/*             局部变量指针管理                */
/**********************************************/
/**
 * @brief 根据变量类型记录FP从上一个活动记录到当前活动记录的累计偏移值
 * @birth: Created by LGD on 2023-5-22
*/
void fp_offset_counter(Value* val);

/**
 * @brief 遍历链表将所有allocate的数组分配内存空间并将基地址装载到对应位置
 * @birth: Created by LGD on 2023-5-23
*/
void traverse_and_load_arrayBase_to_recorded_place(List* this);


/**
 * @brief 遍历链表并对所有allocate param语句完成指针的偏移映射
 * @birth: Created by LGD on 2023-5-22
*/
void traverse_and_fix_pointer_offset(List* this);

/**
 * @brief 初始化被管理的内存单元
 * @param align 对齐单位
*/
void init_memory_unit(size_t align);
/**
 * @brief 选取一块未使用的内存单元
*/
size_t pick_a_free_memoryUnit();
/**
 * @brief 回收一块内存单元
*/
void return_a_memoryUnit(size_t memAddr);

#endif