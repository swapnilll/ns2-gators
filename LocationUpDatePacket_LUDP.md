# LUDP - Location UpDate Packet #

  * Location updates are sent only if the following are both true:
    1. a time of > T has expired since the last location update and ...
    1. the current node has changed locations since the last location update.

  * Location updates are sent by a destination node to all the nodes it is communicating with.

  * Location updates are sent with period T = R - CD/2`*`(v1-v2) where CD is the distance between a destination node D and its one-hop neighbor (in the path to the source) C. R is the transmission range of D or C (whichever is smaller). v1 and v2 are the velocities of D and C respectively.

  * It is required that each node can stores its own location in its location cache on a location update.

  * The LUDP sending node must know:
    * its current location (compared to its position in its location cache)
    * its transmission range
    * its velocity
    * the velocity of its one-hop neighbor on the path to the receiving node (C)
    * the location of (C)
    * the transmission range of (C)


# Necessary Changes (list) #
  1. Add LUDP packet header structure to aorglu\_packet.h
  1. Add sendLudp(...) and recvLudp(...) function prototypes to aorglu.h
  1. Define sendLudp(...) function in aorglu.cc
  1. Define recvLudp(...) function in aorglu.cc
  1. Add code to update receiving nodes location cache
  1. Create timer to trigger sending of Location update


# Implementation of changes #
1. Set up LUDP Packet header structure.
```
//--------- LOCATION UPDATE PACKET HEADER ---------------------
//csh - Add LUDP packet header struct
struct hdr_aorglu_ludp {
        u_int8_t        lu_type;        // Packet Type
        nsaddr_t        lu_dst;         // Destination IP Address
        nsaddr_t        lu_src;         // Source IP Address
	double		lu_x;		// X coordinate of node sending reply
	double		lu_y;		// Y coordinate of node sending reply
	double		lu_z;		// Z coordinate of node sending reply
						
  inline int size() { 
  int sz = 0;
  
  	sz = sizeof(u_int8_t)		// lu_type
	     + sizeof(nsaddr_t)		// lu_dst
	     + sizeof(nsaddr_t)		// lu_src
	     + sizeof(double)		// lu_x
	     + sizeof(double)		// lu_y
	     + sizeof(double);		// lu_z
  
  	assert (sz >= 0);
	return sz;
  }

};
//--------------------------------------------------------------

AND

// for size calculation of header-space reservation
union hdr_all_aorglu {
  hdr_aorglu          ah;
  hdr_aorglu_request  rreq;
  hdr_aorglu_reply    rrep;
  hdr_aorglu_ludp     ludp; //csh
  hdr_aorglu_error    rerr;
  hdr_aorglu_rrep_ack rrep_ack;
};
```

2. List send and recv function prototypes in aorglu.h
```
        /*
         * csh - LUDP functions
         */
	void		sendLudp();
	void		recvLudp(Packet *p);
```

3. Define the sendLudp() function
```
//------ sendLudp() function definition ---------------------------------//
//csh
void
AORGLU::sendLudp(nsaddr_t ipdst) {
Packet *p = Packet::alloc();
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_aorglu_ludp *lu = HDR_AORGLU_LUDP(p);
aorglu_rt_entry *rt = rtable.rt_lookup(ipdst);

//What if no route exists? The route expiration timer and chatter cache timer are different

if(!rt) 
  return;

 fprintf(stderr, "Sending LUDP from %d at %.2f", index, Scheduler::instance().clock());
 assert(rt);
 fprintf(stderr, "...yes\n");

  //csh - Get the x, y, and z coordinates from the current node
  //and add them to the packet header.
  MobileNode *currNode = (MobileNode*) Node::get_node_by_address(index);
  lu->lu_x = currNode->X();
  lu->lu_y = currNode->Y();
  lu->lu_z = currNode->Z();

 //update LUDP packet header info
 lu->lu_type = AORGLUTYPE_LUDP;
 lu->lu_dst = ipdst;
 lu->lu_src = index;
   
 //update common header info
 ch->ptype() = PT_AORGLU;
 ch->size() = IP_HDR_LEN + lu->size();
 ch->iface() = -2;
 ch->error() = 0;
 ch->addr_type() = NS_AF_INET;
 ch->next_hop_ = rt->rt_nexthop;
 ch->prev_hop_ = index;          // AORGLU hack
 ch->direction() = hdr_cmn::DOWN;

 //update the IP packet header info
 ih->saddr() = index;
 ih->daddr() = ipdst;
 ih->sport() = RT_PORT;
 ih->dport() = RT_PORT;
 ih->ttl_ = NETWORK_DIAMETER;

 //schedule the LUDP packet to be sent
 //NOTE: Will need to send to all nodes in the recently communicated list
 Scheduler::instance().schedule(target_, p, 0);

}
//-----END sendLudp(...) ---------------------------------------//
```

