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

#ifndef __aorglu_loctable_h__
#define __aorglu_loctable_h__

#include <assert.h>
#include <sys/types.h>
#include <config.h>
#include <lib/bsd-list.h>
#include <scheduler.h>

#define CURRENT_TIME Scheduler::instance().clock()

/*
   Location Table Entry
*/

class aorglu_loc_entry {
        friend class AORGLU;
	friend class aorglu_loctable;
        friend class AORGLULocationCacheTimer;
 public:
	//aorglu_loc_entry();
	//~aorglu_loc_entry();

	/*When does the location cache entry expire?*/
        double          loc_expire;   
	nsaddr_t        id;
        double X_;
	double Y_;
	double Z_;
 protected:
        LIST_ENTRY(aorglu_loc_entry) loc_link;
};

/*
  The Location Table
*/

class aorglu_loctable {
 public:
	aorglu_loctable(); 
	~aorglu_loctable();

        aorglu_loc_entry* head();
	void print();
        aorglu_loc_entry* loc_add(nsaddr_t id, double, double, double);
        void              loc_delete(nsaddr_t id);
        aorglu_loc_entry* loc_lookup(nsaddr_t id);

 private:
        LIST_HEAD(aorglu_lochead, aorglu_loc_entry) lochead;
};

#endif /* _aorglu__loctable_h__ */
