/*
Debug Stuff
Copyright (C) 2009 RGK 

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

This source was created to use with NS-2.
*/

#ifndef __aorglu_debug_h__
#define __aorglu_debug_h__

#include <assert.h>
#include <sys/types.h>
#include <config.h>
#include <lib/bsd-list.h>
#include <scheduler.h>
#define DEBUG
class AORGLUDebugger
{
  public:
    AORGLUDebugger();
    ~AORGLUDebugger();
    void debug(const char file[],const char fn[],const int line, const char format[], ...);
};

extern AORGLUDebugger debugger;

#ifdef DEBUG
#define _DEBUG(format,...) debugger.debug(__FILE__,__FUNCTION__,__LINE__,format, ##__VA_ARGS__)
#else
#define _DEBUG(format,...) ((void*)0)
#endif

#define CURRENT_TIME    Scheduler::instance().clock()

#endif /* _aorglu__debug_h__ */
