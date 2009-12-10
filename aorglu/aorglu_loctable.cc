/*
Location Cache
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
#include <aorglu/aorglu_loctable.h>
#include <aorglu/aorglu.h>

#if 0
/*Testing code*/
int main()
{
  aorglu_loctable *lt = new aorglu_loctable();
  /*Add some stuff to the list*/ 
  lt->loc_add(1, 0.0, 1.0, 2.0);
  lt->loc_add(2, 1.0, 2.1, 3.0); 
  lt->print(); 
  
  lt->loc_add(2, 10.0, 0.0, 0.0);
  lt->loc_delete(1);
  lt->print();
  delete lt;

  return 0;
}
#endif

/*Route Maintenance Functions*/
nsaddr_t 
greedy_next_node(double X_, double Y_, double Z_)
{

}

/** Need to be given a neighbor cache list*/
aorglu_loctable::aorglu_loctable(aorglu_ncache *nbhead) : nbhead(nbhead)
{
  /*Initialize the list*/
  LIST_INIT(&lochead);
}

aorglu_loctable::~aorglu_loctable()
{
  aorglu_loc_entry *le;
  /*Delete the list*/
  while((le=lochead.lh_first)) {
 	LIST_REMOVE(le, loc_link);
	#ifdef DEBUG
	fprintf(stderr,"~Deleting loc entry %d\n", le->id);
        #endif
 	delete le;
  }
}


aorglu_loc_entry*
aorglu_loctable::head()
{
  /*Return the first list entry*/
  return this->lochead.lh_first;
}

/**Check if there is a location entry for a node.*/
aorglu_loc_entry*
aorglu_loctable::loc_lookup(nsaddr_t id)
{
  aorglu_loc_entry *le = lochead.lh_first;

  #ifdef DEBUG
  fprintf(stderr, "loc_lookup: Looking up address.\n");
  #endif

  for(;le;le = le->loc_link.le_next) {
	if(le->id == id)
	  break;
  }

  return le;
}

/**Delete a location table entry.*/
void
aorglu_loctable::loc_delete(nsaddr_t id)
{
  aorglu_loc_entry *le = lochead.lh_first;
 
  #ifdef DEBUG
  fprintf(stderr, "loc_delete() Deleting list entry!\n");
  #endif

  for(;le;le=le->loc_link.le_next) {
     if(le->id == id){
       LIST_REMOVE(le,loc_link);
       delete le;
     } 
  }
}

/*Add new route entry.*/
aorglu_loc_entry*
aorglu_loctable::loc_add(nsaddr_t id, double X, double Y, double Z)
{
  aorglu_loc_entry *le;
  
  #ifdef DEBUG
  fprintf(stderr, "loc_add() Adding new list entry.\n");
  #endif

  /*Check to see if there is already a loc-entry*/
  le = loc_lookup(id);

  /*No entry, so we make a new one!*/
  if(le == NULL) {
     le = new aorglu_loc_entry();
     LIST_INSERT_HEAD(&lochead, le, loc_link); 
     le->id = id;
  }

  /*Update the node location information*/
  le->X_ = X;
  le->Y_ = Y;
  le->Z_ = Z;

  /*Update the entry expiration timer*/
  le->loc_expire = (CURRENT_TIME + LOC_CACHE_EXP);
 
  return le;
}

void
aorglu_loctable::print()
{
  aorglu_loc_entry *le = lochead.lh_first;
  fprintf(stderr,"%s%9s%5s%10s%10s\n", "Node", "Expire", "X", "Y", "Z"); 
  for(;le;le=le->loc_link.le_next) {
    fprintf(stderr, "%d%10.2lf%10.2lf%10.2lf%10.2lf\n", le->id, le->loc_expire, le->X_, le->Y_, le->Z_); 
  }
}
