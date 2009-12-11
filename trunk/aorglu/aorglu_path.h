/*
Location Cache
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

#ifndef __aorglu_path_h__
#define __aorglu_path_h__

#include <assert.h>
#include <sys/types.h>
#include <config.h>
#include <lib/bsd-list.h>
#include <scheduler.h>

#define CURRENT_TIME Scheduler::instance().clock()

/*
   Path Entry
*/

class aorglu_path_entry {
        friend class AORGLU;
	friend class aorglu_path;
 public:
	//aorglu_path_entry();
	//~aorglu_path_entry();

	nsaddr_t id;
        double X_;
	double Y_;
	double Z_;
 protected:
        LIST_ENTRY(aorglu_path_entry) path_link;
};

/*
  The Actual Path list
*/

class aorglu_path {
 public:
	aorglu_path(); 
	~aorglu_path();

        aorglu_path_entry* head();
        aorglu_path_entry* path_add(nsaddr_t id, double, double, double);
        bool path_lookup(nsaddr_t id);
	void path_delete(nsaddr_t id);
	
	void print();
        int length() { return this->len; }
 private:
        int len;
        LIST_HEAD(aorglu_pathhead, aorglu_path_entry) pathhead;
};

#endif /* __aorglu_path_h__ */

