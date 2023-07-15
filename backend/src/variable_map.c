#include "variable_map.h"
#include "interface_zzq.h"
#include "dependency.h"
#include "arm_assembly.h"
#include "cds.h"
#include <string.h>
#include "config.h"

#include "memory_manager.h"

//记录当前函数的栈总容量
size_t stackSize;
//记录当前已经分配了多少容量
size_t currentDistributeSize = 0;


/**
 * @brief 重设了键的判等依据等
*/
int CompareKey_Value(void* lhs,void* rhs)       {
#ifdef VARIABLE_MAP_BASE_ON_NAME
    return strcmp(lhs,rhs);
#elif defined VARIABLE_MAP_BASE_ON_VALUE_ADDRESS
    return lhs != rhs;
#else 
#error "需要指定至少一种变量信息表的键形式"
#endif
    }
int CleanKey_Value(void* key)                   {}
int CleanValue_VarSpace(void* value)            {free(value);}

/**
 * @brief   对一个变量信息表进行初始化
 * @author  Created by LGD on 20230110
*/
void variable_map_init(HashMap** map)
{
    *map = HashMapInit();

    HashMapSetHash(*map,HashDjb2);
    HashMapSetCompare(*map,CompareKey_Value);

    HashMapSetCleanKey(*map,CleanKey_Value);
    HashMapSetCleanValue(*map,CleanValue_VarSpace);

}

/**
 * @brief   遍历变量信息表一次并返回一个键
 * @author  Created by LGD on 20230110
 * @update: 2023-3-26 添加名字和Value*查表的选项
*/
#ifdef VARIABLE_MAP_BASE_ON_VALUE_ADDRESS 
Value*  
#elif defined VARIABLE_MAP_BASE_ON_NAME
char*   
#else 
#error "需要指定至少一种变量信息表的键形式"
#endif
traverse_variable_map_get_key(HashMap* map)
{
    Pair* pair_ptr = HashMapNext(map);
    if(pair_ptr)
        return pair_ptr->key;
    else
        return NULL;
}

/**
 * @brief   在指定变量状态表中根据指定键Value*获取值
 * @author  LGD
 * @date    20230110
 * @update: 2023-3-26 添加基于名字的条件编译
*/
VarInfo* variable_map_get_value(HashMap* map,Value* key)
{
#ifdef VARIABLE_MAP_BASE_ON_VALUE_ADDRESS
    return HashMapGet(map,key);
#elif defined VARIABLE_MAP_BASE_ON_NAME
    return  HashMapGet(map,key->name);
#else 
#error "需要指定至少一种变量信息表的键形式"
#endif
}

/**
 * @brief   为变量状态表插入新的键值对，若键已存在则覆盖值
 * @author  LGD
 * @date    20221220
 * @update: 2023-3-26 添加基于名字的条件编译
*/
void variable_map_insert_pair(HashMap* map,Value* key,VarInfo* value)
{
#ifdef VARIABLE_MAP_BASE_ON_VALUE_ADDRESS
    HashMapPut(map,key,value);
#elif defined VARIABLE_MAP_BASE_ON_NAME
    HashMapPut(map,key->name,value);
#else 
#error "需要指定至少一种变量信息表的键形式"
#endif
}


#ifdef VARIABLE_MAP_BASE_ON_NAME
/**
 * @brief 依据变量名设定变量在指定map的栈帧偏移
 * @birth: Created by LGD on 2023-3-26
 * @update: update 2023-4-4 添加寻址方式
*/
void set_variable_stack_offset_by_name(HashMap* map,char* name,size_t offset)
{
    VarInfo* vi = HashMapGet(map,name);
    vi->ori.addrMode = REGISTER_INDIRECT_WITH_OFFSET;
    vi->ori.oprendVal = FP;
    vi->ori.addtion = offset;
}

/**
 * @brief 在map中设置变量当前的寄存器编号
 * @birth: Created by LGD on 20230305
*/
void set_variable_register_order_by_name(HashMap* map,char* name,RegisterOrder reg_order)
{
    VarInfo* vi = HashMapGet(map,name);
    vi->ori.addrMode = REGISTER_DIRECT;
    vi->ori.oprendVal = reg_order;
}

