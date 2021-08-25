// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.linklayer;

moduleinterface IOpportunisticLinkLayer
{
    parameters:
        // WakeUpMac retry signals and statistics
        @signal[transmissionTries](type=long);
        @signal[ackContentionRounds](type=long);
        @statistic[transmissionTries](title="Number of Tries till packet discarded or received"; record=histogram,vector);
        @statistic[ackContentionRounds](title="Number of ack contention rounds for each packet"; record=histogram,vector);
        
        // Weighted Encounters signals and with attached L2Addr and EDC/EquivalentTx(EqTx) details
        @signal[coincidentalEncounter](type=double);
        @signal[expectedEncounter](type=double);
        @signal[listenForEncountersEnded](type=double);
        @statistic[coincidentalEncounter](title="Weighted coincidental neighbor encounter"; record=vector,count);
        @statistic[expectedEncounter](title="Weighted expected neighbor encounter"; record=vector,count);
        @statistic[listenForEncountersEnded](title="Ended expected encounters listening period"; record=vector,count);
}