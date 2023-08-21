#include "instruction.h"

enum _LSL_FEATURE_ENUM{
    LSL_NORMAL,
    LSL_ZERO,
    _2_N,
    _2_N_ADD_1,
    _2_N_SUB_1,
    _2_M_N_2_N_1
};

struct _LSL_FEATURE  {
    enum _LSL_FEATURE_ENUM feature;
    uint8_t b1;  /* 对前两种枚举生效 描述1在第几位 */
    uint8_t b2;  /* 对后一种枚举生效 描述第二个1在第几位 */
    uint8_t op_idx;
};

/**
 * @brief 获取右移位数
 * @birth: Created by LGD on 2023-8-20
*/
uint8_t number_get_shift_time(int32_t num);

bool number_is_lsl_trigger(Instruction* this,struct _LSL_FEATURE* feat);

/**
 * @brief 判断除数是否可以优化
 * @birth: Created by LGD on 2023-8-20
*/
bool div_optimize_trigger(Instruction* this);

/**
 * @brief 乘法可优化数
 * @birth: Created by LGD on 2023-8-20
*/
struct _LSL_FEATURE number_is_lsl(int32_t num);

/**
 * @brief 乘法转移码条件触发器
 * @update: Created by LGD on 2023-4-20
*/
bool ins_mul_2_lsl_trigger(Instruction* ins);

/**
 * @brief 乘法移位优化
 * @update: Created by LGD on 2023-4-19
*/
void ins_mul_2_lsl(Instruction* ins);

/**
 * @brief 当判断出强制跳转语句接一个标号时，可以删去该跳转
 * @birth: Created by LGD on 2023-8-9
*/
void remove_unnessary_branch(void);

/**
 * @brief 删除和消去多余的寄存器
 * @birth: Created by LGD on 2023-8-21
*/
void delete_none_used_reg();