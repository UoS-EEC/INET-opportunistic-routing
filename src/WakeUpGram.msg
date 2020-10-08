

//
// TODO generated message class
//

import inet.common.packet.chunk.Chunk;
import inet.linklayer.common.MacAddress;

enum WakeUpGramType {
    WU_DATA = 1;
    WU_ACK = 2;
    WU_BEACON = 3;
};

class WakeUpGram extends inet::FieldsChunk
{
    WakeUpGramType type;    // header type (1 byte)
    uint8_t headerLengthField;    // header length in bytes, max value is 255 (1 byte)
    inet::MacAddress transmitterAddress;    // (6 byte)
    inet::MacAddress receiverAddress;    // (6 byte)
}

