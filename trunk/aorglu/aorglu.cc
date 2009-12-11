/*
Copyright (c) 1997, 1998 Carnegie Mellon University.  All Rights
Reserved. 

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The AORGLU code developed by the CMU/MONARCH group was optimized and tuned by Samir Das and Mahesh Marina, University of Cincinnati. The work was partially done in Sun Microsystems. Modified for gratuitous replies by Anant Utgikar, 09/16/02.

*/

//#include <ip.h>

#include <aorglu/aorglu.h>
#include <aorglu/aorglu_packet.h>
#include <random.h>
#include <mobilenode.h>
#include <node.h>
#include <cmu-trace.h>
//#include <energy-model.h>

#define max(a,b)        ( (a) > (b) ? (a) : (b) )
#define CURRENT_TIME    Scheduler::instance().clock()

//#define DEBUG
//#define ERROR

#define test(a,...) fprintf(stderr,a,__VA_ARGS__)

#ifdef DEBUG
static int extra_route_reply = 0;
static int limit_route_request = 0;
static int route_request = 0;
#endif

/*
  TCL Hooks
*/


int hdr_aorglu::offset_;
static class AORGLUHeaderClass : public PacketHeaderClass {
public:
        AORGLUHeaderClass() : PacketHeaderClass("PacketHeader/AORGLU",
                                              sizeof(hdr_all_aorglu)) {
	  bind_offset(&hdr_aorglu::offset_);
	} 
} class_rtProtoAORGLU_hdr;

static class AORGLUclass : public TclClass {
public:
        AORGLUclass() : TclClass("Agent/AORGLU") {}
        TclObject* create(int argc, const char*const* argv) {
          assert(argc == 5);
          //return (new AORGLU((nsaddr_t) atoi(argv[4])));
	  return (new AORGLU((nsaddr_t) Address::instance().str2addr(argv[4])));
        }
} class_rtProtoAORGLU;


int
AORGLU::command(int argc, const char*const* argv) {
  MobileNode *mn;

  if(argc == 2) {
  Tcl& tcl = Tcl::instance();
    
    if(strncasecmp(argv[1], "id", 2) == 0) {
      tcl.resultf("%d", index);
      return TCL_OK;
    }
    
    if(strncasecmp(argv[1], "start", 2) == 0) {
      /*Set my initial location*/
      mn = (MobileNode*) Node::get_node_by_address(index);
      lastX_ = mn->X();
      lastY_ = mn->Y();
      lastZ_ = mn->Z();

      lutimer.handle((Event*) 0); /*Init the LUDP timer*/
      
      if(LOC_CACHE_EXP != -1) {
        loctimer.handle((Event*) 0); /*Init loc cache expiration timer*/
      }

      cctimer.handle((Event*) 0);  /*Init LUDP cache timer*/
      btimer.handle((Event*) 0);  /*Init broadcast timer*/

#ifndef AORGLU_LINK_LAYER_DETECTION
      htimer.handle((Event*) 0);
      ntimer.handle((Event*) 0);
#endif // LINK LAYER DETECTION

      rtimer.handle((Event*) 0);
      return TCL_OK;
     }               
  }
  else if(argc == 3) {
    if(strcmp(argv[1], "index") == 0) {
      index = atoi(argv[2]);
      return TCL_OK;
    }

    else if(strcmp(argv[1], "log-target") == 0 || strcmp(argv[1], "tracetarget") == 0) {
      logtarget = (Trace*) TclObject::lookup(argv[2]);
      if(logtarget == 0)
	return TCL_ERROR;
      return TCL_OK;
    }
    else if(strcmp(argv[1], "drop-target") == 0) {
    int stat = rqueue.command(argc,argv);
      if (stat != TCL_OK) return stat;
      return Agent::command(argc, argv);
    }
    else if(strcmp(argv[1], "if-queue") == 0) {
    ifqueue = (PriQueue*) TclObject::lookup(argv[2]);
      
      if(ifqueue == 0)
	return TCL_ERROR;
      return TCL_OK;
    }
    else if (strcmp(argv[1], "port-dmux") == 0) {
    	dmux_ = (PortClassifier *)TclObject::lookup(argv[2]);
	if (dmux_ == 0) {
		fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__,
		argv[1], argv[2]);
		return TCL_ERROR;
	}
	return TCL_OK;
    }
  }
  return Agent::command(argc, argv);
}

/* 
   Constructor
*/

/*RGK - Added loctimer to constructor list*/
AORGLU::AORGLU(nsaddr_t id) : Agent(PT_AORGLU), reparetxtimer(this), locexptimer(this), loctimer(this), cctimer(this),
			  lutimer(this), btimer(this), htimer(this), ntimer(this), 
			  rtimer(this), loctable(this), rqueue() {
  index = id;
  seqno = 2;
  bid = 1;

  LIST_INIT(&nbhead);
  LIST_INIT(&bihead);

  /*RGK - Initialize the chatter cache*/
  LIST_INIT(&lchead);

  logtarget = 0;
  ifqueue = 0;
}

/*
  Timers
*/

#define FREQUENCY 0.5

/*Maintenance Timers*/
void
AORGLU_LOC_EXP_Timer::handle(Event *)
{
}

void
AORGLU_REPA_RETX_Timer::handle(Event *)
{
}

/*RGK - Location Cache Timer*/
void
AORGLULocationCacheTimer::handle(Event *)
{
  agent->loc_purge(); /*Purge expired locations*/
  Scheduler::instance().schedule(this,&intr, FREQUENCY);
}  

void
AORGLUBroadcastTimer::handle(Event*) {
  agent->id_purge();
  Scheduler::instance().schedule(this, &intr, BCAST_ID_SAVE);
}