/**
 * @brief 在map中标定当前变量为未分配
 * @birth: Created by LGD on 2023-7-9
*/
void set_variable_unallocated_by_name(HashMap* map,char* name,bool is_existed)
{
    VarInfo* vi;
    if(!is_existed)
    {
        vi = (VarInfo*)malloc(sizeof(VarInfo));
        HashMapPut(map, name, vi);
    }

    else
        vi = HashMapGet(map,name);

    vi->ori.addrMode = NONE_ADDRMODE;
    vi->ori.oprendVal = 0;
}
#endif

/**
 * @brief 判断一个变量在当前指令中是否是活跃的
*/
bool variable_is_live(Instruction* this,Value* var)
{

    return variable_map_get_value(this->map,var)->isLive;
}

/**
 * @brief   遍历变量信息表一次并返回一个键
 * @author  Created by LGD on 20230110
*/
VarInfo* traverse_variable_map_get_value(HashMap* map)
{
    Pair* pair_ptr = HashMapNext(map);
    if(pair_ptr)
        return pair_ptr->value;
    else
        return NULL;

}

/**
 * @brief   将指定的变量信息表赋给指令，这是浅拷贝
 * @author  LGD
 * @date    20221218
*/
void ins_shallowSet_varMap(Instruction* this,HashMap* map)
{
    this->map = map;
}

/**
 * @brief   将指定的变量信息表赋给指令，这是深拷贝，会将map中所有的变量信息值的内存复制一份新的
 * @birth:  Created by LGD on 20221218
 * @update: 2023-3-28 依据哈希表的键值进行条件编译
*/
void ins_deepSet_varMap(Instruction* this,HashMap* map)
{
    HashMap* newMap;
    variable_map_init(&newMap);
#ifdef VARIABLE_MAP_BASE_ON_VALUE_ADDRESS
    Value* key;
#elif defined VARIABLE_MAP_BASE_ON_NAME
    char* key;
#endif
    VarInfo* value;
    VarInfo* new_var_info;
    HashMap_foreach(map,key,value)
    {
        new_var_info = (VarInfo*)malloc(sizeof(VarInfo));
        memcpy(new_var_info,value,sizeof(VarInfo));
        HashMapPut(newMap,key,value);
    }
    this->map = newMap;
}


/**
 * @brief 将变量信息表拷贝给当前指令，但是仅拷贝存储位置和偏移（不拷贝活跃信息）
 * @author Created by LGD on 20230109
*/
void ins_cpy_varinfo_space(Instruction* this,HashMap* map)
{
    Value* key;
    VarInfo* src_var_info;
    VarInfo* dst_var_info;
    if(this==NULL)return;
    HashMap_foreach(map,key,src_var_info)
    {
        dst_var_info = variable_map_get_value(this->map,key);
        dst_var_info->ori = src_var_info->ori;
    }
}


/**
 * @brief 获取操作数的位置，如果是变量查变量表，如果是常数直接返回IN_INSTRUCTION
 * @author Created by LGD on 20220106
 * @update 20230305 完善了zzq return_var的设定
*/
//TODO 2023-7-9 回顾一下
RegorMem get_variable_place(Instruction* this,Value* var)
{
    if(name_is_global(var->name))
    {
        return IN_DATA_SEC;
    }
    else
    {
        if(var_is_return_val(var))
            return IN_REGISTER;
        if(op_is_in_instruction(var))
            return IN_INSTRUCTION;
        VarInfo* vs = variable_map_get_value(this->map,var);
        return judge_operand_in_RegOrMem(vs->ori);
    }

}

/**
 * @brief 返回对应变量的位置枚举值（指令、寄存器或内存）
 * @author Created by LGD on ??
 * @update: Lastest 20230109 如果变量是return_var 即刻返回“在寄存器中”
 *          20230101 从ins_get_ariable_place 更名为ins_get_op_place
 *          20221224 弃用三元素数组，改用查表
 *          这里涉及一个判断，如果该变量为立即数，则不应该在变量表中找它的位置
 *          20230305 内核变为 get_variable_place
*/
RegorMem get_variable_place_by_order(Instruction* this,int i)
{
    return get_variable_place(this,ins_get_operand(this,i));
}


