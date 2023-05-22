#include <stdarg.h>  //变长参数函数所需的头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Ast.h"
#include "Pass.h"
#include "bblock.h"
#include "c_container_auxiliary.h"
#include "cds.h"
#include "symbol_table.h"
#include "print_format.h"

extern List *ins_list;
extern List *func_list;

SymbolTable *cur_symboltable = NULL;

Value *return_val = NULL;

ast *pre_astnode = NULL;

int yyparse(void);

int parser(char *input);

char *tty_path;

extern FILE *yyin;
int main() {
  // 获取当前进程所在的终端设备路径
  tty_path = ttyname(STDIN_FILENO);

  AllInit();

  printf("开始遍历\n");

  char *arr_func_call =
      "int add_arr(int arr[][20]) {"
      "int a = arr[1][2];"
      "arr[3][4] = 10;"
      "int b = arr[3][4];"
      "return a + b;"
      "}"
      "int main() {"
      "int arr[10][20];"
      "arr[1][2] = 10;"
      "arr[3][4] = 20;"
      "int res1 = add_arr(arr);"
      "return res1;"
      "}";
  
    char *func_call =
      "int add(int a, int b) {"
      "a = 10;"
      "int c = a + b;"
      "return c;"
      "}"
      "void main() {"
      "int a = 10;"
      "int b = 20;"
      "int c = add(add(a,b), b);"
      "return c;}";

    char *func_call_2 =
      "int add(int a, int b,int c,int d,int e,int f) {"
      "a = 10;"
      "int g = a + b + c + d + e + f;"
      "return g;"
      "}"
      "void main() {"
      "int a = 10;"
      "int b = 20;"
      "int c = add(add(a,b,3,4,5,6), b,7,8,9,10);"
      "return c;}";

  char *multi_add =
      "int multi_add() {"
      "int a = 1;"
      "int b = 2;"
      "int c = 3;"
      "int d = 4;"
      "int e = 5;"
      "int f = a + b + c + d + e;"
      "}";

  char *multidimensional_arrays =
    "int main() {"
    "  int b = 10;"
    "  int c = 233,arr[10][20][30];"
    "  arr[3][5][b] = 100;"
    "  int d = arr[3][5][b];"
    "  return d;"
    "}";
  
  // 多维度数组
  char *multidimensional_arrays_2 =
      "int main() {"
      "  int b = 10;"
      "  int m = b + 10;" 
      "  arr[3][5][m] = 100;"
      "  int d = arr[3][5][b+10];"
      "  return d;"
      "}";

  char *_003_var_defn3 = 
    "int main(){"
    "int a, b0, _c;"
    "a = 1;"
    "b0 = 2;"
    "_c = 3;"
    "return b0 + _c;"
    "}";

  char* sort_test4_58 = 
  "int select_sort(int A[],int n)"
  "{"
  "    int i;"
  "    int j;"
  "    int min;"
  "    i =0;"
  "    while(i < n-1)"
  "    {"
  "        min=i;//"
  "        j = i + 1;"
  "        while(j < n)"
  "        {"
  "            if(A[min]>A[j])"
  "            {"
  "                min=j;"
  "            }"
  "            j=j+1;"
  "        }"
  "        if(min!=i)"
  "        {"
  "            int tmp;"
  "            tmp = A[min];"
  "            A[min] = A[i];"
  "            A[i] = tmp;"
  "        }"
  "        i = i + 1;"
  "    }"
  "    return 0;"
  "}"
  "int main(){"
  "    int n = 10;"
  "    int a[10];"
  "    a[0]=4;a[1]=3;a[2]=9;a[3]=2;a[4]=0;"
  "    a[5]=1;a[6]=6;a[7]=5;a[8]=7;a[9]=8;"
  "    int i;"
  "    i = 0;"
  "    i = select_sort(a, n);"
  "    while (i < n) {"
  "        int tmp;"
  "        tmp = a[i];"
  "        putint(tmp);"
  "        tmp = 10;"
  "        putch(tmp);"
  "        i = i + 1;"
  "    }"
  "    return 0;"
  "}";


  if (freopen("printf_ast.txt", "w", stdout) == NULL) {
    fprintf(stderr, "打开文件printf_ast失败！");
    exit(-1);
  }

  // yyin = fopen("../example/003_var_defn3.sy","r");
  // yyparse();
  parser(arr_func_call);

  // 重定向输出回终端
  if (freopen(tty_path, "w", stdout) == NULL) {
    fprintf(stderr, "打开文件tty失败！");
    exit(-1);
  }

  print_ins_pass(ins_list);

  // if (freopen("out.txt", "w", stdout) == NULL) {
  //   fprintf(stderr, "打开文件失败！");
  //   exit(-1);
  // }

  // delete_return_deadcode_pass(ins_list);

  ins_toBBlock_pass(ins_list);

  //LGD backend 2023-5-9
  TranslateInit();

  ListFirst(func_list, false);
  void *element;
  while (ListNext(func_list, &element)) {
    puts(((Function *)element)->label->name);
    bblock_to_dom_graph_pass((Function *)element);
  }

  free(tty_path);
  printf("All over!\n");

  if (freopen("out.txt", "w", stdout) == NULL) {
    fprintf(stderr, "打开文件out.txt失败！");
    exit(-1);
  }
  print_model();

  return 0;
}
