CC = gcc
CFLAGS = -Wall -Wextra -g
SRCDIR = .


BUILDDIR = build
TESTDIR = test_cases

# 递归查找当前目录及子目录中的所有以.c后缀结尾的文件
SRC := $(shell find $(SRCDIR) -name "*.c" -not -path "$(SRCDIR)/$(BUILDDIR)*" -not -path "$(SRCDIR)/$(TESTDIR)*")
OBJ := $(SRC:%.c=$(BUILDDIR)/%.o)

HEAD := $(shell find $(SRCDIR) -name "*.h" -not -path "$(SRCDIR)/$(BUILDDIR)*" -not -path "$(SRCDIR)/$(TESTDIR)*")

C_INCLUDES := \
-Iinclude \
-Ibackend/inc \
-Iinterface \
-Ilib/include \
-I.

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