//csh - LUDP Timer handler
void
AORGLULocationUpdateTimer::handle(Event*) 
{
  MobileNode *mn;
  LUDPCacheEntry *lce;

  double currX, currY, currZ, dD;

  mn = (MobileNode*) Node::get_node_by_address(agent->index);
  assert(mn);

  /*Get current location when update occured*/
  currX = mn->X();
  currY = mn->Y();
  currZ = mn->Z();
  
  /*Calculate how far we traveled*/
  dD = sqrt( pow(currX - agent->lastX_, 2) +
             pow(currY - agent->lastY_, 2) +
             pow(currZ - agent->lastZ_, 2) );
  
  fprintf(stderr, "Node %d : Distance Moved: %lf \n", agent->index, dD); 
  
  /*If we moved outside of the allowed radius, we need to perform an update.*/ 
  if( dD >= LUDP_RADIUS ) { 
    fprintf(stderr, "--Sending LUDPs...\n"); 

    agent->lastX_ = currX;
    agent->lastY_ = currY;
    agent->lastZ_ = currZ;
    
    lce = agent->lchead.lh_first;
  
    /*rgk - Send ludp to actively communicating nodes*/
    for(;lce;lce=lce->lclink.le_next) {
      /*Check if the src has been active*/
      if(lce->active) {
        fprintf(stderr, "---Sending to %d\n", lce->dst);
        agent->sendLudp(lce->dst); /*Send LUDP to each node in the LUDPCacheUpdate.*/
      }
      else {
        fprintf(stderr, "---Link from %d\n has been inactive, No LUDP.\n", lce->dst);
      }    
    } 
  }

  Scheduler::instance().schedule(this, &intr, LUDP_INTERVAL);
}

void
AORGLULUDPCacheTimer::handle(Event*) 
{
  agent->ludpcache_checkactive(); /*Check which links are active in the LUDPCache*/
  Scheduler::instance().schedule(this, &intr, LUDP_CACHE_SAVE);
}

void
AORGLUHelloTimer::handle(Event*) {
   agent->sendHello();
   double interval = MinHelloInterval + 
                 ((MaxHelloInterval - MinHelloInterval) * Random::uniform());
   assert(interval >= 0);
   Scheduler::instance().schedule(this, &intr, interval);
}

void
AORGLUNeighborTimer::handle(Event*) {
  agent->nb_purge();
  Scheduler::instance().schedule(this, &intr, HELLO_INTERVAL);
}

void
AORGLURouteCacheTimer::handle(Event*) {
  agent->rt_purge();
  Scheduler::instance().schedule(this, &intr, FREQUENCY);
}

//rgk - deleted local repair timer

/*
 * RGK -  LUDP Cache Management
 */
void            
AORGLU::ludpcache_insert(nsaddr_t id)
{
  LUDPCacheEntry *lce;
  double now = CURRENT_TIME;

  /*If no existing entry*/
  if(!(lce = ludpcache_lookup(id))) {
     lce = new LUDPCacheEntry(id); 
     LIST_INSERT_HEAD(&lchead, lce, lclink);
  }
    
   assert(lce);

   /*Mark the entry active*/
   lce->active = true;

   /*Renew the Expiration Timer*/ 
   lce->expire = now + LUDP_CACHE_SAVE;

}

LUDPCacheEntry *            
AORGLU::ludpcache_lookup(nsaddr_t id)
{
  LUDPCacheEntry *lce = lchead.lh_first;

  for(;lce;lce=lce->lclink.le_next) {
      if(lce->dst == id) {
	break;	
      }
  }
  return lce;
} 

void            
AORGLU::ludpcache_checkactive()
{
  LUDPCacheEntry *lce, *nlce; 
  double now = CURRENT_TIME;

  for(lce=lchead.lh_first; lce; lce = nlce) {
      nlce = lce->lclink.le_next;
     
      if(lce->expire <= now) {
        lce->active = false; /*Mark the entry inactive*/	
        delete lce;      
      } 
   }
}

/*Update the timer for a node already in the ludp cache*/
void
AORGLU::ludpcache_update(nsaddr_t id)
{
  LUDPCacheEntry *lce;
  double now = CURRENT_TIME;

  lce = ludpcache_lookup(id);

  if(lce) {
   lce->expire = now + LUDP_CACHE_SAVE;
   lce->active = true;
  }
}

/*Delete an entry from the list*/
void
AORGLU::ludpcache_delete(nsaddr_t id)
{
  LUDPCacheEntry *lce;
  
  lce = ludpcache_lookup(id);
  
  if(lce) {
    LIST_REMOVE(lce, lclink);
  }

}


/*
   Broadcast ID Management  Functions
*/

void
AORGLU::id_insert(nsaddr_t id, u_int32_t bid) {
BroadcastID *b = new BroadcastID(id, bid);

 assert(b);
 b->expire = CURRENT_TIME + BCAST_ID_SAVE;
 LIST_INSERT_HEAD(&bihead, b, link);
}

/* SRD */
bool
AORGLU::id_lookup(nsaddr_t id, u_int32_t bid) {
BroadcastID *b = bihead.lh_first;
 
 // Search the list for a match of source and bid
 for( ; b; b = b->link.le_next) {
   if ((b->src == id) && (b->id == bid))
     return true;     
 }
 return false;
}

void
AORGLU::id_purge() {
BroadcastID *b = bihead.lh_first;
BroadcastID *bn;
double now = CURRENT_TIME;

 for(; b; b = bn) {
   bn = b->link.le_next;
   if(b->expire <= now) {
     LIST_REMOVE(b,link);
     delete b;
   }
 }
}

/*
  Helper Functions
*/

double
AORGLU::PerHopTime(aorglu_rt_entry *rt) {
int num_non_zero = 0, i;
double total_latency = 0.0;

 if (!rt)
   return ((double) NODE_TRAVERSAL_TIME );
	
 for (i=0; i < MAX_HISTORY; i++) {
   if (rt->rt_disc_latency[i] > 0.0) {
      num_non_zero++;
      total_latency += rt->rt_disc_latency[i];
   }
 }
 if (num_non_zero > 0)
   return(total_latency / (double) num_non_zero);
 else
   return((double) NODE_TRAVERSAL_TIME);

}

/*
  Link Failure Management Functions
*/

/////////////////////////////////////////////////////////////////////
//CALLED BY PACKET WHEN THE TRANSMISSION FAILS AT THE LINK LAYER. 
static void
aorglu_rt_failed_callback(Packet *p, void *arg) {
  fprintf(stderr,"Route Failure Detected.\n");
  ((AORGLU*) arg)->rt_ll_failed(p);
}

void
AORGLU::rt_ll_failed(Packet *p) {
  sendError(p,false);
 /*We arent using link layer detection, so drop the packet*/
 drop(p, DROP_RTR_MAC_CALLBACK);
}
/////////////////////////////////////////////////////////////////////