/**
 * @brief   根据变量状态表获取寄存器编号
 * @author  Created by LGD on 20230305
*/
size_t get_variable_register_order(Instruction* this,Value* var)
{ 
    if(var_is_return_val(var))return R0;
    VarInfo* vs = variable_map_get_value(this->map,var);
    return vs->ori.oprendVal;
}


/**
 * @brief   根据变量状态表获取寄存器编号
 * @author  Created by LGD on 20230305
*/
size_t get_variable_register_order_by_order(Instruction* this,int i)
{ 
    return get_variable_register_order(this,ins_get_operand(this,i));
}

/**
 * @brief   根据变量状态表获取相对栈帧偏移
 * @author  Created by LGD on 20230305
*/
size_t get_variable_stack_offset(Instruction* this,Value* var)
{ 
    VarInfo* vs = variable_map_get_value(this->map,var);
    return vs->ori.addtion;
}

/**
 * @brief   根据变量状态表获取相对栈帧偏移
 * @author  Created by LGD on 20230305
*/
size_t get_variable_stack_offset_by_order(Instruction* this,int i)
{ 
    return get_variable_stack_offset(this,ins_get_operand(this,i));
}

/**
 * @brief   根据变量状态表获取内存偏移值或寄存器编号
 * @author  Created by LGD on 20221214
 * @update  20221224 将哈希表更新为64s
 *          20221224 方便观察VarSpace
 *          20230305 根据VarInfo新的设计修改
*/
size_t get_variable_register_order_or_memory_offset_test(Instruction* this,Value* var)
{ 
    if(var_is_return_val(var))return R0;
    VarInfo* vs = variable_map_get_value(this->map,var);
    if(variable_is_in_memory(this,var))
    {
        return vs->ori.addtion;
    }
    else
        return vs->ori.oprendVal;
    
}

/**
 * @brief 获取变量原始存放的操作数单元
 * @birth: Created by LGD on 20230305
*/
AssembleOperand get_variable_origin_operand(Instruction* this,Value* var)
{
    VarInfo* vi = variable_map_get_value(this->map,var);
    return vi->ori;
}

/**
 * @brief 获取变量目前存放的操作数单元
 * @birth: Created by LGD on 20230305
*/
AssembleOperand get_variable_current_operand(Instruction* this,Value* var)
{
    VarInfo* vi = variable_map_get_value(this->map,var);
    return vi->current;
}

/**
 * @brief 将寄存器位置转换为对应的寻址方式
 * @birth: Created by LGD on 20230305
*/
AddrMode judge_addrMode_by_place(RegorMem place)
{
    switch(place)
    {
        case IN_MEMORY:
            return REGISTER_INDIRECT_WITH_OFFSET;
        case IN_REGISTER:
            return REGISTER_DIRECT;
        case IN_INSTRUCTION:
            return IMMEDIATE;
        case UNALLOCATED:
            return NONE_ADDRMODE;
    }
}

/**
 * @brief 设置变量在当前指令的变量状态表中的位置
 * @author Created by LGD on 20230108
 * @update: 20230305 根据VarInfo新的设计修改
*/
void set_variable_place(Instruction* this,Value* var,RegorMem place)
{
    VarInfo* vs = variable_map_get_value(this->map,var);
    vs->ori.addrMode = judge_addrMode_by_place(place);
}

/**
 * @brief 设置变量在当前指令的变量状态表中的具体的偏移值
 * @author Created by LGD on 20230108
 * @update: 20230305 根据VarInfo新的设计修改
*/
void set_variable_register_order_or_memory_offset_test(Instruction* this,Value* var,int order)
{
    VarInfo* vs = variable_map_get_value(this->map,var);
    if(variable_is_in_memory(this,var))
    {
        vs->ori.addtion = order;
    }
    else
        vs->ori.oprendVal = order;
}

/**
 * @brief 在map中设置变量当前的寄存器编号
 * @birth: Created by LGD on 20230305
*/
void set_variable_register_order(HashMap* map,Value* var,RegisterOrder reg_order)
{
    VarInfo* vi = variable_map_get_value(map,var);
    vi->ori.oprendVal = reg_order;
}

