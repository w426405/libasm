/*
 *	HT Editor
 *	data.h
 *
 *	Copyright (C) 2002, 2003 Stefan Weyergraf (stefan@weyergraf.de)
 *	Copyright (C) 2002, 2003 Sebastian Biallas (sb@biallas.net)
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

#ifndef __DATA_H__
#define __DATA_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include "types.h"
#include "snprintf.h"


typedef uint32 ObjectID;
typedef uint32 ID;

class ObjectStream;

struct BuildCtorArg {
};


template <typename T1, typename T2>
inline bool instanceOf(const T2 *o)
{
	return (dynamic_cast<const T1*>(o) != NULL);
} 

/*
 *	C style malloc support
 */

class HTMallocRes;
HTMallocRes ht_malloc(size_t);

class HTMallocRes
{
private:
	friend HTMallocRes ht_malloc(size_t);
	const size_t mSize;

	HTMallocRes(size_t size)
		: mSize(size)
	{
	}

	HTMallocRes operator=(const HTMallocRes &); // not implemented

public:
	template <typename T> operator T* () const
	{
		return static_cast<T*>(::malloc(mSize));
	}
};

inline HTMallocRes ht_malloc(size_t size)
{
	return HTMallocRes(size);
}


/**
 *	Macro for creating object build functions
 */
#define BUILDER(reg, obj, parent) Object *build_##obj(){BuildCtorArg a;return new obj(a);}
#define BUILDER2(reg, obj) Object *build_##obj(){BuildCtorArg a;return new obj(a);}

/**
 *	Registers builder function by object id.
 */
#define REGISTER(reg, obj) registerAtom(reg, (void*)build_##obj);

/**
 *	Unregisters builder function by object id.
 */
#define UNREGISTER(reg, obj) unregisterAtom(reg);

/* actually a str => bigendian-int */
/** used to define ObjectIDs */
#define MAGIC32(magic) (unsigned long)(((unsigned char)magic[0]<<24) | ((unsigned char)magic[1]<<16) | ((unsigned char)magic[2]<<8) | (unsigned char)magic[3])

/** No/invalid object */
#define OBJID_INVALID			((ObjectID)0)
/** A placeholder object id */
#define OBJID_TEMP			((ObjectID)-1)

#define OBJID_OBJECT			MAGIC32("DAT\x00")

#define OBJID_ARRAY			MAGIC32("DAT\x10")
#define OBJID_STACK			MAGIC32("DAT\x11")

#define OBJID_BINARY_TREE		MAGIC32("DAT\x20")
#define OBJID_AVL_TREE			MAGIC32("DAT\x21")
#define OBJID_SET			MAGIC32("DAT\x22")

#define OBJID_SLINKED_LIST		MAGIC32("DAT\x30")
#define OBJID_QUEUE			MAGIC32("DAT\x31")
#define OBJID_DLINKED_LIST		MAGIC32("DAT\x32")

#define OBJID_KEYVALUE			MAGIC32("DAT\x40")
#define OBJID_SINT			MAGIC32("DAT\x41")
#define OBJID_SINT64			MAGIC32("DAT\x42")
#define OBJID_UINT			MAGIC32("DAT\x43")
#define OBJID_UINT64			MAGIC32("DAT\x44")
#define OBJID_FLOAT			MAGIC32("DAT\x45")

#define OBJID_MEMAREA			MAGIC32("DAT\x48")

#define OBJID_STRING			MAGIC32("DAT\x50")
#define OBJID_ISTRING			MAGIC32("DAT\x51")

#define OBJID_AUTO_COMPARE		MAGIC32("DAT\xc0")

/**
 *	This is THE base class.
 */
class Object {
public:
				Object(BuildCtorArg&) {};
				Object() {};

	virtual			~Object() {};
		void		init() {};
	virtual	void		done() {};
/* new */

/**
 *	Standard object duplicator.
 *	@returns copy of object
 */
	virtual	Object *	clone() const;
/**
 *	Standard Object comparator.
 *	@param obj object to compare to
 *	@returns 0 for equality, negative number if |this<obj| and positive number if |this>obj|
 */
	virtual	int		compareTo(const Object *obj) const;
/**
 *	Stringify object.
 *	Stringify object in string-buffer <i>s</i>. Never writes more than
 *	<i>maxlen</i> characters to <i>s</i>. If <i>maxlen</i> is > 0, a
 *	trailing zero-character is written.
 *
 *	@param buf pointer to buffer that receives object stringification
 *	@param buflen size of buffer that receives object stringification
 *	@returns number of characters written to <i>s</i>, not including the trailing zero
 */
	virtual	int		toString(char *buf, int buflen) const;
/**
 *	Standard Object idle function.
 *	Overwrite and register with htidle.cc::register_idle()
 *	(FIXME)
 *
 *	@returns true if working, false if really idle
 */
	virtual	bool		idle();
/**
 *	@returns unique object id.
 */
	virtual	ObjectID	getObjectID() const;
};

typedef int (*Comparator)(const Object *a, const Object *b);

int autoCompare(const Object *a, const Object *b);

typedef void* ObjHandle;
const ObjHandle invObjHandle = NULL;
const uint invIdx = ((uint)-1);

#endif /* __DATA_H__ */
