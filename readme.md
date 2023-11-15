### SYSY编译器

#### 简述

​		这是一个基于sysy语法范式，对目标平台Armv7 Architecture生成A32指令的编译器，主要包括两个模块：

+ 词法、语法分析、对Intermediate Representation的优化工作。

+ 中间代码到目标指令集的等效表达



#### 关于分支

+ dev分支为开发分支，合并来自上游仓库的稳定版本，请确保当前dev分支的最新提交解决了正在解决的问题后合并到main分支。

+ main分支的每次提交往往合并自dev分支提交的稳定版本，main分支的每次提交一般可以直接提交到oj平台。



#### 工程构建

dev分支使用cmake工具构建工程，构建步骤如下：

```
cd build
cmake ..
make
```

后在仓库第一级路径得到单一的编译器可执行文件compiler



main分支为了符合oj编译方式，采用make工具构建工程，构建步骤如下：

```
make
```

后在仓库第一级路径得到单一的编译器可执行文件compiler



#### 当前进度

| progress                     | check                |
| ---------------------------- | -------------------- |
| GNU assembler syntax support | :white_check_mark:   |
| Public Interface Callable    | :white_check_mark:   |
| Zero Init Section            | :white_check_mark:   |
| SIMD                         | :white_large_square: |



#### 样例测试

计算机系统能力大赛（CSCC）提供测试样例库

