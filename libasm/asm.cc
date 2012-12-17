/*
 *	HT Editor
 *	asm.cc
 *
 *	Copyright (C) 1999-2002 Stefan Weyergraf
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2 as
 *	published by the Free Software Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "asm.h"
#include <cstring>
#include <cstdio>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>

#include "alphadis.h"
#include "ia64dis.h"
#include "ildis.h"
#include "javadis.h"
#include "x86dis.h"
#include "ppcdis.h"
#include "armdis.h"

/*
 *	CLASS Assembler
 */

Assembler::Assembler(bool b)
{
	codes = 0;
	bigendian = b;
}

Assembler::~Assembler()
{
	free_asm_codes();
}

asm_insn *Assembler::alloc_insn()
{
	return NULL;
}

void Assembler::deletecode(asm_code *code)
{
	asm_code **p=&codes, *c=codes;
	while (c) {
		if (c == code) {
			*p = c->next;
			delete c;
			return;
		}
		c = c->next;
		p = &(*p)->next;
	}
}

asm_code *Assembler::encode(asm_insn *asm_insn, int _options, CPU_ADDR cur_address)
{
	free_asm_codes();
	error = 0;
	options = _options;
	return 0;
}

void Assembler::clearcode()
{
	code.size = 0;
}

void Assembler::emitbyte(byte b)
{
	code.data[code.size] = b;
	code.size++;
}

void Assembler::emitword(uint16 w)
{
	if (bigendian) {
		code.data[code.size+1] = (byte)w;
		code.data[code.size+0] = (byte)(w>>8);
	} else {
		code.data[code.size+0] = (byte)w;
		code.data[code.size+1] = (byte)(w>>8);
	}
	code.size += 2;
}

void Assembler::emitdword(uint32 d)
{
	if (bigendian) {
		code.data[code.size+3] = (byte)d;
		code.data[code.size+2] = (byte)(d>>8);
		code.data[code.size+1] = (byte)(d>>16);
		code.data[code.size+0] = (byte)(d>>24);
	} else {
		code.data[code.size+0] = (byte)d;
		code.data[code.size+1] = (byte)(d>>8);
		code.data[code.size+2] = (byte)(d>>16);
		code.data[code.size+3] = (byte)(d>>24);
	}
	code.size += 4;
}

void Assembler::emitqword(uint64 q)
{
	if (bigendian) {
		code.data[code.size+7] = (byte)q;
		code.data[code.size+6] = (byte)(q>>8);
		code.data[code.size+5] = (byte)(q>>16);
		code.data[code.size+4] = (byte)(q>>24);
		code.data[code.size+3] = (byte)(q>>32);
		code.data[code.size+2] = (byte)(q>>40);
		code.data[code.size+1] = (byte)(q>>48);
		code.data[code.size+0] = (byte)(q>>56);
	} else {
		code.data[code.size+0] = (byte)q;
		code.data[code.size+1] = (byte)(q>>8);
		code.data[code.size+2] = (byte)(q>>16);
		code.data[code.size+3] = (byte)(q>>24);
		code.data[code.size+4] = (byte)(q>>32);
		code.data[code.size+5] = (byte)(q>>40);
		code.data[code.size+6] = (byte)(q>>48);
		code.data[code.size+7] = (byte)(q>>56);
	}
	code.size += 8;
}

void Assembler::free_asm_codes()
{
	while (codes) {
		asm_code *t = codes->next;
		delete codes;
		codes = t;
	}
}

const char *Assembler::get_error_msg()
{
	return error_msg;
}

const char *Assembler::get_name()
{
	return "generic asm";
}

void Assembler::newcode()
{
	code.size = 0;
}

asm_code *Assembler::shortest(asm_code *codes)
{
	asm_code *best = NULL;
	int bestv = INT_MAX;
	while (codes) {
		if (codes->size < bestv) {
			best = codes;
			bestv = codes->size;
		}
		codes = codes->next;
	}
	return best;
}

void Assembler::pushcode()
{
	asm_code **t=&codes;
	while (*t) {
		t = &(*t)->next;
	}
	*t = new asm_code;

	memcpy(*t, &code, sizeof code);
	(*t)->next = NULL;
}

void Assembler::set_error_msg(const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	vsprintf(error_msg, format, arg);
	va_end(arg);
	error=1;
}

void Assembler::set_imm_eval_proc(int (*p)(void *context, const char *s, uint64 &v), void *c)
{
	imm_eval_proc = p;
	imm_eval_context = c;
}