/**
 * @brief 设置变量在当前指令的寄存器编号
 * @birth: Created by LGD on 20230305
 * @update: 2023-3-26 更改了基本方法
*/
void ins_set_variable_register_order(Instruction* this,Value* var,RegisterOrder reg_order)
{
    set_variable_register_order(this->map,var,reg_order);
}

/**
 * @brief 设置变量当前的寄存器编号
 * @birth: Created by LGD on 20230305
*/
void ins_set_variable_register_order_by_order(Instruction* this,int i,RegisterOrder reg_order)
{
    set_variable_register_order(this,ins_get_operand(this,i),reg_order);
}

/**
 * @brief 设置变量在指定map的栈帧偏移
 * @birth: Created by LGD on 2023-3-26
 * @update: 2023-4-4 将SP改为栈帧FP
*/
void set_variable_stack_offset(HashMap* map,Value* var,size_t offset)
{
    VarInfo* vi = variable_map_get_value(map,var);
    vi->ori.oprendVal = FP;
    vi->ori.addtion = offset;
}

/**
 * @brief 设置变量在当前指令的栈帧偏移
 * @birth: Created by LGD on 20230305
 * @update: 2023-3-26 更改了基础方法
*/
void ins_set_variable_stack_offset(Instruction* this,Value* var,size_t offset)
{
    set_variable_stack_offset(this->map,var,offset);
    // VarInfo* vi = variable_map_get_value(this->map,var);
    // vi->ori.oprendVal = SP;
    // vi->ori.addtion = offset;
}

/**
 * @brief 设置变量在当前指令的栈帧偏移
 * @birth: Created by LGD on 20230305
*/
void ins_set_variable_stack_offset_by_order(Instruction* this,int i,size_t offset)
{
    ins_set_variable_stack_offset(this,ins_get_operand(this,i),offset);
}





/**
 * @brief 重置变量的VarSpace信息
 * @author Created by LGD on 20230109
 * @update: 20230305 根据VarInfo新的设计修改
*/
void reset_var_info(VarInfo* var_info)
{
    var_info->ori.addrMode = NONE_ADDRMODE;
    var_info->ori.oprendVal = 0;
    var_info->ori.addtion = 0;
}
/**
 * @brief 重置变量的VarInfo
 * @author Created by LGD on 20230109
 * @update: 20230305 根据VarInfo新的设计修改
*/
void ins_reset_var_info(Instruction* this,Value* var)
{
    VarInfo* var_info = variable_map_get_value(this->map,var);
    reset_var_info(var_info);
}

/**
 * @brief 根据指定的序号遍历对应的函数，将变量左值全部装入变量信息表，并初始化其信息，同时计算变量的总大小
 * @author Created by LGD on 20230109
 * @param this 指令链表
 * @param order 要遍历第几个函数，以FuncLabelOP作为分割依据
 * @param map 传入一个map的指针，填装满的map将返回
 * @return 计算的栈帧大小
 * @update: 20230109 将到达指定函数入口的方法进行了封装调用
 *          2023-3-26 更新，如果map的值为NULL则初始化，否则为当前map进行增量添加，如果为当前map提供了一个非对应一个HashMap的未定义值，则会发生非法页错误
 *          2023-3-26 更新 使对map的插入键值对受到条件编译的选择
 *          2023-5-3 对于一个数组，其内存单元空间和其首地址均会保存一份
 *          2023-5-3 在这个版本中，traverse_list_and_count_total_size_of_var只负责计算栈大小
*/
int CompareKeyByName(void* dst,void* src){return !strcmp(dst,src);}
void RecordMapCleanKey(void* key){}
void RecordMapCleanValue(void* value){}

bool name_is_parameter(char* name)
{
    bool result = !memcmp(name,"param",5);
    return result;
}

/**
 * @brief 通过名字判断一个变量是否是全局变量
 * @update: Created by LGD on 2023-5-29
*/
bool name_is_global(char* name)
{
    bool result = !memcmp(name,"global",6);
    return result;
}

/**
 * @brief 通过名字获取一个参数的序号
 * @birth: Created by LGD on 2023-6-6
*/
size_t get_parameter_idx_by_name(char* name)
{
    assert(name_is_parameter(name) && "is not a parameter");
    return name[5]-'0';
}

