# INET Opportunistic Routing

Opportunistic Routing is a cross-layer networking technique 
using multiple potential next hop forwarding nodes to increase 
the chance of success. 
This repository is the result of several years of study of 
"Multihop Wireless Networking with RF Wake-up Enabled Intermittently-powered Nodes". 

The routing components contained in this repository have been
designed to work with OMNeT++ 6.0 with INET. 
It consists of an implementation of intermittent ORPL, based off the ORPL protocol, but imlpemented from scratch for OMNeT++.
It also defines extensive interfaces to enable interchangability with other opportunistic cross layer protocols.

Previous versions on the branch labelled inet-4.2 work with OMNeT++ 5.7 but many modifications have not been back ported. OMNeT++ 6.0 is reccomemended going forward.

The layered protocol design method has been published in https://doi.org/10.48550/arXiv.2109.12047

## Cross-layer components
The source contained in `src/` is divided into `networklayer` and `linklayer` components. 
All the other folders are support to this or for specific modelling requirements. 

The Routing components are `ORWRouting` and the extension `ORPLRouting`. 

The MAC protocol exposes opportunistic interfaces for the routing layer to interact with. 
The MAX protocol is implemented in `ORWMac`


## Wake-up Layer components
The `WakeUpMacInterface` extend the MAC layer to enable modelling of wake-up radio behaviour.
This is important given the range and network effect trade offs available.

These interfaces could also be modified to bring Wake-up radios to normal INET, but they need to first be able to address without using the ORW metric.


## Unfinished and unverified components
`MacEnergyMonitor` dnd `PacketConsumptionTracker` 
designed to verify that the EqDC metric is a good approximation of Energy consumed to route packets.
It can measure energy consumption with a combination of Energy measurement and predicted radio consumption, however it has not been verified that the calculations are precise.

`ORWHello` and `ORPLHello`, this has outgrown it's original purpose of acting like the RPL trickle timer.
With a little work it could be better named and work more like its inherited purpose of route discovery.

Neighbor detection and prediction is currently part of the `IOpportunisticRouting` interfaces and `ORWRoutingTable` but would be better split into a previously proposed neighbor prediction link layer component.
