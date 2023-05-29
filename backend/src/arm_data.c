#include <stdio.h>
#include "operand.h"

/**
@brief: .long表达式
@birth:Created by LGD on 2023-5-29
*/
void dot_long_expression(char* name,struct _operand expr)
{
    if(currentSection != DATA)
        change_currentSection(DATA);
        
    assert(expr.addrMode == IMMEDIATE && ".long expression could only be integer literal");
    if(name != NULL)
        printf("%s\t",name);
    else
        printf("\t\t");
    printf("%s\t%ld\n",".long",expr.oprendVal);
}

void dot_zero_expression(char* name)
{
    if(currentSection != BSS)
        change_currentSection(BSS);
    if(name != NULL)
        printf("%s\t",name);
    else
        printf("\t\t");
    printf("%s\t%d\n",".zero",4);
}

