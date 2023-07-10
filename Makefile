CC = gcc
CFLAGS = -Wall -Wextra -std=c99
SRCDIR = .
BUILDDIR = build

# 递归查找当前目录及子目录中的所有以.c后缀结尾的文件
SRC := $(shell find $(SRCDIR) -name "*.c" -not -path "$(SRCDIR)/$(BUILDDIR)*")
OBJ := $(SRC:%.c=$(BUILDDIR)/%.o)

C_INCLUDES := \
-Iinclude \
-Ibackend/inc \
-Iinterface \
-Ilib/include \
-I.

CFLAGS += $(C_INCLUDES)

EXECUTABLE = myprogram

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	@mkdir -p $@