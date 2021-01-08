

//
// TODO generated message class
//

import inet.common.packet.chunk.Chunk;
import inet.linklayer.common.MacAddress;
import Units;

enum WakeUpGramType {
    WU_DATA = 1;
    WU_ACK = 2;
    WU_SQUASH = 3; // Squash the transmitted network packet, as reception confirmed elsewhere
    WU_BEACON = 0xF5;
};

class WakeUpGram extends inet::FieldsChunk
{
    WakeUpGramType type;    // header type (1 byte)
    inet::MacAddress transmitterAddress;    // (2 byte) but inet represents as 6
    inet::MacAddress receiverAddress;    // (2 byte) but inet represents as 6
    orpl::ExpectedCost expectedCostInd = orpl::ExpectedCost(255); // (1 byte)
}
class WakeUpBeacon extends WakeUpGram
{
    type = WU_BEACON; //Max value is 255 (1 byte) in a wake-up this forms the preamble
    orpl::ExpectedCost minExpectedCost;  // Actually represented by the ExpectedCost type
    chunkLength = inet::B(8);
}

class WakeUpDatagram extends WakeUpBeacon
{
    type = WU_DATA;
}

class WakeUpAck extends WakeUpGram
{
    type = WU_ACK;
    uint8_t length;
    chunkLength = inet::B(9);
}