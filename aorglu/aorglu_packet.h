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

The AORGLU code developed by the CMU/MONARCH group was optimized and tuned by Samir Das and Mahesh Marina, University of Cincinnati. The work was partially done in Sun Microsystems.
*/


#ifndef __aorglu_packet_h__
#define __aorglu_packet_h__

//#include <config.h>
//#include "aorglu.h"
#include <aorglu/aorglu_path.h>

#define AORGLU_MAX_ERRORS 100


/* =====================================================================
   Packet Formats...
   ===================================================================== */
#define AORGLUTYPE_HELLO  	0x01
#define AORGLUTYPE_RREQ   	0x02
#define AORGLUTYPE_RREP   	0x04
#define AORGLUTYPE_RERR   	0x08
#define AORGLUTYPE_RREP_ACK  	0x10
#define AORGLUTYPE_LUDP		0x20 //csh - add Location UpDate Packet (LUDP)
#define	AORGLUTYPE_REPA		0x40 //csh - add Route Repair Packet (REPA) (also used for RREC)
#define AORGLUTYPE_REPC         0x80 //rgk - (REPC) type

/*
 * AORGLU Routing Protocol Header Macros
 */
#define HDR_AORGLU(p)		((struct hdr_aorglu*)hdr_aorglu::access(p))
#define HDR_AORGLU_REQUEST(p)  	((struct hdr_aorglu_request*)hdr_aorglu::access(p))
#define HDR_AORGLU_REPLY(p)	((struct hdr_aorglu_reply*)hdr_aorglu::access(p))
#define HDR_AORGLU_ERROR(p)	((struct hdr_aorglu_error*)hdr_aorglu::access(p))
#define HDR_AORGLU_RREP_ACK(p)	((struct hdr_aorglu_rrep_ack*)hdr_aorglu::access(p))
#define HDR_AORGLU_LUDP(p)	((struct hdr_aorglu_ludp*)hdr_aorglu::access(p)) //csh - add LUDP macro to access the header
#define HDR_AORGLU_REPA(p)	((struct hdr_aorglu_repa*)hdr_aorglu::access(p)) //csh - add REPA macro to access the header
#define HDR_AORGLU_REPC(p)      ((struct hdr_aorglu_repa*)hdr_aorglu::access(p)) //rgk - REPC

/*
 * General AORGLU Header - shared by all formats
 */
struct hdr_aorglu {
        u_int8_t        ah_type;
	/*
        u_int8_t        ah_reserved[2];
        u_int8_t        ah_hopcount;
	*/
		// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_aorglu* access(const Packet* p) {
		return (hdr_aorglu*) p->access(offset_);
	}
};

struct hdr_aorglu_request {
        u_int8_t        rq_type;	// Packet Type
        u_int8_t        reserved[2];
        u_int8_t        rq_hop_count;   // Hop Count
        u_int32_t       rq_bcast_id;    // Broadcast ID

        nsaddr_t        rq_dst;         // Destination IP Address
        u_int32_t       rq_dst_seqno;   // Destination Sequence Number
        nsaddr_t        rq_src;         // Source IP Address
        u_int32_t       rq_src_seqno;   // Source Sequence Number
	double		rq_x;		// Source X coordinate - csh
	double		rq_y;		// Source Y coordinate - csh
	double		rq_z;		// Source Z coordinate - csh

        double          rq_timestamp;   // when REQUEST sent;
					// used to compute route discovery latency

  // This define turns on gratuitous replies- see aorglu.cc for implementation contributed by
  // Anant Utgikar, 09/16/02.
  //#define RREQ_GRAT_RREP	0x80

  inline int size() { 
  int sz = 0;
  /*
  	sz = sizeof(u_int8_t)		// rq_type
	     + 2*sizeof(u_int8_t) 	// reserved
	     + sizeof(u_int8_t)		// rq_hop_count
	     + sizeof(double)		// rq_timestamp
	     + sizeof(u_int32_t)	// rq_bcast_id
	     + sizeof(nsaddr_t)		// rq_dst
	     + sizeof(u_int32_t)	// rq_dst_seqno
	     + sizeof(nsaddr_t)		// rq_src
	     + sizeof(u_int32_t);	// rq_src_seqno
  */
  	sz = 7*sizeof(u_int32_t) + 3*sizeof(double); //csh - added coordinates to size calc.
  	assert (sz >= 0);
	return sz;
  }
};