4. Define the recvLudp() function
```
//----- Definition of recvLudp() -------------------------------//
void
AORGLU::recvLudp(Packet *p) {
struct hdr_ip *ih = HDR_IP(p);
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_aorglu_ludp *lu = HDR_AORGLU_LUDP(p);
aorglu_rt_entry *rt;

  /*
   * Drop if the current node is the source
   */
  if(lu->lu_src == index) {
#ifdef DEBUG
    fprintf(stderr, "%s: got my own Location Update\n", __FUNCTION__);
#endif // DEBUG
    Packet::free(p);
    return;
  } 

  /*
   * Drop if I am not the destination, or on the
   *  path to the destination (i.e., the intended
   *  next hop).
   */
  if((lu->lu_dst != index) && (ch->prev_hop_ != index)) {
#ifdef DEBUG
    fprintf(stderr, "%s: Unintended recipient of LUDP\n", __FUNCTION__);
#endif // DEBUG
     Packet::free(p);
     return;
  }


 /* 
  * Either this is the destination, or the Location Updated must be forwarded
  */
 // Look up the next-hop info from the route table
 rt = rtable.rt_lookup(lu->lu_dst);

 // First check if I am the destination ..
 if(lu->lu_dst == index) {

#ifdef DEBUG
   fprintf(stderr, "%d - %s: Location Update successfully received\n",
                   index, __FUNCTION__);
#endif // DEBUG
  
//csh
fprintf(stderr, "LUDP successfully received by %d\n", index);

 /*
  *  Add location information from the sending node to the current
  *   node's location cache here
  */
 loctable.loc_add(lu->lu_src, lu->lu_x, lu->lu_y, lu->lu_z);
 loctable.print();

   Packet::free(p);
 }

 /*
  * Not the destination, so forward the Location Update.
  */
 else {
   //Note: The recvReply function does stuff with the queue here before forwarding,
   //       not sure if that should be done here as well.
   fprintf(stderr, "LUDP packet being forwarded by %d\n", index);
   forward(rt, p, DELAY);
 }

}
//------END of recvLudp(...)---------------------------------------//

```

5. Update the receiving nodes Location Cache
```
 /*
  *  Add location information from the sending node to the current
  *   node's location cache here
  */
 loctable.loc_add(lu->lu_src, lu->lu_x, lu->lu_y, lu->lu_z);
 loctable.print();
```

6. Create a timer for the LUDP transmissions (currently hard-coded to 3.0 ms)
```
IN aorglu.h:

#define LUDP_INTERVAL	3	// 3000 ms - csh

//csh - added Location Update Timer prototype
class AORGLULocationUpdateTimer : public Handler {
public:
        AORGLULocationUpdateTimer(AORGLU* a) : agent(a) {}
        void	handle(Event*);
private:
        AORGLU    *agent;
	Event	intr;
};

friend class AORGLULocationUpdateTimer; //csh - friend class

AORGLULocationUpdateTimer	lutimer; //csh - instance of LUDP timer
```

```
IN aorglu.cc

//csh - LUDP Timer handler
void
AORGLULocationUpdateTimer::handle(Event*) {
  agent->sendLudp();
  Scheduler::instance().schedule(this, &intr, LUDP_INTERVAL);
}
```