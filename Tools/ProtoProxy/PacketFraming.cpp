
// PacketFraming.cpp

// Implementation of the packet framing, see PacketFraming.h for the sources.





#include "Globals.h"
#include "PacketFraming.h"





void WriteVarIntTo(ContiguousByteBuffer & a_Out, UInt64 a_Value)
{
	// wiki.vg "Data types#VarInt and VarLong": little-endian 7-bit groups, high bit set on all but the last:
	do
	{
		auto Byte = static_cast<std::byte>(a_Value & 0x7f);
		a_Value >>= 7;
		if (a_Value != 0)
		{
			Byte |= static_cast<std::byte>(0x80);
		}
		a_Out.push_back(Byte);
	} while (a_Value != 0);
}





/** Reads a VarInt from a_Bytes starting at a_Pos.
Returns 1 on success, 0 if more data is needed, -1 if the value is not a valid VarInt. */
static int ReadVarIntFrom(const ContiguousByteBuffer & a_Bytes, size_t & a_Pos, UInt64 & a_Value)
{
	a_Value = 0;
	int Shift = 0;
	int NumBytes = 0;
	while (true)
	{
		if (a_Pos >= a_Bytes.size())
		{
			return 0;
		}
		const auto Byte = static_cast<unsigned char>(a_Bytes[a_Pos]);
		a_Pos += 1;
		a_Value |= static_cast<UInt64>(Byte & 0x7f) << Shift;
		Shift += 7;
		NumBytes += 1;
		if ((Byte & 0x80) == 0)
		{
			return 1;
		}
		if (NumBytes >= 5)
		{
			// A packet length / packet ID never exceeds 32 bits:
			return -1;
		}
	}
}





////////////////////////////////////////////////////////////////////////////////
// cInboundPackets:

cInboundPackets::cInboundPackets(void):
	m_Consumed(0),
	m_Threshold(-1),
	m_HasError(false)
{
}





/** Returns the number of bytes a_VarInt would occupy. */
static size_t GetVarIntSize(UInt64 a_Value)
{
	size_t Size = 1;
	while (a_Value >= 0x80)
	{
		a_Value >>= 7;
		Size += 1;
	}
	return Size;
}





void cInboundPackets::AddData(const void * a_Data, size_t a_Size)
{
	// Drop what has already been handed out, so that the buffer doesn't grow without bounds:
	if (m_Consumed > 0)
	{
		m_Buffer.erase(0, m_Consumed);
		m_Consumed = 0;
	}
	m_Buffer.append(static_cast<const std::byte *>(a_Data), a_Size);
}





void cInboundPackets::SetThreshold(Int32 a_Threshold)
{
	m_Threshold = a_Threshold;
}





bool cInboundPackets::Inflate(const ContiguousByteBufferView a_Payload, size_t a_UncompressedSize)
{
	try
	{
		const auto Result = m_Extractor.ExtractZLib(a_Payload, a_UncompressedSize);
		const auto View = Result.GetView();
		if (View.size() != a_UncompressedSize)
		{
			return false;
		}
		m_Decompressed.assign(View.data(), View.size());
		return true;
	}
	catch (const std::runtime_error &)
	{
		return false;
	}
}





bool cInboundPackets::NextPacket(sCapturedPacket & a_Packet)
{
	auto Pos = m_Consumed;
	UInt64 PacketLength = 0;
	const int LengthRes = ReadVarIntFrom(m_Buffer, Pos, PacketLength);
	if (LengthRes < 0)
	{
		// A VarInt that runs over 5 bytes can only come from a broken or non-Minecraft peer:
		m_HasError = true;
		return false;
	}
	if (LengthRes == 0)
	{
		return false;
	}
	if ((PacketLength == 0) || (Pos + PacketLength > m_Buffer.size()))
	{
		// Not a whole packet yet (or a length of zero, which no sender should produce):
		return false;
	}
	const size_t FrameStart = m_Consumed;
	const size_t FrameEnd = Pos + static_cast<size_t>(PacketLength);
	m_Consumed = FrameEnd;

	UInt64 DataLength = 0;
	if (m_Threshold >= 0)
	{
		// Compressed framing, wiki.vg "Protocol#With compression": the payload starts with the
		// uncompressed size, zero meaning "this packet was sent as-is":
		if (ReadVarIntFrom(m_Buffer, Pos, DataLength) != 1)
		{
			m_HasError = true;
			return false;
		}
	}
	auto Payload = ContiguousByteBufferView(m_Buffer).substr(Pos, FrameEnd - Pos);
	const size_t PacketIDPos = Pos;
	UInt64 PacketID = 0;
	if (DataLength == 0)
	{
		size_t ReadPos = PacketIDPos;
		if (ReadVarIntFrom(m_Buffer, ReadPos, PacketID) != 1)
		{
			m_HasError = true;
			return false;
		}
		a_Packet.m_PacketID = static_cast<UInt32>(PacketID);
		a_Packet.m_Raw = ContiguousByteBufferView(m_Buffer).substr(FrameStart, FrameEnd - FrameStart);
		a_Packet.m_Payload = ContiguousByteBufferView(m_Buffer).substr(ReadPos, FrameEnd - ReadPos);
		a_Packet.m_IsCompressed = false;
		return true;
	}

	// The payload is deflate-compressed, inflate it into our own storage before reading the packet ID:
	if (!Inflate(Payload, static_cast<size_t>(DataLength)))
	{
		m_HasError = true;
		return false;
	}
	size_t DecompressedPos = 0;
	if (ReadVarIntFrom(m_Decompressed, DecompressedPos, PacketID) != 1)
	{
		m_HasError = true;
		return false;
	}
	a_Packet.m_PacketID = static_cast<UInt32>(PacketID);
	a_Packet.m_Raw = ContiguousByteBufferView(m_Buffer).substr(FrameStart, FrameEnd - FrameStart);
	a_Packet.m_Payload = ContiguousByteBufferView(m_Decompressed).substr(DecompressedPos);
	a_Packet.m_IsCompressed = true;
	return true;
}





////////////////////////////////////////////////////////////////////////////////
// cOutboundPackets:

cOutboundPackets::cOutboundPackets(void):
	m_Threshold(-1),
	m_Compressor(6)
{
}





void cOutboundPackets::SetThreshold(Int32 a_Threshold)
{
	m_Threshold = a_Threshold;
}





void cOutboundPackets::Serialize(UInt32 a_PacketID, const ContiguousByteBufferView a_Payload, ContiguousByteBuffer & a_Out)
{
	// Build "VarInt PacketID + payload", the unit that gets compressed as a whole:
	ContiguousByteBuffer Contents;
	Contents.reserve(a_Payload.size() + 5);
	WriteVarIntTo(Contents, a_PacketID);
	Contents.append(a_Payload.data(), a_Payload.size());

	if ((m_Threshold < 0) || (Contents.size() < static_cast<size_t>(m_Threshold)))
	{
		// Sent uncompressed, wiki.vg "Protocol#With compression": DataLength is zero in that case:
		if (m_Threshold < 0)
		{
			WriteVarIntTo(a_Out, Contents.size());
		}
		else
		{
			WriteVarIntTo(a_Out, Contents.size() + 1);
			WriteVarIntTo(a_Out, 0);
		}
		a_Out.append(Contents.data(), Contents.size());
		return;
	}

	const auto Compressed = m_Compressor.CompressZLib(Contents);
	const auto View = Compressed.GetView();
	WriteVarIntTo(a_Out, View.size() + GetVarIntSize(Contents.size()));
	WriteVarIntTo(a_Out, Contents.size());
	a_Out.append(View.data(), View.size());
}