/*
 *	CLASS disassembler
 */

Disassembler::Disassembler()
{
	disable_highlighting();
}

char* (*addr_sym_func)(CPU_ADDR addr, int *symstrlen, void *context) = NULL;
void* addr_sym_func_context = NULL;

dis_insn *Disassembler::createInvalidInsn()
{
	return NULL;
}

void Disassembler::hexd(char **s, int size, int options, uint32 imm)
{
	char ff[16];
	char *f = (char*)&ff;
	char *t = *s;
	*f++ = '%';
	if (imm >= 0 && imm <= 9) {
		*s += sprintf(*s, "%d", imm);
	} else if (options & DIS_STYLE_SIGNED) {
		if (!(options & DIS_STYLE_HEX_NOZEROPAD)) f += sprintf(f, "0%d", size);
		*f++ = 'd';
		*f = 0;
		*s += sprintf(*s, ff, imm);
	} else {
		if (options & DIS_STYLE_HEX_CSTYLE) *f++ = '#';
		if (!(options & DIS_STYLE_HEX_NOZEROPAD)) f += sprintf(f, "0%d", size);
		if (options & DIS_STYLE_HEX_UPPERCASE) *f++ = 'X'; else
			*f++ = 'x';
		if (options & DIS_STYLE_HEX_ASMSTYLE) *f++ = 'h';
		*f = 0;
		*s += sprintf(*s, ff, imm);
		if ((options & DIS_STYLE_HEX_NOZEROPAD) && (*t-'0'>9)) {
			memmove(t+1, t, strlen(t)+1);
			*t = '0';
			(*s)++;
		}
	}
}

void Disassembler::hexq(char **s, int size, int options, uint64 imm)
{
	char ff[32];
	char *f = (char*)&ff;
	char *t = *s;
	*f++ = '%';
	if (imm >= 0 && imm <= 9) {
		*s += ht_snprintf(*s, 32, "%qd", imm);
	} else if (options & DIS_STYLE_SIGNED) {
		if (!(options & DIS_STYLE_HEX_NOZEROPAD)) f += sprintf(f, "0%d", size);
		*f++ = 'q';
		*f++ = 'd';
		*f = 0;
		*s += ht_snprintf(*s, 32, ff, imm);
	} else {
		if (options & DIS_STYLE_HEX_CSTYLE) *f++ = '#';
		if (!(options & DIS_STYLE_HEX_NOZEROPAD)) f += sprintf(f, "0%d", size);
		if (options & DIS_STYLE_HEX_UPPERCASE) *f++ = 'X'; else
		*f++ = 'q';
		*f++ = 'x';
		if (options & DIS_STYLE_HEX_ASMSTYLE) *f++ = 'h';
		*f = 0;
		*s += ht_snprintf(*s, 32, ff, imm);
		if ((options & DIS_STYLE_HEX_NOZEROPAD) && (*t-'0'>9)) {
			memmove(t+1, t, strlen(t)+1);
			*t = '0';
			(*s)++;
		}
	}
}

bool Disassembler::selectNext(dis_insn *disasm_insn)
{
	return false;
}

const char *Disassembler::str(dis_insn *disasm_insn, int style)
{
	return strf(disasm_insn, style, DISASM_STRF_DEFAULT_FORMAT);
}

const char *Disassembler::get_cs(AsmSyntaxHighlightEnum style)
{
	const char *highlights[] = {
		ASM_SYNTAX_DEFAULT,
		ASM_SYNTAX_COMMENT,
		ASM_SYNTAX_NUMBER,
		ASM_SYNTAX_SYMBOL,
		ASM_SYNTAX_STRING
	};
	return highlight ? highlights[(int)style] : "";
}

void Disassembler::enable_highlighting()
{
	highlight = true;
}

void Disassembler::disable_highlighting()
{
	highlight = false;
}

bool init_asm()
{
// 	REGISTER(ATOM_DISASM_X86, x86dis)
// 	REGISTER(ATOM_DISASM_X86_VXD, x86dis_vxd)
// 	REGISTER(ATOM_DISASM_ALPHA, Alphadis)
// 	REGISTER(ATOM_DISASM_JAVA, javadis)
// 	REGISTER(ATOM_DISASM_IA64, IA64Disassembler)
// 	REGISTER(ATOM_DISASM_PPC, PPCDisassembler)
// 	REGISTER(ATOM_DISASM_IL, ILDisassembler)
// 	REGISTER(ATOM_DISASM_X86_64, x86_64dis)
// 	REGISTER(ATOM_DISASM_ARM, ArmDisassembler)
	return true;
}