size_t traverse_list_and_count_total_size_of_var(List* this,int order)
{
    Instruction* p;
    size_t totalSize = 0;
    const HashMap* recordRepeatMap = NULL;
    if(!recordRepeatMap)
    {
        recordRepeatMap = HashMapInit();
        HashMapSetHash(recordRepeatMap,HashDjb2);
        HashMapSetCompare(recordRepeatMap,CompareKeyByName);
        HashMapSetCleanKey(recordRepeatMap,RecordMapCleanKey);
        HashMapSetCleanValue(recordRepeatMap,RecordMapCleanValue);
    }
    p = traverse_to_specified_function(this,order);

    Value* val = ins_get_assign_left_value(p);
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
            //2023-4-28
            case GetelementptrOP:
            //2023-5-1
            case LoadOP:
            {
                val = ins_get_assign_left_value(p);
                if(HashMapGet(recordRepeatMap,val->name)){continue;}
                HashMapPut(recordRepeatMap,val->name,(void*)true);
                totalSize += opTye2size(p->user.value.VTy->TID);
            }
            break;
            case AllocateOP:
            {
                val = ins_get_assign_left_value(p);
                if(name_is_parameter(val->name))continue;
                assert(!HashMapGet(recordRepeatMap,val->name) && "这个数组没有正确的分配空间");
                totalSize += val->pdata->array_pdata.total_member*4 + 4;
            }
            break;
        }
        
    }while(ListNext(this,&p) && ins_get_opCode(p)!=FuncLabelOP);
    return totalSize;
}

size_t get_parameter_order(char* name)
{
    return name[5] - '0';
}

/**
 * @brief 整合了开辟栈空间和寄存器分配
 * @birth: Created by LGD on 2023-5-3
*/
void traverse_list_and_allocate_for_variable(List* this,HashMap* zzqMap,HashMap** myMap)
{
    Instruction* p;
    size_t totalSize = 0;
    void* isFound;
    VarInfo* var_info;
    int arrayOffset;
    if(*myMap == NULL)
        variable_map_init(myMap);
    //p = traverse_to_specified_function(this,order);
    //@birth: 2023-5-9  为形式参数分配寄存器
    char* name;
    enum _LOCATION loc;
    ListFirst(this,false);
    ListNext(this,&p);
    HashMap_foreach(zzqMap,name,loc)
    {
        /* 已作寄存器分配的变量 实参 */
        isFound = HashMapGet(*myMap,name);
        if(isFound)break;
        //新建变量信息项
        var_info = (VarInfo*)malloc(sizeof(VarInfo));
        memset(var_info,0,sizeof(VarInfo));
#ifdef DEBUG_MODE
        printf("插入新的实参名：%s \n",name);
#endif
        HashMapPut(*myMap,name,var_info);

        //分配寄存器或内存单元
        if(*((enum _LOCATION*)HashMapGet(zzqMap,name)) == MEMORY)
        {
            int offset = request_new_local_variable_memory_unit(IntegerTyID);
            set_variable_stack_offset_by_name(*myMap,name,offset);
#ifdef DEBUG_MODE
            printf("%s分配了地址%d\n",name,offset);
#endif
        }
        else
        {
            int idx = *((enum _LOCATION*)HashMapGet(zzqMap,name));
            RegisterOrder reg_order = request_new_allocatble_register_by_specified_ids(idx);
            //为该变量(名)创建寄存器映射
            set_variable_register_order_by_name(*myMap,name,reg_order);
            //打印分配结果
#ifdef DEBUG
            printf("%s分配了寄存器%d\n",name,reg_order);    
#endif                
        }    
    }
    

    /* allocate 和 未作寄存器分配的变量 */
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
            case LogicOrOP:
            case LogicAndOP:
            {
                Value* val = ins_get_assign_left_value(p);
                //分配寄存器或内存单元
                //从zzq 的 变量表 获取变量信息
                enum _LOCATION* loc = (enum _LOCATION*)HashMapGet(zzqMap,val->name);
                if(loc == NULL)
                {
                    //当前变量不存在寄存器分配的结果，分配UNALLOCATE
                    //assert(false && "No Attributed Result");
                    set_variable_unallocated_by_name(*myMap,val->name,false);
                    continue;
                }
            }
            break;
            //为数组也分配空间
        }
    }while(ListNext(this,&p) && ins_get_opCode(p)!=FuncLabelOP);
     
}