void
AORGLU::rt_update(aorglu_rt_entry *rt, u_int32_t seqnum, u_int16_t metric,
	       	nsaddr_t nexthop, double expire_time) {

     rt->rt_seqno = seqnum;
     rt->rt_hops = metric;
     rt->rt_flags = RTF_UP;
     rt->rt_nexthop = nexthop;
     rt->rt_expire = expire_time;
}

void
AORGLU::rt_down(aorglu_rt_entry *rt) {
  /*
   *  Make sure that you don't "down" a route more than once.
   */

  if(rt->rt_flags == RTF_DOWN) {
    return;
  }

  // assert (rt->rt_seqno%2); // is the seqno odd?
  rt->rt_last_hop_count = rt->rt_hops;
  rt->rt_hops = INFINITY2;
  rt->rt_flags = RTF_DOWN;
  rt->rt_nexthop = 0;
  rt->rt_expire = 0;

} /* rt_down function */

/*
  Route Handling Functions
*/

void
AORGLU::rt_resolve(Packet *p) {
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
aorglu_rt_entry *rt;

 /*
  *  Set the transmit failure callback.  That
  *  won't change.
  */
 ch->xmit_failure_ = aorglu_rt_failed_callback;
 ch->xmit_failure_data_ = (void*) this;
 
 /*Check to see if the route is already in the route table.*/ 
 rt = rtable.rt_lookup(ih->daddr());
 if(rt == 0) { /*Its not?? So add it!*/
	  rt = rtable.rt_add(ih->daddr());
 }

 /*
  * If the route is up, forward the packet 
  */
	
 if(rt->rt_flags == RTF_UP) {
   assert(rt->rt_hops != INFINITY2);
   forward(rt, p, NO_DELAY);
 }
 /*
  *  if I am the source of the packet, then do a Route Request.
  */
 else if(ih->saddr() == index) {
   rqueue.enque(p);
   sendRequest(rt->rt_dst);
 }
 /*
  *	A local repair is in progress. Buffer the packet. 
  */
 else if (rt->rt_flags == RTF_IN_REPAIR) {
   rqueue.enque(p);
 }

 /*
  * I am trying to forward a packet for someone else to which
  * I don't have a route.
  */
 else {
 Packet *rerr = Packet::alloc();
 struct hdr_aorglu_error *re = HDR_AORGLU_ERROR(rerr);
 struct hdr_ip *ih = HDR_IP(p);

 /* 
  * For now, drop the packet and send error upstream.
  * Now the route errors are broadcast to upstream
  * neighbors - Mahesh 09/11/99
  */	
 
   assert (rt->rt_flags == RTF_DOWN);
   re->DestCount = 0;
   re->unreachable_dst[re->DestCount] = rt->rt_dst;
   re->unreachable_dst_seqno[re->DestCount] = rt->rt_seqno;
   re->DestCount += 1;
#ifdef DEBUG
   fprintf(stderr, "%s: sending RERR...\n", __FUNCTION__);
#endif

   /*Error here means it was caused by a packet being set
     so set the flag and the originating address*/
   re->orig_addr = ih->saddr(); /*Get src address to send to*/
   re->reason = RERR_REASON_SEND;
   sendError(rerr, false);

   drop(p, DROP_RTR_NO_ROUTE);
 }

}

/*RGK - loc_purge
 Purge any expired location entries!
*/
void 
AORGLU::loc_purge()
{
  double now = CURRENT_TIME;
  aorglu_loc_entry *le, *nle;

  for(le = loctable.head(); le; le = nle) {
      /*Get the next entry*/
      nle = le->loc_link.le_next;
 
      /*Check if the entry expired*/
      if(le->loc_expire < now) {
	loctable.loc_delete(le->id);
	
	#ifdef DEBUG
	fprintf(stderr,"Location Entry for Node %d Expired!\n", le->id);
	#endif

      }
   }
} 

void
AORGLU::rt_purge() {
aorglu_rt_entry *rt, *rtn;
double now = CURRENT_TIME;
double delay = 0.0;
Packet *p;

 for(rt = rtable.head(); rt; rt = rtn) {  // for each rt entry
   rtn = rt->rt_link.le_next;
   if ((rt->rt_flags == RTF_UP) && (rt->rt_expire < now)) {

   /*rgk - IF a route to a node has expired, just delete it from the LUDP list*/
      ludpcache_delete(rt->rt_dst);
 
   // if a valid route has expired, purge all packets from 
   // send buffer and invalidate the route.                    
	assert(rt->rt_hops != INFINITY2);
     while((p = rqueue.deque(rt->rt_dst))) {
#ifdef DEBUG
       fprintf(stderr, "%s: calling drop()\n",
                       __FUNCTION__);
#endif // DEBUG
       drop(p, DROP_RTR_NO_ROUTE);
     }
     rt->rt_seqno++;
     assert (rt->rt_seqno%2);
     rt_down(rt);
   }
   else if (rt->rt_flags == RTF_UP) {
   // If the route is not expired,
   // and there are packets in the sendbuffer waiting,
   // forward them. This should not be needed, but this extra 
   // check does no harm.
     assert(rt->rt_hops != INFINITY2);
     while((p = rqueue.deque(rt->rt_dst))) {
       forward (rt, p, delay);
       delay += ARP_DELAY;
     }
   } 
   else if (rqueue.find(rt->rt_dst))
   // If the route is down and 
   // if there is a packet for this destination waiting in
   // the sendbuffer, then send out route request. sendRequest
   // will check whether it is time to really send out request
   // or not.
   // This may not be crucial to do it here, as each generated 
   // packet will do a sendRequest anyway.

     sendRequest(rt->rt_dst); 
   }

}

/*
  Packet Reception Routines
*/

void
AORGLU::recv(Packet *p, Handler*) {
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p); /*Get the IP header*/

 assert(initialized());
 //assert(p->incoming == 0);
 // XXXXX NOTE: use of incoming flag has been depracated; In order to track direction of pkt flow, direction_ in hdr_cmn is used instead. see packet.h for details.

  /*rgk - If this is a node we are communicating with, we need to update the LUDP timer*/
  ludpcache_update(ih->saddr());

 if(ch->ptype() == PT_AORGLU) { /*If we received a AORGLU packet, process it with recvAORGLU*/
   ih->ttl_ -= 1;
   recvAORGLU(p);
   return;
 }

 /*
  *  Must be a packet I'm originating...
  */
