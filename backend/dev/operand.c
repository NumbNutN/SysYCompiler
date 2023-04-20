/**
 * @brief AssembleOperand 将内存中的操作数加载到临时寄存器
 * @birth: Created by LGD on 20230130
*/
AssembleOperand operand_load_in_mem_throw(AssembleOperand op)
{
    AssembleOperand tempReg;
    switch(judge_operand_in_RegOrMem(op))
    {
        case IN_REGISTER:
            return op;
        case IN_MEMORY:
            switch(op.format)
            {
                case INTERGER_TWOSCOMPLEMENT:
                    tempReg.addrMode = REGISTER_DIRECT;
                    tempReg.oprendVal = pick_one_free_temp_arm_register();
                    tempReg.format = INTERGER_TWOSCOMPLEMENT;
                    memory_access_instructions("LDR",tempReg,op,NONESUFFIX,false,NONELABEL);
                break;
                case IEEE754_32BITS:
                    tempReg.addrMode = REGISTER_DIRECT;
                    tempReg.oprendVal = pick_one_free_vfp_register();
                    tempReg.format = IEEE754_32BITS;
                    vfp_memory_access_instructions("FLD",tempReg,op,FloatTyID);
                break;
            }
        return tempReg;
    }
    assert(judge_operand_in_RegOrMem(op) != IN_INSTRUCTION);
}