[公开样例与运行时库/functional · master · CSC-Compiler / Compiler2023 · GitLab (eduxiji.net)](https://gitlab.eduxiji.net/nscscc/compiler2023/-/tree/master/公开样例与运行时库/functional)



| example | correction |
| ------- | ---------- |
| functional |         |
|00_main.sy|:heavy_check_mark:|
|01_var_defn2.sy|:heavy_check_mark:|
|02_var_defn3.sy|:heavy_check_mark:|
|03_arr_defn2.sy|:heavy_check_mark:|
|04_arr_defn3.sy|:heavy_check_mark:|
|05_arr_defn4.sy|:heavy_check_mark:|
|06_const_var_defn2.sy|:heavy_check_mark:|
|07_const_var_defn3.sy|:heavy_check_mark:|
|08_const_array_defn.sy|:heavy_check_mark:|
|09_func_defn.sy|:heavy_check_mark:|
|10_var_defn_func.sy|:heavy_check_mark:|
|11_add2.sy|:heavy_check_mark:|
|12_addc.sy|:heavy_check_mark:|
|13_sub2.sy|:heavy_check_mark:|
|14_subc.sy|:heavy_check_mark:|
|15_mul.sy|:heavy_check_mark:|
|16_mulc.sy|:heavy_check_mark:|
|17_div.sy|:heavy_check_mark:|
|18_divc.sy|:heavy_check_mark:|
|19_mod.sy|:heavy_check_mark:|
|20_rem.sy|:x:|
|21_if_test2.sy|:heavy_check_mark:|
|22_if_test3.sy|:heavy_check_mark:|
|23_if_test4.sy|:heavy_check_mark:|
|24_if_test5.sy|:heavy_check_mark:|
|25_while_if.sy|:heavy_check_mark:|
|26_while_test1.sy|:heavy_check_mark:|
|27_while_test2.sy|:heavy_check_mark:|
|28_while_test3.sy|:heavy_check_mark:|
|29_break.sy|:heavy_check_mark:|
|30_continue.sy|:heavy_check_mark:|
|31_while_if_test1.sy|:heavy_check_mark:|
|32_while_if_test2.sy|:heavy_check_mark:|
|33_while_if_test3.sy|:heavy_check_mark:|
|34_arr_expr_len.sy|:heavy_check_mark:|
|35_op_priority1.sy|:heavy_check_mark:|
|36_op_priority2.sy|:heavy_check_mark:|
|37_op_priority3.sy|:heavy_check_mark:|
|38_op_priority4.sy|:heavy_check_mark:|
|39_op_priority5.sy|:heavy_check_mark:|
|40_unary_op.sy|:heavy_check_mark:|
|41_unary_op2.sy|:heavy_check_mark:|
|42_empty_stmt.sy|:heavy_check_mark:|
|43_logi_assign.sy|:heavy_check_mark:|
|44_stmt_expr.sy|:heavy_check_mark:|
|45_comment1.sy|:heavy_check_mark:|
|46_hex_defn.sy|:heavy_check_mark:|
|47_hex_oct_add.sy|:heavy_check_mark:|
|48_assign_complex_expr.sy|:x:|
|49_if_complex_expr.sy|:heavy_check_mark:|
|50_short_circuit.sy|:heavy_check_mark:|
|51_short_circuit3.sy|:heavy_check_mark:|
|52_scope.sy|:heavy_check_mark:|
|53_scope2.sy|:heavy_check_mark:|
|54_hidden_var.sy|:heavy_check_mark:|
|55_sort_test1.sy|:heavy_check_mark:|
|56_sort_test2.sy|:heavy_check_mark:|
|57_sort_test3.sy|:heavy_check_mark:|
|58_sort_test4.sy|:heavy_check_mark:|
|59_sort_test5.sy|:heavy_check_mark:|
|60_sort_test6.sy|:heavy_check_mark:|
|61_sort_test7.sy|:heavy_check_mark:|
|62_percolation.sy|:heavy_check_mark:|
|63_big_int_mul.sy|:heavy_check_mark:|
|64_calculator.sy|:heavy_check_mark:|
|65_color.sy|:heavy_check_mark:|
|66_exgcd.sy|:heavy_check_mark:|
|67_reverse_output.sy|:heavy_check_mark:|
|68_brainfk.sy|:heavy_check_mark:|
|69_expr_eval.sy|:heavy_check_mark:|
|70_dijkstra.sy|:heavy_check_mark:|
|71_full_conn.sy|:heavy_check_mark:|
|72_hanoi.sy|:heavy_check_mark:|
|73_int_io.sy|:heavy_check_mark:|
|74_kmp.sy|:heavy_check_mark:|
|75_max_flow.sy|:heavy_check_mark:|
|76_n_queens.sy|:heavy_check_mark:|
|77_substr.sy|:heavy_check_mark:|
|78_side_effect.sy|:heavy_check_mark:|
|79_var_name.sy|:heavy_check_mark:|
|80_chaos_token.sy|:heavy_check_mark:|
|81_skip_spaces.sy|:heavy_check_mark:|
|82_long_func.sy|:heavy_check_mark:|
|83_long_array.sy|:heavy_check_mark:|
|84_long_array2.sy|:heavy_check_mark:|
|85_long_code.sy|:heavy_check_mark:|
|86_long_code2.sy|:heavy_check_mark:|
|87_many_params.sy|:x:|
|88_many_params2.sy|:heavy_check_mark:|
|89_many_globals.sy|:heavy_check_mark:|
|90_many_locals.sy|:heavy_check_mark:|
|91_many_locals2.sy|:heavy_check_mark:|
|92_register_alloc.sy|:heavy_check_mark:|
|93_nested_calls.sy|:heavy_check_mark:|
|94_nested_loops.sy|:heavy_check_mark:|
|95_float.sy|:x:|
|96_matrix_add.sy|:heavy_check_mark:|
|97_matrix_sub.sy|:heavy_check_mark:|
|98_matrix_mul.sy|:heavy_check_mark:|
|99_matrix_tran.sy|:heavy_check_mark:|
| hidden_function |                  |
|00_comment2.sy|:heavy_check_mark:|
|01_multiple_returns.sy|:heavy_check_mark:|
|02_ret_in_block.sy|:x:|
|03_branch.sy|:heavy_check_mark:|
|04_break_continue.sy|:heavy_check_mark:|
|05_param_name.sy|:heavy_check_mark:|
|06_func_name.sy|:heavy_check_mark:|
|07_arr_init_nd.sy|:heavy_check_mark:|
|08_global_arr_init.sy|:heavy_check_mark:|
|09_BFS.sy|:heavy_check_mark:|
|10_DFS.sy|:heavy_check_mark:|
|11_BST.sy|:heavy_check_mark:|
|12_DSU.sy|:heavy_check_mark:|
|13_LCA.sy|:heavy_check_mark:|
|14_dp.sy|:heavy_check_mark:|
|15_graph_coloring.sy|:heavy_check_mark:|
|16_k_smallest.sy|:heavy_check_mark:|
|17_maximal_clique.sy|:heavy_check_mark:|
|18_prim.sy|:x:|
|19_search.sy|:heavy_check_mark:|
|20_sort.sy|:heavy_check_mark:|
|21_union_find.sy|:heavy_check_mark:|
|22_matrix_multiply.sy|:heavy_check_mark:|
|23_json.sy|:heavy_check_mark:|
|24_array_only.sy|:heavy_check_mark:|
|25_scope3.sy|:x:|
|26_scope4.sy|:heavy_check_mark:|
|27_scope5.sy|:heavy_check_mark:|
|28_side_effect2.sy|:heavy_check_mark:|
|29_long_line.sy|:x:|
|30_many_dimensions.sy|:heavy_check_mark:|
|31_many_indirections.sy|:heavy_check_mark:|
|32_many_params3.sy|:heavy_check_mark:|
|33_multi_branch.sy|:heavy_check_mark:|
|34_multi_loop.sy|:heavy_check_mark:|
|35_math.sy|:x:|
|36_rotate.sy|:x:|
|37_dct.sy|:x:|
|38_light2d.sy|:x:|
|39_fp_params.sy|:x:|