if((ih->saddr() == index) && (ch->num_forwards() == 0)) {
 /*
  * Add the IP Header.  
  * TCP adds the IP header too, so to avoid setting it twice, we check if
  * this packet is not a TCP or ACK segment.
  */
  if (ch->ptype() != PT_TCP && ch->ptype() != PT_ACK) {
    ch->size() += IP_HDR_LEN;
  }
   // Added by Parag Dadhania && John Novatnack to handle broadcasting
  if ( (u_int32_t)ih->daddr() != IP_BROADCAST) {
    ih->ttl_ = NETWORK_DIAMETER;
  }
}
 /*
  *  I received a packet that I sent.  Probably
  *  a routing loop.
  */
else if(ih->saddr() == index) {
   drop(p, DROP_RTR_ROUTE_LOOP);
   return;
 }
 /*
  *  Packet I'm forwarding...
  */
 else {
 /*
  *  Check the TTL.  If it is zero, then discard.
  */
   if(--ih->ttl_ == 0) {
     drop(p, DROP_RTR_TTL);
     return;
   }
 }
// Added by Parag Dadhania && John Novatnack to handle broadcasting
 if ( (u_int32_t)ih->daddr() != IP_BROADCAST)
   rt_resolve(p);
 else
   forward((aorglu_rt_entry*) 0, p, NO_DELAY);
}

/*Process a AORGLU packet*/
void
AORGLU::recvAORGLU(Packet *p) {
 struct hdr_aorglu *ah = HDR_AORGLU(p);

 assert(HDR_IP (p)->sport() == RT_PORT);
 assert(HDR_IP (p)->dport() == RT_PORT);

 /*
  * Incoming Packets.
  */
 switch(ah->ah_type) {

 case AORGLUTYPE_RREQ:
   recvRequest(p);
   break;

 case AORGLUTYPE_RREP:
   recvReply(p);
   break;

 case AORGLUTYPE_RERR:
   recvError(p);
   break;

 case AORGLUTYPE_HELLO:
   recvHello(p);
   break;

 case AORGLUTYPE_LUDP:
   recvLudp(p);
   break;

 case AORGLUTYPE_REPA:
   recvRepa(p);
   break;

 case AORGLUTYPE_REPC:
   recvRepc(p);
   break;
 
 default:
   fprintf(stderr, "Invalid AORGLU type (%x)\n", ah->ah_type);
   exit(1);
 }

}


void
AORGLU::recvRequest(Packet *p) {
struct hdr_ip *ih = HDR_IP(p);
struct hdr_aorglu_request *rq = HDR_AORGLU_REQUEST(p);
aorglu_rt_entry *rt;

  /*
   * Drop if:
   *      - I'm the source
   *      - I recently heard this request.
   */

  if(rq->rq_src == index) {
#ifdef DEBUG
    fprintf(stderr, "%s: got my own REQUEST\n", __FUNCTION__);
#endif // DEBUG
    Packet::free(p);
    return;
  } 

 if (id_lookup(rq->rq_src, rq->rq_bcast_id)) {

#ifdef DEBUG
   fprintf(stderr, "%s: discarding request\n", __FUNCTION__);
#endif // DEBUG
 
   Packet::free(p);
   return;
 }

 /*
  * Cache the broadcast ID
  */
 id_insert(rq->rq_src, rq->rq_bcast_id);

 /* 
  * We are either going to forward the REQUEST or generate a
  * REPLY. Before we do anything, we make sure that the REVERSE
  * route is in the route table.
  */
 aorglu_rt_entry *rt0; // rt0 is the reverse route 
   
   rt0 = rtable.rt_lookup(rq->rq_src);
   if(rt0 == 0) { /* if not in the route table */
   // create an entry for the reverse route.
     rt0 = rtable.rt_add(rq->rq_src);
   }
  
   rt0->rt_expire = max(rt0->rt_expire, (CURRENT_TIME + REV_ROUTE_LIFE));

   if ( (rq->rq_src_seqno > rt0->rt_seqno ) ||
    	((rq->rq_src_seqno == rt0->rt_seqno) && 
	 (rq->rq_hop_count < rt0->rt_hops)) ) {
   // If we have a fresher seq no. or lesser #hops for the 
   // same seq no., update the rt entry. Else don't bother.
rt_update(rt0, rq->rq_src_seqno, rq->rq_hop_count, ih->saddr(),
     	       max(rt0->rt_expire, (CURRENT_TIME + REV_ROUTE_LIFE)) );
     if (rt0->rt_req_timeout > 0.0) {
     // Reset the soft state and 
     // Set expiry time to CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT
     // This is because route is used in the forward direction,
     // but only sources get benefited by this change
       rt0->rt_req_cnt = 0;
       rt0->rt_req_timeout = 0.0; 
       rt0->rt_req_last_ttl = rq->rq_hop_count;
       rt0->rt_expire = CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT;
     }

     /* Find out whether any buffered packet can benefit from the 
      * reverse route.
      * May need some change in the following code - Mahesh 09/11/99
      */
     assert (rt0->rt_flags == RTF_UP);
     Packet *buffered_pkt;
     while ((buffered_pkt = rqueue.deque(rt0->rt_dst))) {
       if (rt0 && (rt0->rt_flags == RTF_UP)) {
	assert(rt0->rt_hops != INFINITY2);
         forward(rt0, buffered_pkt, NO_DELAY);
       }
     }
   } 
   // End for putting reverse route in rt table


 /*
  * We have taken care of the reverse route stuff.
  * Now see whether we can send a route reply. 
  */

 rt = rtable.rt_lookup(rq->rq_dst);

 // First check if I am the destination ..

 if(rq->rq_dst == index) {

#ifdef DEBUG
   fprintf(stderr, "%d - %s: destination sending reply\n",
                   index, __FUNCTION__);
#endif // DEBUG

               
   // Just to be safe, I use the max. Somebody may have
   // incremented the dst seqno.
   seqno = max(seqno, rq->rq_dst_seqno)+1;
   if (seqno%2) seqno++;

   /*rgk - Node wants to talk to me, add him to the LUDPCache*/
   ludpcache_insert(rq->rq_src);

   sendReply(rq->rq_src,           // IP Destination
             1,                    // Hop Count
             index,                // Dest IP Address (csh - index is the addr of the current node)
             seqno,                // Dest Sequence Num
             MY_ROUTE_TIMEOUT,     // Lifetime
             rq->rq_timestamp);    // timestamp

   //csh - Add location entry
   loctable.loc_add(ih->saddr(), rq->rq_x, rq->rq_y, rq->rq_z);


   Packet::free(p);
 }

 /*
  * Can't reply. So forward the  Route Request
  */
 else {
   ih->saddr() = index;
   ih->daddr() = IP_BROADCAST;
   rq->rq_hop_count += 1;
   // Maximum sequence number seen en route
   if (rt) rq->rq_dst_seqno = max(rt->rt_seqno, rq->rq_dst_seqno);
   forward((aorglu_rt_entry*) 0, p, DELAY);
 }

}


