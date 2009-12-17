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

#include <aorglu/aorglu_debug.h>

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
aorglu_loctable::left_hand_node(double X_, double Y_, double Z_, aorglu_path *path)
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
      if(!valid_location(le, path)) {
       // printf("INVALID EDGE DETECTED ON NODE %d, skipping.\n", le->id);
        continue;
      }

      a = DISTANCE(myX, myY, 0, X_, Y_, 0);
      b = DISTANCE(myX, myY, 0, le->X_, le->Y_, 0);
      c = DISTANCE(X_, Y_, 0, le->X_, le->Y_, 0);
      
      /*Law of cosines*/
      v = 0.5 * ( (a*a + b*b - c*c)/(a*b) ); 
      cAngle = acos(v);

      if((X_ - myX) >= 0) {  /*QUADRANT I and IV*/
         if((mDy*(le->X_-myX)+(myY-le->Y_)) > 0) {
                cAngle = 2*PI - cAngle;
         } 
      }
      else {                /*QUADRANT II and III*/
       if((mDy*(le->X_-myX)+(myY-le->Y_)) < 0) {
                cAngle = 2*PI - cAngle;
         } 
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
aorglu_loctable::right_hand_node(double X_, double Y_, double Z_, aorglu_path *path)
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
      if(!valid_location(le, path)) {
        //printf("INVALID EDGE DETECTED ON NODE %d, skipping.\n", le->id);
        continue;
      }

      a = DISTANCE(myX, myY, 0, X_, Y_, 0);
      b = DISTANCE(myX, myY, 0, le->X_, le->Y_, 0);
      c = DISTANCE(X_, Y_, 0, le->X_, le->Y_, 0);
      
      /*Law of cosines*/
      v = 0.5 * ( (a*a + b*b - c*c)/(a*b) ); 
      cAngle = 2*PI-acos(v);

      if((X_ - myX) >= 0) {  /*QUADRANT I and IV*/
         if((mDy*(le->X_-myX)+(myY-le->Y_)) > 0) {
                cAngle = 2*PI - cAngle;
         } 
      }
      else {                /*QUADRANT II and III*/
       if((mDy*(le->X_-myX)+(myY-le->Y_)) < 0) {
                cAngle = 2*PI - cAngle;
         } 
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

bool
aorglu_loctable::valid_location(aorglu_loc_entry *le, aorglu_path *path)
{
   aorglu_path_entry* pe, *npe;
   
   double mDy;
   double dAB, dBC, dCA, dBX, dCX;
   double aAlpha, aBeta, v;

   double myX, myY, myZ;
   myX = myY = myZ = 0.0; /*Need to change this later*/

   /*Iterate over the path entry list*/
   pe=path->head();

   while((npe=pe->path_link.le_next)) {
      /*Make sure the current candidate location isn't in the path or same location.*/
      if( (le->id == pe->id)) {
        //printf("INVALID PATH: CURRENT NODE IS ALREADY IN PATH!\n");
        return false;
      }

      if(((le->X_ == pe->X_) && (le->Y_ == pe->Y_) && (le->Z_ == pe->Z_))) {
       // printf("INVALID PATH: CURRENT NODE IS AT SAME LOCATION AS NODE IN PATH.\n");
        return false; 
      }

      /*Get triangle edges*/
      dAB = DISTANCE(npe->X_,npe->Y_,npe->Z_,pe->X_,pe->Y_,pe->Z_);
      dBC = DISTANCE(pe->X_, pe->Y_, pe->Z_, myX, myY, myZ);
      dCA = DISTANCE(myX, myY, myZ, npe->X_, npe->Y_, npe->Z_);

      dBX = DISTANCE(pe->X_, pe->Y_, pe->Z_, le->X_, le->Y_, le->Z_);
      dCX = DISTANCE(myX, myY, myZ, le->X_, le->Y_, le->Z_);

      /*Get internal triangle angle*/
      v = 0.5 * ( (dCA*dCA + dBC*dBC - dAB*dAB)/(dCA*dBC) ); 
      aAlpha = acos(v); 

      /*Get angle of the opposing point*/
      v = 0.5 * ( (dCX*dCX + dBC*dBC - dBX*dBX)/(dCX*dBC) ); 
      aBeta = acos(v); 
      
     // printf("Node A = (%.2lf, %.2lf)\n", npe->X_, npe->Y_);
     // printf("Node B = (%.2lf, %.2lf)\n", pe->X_, pe->Y_);
     // printf("Node C = (%.2lf, %.2lf)\n", myX, myY);
    //  printf("Node X = (%.2lf, %.2lf)\n", le->X_, le->Y_);

     // printf("dCX = %lf ; dBC = %lf ; dBX = %lf ; v= %lf\n", dCX, dBC, dBX, v);

     // printf("-->alpha= %lf beta = %lf\n", aAlpha, aBeta);
 

      /*Check the relative location of the node WRT the line BC*/
      mDy = (pe->Y_ - myY)/(pe->X_ - myX);
      if((mDy*(pe->X_-npe->X_)+(npe->Y_-pe->Y_)) >= 0) { /*Current node is in the RHP*/
             if((mDy*(pe->X_-le->X_)+(le->Y_-pe->Y_)) <= 0) { /*Check if the other node is in the LHP*/
 		aBeta = 2*PI - aBeta;
             }
       }
       else { /*Current node is in the LHP*/
            if((mDy*(pe->X_-le->X_)+(le->Y_-pe->Y_)) > 0) { /*Check if the other node is in the RHP*/
 		aBeta = 2*PI - aBeta;
            }
       }


     /*The point lies in a similar triangle*/
     if(aBeta <= aAlpha) { 
       mDy = (pe->Y_-npe->Y_)/(pe->X_-npe->X_); /*Calculate the slope*/
 
       if((mDy*(pe->X_-myX)+(myY-pe->Y_)) >= 0) { /*Current node is in the RHP*/
           //  printf("--Current node detected in RHP\n");
             if((mDy*(pe->X_-le->X_)+(le->Y_-pe->Y_)) <= 0) { /*Check if the other node is in the LHP*/
            //    printf("INVALID EDGE: CROSS EDGE DETECTED\n");
                return false; 
             }
       }
       else { /*Current node is in the LHP*/
           // printf("--Current node detected in LHP\n");
            if((mDy*(pe->X_-le->X_)+(le->Y_-pe->Y_)) > 0) { /*Check if the other node is in the RHP*/
               // printf("INVALID EDGE: CROSS EDGE DETECTED\n");
                return false; 
            }
       }
     }
      pe = npe;
   }
   
   return true; /*No edges crossed*/
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
	_DEBUG("~Deleting loc entry %d\n", le->id);
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

  _DEBUG( "loc_lookup: Looking up address.\n");

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
 
  _DEBUG( "loc_delete() Deleting list entry!\n");

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
  
  _DEBUG( "loc_add() Adding new list entry.\n");

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
  _DEBUG("%s%9s%5s%10s%10s\n", "Node", "Expire", "X", "Y", "Z"); 
  for(;le;le=le->loc_link.le_next) {
    _DEBUG( "%d%10.2lf%10.2lf%10.2lf%10.2lf\n", le->id, le->loc_expire, le->X_, le->Y_, le->Z_); 
  }
}
