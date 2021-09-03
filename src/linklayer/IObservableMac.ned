// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

package oppostack.linklayer;

// Interface for signals and properties to observe energy consumptions of MAC layer 
moduleinterface IObservableMac
{
    // Must have energyStorage module defined to observe consumption
    string energyStorage;
    
    // Signals to signal start and end of messages
    @signal[receptionStarted](type=bool);
    @signal[receptionEnded](type=bool);
    @signal[receptionDropped](type=bool);
    @signal[transmissionStarted](type=bool);
    @signal[transmissionEnded](type=bool);
}
