/*
Location Cache
Copyright (C) 2009 RGK, C.Hett C.Holpensen 

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
#include <aorglu/aorglu_path.h>
#include <aorglu/aorglu.h>


aorglu_path::aorglu_path()
{
  this->len = 0;
  /*Initialize the list*/
  LIST_INIT(&pathhead);
}

aorglu_path::~aorglu_path()
{
  aorglu_path_entry *pe;

  /*Depete the list*/
  while((pe=pathhead.lh_first)) {
 	LIST_REMOVE(pe, path_link);
	#ifdef DEBUG
	fprintf(stderr,"~Depeting path node %d\n", pe->id);
        #endif
 	delete pe;
  }
}


aorglu_path_entry*
aorglu_path::head()
{
  /*Return the first list entry*/
  return this->pathhead.lh_first;
}

/**Check if there is a pathation entry for a node.*/
bool
aorglu_path::path_lookup(nsaddr_t id)
{
  aorglu_path_entry *pe = pathhead.lh_first;

  #ifdef DEBUG
  fprintf(stderr, "path_lookup: Looking up address.\n");
  #endif

  for(;pe;pe = pe->path_link.le_next) {
	if(pe->id == id)
	  return true;
  }

  return false;
}

/**Delete a pathation tabpe entry.*/
void
aorglu_path::path_delete(nsaddr_t id)
{
  aorglu_path_entry *pe = pathhead.lh_first;
 
  #ifdef DEBUG
  fprintf(stderr, "path_delete() Depeting list entry!\n");
  #endif

  for(;pe;pe=pe->path_link.le_next) {
     if(pe->id == id){
       LIST_REMOVE(pe,path_link);
       this->len--;
       delete pe;
     } 
  }
}

/*Add new route entry.*/
aorglu_path_entry*
aorglu_path::path_add(nsaddr_t id, double X, double Y, double Z)
{
  aorglu_path_entry *pe;
  
  #ifdef DEBUG
  fprintf(stderr, "path_add() Adding new list entry.\n");
  #endif

  pe = new aorglu_path_entry();
  LIST_INSERT_HEAD(&pathhead, pe, path_link); 
  pe->id = id;

  pe->X_ = X; 
  pe->Y_ = Y;
  pe->Z_ = Z;
  
  this->len++;

  return pe;
}

void
aorglu_path::print()
{
  aorglu_path_entry *pe = pathhead.lh_first;
  fprintf(stderr,"%s%9s%10s%10s\n", "Node", "X", "Y", "Z"); 
  for(;pe;pe=pe->path_link.le_next) {
    fprintf(stderr, "%d%10.2lf%10.2lf%10.2lf\n", pe->id, pe->X_, pe->Y_, pe->Z_); 
  }
}
