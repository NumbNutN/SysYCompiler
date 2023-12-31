
/// LLVM Basic Block Representation
///
/// This represents a single basic block in LLVM. A basic block is simply a
/// container of instructions that execute sequentially. Basic blocks are Values
/// because they are referenced by instructions such as branches and switch
/// tables. The type of a BasicBlock is "Type::LabelTy" because the basic block
/// represents a label to which a branch can jump.
///
/// A well formed basic block is formed of a list of non-terminating
/// instructions followed by a single terminator instruction. Terminator
/// instructions may not occur in the middle of basic blocks, and must terminate
/// the blocks. The BasicBlock class allows malformed basic blocks to occur
/// because it may be useful in the intermediate stage of constructing or
/// modifying a program. However, the verifier will ensure that basic blocks are
/// "well formed".

#ifndef BASIC_BLOCK_H
#define BASIC_BLOCK_H
#include "c_container_auxiliary.h"
#include "cds.h"
#include "function.h"
#include "instruction.h"
#include "value.h"

typedef struct _BasicBlock {
  // 指向label节点的value 等同于label节点里所包含的value*
  Value* label;

  // basicblock中的语句链
  List* inst_list;
  // 父块和子块
  List* father_bblock_list;

  struct _BasicBlock* true_bblock;
  struct _BasicBlock* false_bblock;

  // 所属的函数
  Function* parent;

  HashSet* live_def;
  HashSet* live_use;
  HashSet* live_in;
  HashSet* live_out;
} BasicBlock;

void bblock_init(BasicBlock* this, Function* parent);

void bblock_add_inst_back(BasicBlock* this, Instruction* inst);

Instruction* bblock_pop_inst_back(BasicBlock* this);

#endif