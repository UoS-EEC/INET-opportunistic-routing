//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

import inet.common.packet.chunk.Chunk;
import inet.common.Units;
import inet.common.TlvOptions;
import inet.networklayer.common.L3Address;

namespace oppostack;

cplusplus{{
}}
class RoutingSetExt extends inet::TlvOptionBase{
	inet::L3Address entry[];
	length = 512;
	type = 241;
};

cplusplus(RoutingSetExt) {{
    static const short extType = 241;
	virtual short getLength() const override; // Get the compressed length of the Extension
}}