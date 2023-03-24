  // if嵌套的例子
  char *input1 =
      "int main() {"
      "  int a;"
      "  int b;"
      "  int c;"
      "  a = 5;"
      "  b = 10;"
      "  if (a == 5) {"
      "    a = 30;"
      "    b = 40;"
      "    c = a + b;"
      "    if (b == 20) {"
      "      c = 19;"
      "      a = 233;"
      "    }"
      "    a = 156;"
      "  } else {"
      "    a = 100;"
      "    b = 200;"
      "    c = 120 + a;"
      "  }"
      "  int m = 30, n = 23;"
      "  return a;"
      "}";

  // 函数调用的例子
  char *input2 =
      "int binary_add(int x, int y) {"
      "  int z = x + y;"
      "  return z;"
      "}"
      "int main() {"
      "  int a;"
      "  int b;"
      "  int c;"
      "  a = 10;"
      "  b = 20;"
      "  c = binary_add(a, (b + c));"
      "  return c;"
      "}";

  char *input3 =
      "int if_if_Else() {"
      "   int a;"
      "   a = 5;"
      "   int b;"
      "   b = 10;"
      "   if(a == 5){"
      "     if (b == 10)"
      "       a = 25;"
      "   }"
      "   else {"
      "     a = a + 15;"
      "   }"
      "   return (a);"
      " }"
      " int main(){"
      "   return (if_if_Else());"
      " }";

//多变量 （超过通用寄存器个数）
    char* input4 = 
        "int EightWhile() {"
        "int g;"
        "int h;"
        "int f;"
        "int e;"
        "int a;"
        "a = 5;"
        "int b;"
        "int c;"
        "b = 6;"
        "c = 7;"
        "int d;"
        "d = 10;"
        "while (a < 20) {"
        "    a = a + 3;"
        "    while(b < 10){"
        "    b = b + 1;"
        "    while(c == 7){"
        "        c = c - 1;"
        "        while(d < 20){"
        "        d = d + 3;"
        "        while(e > 1){"
        "            e = e-1;"
        "            while(f > 2){"
        "            f = f -2;"
        "            while(g < 3){"
        "                g = g +10;"
        "                while(h < 10){"
        "                h = h + 8;"
        "                }"
        "                h = h-1;"
        "            }"
        "            g = g- 8;"
        "            }"
        "            f = f + 1;"
        "        }"
        "        e = e + 1;"
        "        }"
        "        d = d - 1;"
        "    }"
        "    c = c + 1;"
        "    }"
        "    b = b - 2;"
        "}"
        
        "return (a + (b + d) + c)-(e + d - g + h);"
        "}"

        "int main() {"
        "int g;"
        "int h;"
        "int f;"
        "int e;"
        "g = 1;"
        "h = 2;"
        "e = 4;"
        "f = 6;"
        "return EightWhile();"
        "}";

char* input_floatPoint = 
    "int main()"
    "{"
    "   float fa = 3.5;"
    "   float fb = 4.5;"
    "   float fc;"
    "   int ic = 2;"
    "   int id = 6;"
//测试浮点加减
    "   fc = fa + fb;"
//测试f->i隐式转换
    "   ic = fa + fb;"
//测试i->f隐式转换
    "   fc = ic + id;"
//测试f+i->f混合转换
    "   fc = fa + ic;"
//测试f+i->i混合转换
    "   ic = fa + id;"
    "}";

char*   input_floatSimple = 
    "int main()"
    "{"
    "float fa = 3.5;"
    "float fb = 4.5;"
    "   int ic = fa;"
    "fa = ic;"
    "}";

/*
func_label1
entry
fa = immediateFloat
fb = immediateFloat
ic = immediateInt
id = immediateInt
new instruction temp1 = fa + fb
new instruction fc = temp1
new instruction temp2 = fa + fb
new instruction ic = temp2
new instruction temp3 = ic + id
new instruction fc = temp3
new instruction temp4 = fa + ic
new instruction fc = temp4
new instruction temp5 = fa + id
new instruction ic = temp5
func_label1 end
*/