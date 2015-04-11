# Introduction #

In NS-2, AODV by default uses the link-layer to establish when links have been connected. This eliminates the need for the AODV HELLO packets, reducing overhead.

The AOR-GLU protocol, however, relies on HELLO packets for keeping their 1-hop neighbor location cached.

_"The proposed protocol, assumes that each node knows its location and locations of
its 1-hop neighbor by hello message exchanges."(1)_

# Details #
In order to disable Link Layer route establishment, it is necessary to comment-out the line
```
/*
  Allows AORGLU to use link-layer (802.11) feedback in determining when
  links are up/down.
*

#define AORGLU_LINK_LAYER_DETECTION

```

Once this line has been commented out, the nodes will begin sending HELLO packets.

# References #
(1) Hsu, Chia-Chang; Lei, Chin-Laung;
Systems and Networks Communications, 2009. ICSNC '09. Fourth International Conference On
20-25 Sept. 2009 Page(s):43 - 48
Digital Object Identifier 10.1109/ICSNC.2009.83