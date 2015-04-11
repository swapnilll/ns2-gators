# Introduction #

The AOR-GLU protocol is an enhancement of the AODV routing protocol for MANETs. The AOR-GLU attempts to optimize the route maintenance mechanism by removing the need to flood the network while executing the route repair mechanism.

# Required Modifications #
  1. **COMPLETE** Adding the aor-glu routing protocol to NS-2.
  1. **COMPLETE** Add a location structure to the AODV node.
  1. Retrieving a node's location information from a packet.
  1. Add a location cache with location expiration timer to the AODV node. (Robert)
  1. **COMPLETE** Modify the RREP packet to include the destinations current location.
  1. Add a LUDP packet type. The LUDP packet is used to send periodic location updates from a     destination to communicating source(s).
  1. Modify the REPA packet to include the greedy field as well as implement greedy forwarding.
  1. Add the hand-rule mechanism for splitting the REPA packet into two REPC packets.
  1. Implement cross-edge detection scheme.

# Completed Modifications #
## Adding aor-glu routing protocol to NS-2 ##
NS Files Edited:
  * ../ns-2.34/common/packet.h
  * ../ns-2.34/tcl/lib/ns-default.tcl
  * ../ns-2.34/tcl/lib/ns-agent.tcl
  * ../ns-2.34/tcl/lib/ns-mobilenode.tcl
  * ../ns-2.34/tcl/lib/ns-packet.tcl
  * ../ns-2.34/tcl/lib/ns-lib.tcl
  * ../ns-2.34/trace/cmu-trace.cc - contains the trace format for the log file.
  * ../ns-2.34/trace/cmu-trace.h
  * ../ns-2.34/queue/priqueue.cc

To add the AOR-GLU protocol to ns-2 do the following:
  1. Create a directory named aorglu in the ns-2.34 folder.
  1. Copy the aorglu repository folder contents into the folder.
  1. Replace any files needed from the "tcl/lib", "queue" "trace" folders.
  1. Overwrite the Makefile.in with the repository Makefile.in.
  1. Goto the ns-2.34 root directory and type: "./configure && make clean && make"
  1. This should compile the aorglu protocol and add "AORGLU" to the list of available routing agents in NS-2.
  1. Make sure to run the recently compiled NS executable **NOT** the /usr/bin/ns. This is likely an older NS version that does not contain the new AORGLU protocol.


## Add a location structure to the AODV node ##
The MobileNode object (ns-2.34/common/MobileNode.cc;MobileNode.h) already contains getter methods for the current node location. The location of a node can be retrieved by calling
node->getLoc(&x, &y, &z)
Where x, y and z are double values (see MobileNode.h for member function declarations.)