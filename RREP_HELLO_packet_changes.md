# Introduction #

  * Note: in aorglu.cc the variable "index" is the address of the current node
  * Modified files aorglu.cc and aorglu\_packet.h
  * Changes:
  1. Added coordinate variables to the RREP packet header, hdr\_aorglu\_reply struct, in aorglu\_packets.h
  1. Included node.h and mobilenode.h to aorglu.cc
  1. Modified the sendReply(...) function to write the original sending node's coordinates to the RREP packet header. (aorglu.cc)
  1. Modified the sendHello() function to write the sending nodes coordinates to the packet header (aorglu.cc)
  1. Added debug outputs to recvReply(...) and recvHello(...) to print out the coordinates once the RREP and HELLO packets are received. (aorglu.cc)

# Details #

The following changes were made to RREP packet header in aorglu\_packet.h. The parameters rp\_x, rp\_y and rp\_z (all doubles) were added to the packet header struct hdr\_aorglu\_reply. Also, the new parameters were added to the packet header size calculation.
```
struct hdr_aorglu_reply {
        u_int8_t   rp_type;         // Packet Type
        u_int8_t   reserved[2];
        u_int8_t   rp_hop_count;    // Hop Count
        nsaddr_t   rp_dst;          // Destination IP Address
        u_int32_t  rp_dst_seqno;    // Destination Sequence Number
        nsaddr_t   rp_src;          // Source IP Address
	double	   rp_x;	    // csh - X coordinate of node sending reply
	double	   rp_y;	    // csh - Y coordinate of node sending reply
	double	   rp_z;	    // csh - Z coordinate of node sending reply
        double	   rp_lifetime;     // Lifetime
        double     rp_timestamp;    // when corresponding REQ sent;
				    // used to compute route discovery latency
						
  inline int size() { 
  int sz = 0;
  
  	sz = sizeof(u_int8_t)		// rp_type
	     + 2*sizeof(u_int8_t) 	// rp_flags + reserved
	     + sizeof(u_int8_t)		// rp_hop_count
	     + sizeof(double)		// rp_timestamp
	     + sizeof(nsaddr_t)		// rp_dst
	     + sizeof(u_int32_t)	// rp_dst_seqno
	     + sizeof(nsaddr_t)		// rp_src
	     + sizeof(u_int32_t)	// rp_lifetime
	     + sizeof(double)		// rp_x (csh)
	     + sizeof(double)		// rp_y (csh)
	     + sizeof(double);		// rp_z (csh)
  
  	assert (sz >= 0);
	return sz;
  }

};
```

node.h and mobilenode.h were included in aorglu.cc
```
#include <aorglu/aorglu.h>
#include <aorglu/aorglu_packet.h>
#include <random.h>
#include <mobilenode.h>
#include <node.h>
#include <cmu-trace.h>
//#include <energy-model.h>
```

The following lines of code were added to the sendReply(...) function in aorglu.cc.  The purpose of this was to add the coordinates of the original sending node to the RREP packet header. First the MobileNode object is retrieved from the node address (index). Then the RREP packet header information is updated.
```
  //csh - Get the x and y coordinates from the current node
  //and add them to the packet header.
  MobileNode *currNode = (MobileNode*) Node::get_node_by_address(index);
  rp->rp_x = currNode->X();
  rp->rp_y = currNode->Y();
  rp->rp_z = currNode->Z();
```

The following lines of code were added to the recvReply(...) function in aorglu.cc.  The purpose of this was to print out the address and coordinates of the original node that sent the RREP. The address is taken from the IP packet header (ih->saddr()) and not the RREP packet header (rp->rp\_src) which only provides the address of the one-hop previous node.
```
 /*
  * If reply is for me, discard it.
  */

if(ih->daddr() == index || suppress_reply) {
   //csh - print out coordinates when reply is received
#ifdef DEBUG
   fprintf(stderr, "The current node address is %d\n", index);
   fprintf(stderr, "Node %d coordinates: (%.2lf, %.2lf, %.2lf)\n", ih->saddr(), rp->rp_x, rp->rp_y, rp->rp_z);
#endif //DEBUG
   Packet::free(p);
 }
```

The following lines of code were added to the sendHello() function in aorglu.cc
```
  //csh - Get the x and y coordinates from the current node
  //and add them to the packet header.
  MobileNode *currNode = (MobileNode*) Node::get_node_by_address(index);
  rh->rp_x = currNode->X();
  rh->rp_y = currNode->Y();
  rh->rp_z = currNode->Z();
```

The recvHello(...) function in aorglu.cc was modified in the following manner:
```
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

   //csh - print out coordinates when reply is received
#ifdef DEBUG
   fprintf(stderr, "The current node address is %d\n", index);
   fprintf(stderr, "Node %d coordinates: (%.2lf, %.2lf, %.2lf)\n", ih->saddr(), rp->rp_x, rp->rp_y, rp->rp_z);
#endif //DEBUG

 Packet::free(p);
}
```