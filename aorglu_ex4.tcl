#### AORGLU Example #4 ##########################################################
# This example tests the Route Repair functionality with no local maximums.
#  N0 sends to N5 through 0->1->2->3->4->5. N4 then moves out of the path
#  causing the route to break. N6 comes and takes the place of N4, so the
#  route can be re-established using REPA (or RREQ if REPA times out)
#################################################################################

# Define options
set val(chan)           Channel/WirelessChannel    ;# channel type
set val(prop)           Propagation/TwoRayGround   ;# radio-propagation
set val(netif)          Phy/WirelessPhy            ;# network interface type

set val(mac)            Mac/802_11                 ;# MAC type
set val(ifq)            Queue/DropTail/PriQueue    ;# interface queue type
set val(ll)             LL                         ;# link layer type
set val(ant)            Antenna/OmniAntenna        ;# antenna model
set val(ifqlen)         50                         ;# max packet in ifq
set val(nn)             7                          ;# number of mobilenodes
set val(rp)             AORGLU                     ;# routing protocol
set val(x)              800                  ;# X dimension of topography
set val(y)              400                  ;# Y dimension of topography
set val(stop)        	35               ;# time of simulation end

set ns            [new Simulator]
set tracefd       [open tr4.tr w]
set windowVsTime2 [open win.tr w]
set namtrace      [open nm4.nam w]

$ns trace-all $tracefd
$ns namtrace-all-wireless $namtrace $val(x) $val(y)

# set up topography object
set topo       [new Topography]

$topo load_flatgrid $val(x) $val(y)

create-god $val(nn)

#
#  Create nn mobilenodes [$val(nn)] and attach them to the channel.
#

# configure the nodes
        $ns node-config -adhocRouting $val(rp) \
             -llType $val(ll) \
             -macType $val(mac) \
             -ifqType $val(ifq) \
             -ifqLen $val(ifqlen) \
             -antType $val(ant) \
             -propType $val(prop) \
             -phyType $val(netif) \
             -channelType $val(chan) \
             -topoInstance $topo \
             -agentTrace ON \
             -routerTrace ON \
             -macTrace OFF \
             -movementTrace ON

    for {set i 0} {$i < $val(nn) } { incr i } {
        set node_($i) [$ns node]
    }

# Provide initial location of mobilenodes
$node_(0) set X_ 0.0
$node_(0) set Y_ 300.0
$node_(0) set Z_ 0.0

$node_(1) set X_ 150.0
$node_(1) set Y_ 300.0
$node_(1) set Z_ 0.0

$node_(2) set X_ 300.0
$node_(2) set Y_ 300.0
$node_(2) set Z_ 0.0

$node_(3) set X_ 450.0
$node_(3) set Y_ 300.0
$node_(3) set Z_ 0.0

$node_(4) set X_ 600.0
$node_(4) set Y_ 300.0
$node_(4) set Z_ 0.0

$node_(5) set X_ 750.0
$node_(5) set Y_ 300.0
$node_(5) set Z_ 0.0

$node_(6) set X_ 600.0
$node_(6) set Y_ 0.0
$node_(6) set Z_ 0.0


# Generation of movements
$ns at 2.0 "$node_(4) setdest 100.0 1.0 15.0"
$ns at 4.0 "$node_(6) setdest 600.0 300.0 19.0"
#$ns at 110.0 "$node_(0) setdest 480.0 300.0 5.0"

# Set a TCP connection between node_(0) and node_(5)
set tcp [new Agent/TCP/Newreno]
$tcp set class_ 2
set sink [new Agent/TCPSink]
$ns attach-agent $node_(0) $tcp
$ns attach-agent $node_(5) $sink
$ns connect $tcp $sink
set ftp [new Application/FTP]
$ftp attach-agent $tcp
$ns at 1.0 "$ftp start"

# Printing the window size
proc plotWindow {tcpSource file} {
global ns
set time 0.01
set now [$ns now]
set cwnd [$tcpSource set cwnd_]
puts $file "$now $cwnd"
$ns at [expr $now+$time] "plotWindow $tcpSource $file" }
$ns at 10.1 "plotWindow $tcp $windowVsTime2"

# Define node initial position in nam
for {set i 0} {$i < $val(nn)} { incr i } {
# 30 defines the node size for nam
$ns initial_node_pos $node_($i) 30
}

# Telling nodes when the simulation ends
for {set i 0} {$i < $val(nn) } { incr i } {
    $ns at $val(stop) "$node_($i) reset";
}

# ending nam and the simulation
$ns at $val(stop) "$ns nam-end-wireless $val(stop)"
$ns at $val(stop) "stop"
$ns at 35.01 "puts \"end simulation\" ; $ns halt"
proc stop {} {
    global ns tracefd namtrace
    $ns flush-trace
    close $tracefd
    close $namtrace
}

$ns run


