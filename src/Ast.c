#include "Ast.h"
#include "c_container_auxiliary.h"
#include "container/hash_map.h"
#include "container/list.h"
#include "container/stack.h"
#include "type.h"
#include "value.h"

#include <math.h>
#include <stdarg.h> //变长参数函数所需的头文件
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern Stack *stack_ast_pre;
extern Stack *stack_symbol_table;
extern Stack *stack_else_label;
extern Stack *stack_then_label;
extern Stack *stack_while_head_label; // while循环头(条件判断)
extern Stack *stack_while_then_label; // while条件为false时所要跳转的label栈
extern Stack *stack_param;
static ast *pre_astnode = NULL;
extern List *ins_list;
extern HashMap *func_hashMap;
extern SymbolTable *cur_symboltable;
extern HashMap *global_array_init_hashmap;
extern HashMap *constant_single_value_hashmap;
extern Stack *array_get;
extern bool is_functional_test;
extern bool global_optimization;

void CleanObject(void *element);

enum NowVarDecType { NowVoid, NowInt, NowFloat, NowStruct } nowVarDecType;

bool NowConst = false;

enum NameSeed {
  FUNC_LABEL_END,
  TEMP_VAR,
  FUNC_LABEL,
  LABEL,
  PARAM,
  PARAM_CONVERT,
  GLOBAL,
  ARRAY,
  POINT
};

#define N_OP_NUM 12
static char *NORMAL_OPERATOR[N_OP_NUM] = {
    "ERROROP", "PLUS",     "MINUS", "MOD",  "STAR",       "DIV",
    "EQUAL",   "NOTEQUAL", "GREAT", "LESS", "GREATEQUAL", "LESSEQUAL",
};

#define L_OP_NUM 2
static char *LOGIC_OPERATOR[L_OP_NUM] = {"AND", "OR"};

struct {
  int nest_levels;
  int array_level;
  int *added;
  int *array_info;
  bool is_init_array;
  Value *cur_init_array;
  bool is_empty;
  List *offset_list;
} array_init_assist;

#define ARRAY_DEREFERENCE 0
extern HashMap *assist_is_local_val;

static void array_init_assist_func(List *array_list, Value *cur_init_array) {
  if (array_list != NULL) {
    int list_size = ListSize(array_list);
    array_init_assist.nest_levels = 0;
    array_init_assist.array_level = list_size;
    array_init_assist.is_init_array = true;
    array_init_assist.cur_init_array = cur_init_array;
    array_init_assist.added = malloc(sizeof(int) * list_size);
    memset(array_init_assist.added, 0, list_size * sizeof(int));
    array_init_assist.array_info = malloc(sizeof(int) * list_size);
    array_init_assist.offset_list = ListInit();
    ListSetClean(array_init_assist.offset_list, CleanObject);

    ListFirst(array_list, true);
    void *element;
    int i = 0;
    while (ListReverseNext(array_list, &element))
      array_init_assist.array_info[i++] = (intptr_t)element;
  } else {
    array_init_assist.nest_levels = 0;
    array_init_assist.array_level = 0;
    array_init_assist.is_init_array = false;
    array_init_assist.cur_init_array = NULL;
    array_init_assist.offset_list = NULL;
    free(array_init_assist.added);
    array_init_assist.added = NULL;
    free(array_init_assist.array_info);
    array_init_assist.array_info = NULL;
  }
}

struct {
  Stack *and_stack;
  Stack *or_stack;
  Value *true_label;
  Value *false_label;
  // bool is_if_init;
} logic_goto_assist;

static void logic_goto_assist_func(Value *true_label, Value *false_label) {
  if (true_label != NULL) {
    logic_goto_assist.and_stack = StackInit();
    StackSetClean(logic_goto_assist.and_stack, CleanObject);
    logic_goto_assist.or_stack = StackInit();
    StackSetClean(logic_goto_assist.or_stack, CleanObject);
    logic_goto_assist.true_label = true_label;
    logic_goto_assist.false_label = false_label;
    // logic_goto_assist.is_if_init = true;
  } else {
    StackDeinit(logic_goto_assist.and_stack);
    StackDeinit(logic_goto_assist.or_stack);
    logic_goto_assist.true_label = NULL;
    logic_goto_assist.false_label = NULL;
    logic_goto_assist.and_stack = NULL;
    logic_goto_assist.or_stack = NULL;
    // logic_goto_assist.is_if_init = false;
  }
}

static char *NowVarDecStr[] = {"void", "int", "float", "struct"};

static int temp_var_seed = 10;  // 用于标识变量的名字
static int label_var_seed = 1;  // 用于标识label的名字
static int label_func_seed = 1; // 用于表示func_label的名字
static int point_seed = 1;      // 用于表示指针变量名 用于alloca
static int array_seed = 1;      // 用于表示数组变量名 用于alloca
static int global_seed = 1;     // 用于表示全局变量
static int param_seed = 0;      // 用于标识函数参数的个数
static int total_array_member = 1;
static List *array_list = NULL;
static List *param_type_list = NULL;
static Value *cur_construction_func = NULL;
static char *cur_handle_func = NULL;

char *name_generate(enum NameSeed cur_handle) {
  char temp_str[50];
  switch (cur_handle) {
  case TEMP_VAR:
    sprintf(temp_str, "temp%d", temp_var_seed++);
    break;
  case FUNC_LABEL:
    sprintf(temp_str, "func_label%d", label_func_seed++);
    break;
  case FUNC_LABEL_END:
    sprintf(temp_str, "func_label_end%d", label_func_seed++);
    break;
  case LABEL:
    sprintf(temp_str, "%slabel%d", cur_handle_func, label_var_seed++);
    break;
  case PARAM:
    sprintf(temp_str, "param%d", param_seed++);
    break;
  case PARAM_CONVERT:
    sprintf(temp_str, "param%d", --param_seed);
    break;
  case GLOBAL:
    sprintf(temp_str, "global%d", global_seed++);
    break;
  case ARRAY:
    sprintf(temp_str, "array%d", array_seed++);
    break;
  case POINT:
    sprintf(temp_str, "point%d", point_seed++);
    break;
  default:
    break;
  }
  return strdup(temp_str);
}

// 判断当前if是否含有else
bool have_else = false;

ast *newast(char *name, int num, ...) // 抽象语法树建立
{
  va_list valist;                      // 定义变长参数列表
  ast *a = (ast *)malloc(sizeof(ast)); // 新生成的父节点
  a->l = NULL;
  a->r = NULL;
  ast *temp; // 修改为局部指针而不分配所指向的内存空间
  if (!a) {
    yyerror("out of space");
    exit(0);
  }
  a->name = name;        // 语法单元名字
  va_start(valist, num); // 初始化变长参数为num后的参数

  if (num > 0) // num>0为非终结符：变长参数均为语法树结点，孩子兄弟表示法
  {
    temp = va_arg(valist,
                  ast *); // 取变长参数列表中的第一个结点设为a的左孩子
    a->l = temp;
    a->line = temp->line; // 父节点a的行号等于左孩子的行号

    if (num >= 2) // 可以规约到a的语法单元>=2
    {
      for (int i = 0; i < num - 1;
           ++i) // 取变长参数列表中的剩余结点，依次设置成兄弟结点
      {
        temp->r = va_arg(valist, ast *);
        temp = temp->r;
      }
      temp->r = NULL;
    }
  } else // num==0为终结符或产生空的语法单元：第1个变长参数表示行号，产生空的语法单元行号为-1。
  {
    int t = va_arg(valist, int); // 取第1个变长参数
    a->line = t;
    if ((SEQ(a->name, "ID")) ||
        (SEQ(a->name, "TYPE"))) //"ID,TYPE,INTEGER，借助union保存yytext的值
    {
      char *t = strdup(yytext);
      a->idtype = t;
    } else if (SEQ(a->name, "INTEGER")) {
      a->intgr = atoi(yytext);
    } else if (SEQ(a->name, "FLOAT")) {
      a->flt = atof(yytext);
    } else if (SEQ(a->name, "OCT_INT")) {
      a->name = "INTEGER";
      a->intgr = strtol(yytext, NULL, 8);
    } else if (SEQ(a->name, "HEX_INT")) {
      a->name = "INTEGER";
      a->intgr = strtol(yytext, NULL, 16);
    } else if (SEQ(a->name, "SCI_INT")) {
      a->name = "INTEGER";
      a->intgr = (int)strtod(yytext, NULL);
    } else if (SEQ(a->name, "SCI_FLOAT")) {
      a->name = "FLOAT";
      a->flt = strtof(yytext, NULL);
    } else if (SEQ(a->name, "HEX_FLOAT")) {
      a->name = "FLOAT";
      a->flt = (float)strtod(yytext, NULL);
    } else {
    }
  }
  return a;
}

