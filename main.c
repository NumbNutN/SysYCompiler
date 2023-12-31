#include <stdarg.h> //变长参数函数所需的头文件
#include <stdbool.h>
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

#include "config.h"

#include "print_format.h"
#include "interface_zzq.h"
#include "optimize.h"

extern List *ins_list;
extern List *global_func_list;
extern List *global_var_list;

SymbolTable *cur_symboltable = NULL;

bool is_functional_test = true;
bool global_optimization = true;

int yyparse(void);

int parser(char *input);

char *tty_path;

char *read_code_from_file(const char *);
void register_replace(Function *func_list);

// --------------------------------------------------
int main(int argc, char **argv) {
  TIMER_BEGIN;

#ifndef DEBUG_MODE
  freopen("/dev/null", "w", stdout);
#endif
  tty_path = ttyname(STDIN_FILENO);

  AllInit();


#ifdef DEBUG_MODE
  printf("%%begin the pass\n");
#endif
  char *choose_case = NULL;
  if (argc == 5) {
    is_functional_test = true;
    choose_case = read_code_from_file(argv[4]);
  } else if (argc == 6) {
    is_functional_test = false;
    choose_case = read_code_from_file(argv[4]);
  } else {
    assert("invalid parameters");
  }
  if (choose_case == NULL)
    return 1;

#ifdef DEBUG_MODE
#ifndef PRINT_TO_TERMINAL
  freopen("./output/printf_ast.txt", "w", stdout);
#endif
#endif

  parser(choose_case);
  TIMER_END("parser over!"); 

#ifdef DEBUG_MODE
#ifndef PRINT_TO_TERMINAL
  freopen(tty_path, "w", stdout);
  freopen("./output/out.txt", "w", stdout);
  setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
#endif
#endif

#ifdef DEBUG_MODE
  print_ins_pass(ins_list);
  fflush(stdout);
  printf("\n\n\n\n");
#endif

  TIMER_BEGIN;
  delete_return_deadcode_pass(ins_list);
  TIMER_END("delete_return_deadcode_pass over!");

  print_ins_pass(ins_list);


  TIMER_BEGIN;
  ins_toBBlock_pass(ins_list);
  TIMER_END("ins_toBBlock_pass over!");

#ifdef DEBUG_MODE
  print_ins_pass(global_var_list);
#endif

  TranslateInit();
  //翻译全局变量表
  translate_global_variable_list(global_var_list);
  
  ListFirst(global_func_list, false);
  void *element;
  while (ListNext(global_func_list, &element)) {
#ifdef DEBUG_MODE
    puts(((Function *)element)->label->name);
#endif
    bblock_to_dom_graph_pass((Function *)element);
    register_replace((Function *)element);
  }

  //定义最后一个节点
  last = prev;
  //添加文字池
  add_interal_pool();
  //去除不必要的分支语句
  remove_unnessary_branch();
  //删除多余的寄存器
  delete_none_used_reg();
  //delete_unused_label();

  /* 生成文件 */
  freopen(argv[3], "w", stdout);
  print_model();

  free(tty_path);

#ifdef DEBUG_MODE
  // printf("%s test All over!\n",
  //        is_functional_test ? "functional" : "performance");
  // dup2(saveSTDOUT,STDOUT_FILENO);
#endif
  return 0;
}

#define MAX_LINE_LENGTH 10240
char *read_code_from_file(const char *file_path) {
  puts(file_path); 
  FILE *fd = fopen(file_path, "r");

  if (fd == NULL) {
    perror("fopen()");
    return NULL;
  }

  fseek(fd, 0, SEEK_END);
  long file_size = ftell(fd);
  fseek(fd, 0, SEEK_SET);

  char *buffer = (char *)malloc(file_size + 100);
  if (buffer == NULL) {
    printf("malloc() error\n");
    fclose(fd);
    return NULL;
  }
  memset(buffer, 0, file_size + 100);

  if (is_functional_test == false) {
    char buffer_line[MAX_LINE_LENGTH];
    int line_num = 0;
    size_t buffer_len = 0;
    while (fgets(buffer_line, MAX_LINE_LENGTH, fd)) {
      line_num++;
      char *found = strstr(buffer_line, "starttime();");
      if (found != NULL) {
        char new_buffer[50];
        sprintf(new_buffer, "_sysy_starttime(%d);\n", line_num);
        strcpy(found, new_buffer);
      }
      found = strstr(buffer_line, "stoptime();");
      if (found != NULL) {
        char new_buffer[50];
        sprintf(new_buffer, "_sysy_stoptime(%d);\n", line_num);
        strcpy(found, new_buffer);
      }
      strcat(buffer, buffer_line);
    }
#ifdef DEBUG_MODE
    printf("%s\n", buffer);
#endif
  } else {
    size_t bytes_read = fread(buffer, 1, file_size, fd);
    buffer[bytes_read] = '\0';
  }

  fclose(fd);
  return buffer;
}
