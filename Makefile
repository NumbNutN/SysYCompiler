CC = clang
CFLAGS = -Wall -Wextra -std=c99 -g
SRCDIR = .


BUILDDIR = build
TESTDIR = test_cases

# 递归查找当前目录及子目录中的所有以.c后缀结尾的文件
SRC := $(shell find $(SRCDIR) -name "*.c" -not -path "$(SRCDIR)/$(BUILDDIR)*" -not -path "$(SRCDIR)/$(TESTDIR)*")
OBJ := $(SRC:%.c=$(BUILDDIR)/%.o)

C_INCLUDES := \
-Iinclude \
-Ibackend/inc \
-Iinterface \
-Ilib/include \
-I.

C_LIB := -lm

CFLAGS += $(C_INCLUDES) 

EXECUTABLE = compiler

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ) 
	$(CC) $(CFLAGS) $^ -o $@ $(C_LIB)

$(BUILDDIR)/%.o: %.c Makefile | $(BUILDDIR)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@ 

$(BUILDDIR):
	@mkdir -p $@

clean:
	rm -rf $(BUILDDIR) $(EXECUTABLE)