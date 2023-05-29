#include <stdio.h>

#include "arm.h"

//enum_2_str
#include "enum_2_str.h"

enum Section currentSection = NONESECTION;

void change_currentSection(enum Section c)
{
    if(c != currentSection)
        printf("%s %s\n",".section",enum_section_2_str(c));
    currentSection = c;
}