void eval_print(ast *a, int level) {
  if (a != NULL) {
    for (int i = 0; i < level; ++i) // 孩子结点相对父节点缩进2个空格
      printf("  ");
    if (a->line != -1) { // 产生空的语法单元不需要打印信息
      printf("%s ",
             a->name); // 打印语法单元名字，ID/TYPE/INTEGER要打印yytext的值
      if ((SEQ(a->name, "ID")) || (SEQ(a->name, "TYPE")))
        printf(":%s ", a->idtype);
      else if (SEQ(a->name, "INTEGER"))
        printf(":%d ", a->intgr);
      else
        printf("(%d)", a->line);
    } else {
      printf("%s ",
             a->name); // 打印语法单元名字，ID/TYPE/INTEGER要打印yytext的值
    }

    // if (SEQ(a->name, "SEMI")) {
    //   printf("     free a node\n");
    //   free(a);
    //   a = NULL;
    //   return NULL;
    // }
    printf("\n");

    eval_print(a->l, level + 1); // 遍历左子树
    eval_print(a->r, level);     // 遍历右子树
  }
}

void pre_eval(ast *a) {
  if (a != NULL) {

    if (SEQ(a->name, "IF")) {
      // 创建false条件下的label标签
      char *temp_str = NULL;

      temp_str = name_generate(LABEL);
      Value *else_label_ins = (Value *)ins_new_no_operator_v2(LabelOP);
      else_label_ins->name = temp_str;
      else_label_ins->VTy->TID = LabelTyID;
      StackPush(stack_else_label, else_label_ins);

      temp_str = name_generate(LABEL);
      Value *true_label_ins = (Value *)ins_new_no_operator_v2(LabelOP);
      true_label_ins->name = temp_str;
      true_label_ins->VTy->TID = LabelTyID;

      logic_goto_assist_func(true_label_ins, else_label_ins);
    }

    if (SEQ(a->name, "WHILE")) {
      // 创建while这条语句的label，用于返回循环头
      char *temp_str = name_generate(LABEL);

      Value *while_head_label_ins = (Value *)ins_new_no_operator_v2(LabelOP);
      while_head_label_ins->name = strdup(temp_str);
      while_head_label_ins->VTy->TID = LabelTyID;
      StackPush(stack_while_head_label, while_head_label_ins);

      char buffer[80];
      // 创建llvm 模式goto语句
      Value *goto_label_ins = (Value *)ins_new_no_operator_v2(GotoOP);
      strcpy(buffer, "goto ");
      strcat(buffer, while_head_label_ins->name);
      goto_label_ins->name = strdup(buffer);
      goto_label_ins->VTy->TID = GotoTyID;
      goto_label_ins->pdata->no_condition_goto.goto_location =
          while_head_label_ins;
#ifdef DEBUG_MODE
      printf("br %s\n", while_head_label_ins->name);
      printf("%s\n", while_head_label_ins->name);
#endif
      // 插入跳转语句
      ListPushBack(ins_list, goto_label_ins);
      // 插入while循环头的label
      ListPushBack(ins_list, while_head_label_ins);

      Value *while_true_label_ins = (Value *)ins_new_no_operator_v2(LabelOP);
      while_true_label_ins->name = name_generate(LABEL);
      while_true_label_ins->VTy->TID = LabelTyID;

      Value *while_false_label_ins = (Value *)ins_new_no_operator_v2(LabelOP);
      while_false_label_ins->name = name_generate(LABEL);
      while_false_label_ins->VTy->TID = LabelTyID;
      StackPush(stack_while_then_label, while_false_label_ins);

      logic_goto_assist_func(while_true_label_ins, while_false_label_ins);
    }

    if (a->r && SEQ(a->r->name, "AND")) {
      char *temp_str = name_generate(LABEL);
      Value *and_label_ins = (Value *)ins_new_no_operator_v2(LabelOP);
      and_label_ins->name = temp_str;
      and_label_ins->VTy->TID = LabelTyID;
      StackPush(logic_goto_assist.and_stack, and_label_ins);
    }

    if (a->r && SEQ(a->r->name, "OR")) {
      char *temp_str = name_generate(LABEL);
      Value *or_label_ins = (Value *)ins_new_no_operator_v2(LabelOP);
      or_label_ins->name = temp_str;
      or_label_ins->VTy->TID = LabelTyID;
      StackPush(logic_goto_assist.or_stack, or_label_ins);
    }

    if (SEQ(a->name, "FunDec")) {
      // 新建一个符号表用于存放参数
      cur_symboltable = (SymbolTable *)malloc(sizeof(SymbolTable));
      symbol_table_init(cur_symboltable);

      Value *func_label_ins = (Value *)ins_new_no_operator_v2(FuncLabelOP);
      // 添加变量的名字
      func_label_ins->name = strdup(a->l->idtype);
      func_label_ins->VTy->TID = FuncLabelTyID;
      func_label_ins->pdata->symtab_func_pdata.return_type = (int)nowVarDecType;
      param_type_list = ListInit();
      ListSetClean(param_type_list, CleanObject);
      cur_construction_func = func_label_ins;

      // 插入
      ListPushBack(ins_list, (void *)func_label_ins);

      if (cur_handle_func != NULL)
        free(cur_handle_func);
      cur_handle_func = strdup(a->l->idtype);

#ifdef DEBUG_MODE
      printf("Func: %s\n", a->l->idtype);
#endif

      // 将函数的<name,label>插入函数表
      HashMapPut(func_hashMap, strdup(a->l->idtype), func_label_ins);

      Value *entry_label_ins = (Value *)ins_new_no_operator_v2(LabelOP);

      // 添加变量的名字
      char entry_label_name[50];
      sprintf(entry_label_name, "%sentry", cur_handle_func);
      entry_label_ins->name = strdup(entry_label_name);
      entry_label_ins->VTy->TID = LabelTyID;

      ListPushBack(ins_list, entry_label_ins);
    }

    if (SEQ(a->name, "LC")) {
      if (array_init_assist.is_init_array) {
        array_init_assist.nest_levels++;
        array_init_assist.is_empty = true;
      } else {
        cur_symboltable = (SymbolTable *)malloc(sizeof(SymbolTable));
        symbol_table_init(cur_symboltable);
      }
    }

    if (SEQ(a->name, "RC")) {
      StackPop(stack_symbol_table);
      // 销毁当前的符号表中的哈希表然后销毁符号表
      HashMapDeinit(cur_symboltable->symbol_map);
      free(cur_symboltable);
      cur_symboltable = NULL;
      // 当前的符号表恢复到上一级的符号表
      StackTop(stack_symbol_table, (void **)&cur_symboltable);
    }

    if (SEQ(a->name, "ELSE")) {
      Value *else_label_ins = NULL;
      StackTop(stack_else_label, (void **)&else_label_ins);
      StackPop(stack_else_label);
      ListPushBack(ins_list, (void *)else_label_ins);
#ifdef DEBUG_MODE
      printf("%s\n", else_label_ins->name);
#endif
    }

    // 全局变量的符号表
    if (SEQ(a->name, "Program") && StackSize(stack_symbol_table) == 0) {
      cur_symboltable = (SymbolTable *)malloc(sizeof(SymbolTable));
      symbol_table_init(cur_symboltable);
    }

    if (SEQ(a->name, "assistELSE")) {
      Value *then_label_ins = (Value *)ins_new_no_operator_v2(LabelOP);

      // 添加变量的名字
      then_label_ins->name = name_generate(LABEL);
      then_label_ins->VTy->TID = LabelTyID;

      StackPush(stack_then_label, then_label_ins);

      // printf("new instruction destination %s and push to the then_stack\n",
      //        then_label->name);

      // cur_symboltable->symbol_map->put(cur_symboltable->symbol_map,
      //                                  strdup(temp_str), goto_else);

      Value *goto_else_ins = (Value *)ins_new_no_operator_v2(GotoOP);
      char temp_str[50];
      strcpy(temp_str, "goto ");
      strcat(temp_str, then_label_ins->name);
      goto_else_ins->name = strdup(temp_str);
      goto_else_ins->VTy->TID = GotoTyID;
      goto_else_ins->pdata->no_condition_goto.goto_location = then_label_ins;

      ListPushBack(ins_list, (void *)goto_else_ins);

#ifdef DEBUG_MODE
      printf("br %s \n", then_label_ins->name);
#endif
    }
  }
}

