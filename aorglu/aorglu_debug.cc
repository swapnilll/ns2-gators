/*
Debugging Stuff
Copyright (C) 2009 RGK, C.Hett C.Hollensen 

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
#include <stdio.h>
#include <stdarg.h>
#include <aorglu/aorglu_debug.h>

AORGLUDebugger debugger;

AORGLUDebugger::AORGLUDebugger()
{
 fprintf(stderr, "%7s | %25s | %25s | %10s | %5s\n","TIME","FILE","FUNCTION","LINE","MESSAGE");
 fprintf(stderr, "=====================================================================================\n");
}

AORGLUDebugger::~AORGLUDebugger()
{
 fprintf(stderr, "================================ END OF SIMULATION =================================\n");
}

void 
AORGLUDebugger::debug(const char file[],const char fn[],const int line, const char format[], ...)
{
  va_list ap;
  fprintf(stderr, "%-7.3lf | %25s | %25s | %5d | ",CURRENT_TIME, file, fn, line);

  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}