/**
 * @brief 依据静态寄存器分配表进行寄存器分配
 * @birth: Created by LGD on 2023-5-14
*/
void traverse_list_and_do_static_register_distribute(List* insList,HashMap* zzqMap)
{

}

/**
 * @brief 这个函数接受一个变量信息表，并将所有的参数传递到它在例程的活动记录被访问的位置
 * @birth: Created by LGD on 2023-5-13
 * @update: 2023-5-16 添加对数组指针访问的重定向
*/
void move_parameter_to_recorded_place(HashMap* varMap)
{
    char* name;
    VarInfo* varInfo;
    HashMap_foreach(varMap,name,varInfo)
    {
        if(name_is_parameter(name))
        {
            //该参数使用相对FP偏移的方式获取存储位置
            if(varInfo->ori.addrMode == REGISTER_INDIRECT_WITH_OFFSET)
            {

            }
            movii(varInfo->ori,r027[get_parameter_order(name)]);  
        }
    }
}

/**
 * @brief 统计当前函数使用的所有R4-R12间的所有寄存器
 * @birth: Created by LGD on 2023-5-13
 * @update:2023-5-14 添加了检查重复项
*/
struct _operand* count_register_change_from_R42R12(HashMap* register_attribute_map,struct _operand* reg_list,size_t* list_size)
{
    char* name;
    VarInfo* reg;
    size_t idx = 0;
    if (reg_list == NULL)
    {
        reg_list = malloc(9*sizeof(struct _operand)); 
        memset(reg_list,0,9*sizeof(struct _operand));
    }

    HashMap_foreach(register_attribute_map,name,reg)
    {
        if(operand_is_via_r4212(reg->ori))
        {
            bool already_included = 0;
            for(int i =0;i<9;i++)
            {
                if(operand_is_same(reg_list[i],reg->ori))
                    already_included = 1;
            }
            if(already_included)continue;
            reg_list[idx] = reg->ori;
            ++idx;
        }
    }
    reg_list[idx] = nullop;
    *list_size = idx*4;
}

/**
 * @brief 打印每个指令变量信息表的格式化输出子方法
 * @birth: Created by LGD on 20230306
*/
void print_info_map(Instruction* p,bool print_info)
{
        Value* var;
        VarInfo* var_info;
        size_t cnt = 0;
        HashMap_foreach(p->map,var,var_info)
        {
            ++cnt;
        }

        HashMap_foreach(p->map,var,var_info)
        {
            if(var_info->isLive)
                printf("| %10s ","Live");
            else
                printf("| %10s ","UnLive");
        }

        printf("\t%s",op_string[ins_get_opCode(p)]);

        printf("\n");
        for(int i=0;i<cnt;++i)
            printf("-------------");
        printf("\n");


        if(print_info)
        {
            HashMap_foreach(p->map,var,var_info)
            {
#ifdef VARIABLE_MAP_BASE_ON_NAME
                printf("| %10s ",AddrMode_2_str(var_info->ori.addrMode));
#elif defined VARIABLE_MAP_BASE_ON_VALUE_ADDRESS
                printf("| %10s ",RegOrMem_2_str(get_variable_place(p,var)));
#endif
            }
            printf("\n");
            for(int i=0;i<cnt;++i)
                printf("-------------");
            printf("\n");
            HashMap_foreach(p->map,var,var_info)
            {
#ifdef VARIABLE_MAP_BASE_ON_VALUE_ADDRESS
                if(get_variable_place(p,var)==IN_REGISTER)
                    printf("| R%-9d ",get_variable_register_order(p,var));
                else
                    printf("| %10d ",get_variable_stack_offset(p,var));
#elif defined VARIABLE_MAP_BASE_ON_NAME
                if(var_info->ori.addrMode == REGISTER_DIRECT || var_info->ori.addrMode == REGISTER_INDIRECT_WITH_OFFSET)
                    printf("| R%-9d ",var_info->ori.oprendVal);
#endif
            }

            for(int j=0;j<2;j++)
            {
                printf("\n");
                for(int i=0;i<cnt;++i)
                    printf("-------------");
                printf("\n");
            }
            
        }
}

