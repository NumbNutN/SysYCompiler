CC = clang
CFLAGS = -Wall -Wextra -g --std=c11 -O2 -lm

SRCDIR = .
BUILDDIR = ./build
TESTCASESDIR = ./test_cases
MYDIR = ./my_cases
SYSYDIR = ./sysy
TESTDIR = ./test

# 递归查找当前目录及子目录中的所有以.c后缀结尾的文件
SRC := $(shell find $(SRCDIR) -name "*.c" -not -path "$(BUILDDIR)*" -not -path "$(TESTDIR)*" -not -path "$(MYDIR)*" -not -path "$(SYSYDIR)*" -not -path "$(TESTCASESDIR)*")
OBJ := $(SRC:%.c=$(BUILDDIR)/%.o)

HEAD := $(shell find $(SRCDIR) -name "*.h" -not -path "$(SRCDIR)/$(BUILDDIR)*" -not -path "$(SRCDIR)/$(TESTDIR)*")

C_INCLUDES := \
-Iinclude \
-Ibackend/inc \
-Iinterface \
-Ilib/include \
-I. \
-Icontainer/include

C_LIB := -lm

CFLAGS += $(C_INCLUDES) $(C_DEFINE) 

EXECUTABLE = compiler

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ) 
	$(CC) $(CFLAGS) $^ -o $@ $(C_LIB)

$(BUILDDIR)/%.o: %.c Makefile $(HEAD) | $(BUILDDIR)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@ 

$(BUILDDIR):
	@mkdir -p $@

clean:
	rm -rf $(BUILDDIR) $(EXECUTABLE)