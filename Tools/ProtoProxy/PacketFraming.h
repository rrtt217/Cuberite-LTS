
// PacketFraming.h

// Splits the TCP stream of one connection direction into Minecraft protocol packets, and serialises
// packets the proxy itself generates.
//
// The framing follows wiki.vg "Protocol#Packet format", revision 1313 (protocol 340 / MC 1.12.2):
//   https://wiki.vg/index.php?title=Protocol&oldid=1313
//	without compression:  VarInt Length, VarInt PacketID, data
//	with compression:     VarInt Length, VarInt DataLength, [zlib-deflated] VarInt PacketID + data
// where DataLength == 0 means "this packet was sent uncompressed".
//
// The class holds no socket state, so it can be unit-tested (see tests/ProtoProxy).





#pragma once

#include "Globals.h"
#include "StringCompression.h"





/** One packet as it was received, plus the decoded view of its contents.
Both views point into the cInboundPackets instance and stay valid only until its next call. */
struct sCapturedPacket
{
	UInt32 m_PacketID;
	ContiguousByteBufferView m_Raw;       ///< The complete frame as received, ready to be relayed verbatim
	ContiguousByteBufferView m_Payload;   ///< Packet ID stripped, inflated if it was compressed
	bool m_IsCompressed;
} ;





/** Reassembles received bytes into complete packets. */
class cInboundPackets
{
public:
	cInboundPackets(void);

	/** Appends freshly received bytes to the internal buffer. Invalidates previously returned views. */
	void AddData(const void * a_Data, size_t a_Size);

	/** Extracts the next complete packet, if the buffer holds one.
	Returns false if more data is needed - the packet is then left in the buffer. */
	bool NextPacket(sCapturedPacket & a_Packet);

	/** Tells the stream that the given compression threshold now applies to everything that follows.
	a_Threshold < 0 disables compression, matching the Set Compression packet's semantics. */
	void SetThreshold(Int32 a_Threshold);

	Int32 GetThreshold(void) const { return m_Threshold; }

	/** Number of bytes buffered so far, for logging. */
	size_t GetBufferedBytes(void) const { return m_Buffer.size() - m_Consumed; }

	/** True once a frame was found that the stream cannot make sense of - the connection is unusable
	from that point on, because the framing can no longer be trusted. */
	bool HasError(void) const { return m_HasError; }

protected:

	/** The received, not-yet-consumed bytes. */
	ContiguousByteBuffer m_Buffer;

	/** Number of leading bytes of m_Buffer already returned by NextPacket(). */
	size_t m_Consumed;

	/** The compression threshold in effect, -1 for "compression off". */
	Int32 m_Threshold;

	/** Set when a frame turned out to be undecodable, see HasError(). */
	bool m_HasError;

	/** Storage for the inflated payload of the packet currently being returned. */
	ContiguousByteBuffer m_Decompressed;

	/** Inflates a_Payload into m_Decompressed, expecting a_UncompressedSize bytes. */
	bool Inflate(const ContiguousByteBufferView a_Payload, size_t a_UncompressedSize);

	/** Decompressor used for the compressed packets. */
	Compression::Extractor m_Extractor;
} ;





/** Serialises a single packet that the proxy generates itself. */
class cOutboundPackets
{
public:
	explicit cOutboundPackets(void);

	/** Tells the stream which compression threshold applies to the packets written from now on. */
	void SetThreshold(Int32 a_Threshold);

	/** Appends the wire representation of one packet to a_Out.
	a_Payload must not contain the packet ID - that is prepended here. */
	void Serialize(UInt32 a_PacketID, const ContiguousByteBufferView a_Payload, ContiguousByteBuffer & a_Out);

protected:

	/** The compression threshold in effect, -1 for "compression off". */
	Int32 m_Threshold;

	/** Compressor used when a packet is large enough to pay off. */
	Compression::Compressor m_Compressor;
} ;





/** Appends a_VarInt to a_Out. */
void WriteVarIntTo(ContiguousByteBuffer & a_Out, UInt64 a_Value);