/**
 * @brief  打印单个变量信息表
 * @birth: Created by LGD on 20230306
*/
void print_single_info_map(List* this,int order,bool print_info)
{

    Instruction* p = traverse_to_specified_function(this,order);
    Value* var;
    VarInfo* var_info;
    size_t cnt = 0;
    HashMap_foreach(p->map,var,var_info)
    {
        ++cnt;
    }
    for(int i=0;i<cnt;++i)
        printf("-------------");
    printf("\n");
    HashMap_foreach(p->map,var,var_info)
    {
       printf("| %10s ",var->name);
    }
    printf("\n");
    for(int i=0;i<cnt;++i)
        printf("-------------");
    printf("\n");

    print_info_map(p,print_info);
    
}

/**
 * @brief  打印单个变量信息表
 * @birth: Created by LGD on 20230306
*/
void print_single_info_map_map(List* this,int order,bool print_info)
{

    Instruction* p = traverse_to_specified_function(this,order);
    Value* var;
    VarInfo* var_info;
    size_t cnt = 0;
    HashMap_foreach(p->map,var,var_info)
    {
        ++cnt;
    }
    for(int i=0;i<cnt;++i)
        printf("-------------");
    printf("\n");
    HashMap_foreach(p->map,var,var_info)
    {
       printf("| %10s ",var->name);
    }
    printf("\n");
    for(int i=0;i<cnt;++i)
        printf("-------------");
    printf("\n");

    print_info_map(p,print_info);
    
}

/**
 * @brief   打印一个函数的变量状态表
 * @param   order 函数的序号
 * @param   print_info 若为假，仅打印活跃信息；若为真，将打印变量的全部信息，包括存储空间和偏移量
 * @update: 20230305 根据VarInfo新的设计修改
 * @update: 2023-4-18 基于名字的查表方式的新修改
*/
void print_list_info_map(List* this,int order,bool print_info)
{

    Instruction* p = traverse_to_specified_function(this,order);
    Value* var;
    VarInfo* var_info;
    size_t cnt = 0;
    HashMap_foreach(p->map,var,var_info)
    {
        ++cnt;
    }
    for(int i=0;i<cnt;++i)
        printf("-------------");
    printf("\n");
    HashMap_foreach(p->map,var,var_info)
    {
#ifdef VARIABLE_MAP_BASE_ON_NAME
       printf("| %10s ",var);
#elif defined VARIABLE_MAP_BASE_ON_VALUE_ADDRESS
        printf("| %10s ",var->name);
#endif
    }
    printf("\n");
    for(int i=0;i<cnt;++i)
        printf("-------------");
    printf("\n");

    do{ 
        print_info_map(p,print_info);
    }while((p = get_next_instruction(this)) && ins_get_opCode(p)!=FuncLabelOP);
    
}



/**
 * @brief 将存储位置枚举类型转化为字符串
 * @author LGD
 * @date 20230109
*/
char* RegOrMem_2_str(RegorMem place)
{
    switch(place)
    {
        case IN_MEMORY:
            return "MEMORY";
        case IN_INSTRUCTION:
            return "INSTRUCT";
        case IN_REGISTER:
            return "REG";
        case UNALLOCATED:
            return "UNALLOCATE";
    }
}

/**
 * @brief 将寄存器编号转换成字符串
 * @author LGD
 * @date 20230109
*/
char* Register_order_2_str(int order)
{
    char* str = (char*)malloc(sizeof(char)*3);
    sprintf(str,"R%d",order);
    return str;
}

/**
 * @brief 将寻址方式转换为字符串
*/
char* AddrMode_2_str(enum _AddrMode addrMode)
{
    switch(addrMode)
    {
        case IMMEDIATE:
            return "IMME";
        case REGISTER_DIRECT:
            return "REG";
        case REGISTER_INDIRECT_WITH_OFFSET:
            return "MEM";
    }
}

