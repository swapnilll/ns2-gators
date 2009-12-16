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

#define PI 3.14159265
#define DISTANCE(x0,y0,z0,x1,y1,z1) (sqrt( pow(x0-x1,2) + pow(y0-y1,2) + pow(z0-z1,2)))

/*Route Maintenance Functions*/
/*If this returns the current node address, we are in a local minimum*/

nsaddr_t 
aorglu_loctable::greedy_next_node(double X_, double Y_, double Z_)
{
  AORGLU_Neighbor *nb;
  aorglu_loc_entry *le;
  MobileNode *mn;

  double myX, myY, myZ, currD, minD;

  /*Initially set the min-addr to the sending node*/
  nsaddr_t addr = ((AORGLU*)agent)->index;

  /*Get My Coordinates*/
  mn = (MobileNode*)Node::get_node_by_address(addr);
  myX = mn->X();
  myY = mn->Y();
  myZ = mn->Z();

  /*Initially set the current distance to my distance*/
  minD = DISTANCE(myX,myY,myZ,X_,Y_,Z_); 

  /*Get agent neighbor list*/
  nb = ((AORGLU*)agent)->nbhead.lh_first;
  assert(nb);
 
  /*For each neighbor*/ 
  for(;nb;nb=nb->nb_link.le_next) {
    le = loc_lookup(nb->nb_addr); /*Lookup the location*/
    
    /*Only include the neighbor IF we know the location*/
    if(le) {
      currD = DISTANCE(le->X_,le->Y_,le->Z_,X_,Y_,Z_);
      
      if(currD < minD) {
         minD = currD;      /*Set new min-distance*/
         addr = nb->nb_addr; /*Set new node address*/
      } 
    }   
  }

  return addr;
}

nsaddr_t 
aorglu_loctable::left_hand_node(double X_, double Y_, double Z_)
{
  AORGLU_Neighbor *nb;
  aorglu_loc_entry *le;
  MobileNode *mn;

  double mDy, mAngle, cAngle;
  double myX, myY, myZ;
  double a,b,c,v; /*used in law of cosines*/

  /*Initially set the min-addr to the sending node*/
  nsaddr_t addr = ((AORGLU*)agent)->index;
  mAngle = 2*PI;

  /*Get My Coordinates*/
  mn = (MobileNode*)Node::get_node_by_address(addr);
  myX = mn->X();
  myY = mn->Y();
  myZ = mn->Z();

  /*Z is currently not used in calculations ;(*/

  /*Get agent neighbor list*/
  nb = ((AORGLU*)agent)->nbhead.lh_first;
  assert(nb);

  mDy = (Y_-myY)/(X_-myX); /*Get the slope*/
 
  /*For each neighbor*/ 
  for(;nb;nb=nb->nb_link.le_next) {
    le = loc_lookup(nb->nb_addr); /*Lookup the location*/
    
    /*Only include the neighbor IF we know the location*/
    if(le) {
      a = DISTANCE(myX, myY, 0, X_, Y_, 0);
      b = DISTANCE(myX, myY, 0, le->X_, le->Y_, 0);
      c = DISTANCE(X_, Y_, 0, le->X_, le->Y_, 0);
      
      /*Law of cosines*/
      v = 0.5 * ( (a*a + b*b - c*c)/(a*b) ); 
      cAngle = acos(v);

      /*Check for which relative side of the line we are on*/
      if((mDy*(le->X_-myX)+(myY-le->Y_)) > 0) {
         cAngle += PI; /*We are on the RIGHT hand plane*/
      }
      
      /*Check for the left-hand node condition*/
      if( cAngle <= mAngle ) {
         mAngle = cAngle;
         addr = nb->nb_addr;
       }
    }   
  }
  return addr;
}

nsaddr_t 
aorglu_loctable::right_hand_node(double X_, double Y_, double Z_)
{
  AORGLU_Neighbor *nb;
  aorglu_loc_entry *le;
  MobileNode *mn;

  double mDy, mAngle, cAngle;
  double myX, myY, myZ;
  double a,b,c,v; /*used in law of cosines*/

  /*Initially set the min-addr to the sending node*/
  nsaddr_t addr = ((AORGLU*)agent)->index;
  mAngle = 0;

  /*Get My Coordinates*/
  mn = (MobileNode*)Node::get_node_by_address(addr);
  myX = mn->X();
  myY = mn->Y();
  myZ = mn->Z();

  /*Z is currently not used in calculations ;(*/

  /*Get agent neighbor list*/
  nb = ((AORGLU*)agent)->nbhead.lh_first;
  assert(nb);

  mDy = (Y_-myY)/(X_-myX); /*Get the slope*/
 
  /*For each neighbor*/ 
  for(;nb;nb=nb->nb_link.le_next) {
    le = loc_lookup(nb->nb_addr); /*Lookup the location*/
    
    /*Only include the neighbor IF we know the location*/
    if(le) {
      a = DISTANCE(myX, myY, 0, X_, Y_, 0);
      b = DISTANCE(myX, myY, 0, le->X_, le->Y_, 0);
      c = DISTANCE(X_, Y_, 0, le->X_, le->Y_, 0);
      
      /*Law of cosines*/
      v = 0.5 * ( (a*a + b*b - c*c)/(a*b) ); 
      cAngle = 2*PI-acos(v);

      /*Check for which relative side of the line we are on*/
      if((mDy*(le->X_-myX)+(myY-le->Y_)) < 0) {
         cAngle -= PI; /*We are on the LEFT hand plane*/
      }
      
      /*Check for the right-hand node condition*/
      if( cAngle >= mAngle ) {
         mAngle = cAngle;
         addr = nb->nb_addr;
       }
    }   
  }
  return addr;
}

/** Need to be given a neighbor cache list*/
aorglu_loctable::aorglu_loctable(Agent *a) : agent(a) 
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
