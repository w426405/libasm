#include "x86RevDis.h"


void x86RevDis::RevDisasm(uint32 uStartAddr, uint32 uCurAddr)
{
	CPU_ADDR	curAddr;
	curAddr.addr32.offset = uCurAddr;

	for (unsigned i=uCurAddr-uStartAddr;i<m_uSize;)
	{
		x86dis_insn* insn = (x86dis_insn*)m_decoder.decode(m_pCode+i,m_uSize-i,curAddr);

		m_mapResult.insert(std::map<uint32,x86dis_insn>::value_type(curAddr.addr32.offset,*insn));

		const char* pcsIns = m_decoder.str(insn,DIS_STYLE_HEX_ASMSTYLE | DIS_STYLE_HEX_UPPERCASE | DIS_STYLE_HEX_NOZEROPAD);
		//printf("%08X\t%s\n",curAddr.addr32.offset, pcsIns);
		i += insn->size;
		curAddr.addr32.offset += insn->size;

		switch (isBranch(insn))
		{
		case br_return:
			return;
		case br_jump:
			{
				CPU_ADDR branch = branchAddr(insn);
				if (branch.addr32.offset<uStartAddr
					|| branch.addr32.offset>uStartAddr+m_uSize)
				{
					//跳转到的地址不在本段代码内
					break;
				}

				std::map<uint32,x86dis_insn>::iterator it = m_mapResult.find(branch.addr32.offset);
				if (it!=m_mapResult.end())
				{
					//已经反过了
					break;
				}
				return RevDisasm(uStartAddr,branch.addr32.offset);
			}
		case br_jXX:
		case br_call:
			{
				CPU_ADDR branch = branchAddr(insn);
				if (branch.addr32.offset<uStartAddr
					|| branch.addr32.offset>uStartAddr+m_uSize)
				{
					//跳转到的地址不在本段代码内
					break;
				}

				std::map<uint32,x86dis_insn>::iterator it = m_mapResult.find(branch.addr32.offset);
				if (it!=m_mapResult.end())
				{
					//已经反过了
					break;
				}
				RevDisasm(uStartAddr,branch.addr32.offset);
			}
			break;
		}
	}

}


x86RevDis::branch_enum_t x86RevDis::isBranch(x86dis_insn* opcode)
{
	const char *opcode_str = opcode->name;
	if (opcode_str[0] == '~')
	{
		opcode_str++;
	}
	if (opcode_str[0] == '|')
	{
		opcode_str++;
	}

	if (opcode_str[0]=='j')
	{
		if (opcode_str[1]=='m') return br_jump; else return br_jXX;
	}
	else if ((opcode_str[0]=='l') && (opcode_str[1]=='o')  && (opcode_str[2]=='o'))
	{
		// loop opcode will be threated like a jXX
		return br_jXX;
	}
	else if ((opcode_str[0]=='c') && (opcode_str[1]=='a'))
	{
		return br_call;
	}
	else if ((opcode_str[0]=='r') && (opcode_str[1]=='e'))
	{
		return br_return;
	}
	else return br_nobranch;
}

CPU_ADDR x86RevDis::branchAddr(x86dis_insn *opcode)
{
	CPU_ADDR addr = {0};
	//assert(o->op[1].type == X86_OPTYPE_EMPTY);
	if (opcode->op[1].type != X86_OPTYPE_EMPTY)
	{
		return addr;
	}
	switch (opcode->op[0].type)
	{
	case X86_OPTYPE_IMM:
		{		
			addr.addr32.offset = opcode->op[0].imm;
		}
		break;
		// 	case X86_OPTYPE_FARPTR:
	case X86_OPTYPE_MEM:
		{
			if (opcode->op[0].mem.hasdisp)
			{
				uint32 disp = opcode->op[0].mem.disp;
				if (disp >= m_uStartAddr && disp <= (m_uStartAddr + m_uSize))
				{
					addr.addr32.offset = *(uint32*)(m_pCode+disp-m_uStartAddr);
				}
			}
		}
		break;
	default: break;
	}
	return addr;
}

void x86RevDis::Process( void )
{
	if (m_vecDisaddr.size() == 0)
	{
		return;
	}

	for (std::vector<uint32>::iterator it=m_vecDisaddr.begin();
		it!=m_vecDisaddr.end();++it)
	{
		RevDisasm(m_uStartAddr,*it);
	}
	uint32	uCurAddr = m_uStartAddr;
	for (int i=0;i<m_uSize;)
	{
		std::map<uint32,x86dis_insn>::iterator it = m_mapResult.find(uCurAddr);
		if (it!=m_mapResult.end())
		{
			x86dis_insn tmp = it->second;
			const char* pcsIns = m_decoder.str(&tmp,
				DIS_STYLE_HEX_ASMSTYLE | DIS_STYLE_HEX_UPPERCASE 
				| DIS_STYLE_HEX_NOZEROPAD | DIS_STYLE_SIGNED);
			printf("%u\t%s\n",uCurAddr,pcsIns);
			uCurAddr += it->second.size;
			i += it->second.size;
		}
		else
		{
			printf("%u\tDB %02X\n",uCurAddr,m_pCode[i]);
			i++;
			uCurAddr++;
		}
	}

}