void in_eval(ast *a, Value *left) {
  // store the para
  if (SEQ(a->name, "ParamDec")) {
    if (left->VTy->TID != ArrayTyID) {
      // 新建一个符号表用于存放参数
      // 在内存中为变量分配空间
      Value *cur_var = (Value *)malloc(sizeof(Value));
      value_init(cur_var);
      // 添加变量类型
      cur_var->VTy->TID = (int)nowVarDecType;
      ListPushBack(param_type_list, (void *)(intptr_t)cur_var->VTy->TID);
      // 添加变量的名字
      cur_var->name = name_generate(PARAM);
      // 返回指针
      Value *store_ins =
          (Value *)ins_new_binary_operator_v2(StoreOP, left, cur_var);
      store_ins->IsInitArgs = 1;
      store_ins->pdata->param_init_pdata.the_param_index = param_seed;
#ifdef DEBUG_MODE
      printf("store %s %s, %s,align 4\n",
             NowVarDecStr[cur_var->VTy->TID < 4 ? cur_var->VTy->TID
                                                : cur_var->VTy->TID - 4],
             cur_var->name, left->name);
#endif
      ListPushBack(ins_list, store_ins);
      return;
    } else {
      ListPushBack(param_type_list, (void *)(intptr_t)ArrayTyID);
      free(left->name);
      // 添加变量的名字
      left->name = name_generate(PARAM);
      // //参数不能作为局部数组?
      // left->pdata->array_pdata.is_local_array = 0;
      return;
    }
  }

  if (SEQ(a->name, "InitList") && a->l && SEQ(a->l->name, "Exp")) {
    if (left->VTy->TID == ImmediateIntTyID ||
        left->VTy->TID == ImmediateFloatTyID) {
      int cur_init_total_offset = array_init_assist.added[0];
      for (int i = 1; i < array_init_assist.array_level; i++) {
        int temp = array_init_assist.added[i];
        for (int j = i - 1; j >= 0; j--) {
          temp *= array_init_assist.array_info[j];
        }
        cur_init_total_offset += temp;
      }
      global_array_init_item *cur_init_offset =
          malloc(sizeof(global_array_init_item));

      cur_init_offset->offset = cur_init_total_offset;
      cur_init_offset->ival = left->pdata->var_pdata.iVal;
      cur_init_offset->fval = left->pdata->var_pdata.fVal;
      ListPushBack(array_init_assist.offset_list, cur_init_offset);
    } else {
      Value *outside_array = array_init_assist.cur_init_array;
      for (int i = array_init_assist.array_level - 1; i >= 0; i--) {
        Value *cur;
        char buffer[50];
        sprintf(buffer, "%d", array_init_assist.added[i]);
        if (HashMapContain(constant_single_value_hashmap, buffer)) {
          cur = HashMapGet(constant_single_value_hashmap, buffer);
        } else {
          cur = (Value *)malloc(sizeof(Value));
          value_init(cur);
          cur->VTy->TID = ImmediateIntTyID;
          cur->name = strdup(buffer);
          cur->pdata->var_pdata.iVal = array_init_assist.added[i];
          cur->pdata->var_pdata.fVal = (float)array_init_assist.added[i];
          HashMapPut(constant_single_value_hashmap, strdup(buffer), cur);
        }

        // oprand the array
        char *temp_str = name_generate(TEMP_VAR);
        Value *cur_ins = (Value *)ins_new_binary_operator_v2(
            GetelementptrOP, outside_array, cur);
        // 将数组的信息拷贝一份
        cur_ins->name = strdup(temp_str);
        cur_ins->VTy->TID = ArrayTyID;
        cur_ins->pdata->array_pdata.array_type =
            outside_array->pdata->array_pdata.array_type;
        cur_ins->pdata->array_pdata.list_para = ListInit();
        cur_ins->pdata->array_pdata.is_local_array = ARRAY_DEREFERENCE;
        Value *top_array = outside_array->pdata->array_pdata.top_array;
        cur_ins->pdata->array_pdata.top_array = top_array;
        ListSetClean(cur_ins->pdata->array_pdata.list_para, CleanObject);
        list_copy(cur_ins->pdata->array_pdata.list_para,
                  outside_array->pdata->array_pdata.list_para);
        cur_ins->pdata->array_pdata.total_member =
            outside_array->pdata->array_pdata.step_long;
        void *element;
        ListGetFront(cur_ins->pdata->array_pdata.list_para, &element);
        ListPopFront(cur_ins->pdata->array_pdata.list_para);
        cur_ins->pdata->array_pdata.step_long =
            cur_ins->pdata->array_pdata.total_member / (intptr_t)element;
        ListPushBack(ins_list, cur_ins);
        outside_array = cur_ins;
      }

      Value *assign_var = left;
      if (outside_array->pdata->array_pdata.array_type != left->VTy->TID &&
          outside_array->pdata->array_pdata.array_type != left->VTy->TID - 4) {
        assign_var = (Value *)ins_new_single_operator_v2(AssignOP, left);
        assign_var->VTy->TID = outside_array->pdata->array_pdata.array_type;
        assign_var->name = name_generate(TEMP_VAR);
        ListPushBack(ins_list, (void *)assign_var);
      }

      // store ready
      Value *store_ins = (Value *)ins_new_binary_operator_v2(
          StoreOP, outside_array, assign_var);
      ListPushBack(ins_list, (void *)store_ins);
    }

    array_init_assist.is_empty = false;
    array_init_assist.added[0]++;
    for (int i = 0; i < array_init_assist.array_level; i++) {
      if (array_init_assist.added[i] == array_init_assist.array_info[i]) {
        array_init_assist.added[i] = 0;
        array_init_assist.added[i + 1]++;
      } else {
        break;
      }
    }
  }

  if (SEQ(a->name, "VarDec")) {
    if (left->VTy->TID == ArrayTyID) {
      if (a->r == NULL || SEQ(a->r->name, "ASSIGNOP")) {
        // TODO init the array
        void *element;
        ListGetFront(array_list, &element);
        left->pdata->array_pdata.step_long =
            total_array_member / (intptr_t)element;

        if (a->r && SEQ(a->r->name, "ASSIGNOP"))
          array_init_assist_func(array_list, left);

        ListPopFront(array_list);
        element = (void *)(uintptr_t)1;
        ListPushBack(array_list, element);
        left->pdata->array_pdata.list_para = array_list;
        left->pdata->array_pdata.total_member = total_array_member;
        left->pdata->array_pdata.array_type = (int)nowVarDecType;
        left->pdata->array_pdata.is_local_array = 2;
        left->pdata->array_pdata.top_array = left;
        // hashmap_init(&left->pdata->array_pdata.local_array_hashmap);
        total_array_member = 1;
        array_list = NULL;
      }
    }
  }

  if (SEQ(a->name, "FunDec")) {
    cur_construction_func->pdata->symtab_func_pdata.param_num = param_seed;
    cur_construction_func->pdata->symtab_func_pdata.param_type_lists =
        malloc(sizeof(TypeID) * param_seed);
    ListFirst(param_type_list, false);
    void *element;
    int i = 0;
    while (ListNext(param_type_list, &element))
      cur_construction_func->pdata->symtab_func_pdata.param_type_lists[i++] =
          (TypeID)(intptr_t)element;
    ListDeinit(param_type_list);
    param_type_list = NULL;

    // 将参数的个数清零
    param_seed = 0;
  }

  // assistIF
  if (a->r && SEQ(a->r->name, "assistIF")) {
    Value *true_label_ins = logic_goto_assist.true_label;
    Value *else_label_ins = logic_goto_assist.false_label;
    if (left) {
      Value *goto_condition_ins =
          (Value *)ins_new_single_operator_v2(GotoWithConditionOP, left);

      char temp_br_label_name[80];
      strcpy(temp_br_label_name, "true:");
      strcat(temp_br_label_name, true_label_ins->name);
      strcat(temp_br_label_name, "  false:");
      strcat(temp_br_label_name, else_label_ins->name);

      goto_condition_ins->name = strdup(temp_br_label_name);
      goto_condition_ins->VTy->TID = GotoTyID;
      goto_condition_ins->pdata->condition_goto.true_goto_location =
          true_label_ins;
      goto_condition_ins->pdata->condition_goto.false_goto_location =
          else_label_ins;

      ListPushBack(ins_list, goto_condition_ins);
    }
    ListPushBack(ins_list, true_label_ins);
    logic_goto_assist_func(NULL, NULL);
  }

  if (a->r) {
    for (int i = 0; i < L_OP_NUM; i++) {
      if (SEQ(a->r->name, LOGIC_OPERATOR[i])) {
        if (left != NULL) {
          Value *goto_condition_ins =
              (Value *)ins_new_single_operator_v2(GotoWithConditionOP, left);
          Value *true_label_ins;
          Value *else_label_ins;
          if (StackSize(logic_goto_assist.and_stack) == 0)
            true_label_ins = logic_goto_assist.true_label;
          else
            StackTop(logic_goto_assist.and_stack, (void **)&true_label_ins);

          if (StackSize(logic_goto_assist.or_stack) == 0)
            else_label_ins = logic_goto_assist.false_label;
          else
            StackTop(logic_goto_assist.or_stack, (void **)&else_label_ins);

          char temp_br_label_name[80];
          strcpy(temp_br_label_name, "true:");
          strcat(temp_br_label_name, true_label_ins->name);
          strcat(temp_br_label_name, "  false:");
          strcat(temp_br_label_name, else_label_ins->name);

          goto_condition_ins->name = strdup(temp_br_label_name);
          goto_condition_ins->VTy->TID = GotoTyID;
          goto_condition_ins->pdata->condition_goto.true_goto_location =
              true_label_ins;
          goto_condition_ins->pdata->condition_goto.false_goto_location =
              else_label_ins;

          ListPushBack(ins_list, goto_condition_ins);
        }
        Instruction *insert_label = NULL;
        StackTop((Stack *)(*((intptr_t *)&logic_goto_assist + i)),
                 (void **)&insert_label);
        StackPop((Stack *)(*((intptr_t *)&logic_goto_assist + i)));
        ListPushBack(ins_list, insert_label);
      }
    }
  }

  // args_insert
  if (a->r && SEQ(a->r->name, "assistArgs")) {
    Value *func_param_ins = (Value *)ins_new_single_operator_v2(ParamOP, left);

    if (left->VTy->TID == ArrayTyID) {
      Stack *assist_array_get_stack = StackInit();
      StackSetClean(assist_array_get_stack, CleanObject);
      void *element = NULL;
      while (StackTop(array_get, &element)) {
        StackPop(array_get);
        if (element == (void *)1)
          break;
        StackPush(assist_array_get_stack, element);
      }
      while (StackTop(assist_array_get_stack, &element)) {
        StackPop(assist_array_get_stack);
        ListPushBack(ins_list, (void *)element);
      }
    }

    // 添加变量的名字 类型 和返回值
    func_param_ins->VTy->TID = ParamTyID;

    StackPush(stack_param, (void *)func_param_ins);

    // 插入
    // ListPushBack(ins_list, (void *)func_param_ins);

#ifdef DEBUG_MODE
    printf("%s %s insert\n", func_param_ins->name, left->name);
#endif
  }

  if (a->r && SEQ(a->r->name, "assistWHILE")) {

    Value *while_true_label_ins = logic_goto_assist.true_label;
    Value *while_false_label_ins = logic_goto_assist.false_label;
    if (left) {
      // 创建跳转语句
      Value *goto_condition_ins =
          (Value *)ins_new_single_operator_v2(GotoWithConditionOP, left);

      char temp_br_label_name[100];
      strcpy(temp_br_label_name, "true:");
      strcat(temp_br_label_name, while_true_label_ins->name);
      strcat(temp_br_label_name, "  false:");
      strcat(temp_br_label_name, while_false_label_ins->name);

      goto_condition_ins->name = strdup(temp_br_label_name);
      goto_condition_ins->VTy->TID = GotoTyID;
      goto_condition_ins->pdata->condition_goto.false_goto_location =
          while_false_label_ins;
      goto_condition_ins->pdata->condition_goto.true_goto_location =
          while_true_label_ins;

      ListPushBack(ins_list, (void *)goto_condition_ins);

#ifdef DEBUG_MODE
      printf("while br %s, true: %s  false : %s \n", left->name,
             while_true_label_ins->name, while_false_label_ins->name);
      printf("%s\n", while_true_label_ins->name);
#endif
    }
    ListPushBack(ins_list, while_true_label_ins);
    logic_goto_assist_func(NULL, NULL);
  }
}

