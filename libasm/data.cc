/*
 *	HT Editor
 *	data.cc
 *
 *	Copyright (C) 2002 Stefan Weyergraf (stefan@weyergraf.de)
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

#include <new>
#include <cstring>
#include <cstdlib>
#include <typeinfo>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "data.h"
#include "snprintf.h"

int autoCompare(const Object *a, const Object *b)
{
// FIXME: better use instanceOf
// SB: warum auskommentieren?
// SW: weil nicht so gute logik
//     wie gesagt FIXME, aber bin mir unsicher wie genau

/*	uint	ida = a->getObjectID();
	uint idb = b->getObjectID();
	if (ida != idb) return ida-idb;*/
	return a->compareTo(b);
}

/*
 *	Object
 */

int Object::compareTo(const Object *obj) const
{
//	int a=1;
	throw "NoImp";
}

int Object::toString(char *buf, int buflen) const
{
	ObjectID oid = getObjectID();
	unsigned char c[20];
	int l = 4;
	c[0] = (oid >> 24) & 0xff;
	c[1] = (oid >> 16) & 0xff;
	c[2] = (oid >>  8) & 0xff;
	c[3] = oid & 0xff;
	for (int i = 0; i < 4; i++) {
		if (c[i] < 32 || c[i] > 127) {
			c[l] = '\\';
			c[l+1] = "0123456789abcdef"[c[i] >> 4];
			c[l+2] = "0123456789abcdef"[c[i] & 0xf];
			l += 3;
		} else {
			c[l] = c[i];
			l++;
		}
	}
	c[l] = 0;
	return ht_snprintf(buf, buflen, "Object-%s", c+4);
}

Object *Object::clone() const
{
	throw "NoImp";
}

bool	Object::idle()
{
	return false;
}

ObjectID Object::getObjectID() const
{
	return OBJID_OBJECT;
}

