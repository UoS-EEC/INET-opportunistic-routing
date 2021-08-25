// Copyright (c) 2021, University of Southampton and Contributors.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.0-or-later

import common.Units;
import inet.common.packet.chunk.Chunk;
import inet.linklayer.common.MacAddress;

namespace oppostack;

enum ORWGramType {
    ORW_DATA = 1;
    ORW_ACK = 2;
    ORW_SQUASH = 3; // Squash the transmitted network packet, as reception confirmed elsewhere
    ORW_BEACON = 0xF5;
};

class ORWGram extends inet::FieldsChunk
{
    ORWGramType type;    // header type (1 byte)
    bool upwards = true; // Upwards routing flag, implemented as part of type byte
    inet::MacAddress transmitterAddress;    // (2 byte) but inet represents as 6
    inet::MacAddress receiverAddress;    // (2 byte) but inet represents as 6
    oppostack::ExpectedCost expectedCostInd = oppostack::ExpectedCost(255); // (1 byte)
}
class ORWBeacon extends ORWGram
{
    type = ORW_BEACON; //Max value is 255 (1 byte) in a wake-up this forms the preamble
    oppostack::ExpectedCost minExpectedCost;  // Actually represented by the ExpectedCost type
    chunkLength = inet::B(8);
}

class ORWDatagram extends ORWBeacon
{
    type = ORW_DATA;
}

class ORWAck extends ORWGram
{
    type = ORW_ACK;
    uint8_t length;
    chunkLength = inet::B(9);
}