void done_asm()
{
// 	UNREGISTER(ATOM_DISASM_ARM, ArmDisassembler)
// 	UNREGISTER(ATOM_DISASM_X86_64, x86dis)
// 	UNREGISTER(ATOM_DISASM_IL, ILDisassembler)
// 	UNREGISTER(ATOM_DISASM_PPC, PPCDisassembler)
// 	UNREGISTER(ATOM_DISASM_IA64, IA64Disassembler)
// 	UNREGISTER(ATOM_DISASM_JAVA, javadis)
// 	UNREGISTER(ATOM_DISASM_ALPHA, Alphadis)
// 	UNREGISTER(ATOM_DISASM_X86_VXD, x86dis_vxd)
// 	UNREGISTER(ATOM_DISASM_X86, x86dis)
}









/**
 *	Like strcpy but copies a maximum of |maxlen| characters
 *	(including trailing zero).
 *	The operation is performed in a way that the trailing zero
 *	is always written if maxlen is > 0.
 *	@returns number of characters copied (without trailing zero)
 */
size_t ht_strlcpy(char *s1, const char *s2, size_t maxlen)
{
	if (!maxlen) return 0;
	char *os1 = s1;
	while (true) {
		if (!--maxlen) {
			*s1 = 0;
			return s1 - os1;
		}
		*s1 = *s2;
		if (!*s2) return s1 - os1;
		s1++; s2++;
	}
}

int ht_strnicmp(const char *s1, const char *s2, size_t max)
{
	if (!s1) return s2 ? -1 : 0;
	if (!s2) return s1 ? 1 : 0;
	while (max--) {
		if (!*s1) return *s2 ? -1 : 0;
		if (!*s2) return *s1 ? 1 : 0;
		char c1=tolower(*s1), c2=tolower(*s2);
		if (c1>c2) {
			return 1;
		} else if (c1<c2) {
			return -1;
		}
		s1++;s2++;
	}
	return 0;
}

/* hex/string functions */

int hexdigit(char a)
{
	if (a >= '0' && a <= '9') {
		return a-'0';
	} else if (a >= 'a' && a <= 'f') {
		return a-'a'+10;
	} else if (a >= 'A' && a <= 'F') {
		return a-'A'+10;
	}
	return -1;
}

bool str2int(const char *str, uint64 &u64, int defaultbase)
{
	uint base = defaultbase;
	size_t len = strlen(str);
	if (!len) return false;
	bool n = false;
	if (defaultbase == 10) {
		if (ht_strnicmp("0x", str, 2) == 0) {
			str += 2;
			base = 16;
			len -= 2;
			if (!len) return false;
		} else {
			switch (tolower(str[len-1])) {
			case 'b': base = 2; break;
			case 'o': base = 8; break;
			case 'h': base = 16; break;
			default: goto skip;
			}
			len--;
			if (!len) return false;
skip:
			if (str[0] == '-') {
				str++; len--;
				if (!len) return false;
				n = true;
			}
		}
	}
	u64 = 0;
	do {
		int c = hexdigit(str[0]);
		if (c == -1 || c >= int(base)) return false;
		u64 *= base;
		u64 += c;
		str++;
	} while (--len);
	if (n) u64 = -u64;
	return true;
}

int ht_strncmp(const char *s1, const char *s2, size_t max)
{
	if (!s1) return s2 ? -1 : 0;
	if (!s2) return s1 ? 1 : 0;
	while (max--) {
		if (!*s1) return *s2 ? -1 : 0;
		if (!*s2) return *s1 ? 1 : 0;
		if (*s1>*s2) {
			return 1;
		} else if (*s1<*s2) {
			return -1;
		}
		s1++;s2++;
	}
	return 0;
}

void whitespaces(const char *&str)
{
	while ((unsigned char)*str <= 32) {
		if (!*str) return;
		str++;
	}
}

void non_whitespaces(const char *&str)
{
	while ((unsigned char)*str > 32) {
		str++;
	}
}

bool waitforchar(const char *&str, char b)
{
	while (*str != b) {
		if (!*str) return false;
		str++;
	}
	return true;
}

/* common string parsing functions */
bool is_whitespace(char c)
{
	return c && (unsigned char)c <= 32;
}