void
AORGLU::recvReply(Packet *p) {
//struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_aorglu_reply *rp = HDR_AORGLU_REPLY(p);
aorglu_rt_entry *rt;
char suppress_reply = 0;
double delay = 0.0;
	
#ifdef DEBUG
 fprintf(stderr, "%d - %s: received a REPLY\n", index, __FUNCTION__);
#endif // DEBUG


 /*
  *  Got a reply. So reset the "soft state" maintained for 
  *  route requests in the request table. We don't really have
  *  have a separate request table. It is just a part of the
  *  routing table itself. 
  */
 // Note that rp_dst is the dest of the data packets, not the
 // the dest of the reply, which is the src of the data packets.

 rt = rtable.rt_lookup(rp->rp_dst);
        
 /*
  *  If I don't have a rt entry to this host... adding
  */
 if(rt == 0) {
   rt = rtable.rt_add(rp->rp_dst);
 }

 /*
  * Add a forward route table entry... here I am following 
  * Perkins-Royer AORGLU paper almost literally - SRD 5/99
  */

 if ( (rt->rt_seqno < rp->rp_dst_seqno) ||   // newer route 
      ((rt->rt_seqno == rp->rp_dst_seqno) &&  
       (rt->rt_hops > rp->rp_hop_count)) ) { // shorter or better route
	
  // Update the rt entry 
  rt_update(rt, rp->rp_dst_seqno, rp->rp_hop_count,
		rp->rp_src, CURRENT_TIME + rp->rp_lifetime);

  // reset the soft state
  rt->rt_req_cnt = 0;
  rt->rt_req_timeout = 0.0; 
  rt->rt_req_last_ttl = rp->rp_hop_count;
  
if (ih->daddr() == index) { // If I am the original source
  // Update the route discovery latency statistics
  // rp->rp_timestamp is the time of request origination
		
    rt->rt_disc_latency[(unsigned char)rt->hist_indx] = (CURRENT_TIME - rp->rp_timestamp)
                                         / (double) rp->rp_hop_count;
    // increment indx for next time
    rt->hist_indx = (rt->hist_indx + 1) % MAX_HISTORY;
  }	

  /*
   * Send all packets queued in the sendbuffer destined for
   * this destination. 
   * XXX - observe the "second" use of p.
   */
  Packet *buf_pkt;
  while((buf_pkt = rqueue.deque(rt->rt_dst))) {
    if(rt->rt_hops != INFINITY2) {
          assert (rt->rt_flags == RTF_UP);
    // Delay them a little to help ARP. Otherwise ARP 
    // may drop packets. -SRD 5/23/99
      forward(rt, buf_pkt, delay);
      delay += ARP_DELAY;
    }
  }
 }
 else {
  suppress_reply = 1;
 }

 /*
  * If reply is for me, discard it.
  */

if(ih->daddr() == index || suppress_reply) {
   
   /*RGK - Add location entry*/
   loctable.loc_add(ih->saddr(), rp->rp_x, rp->rp_y, rp->rp_z);

#ifdef DEBUG
   fprintf(stderr, "recvReply():\n");
   fprintf(stderr, "The current node address is %d\n", index);
   fprintf(stderr, "Node %d coordinates: (%.2lf, %.2lf, %.2lf)\n", ih->saddr(), rp->rp_x, rp->rp_y, rp->rp_z);
   loctable.print();
#endif //DEBUG

   Packet::free(p);
 }
 /*
  * Otherwise, forward the Route Reply.
  */
 else {
 // Find the rt entry
aorglu_rt_entry *rt0 = rtable.rt_lookup(ih->daddr());
   // If the rt is up, forward
   if(rt0 && (rt0->rt_hops != INFINITY2)) {
        assert (rt0->rt_flags == RTF_UP);
     rp->rp_hop_count += 1;
     rp->rp_src = index;
     forward(rt0, p, NO_DELAY);
     // Insert the nexthop towards the RREQ source to 
     // the precursor list of the RREQ destination
     rt->pc_insert(rt0->rt_nexthop); // nexthop to RREQ source
     
   }
   else {
   // I don't know how to forward .. drop the reply. 
#ifdef DEBUG
     fprintf(stderr, "%s: dropping Route Reply\n", __FUNCTION__);
#endif // DEBUG
     drop(p, DROP_RTR_NO_ROUTE);
   }
 }
}


void
AORGLU::recvError(Packet *p) 
{
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_aorglu_error *re = HDR_AORGLU_ERROR(p);
  struct hdr_aorglu_ludp *lu = HDR_AORGLU_LUDP(p);
  aorglu_rt_entry *rt;
  u_int8_t i;
  Packet *rerr = Packet::alloc();
  Packet *pkt;
  struct hdr_aorglu_error *nre = HDR_AORGLU_ERROR(rerr);

  if(lu->lu_dst == index) {
        fprintf(stderr, "Node %d: Recieved Route Error Packet\n", index);
        Packet::free(p);
  }
  else {
     fprintf(stderr, "Node %d: Forwarding Route Error Packet\n", index);
     rt = rtable.rt_lookup(lu->lu_dst);
     forward(rt, p, DELAY);
  }

}