struct hdr_aorglu_reply {
        u_int8_t        rp_type;                // Packet Type
        u_int8_t        reserved[2];
        u_int8_t        rp_hop_count;           // Hop Count
        nsaddr_t        rp_dst;                 // Destination IP Address
        u_int32_t       rp_dst_seqno;           // Destination Sequence Number
        nsaddr_t        rp_src;                 // Source IP Address
	double		rp_x;			// csh - X coordinate of node sending reply
	double		rp_y;			// csh - Y coordinate of node sending reply
	double		rp_z;			// csh - Z coordinate of node sending reply
        double	        rp_lifetime;            // Lifetime

        double          rp_timestamp;           // when corresponding REQ sent;
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

struct hdr_aorglu_error {
        u_int8_t        re_type;        // Type
	nsaddr_t	re_orig_src;	// original address whose sent packet resulted in error.
        nsaddr_t        re_orig_dst;    // orginial address being sent to that cannot be reached! 

  inline int size() { 
  int sz = 0;

  	sz = sizeof(u_int8_t)		// type
	     + 2*sizeof(nsaddr_t); 	// orig_addr

	assert(sz);
        return sz;
  }

};

//--------- LOCATION UPDATE PACKET HEADER ---------------------
//csh - Add LUDP packet header struct
struct hdr_aorglu_ludp {
        u_int8_t        lu_type;        // Packet Type
        nsaddr_t        lu_dst;         // Destination IP Address
        nsaddr_t        lu_src;         // Source IP Address
	double		lu_x;		// X coordinate of node sending LUDP
	double		lu_y;		// Y coordinate of node sending LUDP
	double		lu_z;		// Z coordinate of node sending LUDP
						
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

//--------- ROUTE REPAIR PACKET HEADER ---------------------
//csh - Add REPA packet header struct
struct hdr_aorglu_repa {
        u_int8_t        rpr_type;       // Packet Type
	nsaddr_t        rpr_src;        // Source IP Address
	u_int32_t       rpr_src_seqno;  // Source Sequence Number
	nsaddr_t        rpr_dst;        // Destination IP Address
        u_int32_t       rpr_dst_seqno;  // Destination Sequence Number
	u_int8_t        rpr_hop_count;  // Hop Count
	u_int8_t	rpr_greedy;	// Greedy flag. If set to 1 use greedy forwarding.
	u_int8_t	rpr_dir;	// Direction to search for next hop 0=clockwise 1=counterclockwise.
	double		rpr_x;		// X coordinate of repair destination
	double		rpr_y;		// Y coordinate of repair destination
	double		rpr_z;		// Z coordinate of repair destination
        aorglu_path     *path; 

        double          rpr_timestamp;   // when REQUEST sent;
						
  inline int size() { 
  int sz = 0;
  
  	sz =   sizeof(u_int8_t)		// rpr_type
             + 2*sizeof(u_int32_t)      // rpr_dst_seqno and rpr_src_seqno
	     + 2*sizeof(u_int8_t)	// rpr_greedy and rpr_hop_count;
	     + sizeof(nsaddr_t)		// rpr_dst
	     + sizeof(nsaddr_t)		// rpr_src
	     + sizeof(double)		// rpr_x
	     + sizeof(double)		// rpr_y
	     + sizeof(double)		// rpr_z
             + path->length() * sizeof(aorglu_path_entry)
	     //+ sizeof(int)     
             + sizeof(double);
  
  	assert (sz >= 0);
	return sz;
  }

};
//--------------------------------------------------------------

struct hdr_aorglu_rrep_ack {
	u_int8_t	rpack_type;
	u_int8_t	reserved;
};

// for size calculation of header-space reservation
union hdr_all_aorglu {
  hdr_aorglu          ah;
  hdr_aorglu_request  rreq;
  hdr_aorglu_reply    rrep;
  hdr_aorglu_ludp     ludp; //csh
  hdr_aorglu_repa     repa; //csh,rgk
  hdr_aorglu_error    rerr;
  hdr_aorglu_rrep_ack rrep_ack;
};

#endif /* __aorglu_packet_h__ */