Value *post_eval(ast *a, Value *left, Value *right) {
  if (a != NULL) {
    if (SEQ(a->name, "LC")) {
      if (array_init_assist.is_init_array) {
        int ensure_zero =
            array_init_assist.array_level - array_init_assist.nest_levels;
        array_init_assist.nest_levels--;
        bool is_all_zero = true;
        for (int i = 0; i < ensure_zero; i++) {
          if (array_init_assist.added[i] != 0) {
            is_all_zero = false;
            array_init_assist.added[i] = 0;
          }
        }

        if (!is_all_zero || array_init_assist.is_empty) {
          array_init_assist.added[ensure_zero]++;
          for (int i = ensure_zero; i < array_init_assist.array_level; i++) {
            if (array_init_assist.added[i] == array_init_assist.array_info[i]) {
              array_init_assist.added[i] = 0;
              array_init_assist.added[i + 1]++;
            } else {
              break;
            }
          }
        }

        // if (array_init_assist.is_empty) {
        //   array_init_assist.added[ensure_zero]++;
        //   for (int i = ensure_zero; i < array_init_assist.array_level; i++) {
        //     if (array_init_assist.added[i] ==
        //     array_init_assist.array_info[i]) {
        //       array_init_assist.added[i] = 0;
        //       array_init_assist.added[i + 1]++;
        //     } else {
        //       break;
        //     }
        //   }
        // }

        return NULL;
      }
    }

    if (pre_astnode->l && a == pre_astnode->l) {
      Value *work_ins = NULL;
      bool flag = false;
      if (SEQ(a->name, "PLUS")) {
        return right;
      } else if (SEQ(a->name, "MINUS")) {
        if (right->VTy->TID == ImmediateIntTyID ||
            right->VTy->TID == ImmediateFloatTyID) {
          char buffer[30];
          if (right->name[0] == '-') {
            sprintf(buffer, "%s", right->name + 1);
          } else {
            sprintf(buffer, "-%s", right->name);
          }
          if (HashMapContain(constant_single_value_hashmap, buffer)) {
            work_ins = HashMapGet(constant_single_value_hashmap, buffer);
          } else {
            work_ins = (Value *)malloc(sizeof(Value));
            value_init(work_ins);
            work_ins->VTy->TID = right->VTy->TID;
            work_ins->name = strdup(buffer);
            work_ins->pdata->var_pdata.iVal = -right->pdata->var_pdata.iVal;
            work_ins->pdata->var_pdata.fVal = -right->pdata->var_pdata.fVal;
            HashMapPut(constant_single_value_hashmap, strdup(buffer), work_ins);
          }
          return work_ins;
        } else {
          flag = true;
          work_ins = (Value *)ins_new_single_operator_v2(NegativeOP, right);
        }
      } else if (SEQ(a->name, "NOT")) {
        if (right->VTy->TID == ImmediateIntTyID ||
            right->VTy->TID == ImmediateFloatTyID) {
          if (right->pdata->var_pdata.fVal != 0.0f)
            work_ins = HashMapGet(constant_single_value_hashmap, "0");
          else
            work_ins = HashMapGet(constant_single_value_hashmap, "1");
          return work_ins;
        } else {
          flag = true;
          work_ins = (Value *)ins_new_single_operator_v2(NotOP, right);
          work_ins->VTy->TID = IntegerTyID;
        }
      }
      if (flag) {
        work_ins->name = name_generate(TEMP_VAR);
        work_ins->VTy->TID = right->VTy->TID;
        ListPushBack(ins_list, work_ins);
        return work_ins;
      }
    }

    // 如果要定义数据变量 判断当前定义的数据类型
    // 并且修改 NowVarDecType
    if (SEQ(a->name, "TYPE")) {
      if (SEQ(pre_astnode->name, "Specifire")) {
        if (SEQ(a->idtype, NowVarDecStr[0])) {
          nowVarDecType = NowVoid;
        } else if (SEQ(a->idtype, NowVarDecStr[1])) {
          nowVarDecType = NowInt;
        } else if (SEQ(a->idtype, NowVarDecStr[2]))
          nowVarDecType = NowFloat;
        else if (SEQ(a->idtype, NowVarDecStr[3]))
          nowVarDecType = NowStruct;
      }
      if (a->r && SEQ(a->r->name, "CONST"))
        NowConst = true;
    }

    if (SEQ(a->name, "Dec")) {
      if (a->r == NULL)
        NowConst = false;
      return NULL;
    }

    if (SEQ(a->name, "Specifire")) {
      return right;
    }

    // 判断父节点是不是变量声明 如果是 则创建一个该变量对应的value并返回
    // pre_var_dec
    if (SEQ(pre_astnode->name, "VarDec")) {
      if (SEQ(a->name, "ID")) {
        // allocate for array
        if (pre_astnode->r && SEQ(pre_astnode->r->name, "LB")) {
          char *array_name = NULL;
          // 创建指针
          Value *cur_ins = (Value *)ins_new_no_operator_v2(AllocateOP);

          if (StackSize(stack_symbol_table) == 1) {
            // 全局变量
            array_name = name_generate(GLOBAL);
            cur_ins->IsGlobalVar = 1;
          } else {
            array_name = name_generate(ARRAY);
          }

          if (NowConst) {
            cur_ins->IsConst = 1;
          }

          // 添加变量类型
          cur_ins->VTy->TID = ArrayTyID;
          // 添加指针的名字 映射进哈希表 放入symbol_tabel里面 用于索引
          cur_ins->name = array_name;

          array_list = ListInit();
          ListSetClean(array_list, CleanObject);

#ifdef DEBUG_MODE
          printf("%s = alloca %s array,align 4\n", array_name,
                 NowVarDecStr[nowVarDecType]);
#endif

          ListPushBack(ins_list, cur_ins);

          char *var_name = strdup(a->idtype);
          // 将变量加入符号表
          HashMapPut(cur_symboltable->symbol_map, var_name, cur_ins);
          // 返回指针
          return cur_ins;
        } else {
          char *temp_var = NULL;
          // 创建指针
          Value *cur_ins = (Value *)ins_new_no_operator_v2(AllocateOP);

          // 添加变量类型
          if (StackSize(stack_symbol_table) == 1) {
#ifdef DEBUG_MODE
            puts("global var");
#endif
            temp_var = name_generate(GLOBAL);
            cur_ins->IsGlobalVar = 1;
          } else {
            temp_var = name_generate(POINT);
          }

          // 在内存中为变量分配空间
          Value *cur_var = (Value *)malloc(sizeof(Value));
          value_init(cur_var);
          // 添加变量类型
          cur_var->VTy->TID = (int)nowVarDecType;
          // 添加变量的名字
          cur_var->name = strdup(a->idtype);

          if (NowConst) {
            cur_ins->IsConst = 1;
          }

          cur_ins->VTy->TID = PointerTyID;
          // 添加指针的名字 映射进哈希表 放入symbol_tabel里面 用于索引
          cur_ins->name = temp_var;
          // 设定allocate语句的指针所指向的value*
          cur_ins->pdata->allocate_pdata.point_value = cur_var;

#ifdef DEBUG_MODE
          printf("%s = alloca %s,align 4\n", temp_var,
                 NowVarDecStr[nowVarDecType]);
#endif

          ListPushBack(ins_list, cur_ins);

          char *var_name = strdup(a->idtype);
          // 将变量加入符号表
          HashMapPut(cur_symboltable->symbol_map, var_name, cur_ins);
          // 返回指针
          return cur_ins;
        }
      } else if (SEQ(a->name, "ASSIGNOP")) {
        return right;
      } else if (SEQ(a->name, "LB")) {
        return right;
      }
    }

    if (SEQ(pre_astnode->name, "Exp")) {
      // 父节点是表达式的情况 且不是赋值表达式 且当前节点是ID的情况
      if (SEQ(a->name, "ID") &&
          (pre_astnode->r ? strcmp(pre_astnode->r->name, "ASSIGNOP") : true)) {
        // 被引用变量的名字
        char *load_var_name = strdup(a->idtype);

        // 指向被引用变量的指针
        Value *load_var_pointer =
            HashMapGet(cur_symboltable->symbol_map, load_var_name);
        SymbolTable *pre_symboltable = cur_symboltable->father;
        // 查表 取出名字所指代的Value*
        while (!load_var_pointer) {
          // 如果当前表中没有且该表的父表为空 则报错语义错误
          // 向上一级查表
          load_var_pointer =
              HashMapGet(pre_symboltable->symbol_map, load_var_name);
          pre_symboltable = pre_symboltable->father;
          assert(pre_symboltable || load_var_pointer);
        };

        if (load_var_pointer->VTy->TID == ArrayTyID) {
          return load_var_pointer;
        } else if (load_var_pointer->IsConst) {
          return load_var_pointer->pdata->allocate_pdata.point_value;
        } else {
          if (!is_functional_test && global_optimization) {
            if (load_var_pointer->IsGlobalVar && !load_var_pointer->IsConst &&
                cur_construction_func) {
              if (!HashMapContain(assist_is_local_val,
                                  load_var_pointer->name)) {
                HashMapPut(assist_is_local_val, strdup(load_var_pointer->name),
                           cur_construction_func);
              } else {
                Value *cur_func =
                    HashMapGet(assist_is_local_val, load_var_pointer->name);
                if (cur_func && cur_func != cur_construction_func) {
                  HashMapPut(assist_is_local_val,
                             strdup(load_var_pointer->name), NULL);
                }
              }
            }
          }

          // load instruction
          Value *load_ins =
              (Value *)ins_new_single_operator_v2(LoadOP, load_var_pointer);
          load_ins->name = name_generate(TEMP_VAR);
          // 将内容拷贝
          value_copy(load_ins,
                     load_var_pointer->pdata->allocate_pdata.point_value);
#ifdef DEBUG_MODE
          printf("%s = load %s, %s,align 4\n", load_ins->name,
                 NowVarDecStr[load_ins->VTy->TID < 4 ? load_ins->VTy->TID
                                                     : load_ins->VTy->TID - 4],
                 load_var_pointer->name);
#endif
          ListPushBack(ins_list, (void *)load_ins);
          return load_ins;
        }
      } else if (SEQ(a->name, "ID") && pre_astnode->r &&
                 SEQ(pre_astnode->r->name, "ASSIGNOP")) {
        // 被赋值变量的名字
        char *assign_var_name = strdup(a->idtype);
        // 指向被赋值变量的指针
        Value *assign_var_pointer =
            HashMapGet(cur_symboltable->symbol_map, assign_var_name);
        SymbolTable *pre_symboltable = cur_symboltable->father;
        // 查表 取出名字所指代的Value*
        while (!assign_var_pointer) {
          // 如果当前表中没有且该表的父表为空 则报错语义错误
          // 向上一级查表
          assign_var_pointer =
              HashMapGet(pre_symboltable->symbol_map, assign_var_name);
          pre_symboltable = pre_symboltable->father;
          assert(pre_symboltable || assign_var_pointer);
        };

        if (!is_functional_test && global_optimization) {
          if (assign_var_pointer->IsGlobalVar && !assign_var_pointer->IsConst &&
              cur_construction_func) {
            HashMapPut(assist_is_local_val, strdup(assign_var_pointer->name),
                       NULL);
          }
        }

        return assign_var_pointer;
      } else if (SEQ(a->name, "INTEGER")) {
        Value *cur = NULL;
        char text[20];
        sprintf(text, "%d", a->intgr);
        if (HashMapContain(constant_single_value_hashmap, text)) {
          cur = HashMapGet(constant_single_value_hashmap, text);
        } else {
          cur = (Value *)malloc(sizeof(Value));
          value_init(cur);
          cur->VTy->TID = ImmediateIntTyID;
          // 添加变量的名字
          cur->name = strdup(text);
          // 为padata里的整数字面量常量赋值
          cur->pdata->var_pdata.iVal = a->intgr;
          cur->pdata->var_pdata.fVal = (float)(a->intgr);
          HashMapPut(constant_single_value_hashmap, strdup(text), cur);
        }
        return cur;
      } else if (SEQ(a->name, "FLOAT")) {
        Value *cur = NULL;
        char text[20];
        sprintf(text, "%f", a->flt);
        if (HashMapContain(constant_single_value_hashmap, text)) {
          cur = HashMapGet(constant_single_value_hashmap, text);
        } else {
          cur = (Value *)malloc(sizeof(Value));
          value_init(cur);
          cur->VTy->TID = ImmediateFloatTyID;
          // 添加变量的名字
          cur->name = strdup(text);
          // 为padata里的整数字面量常量赋值
          cur->pdata->var_pdata.iVal = (int)(a->flt);
          cur->pdata->var_pdata.fVal = a->flt;
          HashMapPut(constant_single_value_hashmap, strdup(text), cur);
        }
        return cur;
      }
      // 加减乘除的情况
      else if (SEQ(a->name, "MINUS") || SEQ(a->name, "PLUS") ||
               SEQ(a->name, "STAR") || SEQ(a->name, "DIV") ||
               SEQ(a->name, "MOD") || SEQ(a->name, "ASSIGNOP") ||
               SEQ(a->name, "EQUAL") || SEQ(a->name, "NOTEQUAL") ||
               SEQ(a->name, "GREAT") || SEQ(a->name, "GREATEQUAL") ||
               SEQ(a->name, "LESSEQUAL") || SEQ(a->name, "LESS") ||
               SEQ(a->name, "AND") || SEQ(a->name, "OR") ||
               SEQ(a->name, "NOT") || SEQ(a->name, "LB")) {
        // 返回当前节点的右节点
        return right;
      } else if (SEQ(a->name, "Stmt")) {
        // 返回当前节点的右节点
        return NULL;
      }
    }

    // funccall
    if (SEQ(pre_astnode->name, "assistFuncCall")) {
      if (SEQ(a->name, "ID")) {
        // 要跳转到的func_label
        Value *func_label = HashMapGet(func_hashMap, (void *)a->idtype);

        // oprand the array
        // char temp_str[25];
        // char text[10];
        // sprintf(text, "%d", temp_var_seed);
        // ++temp_var_seed;
        // strcpy(temp_str, "\%temp");
        // strcat(temp_str, text);
        char *temp_str = name_generate(TEMP_VAR);
        HashMapPut(func_hashMap, strdup(temp_str), func_label);

        Value *func_param_ins;
        param_seed = func_label->pdata->symtab_func_pdata.param_num;

        for (int i = func_label->pdata->symtab_func_pdata.param_num - 1; i >= 0;
             i--) {
          StackTop(stack_param, (void **)&func_param_ins);
          StackPop(stack_param);
          if (func_param_ins->VTy->TID == ArrayTyID) {
            func_param_ins->pdata->array_pdata.top_array->pdata->array_pdata
                .is_local_array = 0;
          }
          func_param_ins->name = name_generate(PARAM_CONVERT);
          func_param_ins->pdata->param_pdata.param_type =
              func_label->pdata->symtab_func_pdata.param_type_lists[i];

          // 插入
          ListPushBack(ins_list, (void *)func_param_ins);
        }

        if (func_label->pdata->symtab_func_pdata.return_type == VoidTyID) {
          Value *call_fun_ins = (Value *)ins_new_no_operator_v2(CallOP);

          call_fun_ins->name = strdup(temp_str);

          ListPushBack(ins_list, (void *)call_fun_ins);

#ifdef DEBUG_MODE
          printf(
              "new instruction call func %s and goto %s without return value "
              "\n",
              a->idtype, func_label->name);
#endif

          return NULL;
        } else {
          Value *call_fun_ins =
              (Value *)ins_new_no_operator_v2(CallWithReturnValueOP);
          call_fun_ins->VTy->TID =
              func_label->pdata->symtab_func_pdata.return_type;

          call_fun_ins->name = strdup(temp_str);

          ListPushBack(ins_list, (void *)call_fun_ins);

#ifdef DEBUG_MODE
          printf(
              "new instruction call func %s and goto %s with return value \n",
              a->idtype, func_label->name);
#endif

          return call_fun_ins;
        }
      }
    }

    // 判断当前节点是否是变量声明 如果是则生成一条声明变量的instruction
    if (SEQ(a->name, "VarDec")) {
      char *var_name = strdup(left->name);
      if (left->VTy->TID != ArrayTyID) {
        if (right == NULL) {
          return left;
        } else {
          if (a->r && SEQ(a->r->name, "ASSIGNOP")) {
            // 把right存给left left是赋值号左边操作数的地址
            if (left->IsConst) {
              if (right->VTy->TID == IntegerTyID ||
                  right->VTy->TID == FloatTyID) {
                value_free(left->pdata->allocate_pdata.point_value);
                left->pdata->allocate_pdata.point_value = right;
              } else {
                Value *cur = NULL;
                if (nowVarDecType != (int)right->VTy->TID - 4) {
                  char buffer[30];
                  if (nowVarDecType == NowInt) {
                    sprintf(buffer, "%d", (int)right->pdata->var_pdata.fVal);
                  } else {
                    sprintf(buffer, "%f", (float)right->pdata->var_pdata.iVal);
                  }
                  if (HashMapContain(constant_single_value_hashmap, buffer)) {
                    cur = HashMapGet(constant_single_value_hashmap, buffer);
                  } else {
                    cur = (Value *)malloc(sizeof(Value));
                    value_init(cur);
                    cur->VTy->TID = (int)nowVarDecType + 4;
                    cur->name = strdup(buffer);
                    if (nowVarDecType == NowInt) {
                      cur->pdata->var_pdata.iVal =
                          (int)right->pdata->var_pdata.fVal;
                      cur->pdata->var_pdata.fVal = cur->pdata->var_pdata.iVal;
                    } else {
                      cur->pdata->var_pdata.fVal =
                          (float)right->pdata->var_pdata.iVal;
                      cur->pdata->var_pdata.iVal = cur->pdata->var_pdata.fVal;
                    }
                    HashMapPut(constant_single_value_hashmap, strdup(buffer),
                               cur);
                  }
                } else
                  cur = right;

                value_free(left->pdata->allocate_pdata.point_value);
                left->pdata->allocate_pdata.point_value = cur;
              }
            } else {
              Value *assign_var = right;
              if (left->pdata->allocate_pdata.point_value->VTy->TID !=
                      right->VTy->TID &&
                  left->pdata->allocate_pdata.point_value->VTy->TID !=
                      right->VTy->TID - 4) {
                assign_var =
                    (Value *)ins_new_single_operator_v2(AssignOP, right);
                assign_var->VTy->TID =
                    left->pdata->allocate_pdata.point_value->VTy->TID;
                assign_var->name = name_generate(TEMP_VAR);
                ListPushBack(ins_list, (void *)assign_var);
              }

              Value *store_ins = (Value *)ins_new_binary_operator_v2(
                  StoreOP, left, assign_var);
              ListPushBack(ins_list, (void *)store_ins);
#ifdef DEBUG_MODE
              printf("store %s %s, %s,align 4\n",
                     NowVarDecStr[assign_var->VTy->TID < 4
                                      ? assign_var->VTy->TID
                                      : assign_var->VTy->TID - 4],
                     assign_var->name, var_name);
#endif
            }
            return NULL;
          }
        }
      } else {
        // init the array
        if (a->r && SEQ(a->r->name, "LB")) {
          if (right != NULL) {
            total_array_member *= right->pdata->var_pdata.iVal;
            ListPushBack(array_list,
                         (void *)(intptr_t)(right->pdata->var_pdata.iVal));
          } else {
            //数组作为函数参数
            ListPushBack(array_list, (void *)(intptr_t)(1));
          }
        } else if (a->r && SEQ(a->r->name, "ASSIGNOP")) {

#ifdef DEBUG_MODE
          ListFirst(array_init_assist.offset_list, false);
          global_array_init_item *element;
          printf("cur init array name \t%s\n", left->name);
          while (ListNext(array_init_assist.offset_list, (void *)&element)) {
            printf("offset:\t%d ival:\t%d fval:\t%f\n", element->offset,
                   element->ival, element->fval);
          }
#endif

          HashMapPut(global_array_init_hashmap, strdup(left->name),
                     array_init_assist.offset_list);
          array_init_assist_func(NULL, NULL);
        }
        return left;
      }
    }

    // 判断当前节点是否为表达式节点
    if (SEQ(a->name, "Exp")) {
      char *var_name = NULL;
      if (left)
        var_name = left->name;

      if (right == NULL) {
        return left;
      } else if (SEQ(a->r->name, "ASSIGNOP")) {
        Value *assign_var = right;

        if (left->VTy->TID == PointerTyID &&
                left->pdata->allocate_pdata.point_value->VTy->TID !=
                    right->VTy->TID &&
                left->pdata->allocate_pdata.point_value->VTy->TID !=
                    right->VTy->TID - 4 ||
            left->VTy->TID == ArrayTyID &&
                left->pdata->array_pdata.array_type != right->VTy->TID &&
                left->pdata->array_pdata.array_type != right->VTy->TID - 4) {
          assign_var = (Value *)ins_new_single_operator_v2(AssignOP, right);
          assign_var->VTy->TID =
              left->pdata->allocate_pdata.point_value->VTy->TID;
          assign_var->name = name_generate(TEMP_VAR);
          ListPushBack(ins_list, (void *)assign_var);
        }

        // assign_var_pointer是赋值号左边操作数的地址
        Instruction *store_ins =
            ins_new_binary_operator_v2(StoreOP, left, assign_var);

        if (left->VTy->TID == ArrayTyID) {
          Stack *assist_array_get_stack = StackInit();
          StackSetClean(assist_array_get_stack, CleanObject);
          void *element = NULL;
          while (StackTop(array_get, &element)) {
            StackPop(array_get);
            if (element == (void *)1)
              break;
            StackPush(assist_array_get_stack, element);
          }
          while (StackTop(assist_array_get_stack, &element)) {
            StackPop(assist_array_get_stack);
            ListPushBack(ins_list, (void *)element);
          }
        }

        ListPushBack(ins_list, (void *)store_ins);
#ifdef DEBUG_MODE
        printf("store %s %s, %s,align 4\n",
               NowVarDecStr[right->VTy->TID < 4 ? right->VTy->TID
                                                : right->VTy->TID - 4],
               right->name, left->name);
#endif
        // TODO 返回值是什么有待考虑
        return right;
      } else if (SEQ(a->r->name, "LB")) {
        if (left->pdata->array_pdata.top_array == left)
          StackPush(array_get, (void *)(intptr_t)1);

        // oprand the array
        char *temp_str = name_generate(TEMP_VAR);
        Value *cur_ins =
            (Value *)ins_new_binary_operator_v2(GetelementptrOP, left, right);
        // 将数组的信息拷贝一份
        cur_ins->name = strdup(temp_str);
        cur_ins->VTy->TID = ArrayTyID;
        // cur_ins->IsGlobalVar = left->IsGlobalVar;
        cur_ins->pdata->array_pdata.array_type =
            left->pdata->array_pdata.array_type;
        cur_ins->pdata->array_pdata.is_local_array = ARRAY_DEREFERENCE;
        cur_ins->pdata->array_pdata.top_array =
            left->pdata->array_pdata.top_array;
        // cur_ins->pdata->array_pdata.array_value =
        //     left->pdata->array_pdata.array_value;
        cur_ins->pdata->array_pdata.list_para = ListInit();
        ListSetClean(cur_ins->pdata->array_pdata.list_para, CleanObject);
        list_copy(cur_ins->pdata->array_pdata.list_para,
                  left->pdata->array_pdata.list_para);
        cur_ins->pdata->array_pdata.total_member =
            left->pdata->array_pdata.step_long;
        void *element;
        ListGetFront(cur_ins->pdata->array_pdata.list_para, &element);
        ListPopFront(cur_ins->pdata->array_pdata.list_para);
        cur_ins->pdata->array_pdata.step_long =
            cur_ins->pdata->array_pdata.total_member / (intptr_t)element;

        StackPush(array_get, cur_ins);
        // ListPushBack(ins_list, cur_ins);

        if (ListSize(cur_ins->pdata->array_pdata.list_para) == 0) {
          if (pre_astnode->r ? strcmp(pre_astnode->r->name, "ASSIGNOP")
                             : true) {
            // 链表为空 代表数组退化为普通的指针
            // 如果不是对数组里面的成员赋值则要把内容load出来使用
            char *temp_str = name_generate(TEMP_VAR);
            // cur_ins->VTy->TID = PointerTyID;
            // 内容与指针所指向的pdata完全一样 名字不一样
            // 占用的内存地址也不一样
            Value *load_ins =
                (Value *)ins_new_single_operator_v2(LoadOP, cur_ins);
            load_ins->name = strdup(temp_str);
            load_ins->VTy->TID = left->pdata->array_pdata.array_type;

            Stack *assist_array_get_stack = StackInit();
            StackSetClean(assist_array_get_stack, CleanObject);
            void *element = NULL;
            while (StackTop(array_get, &element)) {
              StackPop(array_get);
              if (element == (void *)1)
                break;
              StackPush(assist_array_get_stack, element);
            }
            while (StackTop(assist_array_get_stack, &element)) {
              StackPop(assist_array_get_stack);
              ListPushBack(ins_list, (void *)element);
            }

            ListPushBack(ins_list, (void *)load_ins);

#ifdef DEBUG_MODE
          printf("%s = load %s, %s,align 4\n", temp_str,
                 NowVarDecStr[load_ins->VTy->TID < 4 ? load_ins->VTy->TID
                                                     : load_ins->VTy->TID - 4],
                 cur_ins->name);
#endif

            return load_ins;
          }
        }
        return cur_ins;
      } else {

        for (int i = 0; i < L_OP_NUM; i++) {
          if (SEQ(a->r->name, LOGIC_OPERATOR[i])) {
            Value *goto_condition_ins =
                (Value *)ins_new_single_operator_v2(GotoWithConditionOP, right);
            Value *true_label_ins;
            Value *else_label_ins;
            if (StackSize(logic_goto_assist.and_stack) == 0)
              true_label_ins = logic_goto_assist.true_label;
            else
              StackTop(logic_goto_assist.and_stack, (void **)&true_label_ins);

            if (StackSize(logic_goto_assist.or_stack) == 0)
              else_label_ins = logic_goto_assist.false_label;
            else
              StackTop(logic_goto_assist.or_stack, (void **)&else_label_ins);

            char temp_br_label_name[80];
            strcpy(temp_br_label_name, "true:");
            strcat(temp_br_label_name, true_label_ins->name);
            strcat(temp_br_label_name, "  false:");
            strcat(temp_br_label_name, else_label_ins->name);

            goto_condition_ins->name = strdup(temp_br_label_name);
            goto_condition_ins->VTy->TID = GotoTyID;
            goto_condition_ins->pdata->condition_goto.true_goto_location =
                true_label_ins;
            goto_condition_ins->pdata->condition_goto.false_goto_location =
                else_label_ins;

            ListPushBack(ins_list, goto_condition_ins);
            return NULL;
          }
        }

        if ((left->VTy->TID == ImmediateIntTyID ||
             left->VTy->TID == ImmediateFloatTyID) &&
            (right->VTy->TID == ImmediateIntTyID ||
             right->VTy->TID == ImmediateFloatTyID)) {
          Value *cur;

          TypeID cur_res_typeid = imm_res_type(left, right);
          char buffer[30];
          float const_float_value = 0.0f;
          int const_int_value = 0;
          if (cur_res_typeid == ImmediateFloatTyID) {
            if (SEQ(a->r->name, "PLUS")) {
              const_float_value =
                  left->pdata->var_pdata.fVal + right->pdata->var_pdata.fVal;
              sprintf(buffer, "%f", const_float_value);
            } else if (SEQ(a->r->name, "MINUS")) {
              const_float_value =
                  left->pdata->var_pdata.fVal - right->pdata->var_pdata.fVal;
              sprintf(buffer, "%f", const_float_value);
            } else if (SEQ(a->r->name, "STAR")) {
              const_float_value =
                  left->pdata->var_pdata.fVal * right->pdata->var_pdata.fVal;
              sprintf(buffer, "%f", const_float_value);
            } else if (SEQ(a->r->name, "DIV")) {
              const_float_value =
                  left->pdata->var_pdata.fVal / right->pdata->var_pdata.fVal;
              sprintf(buffer, "%f", const_float_value);
            } else if (SEQ(a->r->name, "EQUAL")) {
              const_float_value =
                  left->pdata->var_pdata.fVal == right->pdata->var_pdata.fVal;
              sprintf(buffer, "%d", (int)const_float_value);
            } else if (SEQ(a->r->name, "NOTEQUAL")) {
              const_float_value =
                  left->pdata->var_pdata.fVal != right->pdata->var_pdata.fVal;
              sprintf(buffer, "%d", (int)const_float_value);
            } else if (SEQ(a->r->name, "GREAT")) {
              const_float_value =
                  left->pdata->var_pdata.fVal > right->pdata->var_pdata.fVal;
              sprintf(buffer, "%d", (int)const_float_value);
            } else if (SEQ(a->r->name, "LESS")) {
              const_float_value =
                  left->pdata->var_pdata.fVal < right->pdata->var_pdata.fVal;
              sprintf(buffer, "%d", (int)const_float_value);
            } else if (SEQ(a->r->name, "GREATEQUAL")) {
              const_float_value =
                  left->pdata->var_pdata.fVal >= right->pdata->var_pdata.fVal;
              sprintf(buffer, "%d", (int)const_float_value);
            } else if (SEQ(a->r->name, "LESSEQUAL")) {
              const_float_value =
                  left->pdata->var_pdata.fVal <= right->pdata->var_pdata.fVal;
              sprintf(buffer, "%d", (int)const_float_value);
            }
            if (HashMapContain(constant_single_value_hashmap, buffer))
              cur = HashMapGet(constant_single_value_hashmap, buffer);
            else {
              cur = (Value *)malloc(sizeof(Value));
              value_init(cur);
              cur->VTy->TID = ImmediateFloatTyID;
              cur->name = strdup(buffer);
              cur->pdata->var_pdata.iVal = (int)const_float_value;
              cur->pdata->var_pdata.fVal = const_float_value;
              HashMapPut(constant_single_value_hashmap, strdup(buffer), cur);
            }
          } else {
            if (SEQ(a->r->name, "PLUS")) {
              const_int_value =
                  left->pdata->var_pdata.iVal + right->pdata->var_pdata.iVal;
              sprintf(buffer, "%d", const_int_value);
            } else if (SEQ(a->r->name, "MINUS")) {
              const_int_value =
                  left->pdata->var_pdata.iVal - right->pdata->var_pdata.iVal;
              sprintf(buffer, "%d", const_int_value);
            } else if (SEQ(a->r->name, "STAR")) {
              const_int_value =
                  left->pdata->var_pdata.iVal * right->pdata->var_pdata.iVal;
              sprintf(buffer, "%d", const_int_value);
            } else if (SEQ(a->r->name, "DIV")) {
              const_int_value =
                  left->pdata->var_pdata.iVal / right->pdata->var_pdata.iVal;
              sprintf(buffer, "%d", const_int_value);
            } else if (SEQ(a->r->name, "EQUAL")) {
              const_int_value =
                  left->pdata->var_pdata.iVal == right->pdata->var_pdata.iVal;
              sprintf(buffer, "%d", const_int_value);
            } else if (SEQ(a->r->name, "NOTEQUAL")) {
              const_int_value =
                  left->pdata->var_pdata.iVal != right->pdata->var_pdata.iVal;
              sprintf(buffer, "%d", const_int_value);
            } else if (SEQ(a->r->name, "GREAT")) {
              const_int_value =
                  left->pdata->var_pdata.iVal > right->pdata->var_pdata.iVal;
              sprintf(buffer, "%d", const_int_value);
            } else if (SEQ(a->r->name, "LESS")) {
              const_int_value =
                  left->pdata->var_pdata.iVal < right->pdata->var_pdata.iVal;
              sprintf(buffer, "%d", const_int_value);
            } else if (SEQ(a->r->name, "GREATEQUAL")) {
              const_int_value =
                  left->pdata->var_pdata.iVal >= right->pdata->var_pdata.iVal;
              sprintf(buffer, "%d", const_int_value);
            } else if (SEQ(a->r->name, "LESSEQUAL")) {
              const_int_value =
                  left->pdata->var_pdata.iVal <= right->pdata->var_pdata.iVal;
              sprintf(buffer, "%d", const_int_value);
            } else if (SEQ(a->r->name, "MOD")) {
              const_int_value =
                  left->pdata->var_pdata.iVal % right->pdata->var_pdata.iVal;
              sprintf(buffer, "%d", const_int_value);
            }

            if (HashMapContain(constant_single_value_hashmap, buffer))
              cur = HashMapGet(constant_single_value_hashmap, buffer);
            else {
              cur = (Value *)malloc(sizeof(Value));
              value_init(cur);
              cur->VTy->TID = ImmediateIntTyID;
              cur->name = strdup(buffer);
              cur->pdata->var_pdata.iVal = const_int_value;
              cur->pdata->var_pdata.fVal = (float)const_int_value;
              HashMapPut(constant_single_value_hashmap, strdup(buffer), cur);
            }
          }
          return cur;
        }

        for (int i = 1; i < N_OP_NUM; i++) {
          if (SEQ(a->r->name, NORMAL_OPERATOR[i])) {

            if (left->VTy->TID == FloatTyID &&
                right->VTy->TID == ImmediateIntTyID) {
              char buffer[30];
              sprintf(buffer, "%f", (float)right->pdata->var_pdata.iVal);
              if (HashMapContain(constant_single_value_hashmap, buffer)) {
                right = HashMapGet(constant_single_value_hashmap, buffer);
              } else {
                Value *cur = (Value *)malloc(sizeof(Value));
                value_init(cur);
                cur->VTy->TID = ImmediateFloatTyID;
                cur->name = strdup(buffer);
                cur->pdata->var_pdata.iVal = right->pdata->var_pdata.iVal;
                cur->pdata->var_pdata.fVal =
                    (float)right->pdata->var_pdata.iVal;
                HashMapPut(constant_single_value_hashmap, strdup(buffer), cur);
                right = cur;
              }
            }

            if (left->VTy->TID == ImmediateIntTyID &&
                right->VTy->TID == FloatTyID) {
              char buffer[30];
              sprintf(buffer, "%f", (float)left->pdata->var_pdata.iVal);
              if (HashMapContain(constant_single_value_hashmap, buffer)) {
                left = HashMapGet(constant_single_value_hashmap, buffer);
              } else {
                Value *cur = (Value *)malloc(sizeof(Value));
                value_init(cur);
                cur->VTy->TID = ImmediateFloatTyID;
                cur->name = strdup(buffer);
                cur->pdata->var_pdata.iVal = left->pdata->var_pdata.iVal;
                cur->pdata->var_pdata.fVal = (float)left->pdata->var_pdata.iVal;
                HashMapPut(constant_single_value_hashmap, strdup(buffer), cur);
                left = cur;
              }
            }

            TypeID res_typeid;
            if (i >= 6)
              res_typeid = IntegerTyID;
            else
              res_typeid = ins_res_type(left, right);

            Value *cur_ins =
                (Value *)ins_new_binary_operator_v2(DefaultOP, left, right);
            // 添加变量的名字
            cur_ins->name = name_generate(TEMP_VAR);
            if (i >= 6)
              cur_ins->VTy->TID = res_typeid;
            else
              cur_ins->VTy->TID = res_typeid;

            ((Instruction *)cur_ins)->opcode = (TAC_OP)i;
            ListPushBack(ins_list, (void *)cur_ins);
            return cur_ins;
          }
        }

        return NULL;
      }
    }
  }

  if (SEQ(a->name, "assistFuncCall")) {
    return right;
  }

  // 后续遍历到if标识该if管辖的全区域块结束 插入跳转点label
  if (SEQ(a->name, "IF")) {
    Instruction *ins_back = NULL;
    ListGetBack(ins_list, (void **)&ins_back);

    // 当前的if不含else的情况
    if (!have_else) {
      // 删除链尾 并且释放链尾ins的内存
      // ListPopBack(ins_list);
      // value_free((Value *)ins_back);
      // free(ins_back);

      Value *else_label_ins = NULL;
      StackTop(stack_else_label, (void **)&else_label_ins);
      StackPop(stack_else_label);
      // 重定向无条件跳转的指向
      char temp_str[30];
      strcpy(temp_str, "goto ");
      strcat(temp_str, ((Value *)else_label_ins)->name);
      free(((Value *)ins_back)->name);
      ((Value *)ins_back)->name = strdup(temp_str);
      ((Value *)ins_back)->pdata->no_condition_goto.goto_location =
          else_label_ins;
      ListPushBack(ins_list, (void *)else_label_ins);
#ifdef DEBUG_MODE
      printf("%s\n", else_label_ins->name);
#endif

      Instruction *then_label_ins = NULL;
      StackTop(stack_then_label, (void **)&then_label_ins);
      StackPop(stack_then_label);
      // 释放label ins的内存
      // printf("delete the destination %s\n",
      // then_label_ins->user.res->name);
      value_free((Value *)then_label_ins);
      free(then_label_ins);
      return NULL;
    } else {
      have_else = false;
      Value *then_label_ins = NULL;
      StackTop(stack_then_label, (void **)&then_label_ins);
      StackPop(stack_then_label);
      ListPushBack(ins_list, (void *)then_label_ins);
#ifdef DEBUG_MODE
      printf("%s\n", then_label_ins->name);
#endif
      return NULL;
    }
  }

  if (SEQ(a->name, "FunDec")) {
    cur_construction_func = NULL;
    StackPop(stack_symbol_table);
    // 销毁当前的符号表中的哈希表然后销毁符号表
    HashMapDeinit(cur_symboltable->symbol_map);
    free(cur_symboltable);
    cur_symboltable = NULL;
    // 当前的符号表恢复到上一级的符号表
    StackTop(stack_symbol_table, (void **)&cur_symboltable);

    char *func_label_end = NULL;
    func_label_end = name_generate(FUNC_LABEL_END);

    Value *func_end_ins = (Value *)ins_new_no_operator_v2(FuncEndOP);
    // 添加变量的名字
    func_end_ins->name = func_label_end;
    func_end_ins->VTy->TID = FuncEndTyID;
    // // pdata不需要数据所以释放掉
    // free(func_end_ins->pdata);

    // 插入
    ListPushBack(ins_list, (void *)func_end_ins);
    if (StackSize(stack_while_then_label)) {
      assert(0);
    }

#ifdef DEBUG_MODE
    printf("%s\n", func_label_end);
#endif
  }

  if (SEQ(a->name, "RETURN")) {
    Value *func_return_ins = NULL;

    if (right == NULL)
      func_return_ins = (Value *)ins_new_no_operator_v2(ReturnOP);
    else
      func_return_ins = (Value *)ins_new_single_operator_v2(ReturnOP, right);

    func_return_ins->name = strdup("return");
    func_return_ins->VTy->TID = ReturnTyID;
    func_return_ins->pdata->return_pdata.return_type =
        cur_construction_func->pdata->symtab_func_pdata.return_type;

    // 插入
    ListPushBack(ins_list, (void *)func_return_ins);

#ifdef DEBUG_MODE
    printf("%s\n", func_return_ins->name);
#endif
  }

  if (SEQ(a->name, "ELSE")) {
    have_else = true;
    Value *then_label_ins = NULL;
    StackTop(stack_then_label, (void **)&then_label_ins);
    char temp_str[100];
    strcpy(temp_str, "goto ");
    strcat(temp_str, then_label_ins->name);

    Value *goto_then_ins = (Value *)ins_new_no_operator_v2(GotoOP);
    goto_then_ins->name = strdup(temp_str);
    goto_then_ins->VTy->TID = GotoTyID;
    goto_then_ins->pdata->no_condition_goto.goto_location = then_label_ins;

    ListPushBack(ins_list, (void *)goto_then_ins);
  }

  if (SEQ(a->name, "WHILE")) {
    Value *while_head_label_ins = NULL;
    StackTop(stack_while_head_label, (void **)&while_head_label_ins);
    StackPop(stack_while_head_label);

    char temp_str[100];
    strcpy(temp_str, "goto ");
    strcat(temp_str, while_head_label_ins->name);

    // 跳出while循环
    Value *goto_out_while_ins = (Value *)ins_new_no_operator_v2(GotoOP);
    goto_out_while_ins->name = strdup(temp_str);
    goto_out_while_ins->VTy->TID = GotoTyID;
    goto_out_while_ins->pdata->no_condition_goto.goto_location =
        while_head_label_ins;

    ListPushBack(ins_list, (void *)goto_out_while_ins);

    // 跳出while循环后紧接着语句的label
    Value *while_then_label_ins = NULL;
    StackTop(stack_while_then_label, (void **)&while_then_label_ins);
    StackPop(stack_while_then_label);
    ListPushBack(ins_list, (void *)while_then_label_ins);

#ifdef DEBUG_MODE
    printf("%s\n", while_then_label_ins->name);
#endif
    return NULL;
  }

  if (SEQ(a->name, "BREAK")) {
    Value *goto_break_label_ins = NULL;
    assert(StackSize(stack_while_then_label) != 0);

    StackTop(stack_while_then_label, (void **)&goto_break_label_ins);

    char temp_str[40];
    strcpy(temp_str, "(break)");
    strcat(temp_str, goto_break_label_ins->name);

    Value *break_ins = (Value *)ins_new_no_operator_v2(GotoOP);
    // 添加变量的名字 类型 和返回值
    break_ins->name = strdup(temp_str);
    break_ins->VTy->TID = GotoTyID;
    break_ins->pdata->no_condition_goto.goto_location = goto_break_label_ins;

    ListPushBack(ins_list, (void *)break_ins);
#ifdef DEBUG_MODE
    printf("break :br %s \n", goto_break_label_ins->name);
#endif
  }

  if (SEQ(a->name, "CONTINUE")) {
    Value *goto_head_label_ins = NULL;
    if (StackSize(stack_while_head_label) == 0) {
      // 没有地方可以去 报错？
      assert(0);
      return NULL;
    }

    StackTop(stack_while_head_label, (void **)&goto_head_label_ins);

    char temp_str[30];
    strcpy(temp_str, "(continue)br :");
    strcat(temp_str, goto_head_label_ins->name);

    Value *break_ins = (Value *)ins_new_no_operator_v2(GotoOP);
    // 添加变量的名字 类型 和返回值
    break_ins->name = strdup(temp_str);
    break_ins->VTy->TID = GotoTyID;
    break_ins->pdata->no_condition_goto.goto_location = goto_head_label_ins;

    ListPushBack(ins_list, (void *)break_ins);
#ifdef DEBUG_MODE
    printf("continue :br %s \n", goto_head_label_ins->name);
#endif
  }
  return NULL;
}

Value *eval(ast *a) {
  if (a != NULL) {
    // 先序遍历
    pre_eval(a);
    // 将当前的ast节点如栈
    StackPush(stack_ast_pre, a);
    Value *left = eval(a->l); // 遍历左子树
    // 中序遍历
    in_eval(a, left);

    Value *right = eval(a->r); // 遍历右子树

    // 将当前的ast节点出栈
    StackPop(stack_ast_pre);
    // pre_astnode指向栈顶ast节点
    StackTop(stack_ast_pre, (void **)&pre_astnode);

    // 后序遍历
    return post_eval(a, left, right);
  }

  return NULL;
}

void yyerror(char *s, ...) // 变长参数错误处理函数
{
  va_list ap;
  va_start(ap, s);
  fprintf(stderr, "%d:error:", yylineno); // 错误行号
  vfprintf(stderr, s, ap);
  fprintf(stderr, "\n");
}