/*
   Packet Transmission Routines
*/

void
AORGLU::forward(aorglu_rt_entry *rt, Packet *p, double delay) {
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);

 if(ih->ttl_ == 0) {

#ifdef DEBUG
  fprintf(stderr, "%s: calling drop()\n", __PRETTY_FUNCTION__);
#endif // DEBUG
 
  drop(p, DROP_RTR_TTL);
  return;
 }

 if (ch->ptype() != PT_AORGLU && ch->direction() == hdr_cmn::UP &&
	(((u_int32_t)ih->daddr() == IP_BROADCAST)
		|| (ih->daddr() == here_.addr_))) {
	dmux_->recv(p,0);
	return;
 }

 if (rt) {
   assert(rt->rt_flags == RTF_UP);
   rt->rt_expire = CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT;
   ch->next_hop_ = rt->rt_nexthop;
   ch->addr_type() = NS_AF_INET;
   ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
 }
 else { // if it is a broadcast packet
   // assert(ch->ptype() == PT_AORGLU); // maybe a diff pkt type like gaf
   assert(ih->daddr() == (nsaddr_t) IP_BROADCAST);
   ch->addr_type() = NS_AF_NONE;
   ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction
 }

if (ih->daddr() == (nsaddr_t) IP_BROADCAST) {
 // If it is a broadcast packet
   assert(rt == 0);
   if (ch->ptype() == PT_AORGLU) {
     /*
      *  Jitter the sending of AORGLU broadcast packets by 10ms
      */
     Scheduler::instance().schedule(target_, p,
      				   0.01 * Random::uniform());
   } else {
     Scheduler::instance().schedule(target_, p, 0.);  // No jitter
   }
 }
 else { // Not a broadcast packet 
   if(delay > 0.0) {
     Scheduler::instance().schedule(target_, p, delay);
   }
   else {
   // Not a broadcast packet, no delay, send immediately
     Scheduler::instance().schedule(target_, p, 0.);
   }
 }

}


void
AORGLU::sendRequest(nsaddr_t dst) {
// Allocate a RREQ packet 
Packet *p = Packet::alloc();
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_aorglu_request *rq = HDR_AORGLU_REQUEST(p);
aorglu_rt_entry *rt = rtable.rt_lookup(dst);

 assert(rt);

 /*
  *  Rate limit sending of Route Requests. We are very conservative
  *  about sending out route requests. 
  */

 if (rt->rt_flags == RTF_UP) {
   assert(rt->rt_hops != INFINITY2);
   Packet::free((Packet *)p);
   return;
 }

 if (rt->rt_req_timeout > CURRENT_TIME) {
   Packet::free((Packet *)p);
   return;
 }

 // rt_req_cnt is the no. of times we did network-wide broadcast
 // RREQ_RETRIES is the maximum number we will allow before 
 // going to a long timeout.

 if (rt->rt_req_cnt > RREQ_RETRIES) {
   rt->rt_req_timeout = CURRENT_TIME + MAX_RREQ_TIMEOUT;
   rt->rt_req_cnt = 0;
 Packet *buf_pkt;
   while ((buf_pkt = rqueue.deque(rt->rt_dst))) {
       drop(buf_pkt, DROP_RTR_NO_ROUTE);
   }
   Packet::free((Packet *)p);
   return;
 }

#ifdef DEBUG
   fprintf(stderr, "(%2d) - %2d sending Route Request, dst: %d\n",
                    ++route_request, index, rt->rt_dst);
#endif // DEBUG

 // Determine the TTL to be used this time. 
 // Dynamic TTL evaluation - SRD

 rt->rt_req_last_ttl = max(rt->rt_req_last_ttl,rt->rt_last_hop_count);

 if (0 == rt->rt_req_last_ttl) {
 // first time query broadcast
   ih->ttl_ = TTL_START;
 }
 else {
 // Expanding ring search.
   if (rt->rt_req_last_ttl < TTL_THRESHOLD)
     ih->ttl_ = rt->rt_req_last_ttl + TTL_INCREMENT;
   else {
   // network-wide broadcast
     ih->ttl_ = NETWORK_DIAMETER;
     rt->rt_req_cnt += 1;
   }
 }

 // remember the TTL used  for the next time
 rt->rt_req_last_ttl = ih->ttl_;

 // PerHopTime is the roundtrip time per hop for route requests.
 // The factor 2.0 is just to be safe .. SRD 5/22/99
 // Also note that we are making timeouts to be larger if we have 
 // done network wide broadcast before. 

 rt->rt_req_timeout = 2.0 * (double) ih->ttl_ * PerHopTime(rt); 
 if (rt->rt_req_cnt > 0)
   rt->rt_req_timeout *= rt->rt_req_cnt;
 rt->rt_req_timeout += CURRENT_TIME;

 // Don't let the timeout to be too large, however .. SRD 6/8/99
 if (rt->rt_req_timeout > CURRENT_TIME + MAX_RREQ_TIMEOUT)
   rt->rt_req_timeout = CURRENT_TIME + MAX_RREQ_TIMEOUT;
 rt->rt_expire = 0;

#ifdef DEBUG
 fprintf(stderr, "(%2d) - %2d sending Route Request, dst: %d, tout %f ms\n",
	         ++route_request, 
		 index, rt->rt_dst, 
		 rt->rt_req_timeout - CURRENT_TIME);
#endif	// DEBUG
	
  //csh - Get the x,y,z coordinates from the current node
  //and add them to the packet header.
  MobileNode *currNode = (MobileNode*) Node::get_node_by_address(index);
  rq->rq_x = currNode->X();
  rq->rq_y = currNode->Y();
  rq->rq_z = currNode->Z();

 // Fill out the RREQ packet 
 // ch->uid() = 0;
 ch->ptype() = PT_AORGLU;
 ch->size() = IP_HDR_LEN + rq->size();
 ch->iface() = -2;
 ch->error() = 0;
 ch->addr_type() = NS_AF_NONE;
 ch->prev_hop_ = index;          // AORGLU hack

 ih->saddr() = index;
 ih->daddr() = IP_BROADCAST;
 ih->sport() = RT_PORT;
 ih->dport() = RT_PORT;

 // Fill up some more fields. 
 rq->rq_type = AORGLUTYPE_RREQ;
 rq->rq_hop_count = 1;
 rq->rq_bcast_id = bid++;
 rq->rq_dst = dst;
 rq->rq_dst_seqno = (rt ? rt->rt_seqno : 0);
 rq->rq_src = index;
 seqno += 2;
 assert ((seqno%2) == 0);
 rq->rq_src_seqno = seqno;
 rq->rq_timestamp = CURRENT_TIME;

 Scheduler::instance().schedule(target_, p, 0.);

}

