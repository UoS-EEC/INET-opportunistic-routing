

//
// TODO generated message class
//

import inet.common.packet.chunk.Chunk;
import inet.linklayer.common.MacAddress;

enum WakeUpGramType {
    WU_DATA = 1;
    WU_ACK = 2;
    WU_BEACON = 0xF5;
};

class WakeUpGram extends inet::FieldsChunk
{
    WakeUpGramType type;    // header type (1 byte)
    inet::MacAddress transmitterAddress;    // (6 byte)
    inet::MacAddress receiverAddress;    // (6 byte)
}
class WakeUpBeacon extends WakeUpGram
{
    type = WU_BEACON;
    uint16_t minProgress;    // header length in bytes, max value is 255 (1 byte) in a wake-up this forms the preamble
    chunkLength = inet::B(15);
}

class WakeUpDatagram extends WakeUpBeacon
{
    type = WU_DATA;
}

class WakeUpAck extends WakeUpGram
{
    type = WU_ACK;
    chunkLength = inet::B(13);
}