#include "Pass.h"
#include "cds.h"
#include "function.h"

static const int REGISTER_NUM = 3;
typedef enum _LOCATION { R1 = 1, R2, R3, MEMORY } LOCATION;
static char *location_string[] = {"null", "R1", "R2", "R3", "M"};

void register_replace(Function *handle_func) {
  HashMap *var_location = handle_func->var_localtion;
  ALGraph *self_cfg = handle_func->self_cfg;

#ifdef PRINT_OK
  Pair *ptr_pair;
  HashMapFirst(var_location);
  while ((ptr_pair = HashMapNext(var_location)) != NULL) {
    printf("\tvar:%s\taddress:%s\n ", (char *)ptr_pair->key,
           location_string[(LOCATION)(intptr_t)ptr_pair->value]);
  }
#endif

Label(handle_func->label->name);

  //第一次function遍历，遍历所有的变量计算栈帧大小并将变量全部添加到变量信息表
  //遍历每一个block的list
  size_t totalLocalVariableSize = 0;
  HashMap* VariableInfoMap = NULL;

  //翻译前初始化
  //2023-5-3 初始化前移到这个位置，因为分配内存时有可能需要为数组首地址提供存放的寄存器
  InitBeforeFunction();

  //计算栈帧大小（局部变量作用域部分）
  totalLocalVariableSize = getLocalVariableSize(self_cfg);

  //记录局部变量域的大小
  currentPF.local_variable_size = totalLocalVariableSize;

  //初始化函数栈帧
  new_stack_frame_init(totalLocalVariableSize);

  //2023-5-22 这决定了局部变量区间的累计偏移值
  currentPF.fp_offset -= totalLocalVariableSize;

  //堆栈LR寄存器和R7
  bash_push_pop_instruction("PUSH",&fp,&lr,END);
  //2023-5-22 对R7和LR的保存也要计算在偏移值内
  currentPF.fp_offset -= 2*4;
  
  //设置当前函数栈帧
  set_stack_frame_status(0,totalLocalVariableSize/4);

  //根据传递参数的个数修正FP的偏移值
  size_t param_num = func_get_param_numer(self_func);
  if(param_num > 4)
    currentPF.fp_offset -= (param_num-4)*4;
  
  
  variable_map_init(&VariableInfoMap);

  //变量信息表转换  
  //包括对 局部变量 局部数组指针 参数的地址分配
  for (int i = 0; i < self_cfg->node_num; i++) {
    int iter_num = 0;
    ListFirst((self_cfg->node_set)[i]->bblock_head->inst_list,false);
    traverse_list_and_allocate_for_variable((self_cfg->node_set)[i]->bblock_head->inst_list,var_location,&VariableInfoMap); 
  }

  //统计当前函数使用的所有R4-R12的通用寄存器
  //前提 已经完成寄存器分配
  size_t used_reg_size = 0;
  count_register_change_from_R42R12(VariableInfoMap,currentPF.used_reg,&used_reg_size);
  //保存现场
  bash_push_pop_instruction_list("PUSH",currentPF.used_reg);

  //记录保护现场域的大小
  currentPF.env_protected_size = used_reg_size + 8;

  //2023-5-22 这决定了现场保护区域FP的偏移值
  currentPF.fp_offset -= used_reg_size;

  //为所有参数设置初始位置
  set_param_origin_place(VariableInfoMap,param_num);
  
  //为大于4的参数分配空间
  // char* name;
  // VarInfo* varInfo;
  // HashMap_foreach(VariableInfoMap,name,varInfo)
  // {
  //   if(name_is_parameter(name))
  //   {
  //       int idx;
  //       if(((idx =get_parameter_idx_by_name(name)) >= 4) && operand_is_in_memory(varInfo->ori))
  //       {
  //           int offset = get_param_stack_offset_by_idx(idx);
  //           set_variable_stack_offset_by_name(VariableInfoMap,name,offset);
  //       }
  //   }
  // }
  Instruction* element = NULL;
  //第二次function遍历，为每一句Instruction安插一个map
  for (int i = 0; i < self_cfg->node_num; i++) {
    int iter_num = 0;
    ListFirst((self_cfg->node_set)[i]->bblock_head->inst_list,false);
    while(ListNext((self_cfg->node_set)[i]->bblock_head->inst_list,&element))
    ins_deepSet_varMap(element,VariableInfoMap);
  }

  //执行期间使指针变动生效
  update_sp_value();

  //使当前R7与SP保持一致
  general_data_processing_instructions(MOV,fp,nullop,sp,NONESUFFIX,false);

  //为数组分配空间并装载到基址存储位置
  //前提 所有的指令都分配了变量信息表
  for (int i = 0; i < self_cfg->node_num; i++) {
    int iter_num = 0;
    ListFirst((self_cfg->node_set)[i]->bblock_head->inst_list,false);
    traverse_and_load_arrayBase_to_recorded_place((self_cfg->node_set)[i]->bblock_head->inst_list); 
  }
  
  // HashMap_foreach(VariableInfoMap, name, varINfo)
  // {
  //   printf("%s %p\n",name,varINfo);
  // }
  //传递参数
  move_parameter_to_recorded_place(VariableInfoMap,param_num);


  //第三次function遍历，翻译每一个list
  for (int i = 0; i < self_cfg->node_num; i++) {
    ListFirst((self_cfg->node_set)[i]->bblock_head->inst_list,false);
    traverse_list_and_translate_all_instruction((self_cfg->node_set)[i]->bblock_head->inst_list,0);
  }

  //恢复SP
  reset_sp_value(true);
  //恢复现场
  bash_push_pop_instruction_list("POP",currentPF.used_reg);
  //退出函数
  bash_push_pop_instruction("POP",&fp,&pc,END);

  
}