void
AORGLU::sendReply(nsaddr_t ipdst, u_int32_t hop_count, nsaddr_t rpdst,
                u_int32_t rpseq, u_int32_t lifetime, double timestamp) {
Packet *p = Packet::alloc();
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_aorglu_reply *rp = HDR_AORGLU_REPLY(p);
aorglu_rt_entry *rt = rtable.rt_lookup(ipdst);

#ifdef DEBUG
fprintf(stderr, "sending Reply from %d at %.2f\n", index, Scheduler::instance().clock());
#endif // DEBUG
 assert(rt);

  //csh - Get the x and y coordinates from the current node
  //and add them to the packet header.
  MobileNode *currNode = (MobileNode*) Node::get_node_by_address(index); //this will not work if someone other than the RREQ dest sends the reply
  rp->rp_x = currNode->X();
  rp->rp_y = currNode->Y();
  rp->rp_z = currNode->Z();

 rp->rp_type = AORGLUTYPE_RREP;
 //rp->rp_flags = 0x00;
 rp->rp_hop_count = hop_count;
 rp->rp_dst = rpdst;
 rp->rp_dst_seqno = rpseq;
 rp->rp_src = index;
 rp->rp_lifetime = lifetime;
 rp->rp_timestamp = timestamp;
   
 // ch->uid() = 0;
 ch->ptype() = PT_AORGLU;
 ch->size() = IP_HDR_LEN + rp->size();
 ch->iface() = -2;
 ch->error() = 0;
 ch->addr_type() = NS_AF_INET;
 ch->next_hop_ = rt->rt_nexthop;
 ch->prev_hop_ = index;          // AORGLU hack
 ch->direction() = hdr_cmn::DOWN;

 ih->saddr() = index;
 ih->daddr() = ipdst;
 ih->sport() = RT_PORT;
 ih->dport() = RT_PORT;
 ih->ttl_ = NETWORK_DIAMETER;

 Scheduler::instance().schedule(target_, p, 0.);

}


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

//----- Definition of recvLudp() -------------------------------//
void
AORGLU::recvLudp(Packet *p) 
{
  //struct hdr_ip *ih = HDR_IP(p);
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_aorglu_ludp *lu = HDR_AORGLU_LUDP(p);
  aorglu_rt_entry *rt;

  fprintf(stderr, "Node %d got a ludp packet\n", index);

  if(lu->lu_dst == index) { /*This is my LUDP*/
     
     fprintf(stderr, "%d - %s: Location Update successfully received\n", index, __FUNCTION__);

     //csh
     fprintf(stderr, "LUDP successfully received by %d\n", index);

     loctable.loc_add(lu->lu_src, lu->lu_x, lu->lu_y, lu->lu_z);
     Packet::free(p);
   }
   else if(ch->next_hop_ == index) { /*Not my LUDP, but I was the next hop !*/
       // Look up the next-hop info from the route table
     rt = rtable.rt_lookup(lu->lu_dst);
     fprintf(stderr, "LUDP packet being forwarded by %d\n", index);
     forward(rt, p, DELAY);
   }
   else { /*Not my LUDP and I shouldn't forward it!*/
     /*Just throw out the packet.*/
     Packet::free(p);
   }

}
//------END of recvLudp(...)---------------------------------------//


//------ sendRepa() function definition ---------------------------------//
//csh
void
AORGLU::sendRepa(nsaddr_t ipdst, nsaddr_t nexthop) {
Packet *p = Packet::alloc();
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_aorglu_rep *rpr = HDR_AORGLU_REPA(p);

 fprintf(stderr, "Sending REPA from %d at %.2f", index, Scheduler::instance().clock());
 assert(rt);
 fprintf(stderr, "...yes\n");

 //csh - Get the x,y,z coordinates of the destination node
 //and add them to the packet header.
 MobileNode *destNode = (MobileNode*) Node::get_node_by_address(ipdst);
 rpr->rpr_x = destNode->X();
 rpr->rpr_y = destNode->Y();
 rpr->rpr_z = destNode->Z();

/*Do this elsewhere*/
#if 0
 nexthopid = loctable.greedy_next(rpr->rpr_x, rpr->rpr_y, rpr->rpr_z);

 if(nexthopid == index) {
    //no greedy next node (LOCAL MAX OMG)
 }
#endif

 //update REPA packet header info
 rpr->rpr_greedy = 1;
 rpr->rpr_type = AORGLUTYPE_LUDP;
 rpr->rpr_dst = ipdst;
 rpr->rpr_src = index;
   
 //update common header info
 ch->ptype() = PT_AORGLU;
 ch->size() = IP_HDR_LEN + rpr->size();
 ch->iface() = -2;
 ch->error() = 0;
 ch->addr_type() = NS_AF_INET;
 ch->next_hop_ = nexthop;
 ch->prev_hop_ = index;          // AORGLU hack
 ch->direction() = hdr_cmn::DOWN;

 //update the IP packet header info
 ih->saddr() = index;
 ih->daddr() = ipdst;
 ih->sport() = RT_PORT;
 ih->dport() = RT_PORT;
 ih->ttl_ = NETWORK_DIAMETER;

 //schedule the REPA packet to be sent
 Scheduler::instance().schedule(target_, p, 0);

}
//-----END sendLudp(...) ---------------------------------------//

//----- Definition of recvRepa() -------------------------------//
void
AORGLU::recvRepa(Packet *p)
{
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_aorglu_ludp *rpr = HDR_AORGLU_LUDP(p);
  fprintf(stderr, "Node %d: Recieved Repa packet!\n", index);
  Packet::free(p);
}
//------END of sendRepa(...)---------------------------------------//

