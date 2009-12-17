
# Define options
set val(chan)           Channel/WirelessChannel    ;# channel type
set val(prop)           Propagation/TwoRayGround   ;# radio-propagation
set val(netif)          Phy/WirelessPhy            ;# network interface type

set val(mac)            Mac/802_11                 ;# MAC type
set val(ifq)            Queue/DropTail/PriQueue    ;# interface queue type
set val(ll)             LL                         ;# link layer type
set val(ant)            Antenna/OmniAntenna        ;# antenna model
set val(ifqlen)         50                         ;# max packet in ifq
set val(nn)             16                          ;# number of mobilenodes
set val(rp)             AORGLU                     ;# routing protocol
set val(x)              1200                  ;# X dimension of topography
set val(y)              1000                  ;# Y dimension of topography
set val(stop)        	50               ;# time of simulation end

set ns            [new Simulator]
set tracefd       [open tr2.tr w]
set windowVsTime2 [open win.tr w]
set namtrace      [open nm2.nam w]

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
$node_(0) set X_ 125.0
$node_(0) set Y_ 300.0
$node_(0) set Z_ 0.0

$node_(1) set X_ 125.0
$node_(1) set Y_ 250.0
$node_(1) set Z_ 0.0

$node_(2) set X_ 125.0
$node_(2) set Y_ 120.0
$node_(2) set Z_ 0.0

$node_(3) set X_ 350.0
$node_(3) set Y_ 30.0
$node_(3) set Z_ 0.0

$node_(13) set X_ 250.0
$node_(13) set Y_ 30.0
$node_(13) set Z_ 0.0

$node_(4) set X_ 510.0
$node_(4) set Y_ 90.0
$node_(4) set Z_ 0.0

$node_(14) set X_ 100.0
$node_(14) set Y_ 30.0
$node_(14) set Z_ 0.0

$node_(5) set X_ 610.0
$node_(5) set Y_ 210.0
$node_(5) set Z_ 0.0

$node_(15) set X_ 50.0 
$node_(15) set Y_ 30.0
$node_(15) set Z_ 0.0

$node_(6) set X_ 760.0
$node_(6) set Y_ 230.0
$node_(6) set Z_ 0.0

$node_(7) set X_ 840.0
$node_(7) set Y_ 400.0
$node_(7) set Z_ 0.0

$node_(8) set X_ 745.0
$node_(8) set Y_ 500.0
$node_(8) set Z_ 0.0

$node_(9) set X_ 610.0
$node_(9) set Y_ 600.0
$node_(9) set Z_ 0.0

$node_(10) set X_ 420.0
$node_(10) set Y_ 600.0
$node_(10) set Z_ 0.0

$node_(11) set X_ 280.0
$node_(11) set Y_ 590.0
$node_(11) set Z_ 0.0

$node_(12) set X_ 900.0
$node_(12) set Y_ 60.0
$node_(12) set Z_ 0.0


# Generation of movements
$ns at 2.0 "$node_(4) setdest 1100.0 50.0 50.0"
$ns at 11.0 "$node_(12) setdest 510.0 90.0 60.0"
#$ns at 110.0 "$node_(0) setdest 480.0 300.0 5.0"

# Set a TCP connection between node_(0) and node_(1)
set tcp [new Agent/TCP/Newreno]
$tcp set class_ 2
set sink [new Agent/TCPSink]
$ns attach-agent $node_(0) $tcp
$ns attach-agent $node_(11) $sink
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
$ns at 50.01 "puts \"end simulation\" ; $ns halt"
proc stop {} {
    global ns tracefd namtrace
    $ns flush-trace
    close $tracefd
    close $namtrace
}

$ns run