//----- Definition of sendRepc() -------------------------------//
void
AORGLU::sendRepc(nsaddr_t dst) 
{

  fprintf(stderr, "Node %d: Sending Repc packet!\n", index);

}

//------END of sendRepc(...)---------------------------------------//

//----- Definition of recvRepc() -------------------------------//
void
AORGLU::recvRepc(Packet *p) 
{
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_aorglu_ludp *rpr = HDR_AORGLU_LUDP(p);
  fprintf(stderr, "Node %d: Recieved Repc. packet!\n", index);
  Packet::free(p);
}
//------END of recvRepc(...)---------------------------------------//


void
AORGLU::sendError(Packet *p,  bool jitter) {
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_aorglu_error *re = HDR_AORGLU_ERROR(p);
    
fprintf(stderr, "sending Error from %d at %.2f\n", index, Scheduler::instance().clock());

 re->re_type = AORGLUTYPE_RERR;

 // ch->uid() = 0;
 ch->ptype() = PT_AORGLU;
 ch->size() = IP_HDR_LEN + re->size();
 ch->iface() = -2;
 ch->error() = 0;
 ch->addr_type() = NS_AF_NONE;
 ch->next_hop_ = 0;
 ch->prev_hop_ = index;          // AORGLU hack
 ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction

 ih->saddr() = index;
 ih->daddr() = IP_BROADCAST;
 ih->sport() = RT_PORT;
 ih->dport() = RT_PORT;
 ih->ttl_ = 1;

 // Do we need any jitter? Yes
 if (jitter)
 	Scheduler::instance().schedule(target_, p, 0.01*Random::uniform());
 else
 	Scheduler::instance().schedule(target_, p, 0.0);

}

/*
   Neighbor Management Functions
*/

void
AORGLU::sendHello() {
Packet *p = Packet::alloc();
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_aorglu_reply *rh = HDR_AORGLU_REPLY(p);

#ifdef DEBUG
fprintf(stderr, "sending Hello from %d at %.2f\n", index, Scheduler::instance().clock());
#endif

  //csh - Get the x and y coordinates from the current node
  //and add them to the packet header.
  MobileNode *currNode = (MobileNode*) Node::get_node_by_address(index);
  rh->rp_x = currNode->X();
  rh->rp_y = currNode->Y();
  rh->rp_z = currNode->Z();

 rh->rp_type = AORGLUTYPE_HELLO;
 //rh->rp_flags = 0x00;
 rh->rp_hop_count = 1;
 rh->rp_dst = index;
 rh->rp_dst_seqno = seqno;
 rh->rp_lifetime = (1 + ALLOWED_HELLO_LOSS) * HELLO_INTERVAL;

 // ch->uid() = 0;
 ch->ptype() = PT_AORGLU;
 ch->size() = IP_HDR_LEN + rh->size();
 ch->iface() = -2;
 ch->error() = 0;
 ch->addr_type() = NS_AF_NONE;
 ch->prev_hop_ = index;          // AORGLU hack

 ih->saddr() = index;
 ih->daddr() = IP_BROADCAST;
 ih->sport() = RT_PORT;
 ih->dport() = RT_PORT;
 ih->ttl_ = 1;

 Scheduler::instance().schedule(target_, p, 0.0);
}


void
AORGLU::recvHello(Packet *p) {
struct hdr_ip *ih = HDR_IP(p); //csh - uncommented ip header to allow access to sending node address.
struct hdr_aorglu_reply *rp = HDR_AORGLU_REPLY(p);
AORGLU_Neighbor *nb;

 nb = nb_lookup(rp->rp_dst);
 if(nb == 0) {
   nb_insert(rp->rp_dst);
 }
 else {
   nb->nb_expire = CURRENT_TIME +
                   (1.5 * ALLOWED_HELLO_LOSS * HELLO_INTERVAL);
 }

   /*RGK - Add a location table entry*/
   /*12/09/09 - Appears to work as intended!*/
   loctable.loc_add(ih->saddr(), rp->rp_x, rp->rp_y, rp->rp_z);

   //csh - print out coordinates when reply is received
#ifdef DEBUG
   fprintf(stderr, "recvHello:\n");
   fprintf(stderr, "The current node address is %d\n", index);
   fprintf(stderr, "Node %d coordinates: (%.2lf, %.2lf, %.2lf)\n", ih->saddr(), rp->rp_x, rp->rp_y, rp->rp_z);
   loctable.print(); /*Print out the formatted location table*/
#endif //DEBUG

 Packet::free(p);
}

void
AORGLU::nb_insert(nsaddr_t id) {
AORGLU_Neighbor *nb = new AORGLU_Neighbor(id);

 assert(nb);
 nb->nb_expire = CURRENT_TIME +
                (1.5 * ALLOWED_HELLO_LOSS * HELLO_INTERVAL);
 LIST_INSERT_HEAD(&nbhead, nb, nb_link);
 seqno += 2;             // set of neighbors changed
 assert ((seqno%2) == 0);
}


AORGLU_Neighbor*
AORGLU::nb_lookup(nsaddr_t id) {
AORGLU_Neighbor *nb = nbhead.lh_first;

 for(; nb; nb = nb->nb_link.le_next) {
   if(nb->nb_addr == id) break;
 }
 return nb;
}


/*
 * Called when we receive *explicit* notification that a Neighbor
 * is no longer reachable.
 */
void
AORGLU::nb_delete(nsaddr_t id) {
AORGLU_Neighbor *nb = nbhead.lh_first;

 log_link_del(id);
 seqno += 2;     // Set of neighbors changed
 assert ((seqno%2) == 0);

 for(; nb; nb = nb->nb_link.le_next) {
   if(nb->nb_addr == id) {
     LIST_REMOVE(nb,nb_link);
     delete nb;
     break;
   }
 }

 //handle_link_failure(id);

}


/*
 * Purges all timed-out Neighbor Entries - runs every
 * HELLO_INTERVAL * 1.5 seconds.
 */
void
AORGLU::nb_purge() {
AORGLU_Neighbor *nb = nbhead.lh_first;
AORGLU_Neighbor *nbn;
double now = CURRENT_TIME;

 for(; nb; nb = nbn) {
   nbn = nb->nb_link.le_next;
   if(nb->nb_expire <= now) {
     nb_delete(nb->nb_addr);
   }
 }

}
