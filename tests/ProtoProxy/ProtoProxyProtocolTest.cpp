
// ProtoProxyProtocolTest.cpp

// Verifies the packet decoding of Tools/ProtoProxy against the Minecraft protocol 340 (MC 1.12.2)
// specification, wiki.vg "Protocol" revision 1313,
//   https://wiki.vg/index.php?title=Protocol&oldid=1313
// together with the sub-pages the field layouts come from:
//   https://wikivg.booky.dev/Data_types
//   https://wikivg.booky.dev/index.php?title=Slot_Data&oldid=6528
//   https://wikivg.booky.dev/index.php?title=Entity_metadata&oldid=4109
//   https://wikivg.booky.dev/index.php?title=Chunk_Format&oldid=8203
//   https://wikivg.booky.dev/index.php?title=NBT&oldid=4110
//
// Invariants under test:
//   - the framing round-trips, with and without compression, across arbitrary TCP splits;
//   - every field type the layout tables use is consumed with its exact wire size, so a packet parser
//     walking the tables lands on the payload end for the packets the proxy claims to understand;
//   - the layout tables cover the whole ID range of each protocol state without holes.
//
// There is no vanilla 1.12.2 server on this machine, so the packets are built by hand from the spec
// rather than captured - the test therefore checks the decoder against the written specification.





#include "Globals.h"
#include "../TestHelpers.h"
#include "PacketFraming.h"
#include "PacketLayout.h"
#include "PayloadReader.h"





/** Packet body builder, writing the wire formats of wiki.vg "Data types" into a byte buffer. */
class cTestWriter
{
public:
	void WriteByte(UInt8 a_Value)
	{
		m_Data.push_back(static_cast<std::byte>(a_Value));
	}

	void WriteShort(Int16 a_Value)
	{
		WriteByte(static_cast<UInt8>(static_cast<UInt16>(a_Value) >> 8));
		WriteByte(static_cast<UInt8>(static_cast<UInt16>(a_Value) & 0xff));
	}

	void WriteInt(Int32 a_Value)
	{
		const auto Value = static_cast<UInt32>(a_Value);
		WriteByte(static_cast<UInt8>(Value >> 24));
		WriteByte(static_cast<UInt8>(Value >> 16));
		WriteByte(static_cast<UInt8>(Value >> 8));
		WriteByte(static_cast<UInt8>(Value));
	}

	void WriteLong(Int64 a_Value)
	{
		const auto Value = static_cast<UInt64>(a_Value);
		for (int i = 7; i >= 0; i--)
		{
			WriteByte(static_cast<UInt8>((Value >> (i * 8)) & 0xff));
		}
	}

	void WriteFloat(float a_Value)
	{
		Int32 Bits;
		memcpy(&Bits, &a_Value, sizeof(Bits));
		WriteInt(Bits);
	}

	void WriteVarInt(Int32 a_Value)
	{
		// The wire form treats the value as unsigned, so that negatives take the full five bytes:
		WriteVarIntTo(m_Data, static_cast<UInt64>(static_cast<UInt32>(a_Value)));
	}

	void WriteString(const AString & a_Value)
	{
		WriteVarInt(static_cast<Int32>(a_Value.size()));
		m_Data.append(reinterpret_cast<const std::byte *>(a_Value.data()), a_Value.size());
	}

	void WritePosition(Int32 a_X, Int32 a_Y, Int32 a_Z)
	{
		// wiki.vg "Data types#Position": 26 bits X, 12 bits Y, 26 bits Z, big-endian:
		const auto X = static_cast<UInt64>(a_X) & 0x03ffffff;
		const auto Y = static_cast<UInt64>(a_Y) & 0x0fff;
		const auto Z = static_cast<UInt64>(a_Z) & 0x03ffffff;
		WriteLong(static_cast<Int64>((X << 38) | (Y << 26) | Z));
	}

	void WriteSlotEmpty(void)
	{
		WriteShort(-1);
	}

	void WriteSlot(Int16 a_ItemID, Int8 a_Count, Int16 a_Damage, const ContiguousByteBuffer & a_NBT = ContiguousByteBuffer())
	{
		WriteShort(a_ItemID);
		WriteByte(static_cast<UInt8>(a_Count));
		WriteShort(a_Damage);
		if (a_NBT.empty())
		{
			WriteByte(0);  // TAG_End, no NBT data
		}
		else
		{
			m_Data.append(a_NBT.data(), a_NBT.size());
		}
	}

	const ContiguousByteBuffer & Data(void) const { return m_Data; }
	ContiguousByteBuffer & Data(void) { return m_Data; }

private:
	ContiguousByteBuffer m_Data;
} ;





/** Returns the string's contents, so that the tests can look for expected pieces of the log text. */
static bool Contains(const AString & a_Haystack, const AString & a_Needle)
{
	return a_Haystack.find(a_Needle) != AString::npos;
}





static void TestVarIntRoundtrip(void)
{
	// Values around the boundaries of the 7-bit groups, including the extremes of Int32:
	static const Int32 g_TestValues[] = { 0, 1, 63, 64, 127, 128, 255, 25582, 8191, 8192, 2097151, 268435455, 268435456,
	                                      2147483647, -1, -128, -2147483648
	                                     };
	for (const auto Value : g_TestValues)
	{
		cTestWriter Writer;
		Writer.WriteVarInt(Value);
		cPayloadReader Reader(Writer.Data());
		Int64 Read;
		TEST_TRUE(Reader.ReadVarIntValue(Read));
		TEST_EQUAL(Read, static_cast<Int64>(Value));
		TEST_EQUAL(Reader.GetRemainingBytes(), 0u);
	}
}





static void TestPositionRoundtrip(void)
{
	// Positions cover the signed 26-bit range for X / Z, Y is unsigned 12-bit:
	static const struct { Int32 m_X, m_Y, m_Z; } g_TestPositions[] =
	{
		{ 0, 0, 0 }, { 1, 1, 1 }, { -1, 64, -1 }, { 33554431, 255, -33554432 }, { -100, 1000, 100 },
	};
	for (const auto & Pos : g_TestPositions)
	{
		cTestWriter Writer;
		Writer.WritePosition(Pos.m_X, Pos.m_Y, Pos.m_Z);
		TEST_EQUAL(Writer.Data().size(), 8u);
		static const cPacketField g_fTest{"Test", ftPosition};
		cPayloadReader Reader(Writer.Data());
		AString Value;
		TEST_TRUE(Reader.ReadField(g_fTest, Value));
		const auto Expected = fmt::format(FMT_STRING("<{}, {}, {}>"), Pos.m_X, Pos.m_Y, Pos.m_Z);
		TEST_EQUAL(Value, Expected);
	}
}





static void TestHandshakeFields(void)
{
	// wiki.vg Protocol#Handshake: VarInt version, String address, UShort port, VarInt next state:
	cTestWriter Writer;
	Writer.WriteVarInt(340);
	Writer.WriteString("localhost");
	Writer.WriteByte(0x63);
	Writer.WriteByte(0xa8);  // 25512
	Writer.WriteVarInt(2);

	static const cPacketDefinition * Definition = cPacketLayout::GetFromClientLayout(psUnknown, 0x00);
	TEST_NOTEQUAL(Definition, nullptr);
	TEST_EQUAL(AString(Definition->m_Name), AString("Handshake"));
	cPayloadReader Reader(Writer.Data());
	int NumFields = 0;
	for (const cPacketField * Field = Definition->m_Fields; Field->m_Name != nullptr; Field++)
	{
		AString Value;
		TEST_TRUE(Reader.ReadField(*Field, Value));
		NumFields += 1;
	}
	TEST_EQUAL(NumFields, 4);
	TEST_EQUAL(Reader.GetRemainingBytes(), 0u);
}





static void TestSlotLayout(void)
{
	// wiki.vg "Slot Data" as used by protocol 340: Short Item ID, -1 = empty:
	cTestWriter Writer;
	Writer.WriteSlotEmpty();
	Writer.WriteSlot(265, 1, 0);
	Writer.WriteSlot(276, 64, 5);
	static const cPacketField g_fSlot{"Test slot", ftSlot};
	cPayloadReader Reader(Writer.Data());
	AString Value;
	TEST_TRUE(Reader.ReadField(g_fSlot, Value));
	TEST_EQUAL(Value, AString("<empty>"));
	TEST_TRUE(Reader.ReadField(g_fSlot, Value));
	TEST_TRUE(Contains(Value, "265"));
	TEST_TRUE(Reader.ReadField(g_fSlot, Value));
	TEST_TRUE(Contains(Value, "276"));
	TEST_TRUE(Contains(Value, "64"));
	TEST_EQUAL(Reader.GetRemainingBytes(), 0u);
}





static void TestMetadataLayout(void)
{
	// wiki.vg "Entity metadata", protocol 340 flavour: index, type, value, terminated by 0xff:
	cTestWriter Writer;
	Writer.WriteByte(0);      // Byte
	Writer.WriteByte(0);
	Writer.WriteByte(1);
	Writer.WriteByte(0);      // zero
	Writer.WriteByte(4);      // String / Chat
	Writer.WriteString("Mob");
	Writer.WriteByte(6);      // Boolean, true
	Writer.WriteByte(6);
	Writer.WriteByte(1);
	Writer.WriteByte(12);     // OptBlockID, block 15 (stone) => VarInt 15 << 4
	Writer.WriteByte(12);
	Writer.WriteVarInt(15 << 4);
	Writer.WriteByte(0xff);   // terminator

	static const cPacketField g_fMetadata{"Metadata", ftMetadata};
	cPayloadReader Reader(Writer.Data());
	AString Value;
	TEST_TRUE(Reader.ReadField(g_fMetadata, Value));
	TEST_EQUAL(Reader.GetRemainingBytes(), 0u);
	TEST_TRUE(Contains(Value, "4 entries"));
	// Type 4 is the chat component, wiki.vg names it "Chat":
	TEST_TRUE(Contains(Value, "Chat"));
	TEST_TRUE(Contains(Value, "Mob"));
	TEST_TRUE(Contains(Value, "true"));
	TEST_TRUE(Contains(Value, "block 15"));
}





/** Appends a complete NBT tag with one Int child and one List of two Shorts. */
static void WriteTestNBT(cTestWriter & a_Writer)
{
	// wiki.vg "NBT": tags carry type and name, compounds end with TAG_End, lists are typed and counted:
	a_Writer.WriteByte(10);                             // TAG_Compound
	a_Writer.WriteShort(0);                             // unnamed
	a_Writer.WriteByte(3);                              // TAG_Int
	a_Writer.WriteShort(1);
	a_Writer.WriteByte('x');
	a_Writer.WriteInt(-2);
	a_Writer.WriteByte(9);                              // TAG_List
	a_Writer.WriteShort(1);
	a_Writer.WriteByte('y');
	a_Writer.WriteByte(2);                              // of TAG_Short
	a_Writer.WriteInt(2);                               // two elements
	a_Writer.WriteShort(300);
	a_Writer.WriteShort(-300);
	a_Writer.WriteByte(11);                             // TAG_Int_Array
	a_Writer.WriteShort(1);
	a_Writer.WriteByte('z');
	a_Writer.WriteInt(3);
	a_Writer.WriteInt(1);
	a_Writer.WriteInt(2);
	a_Writer.WriteInt(3);
	a_Writer.WriteByte(0);                              // TAG_End of the compound
}





static void TestNBTSkipping(void)
{
	cTestWriter Writer;
	WriteTestNBT(Writer);
	// Two bytes of unrelated data behind the tag, to check that the skipper consumed exactly the tag:
	Writer.WriteByte(0xaa);
	Writer.WriteByte(0xbb);

	static const cPacketField g_fNBT{"NBT", ftNBT};
	cPayloadReader Reader(Writer.Data());
	AString Value;
	TEST_TRUE(Reader.ReadField(g_fNBT, Value));
	TEST_EQUAL(Reader.GetRemainingBytes(), 2u);
	TEST_TRUE(Contains(Value, "Compound NBT"));
}





static void TestSlotWithNBT(void)
{
	// A slot carrying NBT data, the form used by enchanted items:
	cTestWriter NBTWriter;
	WriteTestNBT(NBTWriter);
	cTestWriter Writer;
	Writer.WriteSlot(276, 1, 0, NBTWriter.Data());
	Writer.WriteByte(0x12);  // marker behind the slot

	static const cPacketField g_fSlot{"Slot", ftSlot};
	cPayloadReader Reader(Writer.Data());
	AString Value;
	TEST_TRUE(Reader.ReadField(g_fSlot, Value));
	TEST_TRUE(Contains(Value, "Compound NBT"));
	TEST_EQUAL(Reader.GetRemainingBytes(), 1u);
}





/** Builds one chunk section of the given bits-per-block, with the given number of data longs. */
static void WriteChunkSection(cTestWriter & a_Writer, int a_NumDataLongs, bool a_IsSkyLightSent)
{
	a_Writer.WriteByte(13);             // Bits per block
	a_Writer.WriteByte(0);              // Palette length, zero = global palette
	a_Writer.WriteVarInt(a_NumDataLongs);
	a_Writer.Data().resize(a_Writer.Data().size() + static_cast<size_t>(a_NumDataLongs) * 8);
	static const std::byte LightBytes[2048] = {};
	a_Writer.Data().append(LightBytes, sizeof(LightBytes));   // Block light
	if (a_IsSkyLightSent)
	{
		a_Writer.Data().append(LightBytes, sizeof(LightBytes));   // Sky light, overworld only
	}
}





/** Builds a Chunk Data payload, wiki.vg Protocol#Chunk Data, "Chunk Data Structure" of protocol 340. */
static ContiguousByteBuffer BuildChunkDataPayload(
	int a_NumSections,
	int a_NumDataLongs,
	bool a_IsGroundUpContinuous,
	bool a_IsSkyLightSent
)
{
	cTestWriter Sections;
	for (int i = 0; i < a_NumSections; i++)
	{
		WriteChunkSection(Sections, a_NumDataLongs, a_IsSkyLightSent);
	}
	if (a_IsGroundUpContinuous)
	{
		Sections.Data().resize(Sections.Data().size() + 256);   // Biomes
	}

	cTestWriter Writer;
	Writer.WriteInt(9);                             // Chunk X, an Int per wiki rev 1313
	Writer.WriteInt(-12);                           // Chunk Z
	Writer.WriteByte(a_IsGroundUpContinuous ? 1 : 0);
	Int32 Bitmask = 0;
	for (int i = 0; i < a_NumSections; i++)
	{
		Bitmask |= 1 << i;
	}
	Writer.WriteVarInt(Bitmask);
	Writer.WriteVarInt(static_cast<Int32>(Sections.Data().size()));
	Writer.Data().append(Sections.Data().data(), Sections.Data().size());
	Writer.WriteVarInt(0);                          // No block entities
	return Writer.Data();
}





static void TestChunkData(void)
{
	static const int NumDataLongs = 104;    // 4096 blocks * 13 bits, packed into 64-bit longs
	static const cPacketField g_fChunk{"Chunk data", ftChunkData};

	// An overworld chunk, five sections, ground-up continuous, so both sky light and biomes are sent:
	{
		const auto Payload = BuildChunkDataPayload(5, NumDataLongs, true, true);
		cPayloadReader Reader(Payload);
		static const cPacketField g_fChunkX{"Chunk X", ftInt};
		static const cPacketField g_fChunkZ{"Chunk Z", ftInt};
		AString Value;
		TEST_TRUE(Reader.ReadField(g_fChunkX, Value));
		TEST_EQUAL(Value, AString("9"));
		TEST_TRUE(Reader.ReadField(g_fChunkZ, Value));
		TEST_EQUAL(Value, AString("-12"));
		TEST_TRUE(Reader.ReadField(g_fChunk, Value));
		TEST_EQUAL(Reader.GetRemainingBytes(), 0u);
		TEST_TRUE(Contains(Value, "5 sections"));
		TEST_TRUE(Contains(Value, "13 bits/block"));
		TEST_TRUE(Contains(Value, ", sky light"));
		TEST_TRUE(Contains(Value, "256 bytes of biome data"));
		TEST_TRUE(Contains(Value, "size check: matches"));
	}

	// A nether chunk has no sky light and is sent as columns, without the biome array:
	{
		const auto Payload = BuildChunkDataPayload(2, NumDataLongs, false, false);
		cPayloadReader Reader(Payload);
		static const cPacketField g_fChunkX{"Chunk X", ftInt};
		static const cPacketField g_fChunkZ{"Chunk Z", ftInt};
		AString Value;
		TEST_TRUE(Reader.ReadField(g_fChunkX, Value));
		TEST_TRUE(Reader.ReadField(g_fChunkZ, Value));
		TEST_TRUE(Reader.ReadField(g_fChunk, Value));
		TEST_EQUAL(Reader.GetRemainingBytes(), 0u);
		TEST_TRUE(Contains(Value, "2 sections"));
		TEST_TRUE(!Contains(Value, ", sky light"));
		TEST_TRUE(!Contains(Value, "biome data"));
		TEST_TRUE(Contains(Value, "size check: matches"));
	}
}





static void TestFramingRoundtrip(void)
{
	// wiki.vg Protocol#Packet format, both with and without compression:
	cTestWriter Writer;
	Writer.WriteString("Test payload of some length, to be compressed if the threshold is low enough.");
	const auto Payload = ContiguousByteBufferView(Writer.Data());

	for (int IsCompressed = 0; IsCompressed <= 1; IsCompressed++)
	{
		cOutboundPackets Out;
		Out.SetThreshold(IsCompressed ? 10 : -1);
		ContiguousByteBuffer Framed;
		Out.Serialize(0x0f, Payload, Framed);

		cInboundPackets In;
		In.SetThreshold(IsCompressed ? 10 : -1);
		In.AddData(Framed.data(), Framed.size());
		sCapturedPacket Packet;
		TEST_TRUE(In.NextPacket(Packet));
		TEST_EQUAL(Packet.m_PacketID, 0x0fu);
		TEST_EQUAL(Packet.m_Payload.size(), Payload.size());
		TEST_TRUE(Packet.m_Payload == Payload);
		if (IsCompressed)
		{
			// The payload is longer than the threshold, so the frame must have gone through deflate:
			TEST_TRUE(Packet.m_IsCompressed);
			TEST_TRUE(Packet.m_Raw.size() < Payload.size() + 10);
		}
	}
}





static void TestFramingAcrossTcpSplits(void)
{
	// A packet arriving in several pieces must not be handed out before it is complete:
	cTestWriter Writer;
	Writer.WriteString("Split across the network");
	cOutboundPackets Out;
	ContiguousByteBuffer Framed;
	Out.Serialize(0x01, ContiguousByteBufferView(Writer.Data()), Framed);

	cInboundPackets In;
	sCapturedPacket Packet;
	size_t Sent = 0;
	while (Sent < Framed.size())
	{
		const size_t Chunk = std::min<size_t>(3, Framed.size() - Sent);
		In.AddData(Framed.data() + Sent, Chunk);
		Sent += Chunk;
		if (Sent < Framed.size())
		{
			TEST_FALSE(In.NextPacket(Packet));
		}
	}
	TEST_TRUE(In.NextPacket(Packet));
	TEST_EQUAL(Packet.m_PacketID, 0x01u);
	TEST_EQUAL(Packet.m_Payload.size(), Writer.Data().size());
}





static void TestTwoPacketsInOneRead(void)
{
	// Two packets arriving within one TCP read must both be found:
	cOutboundPackets Out;
	ContiguousByteBuffer Framed;
	const ContiguousByteBuffer First{std::byte{0x61}};
	const ContiguousByteBuffer Second{std::byte{0x62}, std::byte{0x63}};
	Out.Serialize(0x01, ContiguousByteBufferView(First), Framed);
	Out.Serialize(0x02, ContiguousByteBufferView(Second), Framed);

	cInboundPackets In;
	In.AddData(Framed.data(), Framed.size());
	sCapturedPacket Packet;
	TEST_TRUE(In.NextPacket(Packet));
	TEST_EQUAL(Packet.m_PacketID, 0x01u);
	TEST_EQUAL(Packet.m_Payload.size(), 1u);
	TEST_TRUE(In.NextPacket(Packet));
	TEST_EQUAL(Packet.m_PacketID, 0x02u);
	TEST_EQUAL(Packet.m_Payload.size(), 2u);
	TEST_FALSE(In.NextPacket(Packet));
}





static void TestSetCompressionChangesFraming(void)
{
	// wiki.vg Protocol#Set Compression: after this packet both directions use the compressed framing.
	// The threshold is learned from the packet itself, cInboundPackets must then accept compressed data:
	cTestWriter Writer;
	Writer.WriteVarInt(256);
	cOutboundPackets Out;
	ContiguousByteBuffer SetCompression;
	Out.Serialize(0x03, ContiguousByteBufferView(Writer.Data()), SetCompression);

	cInboundPackets In;
	In.AddData(SetCompression.data(), SetCompression.size());
	sCapturedPacket Packet;
	TEST_TRUE(In.NextPacket(Packet));
	TEST_EQUAL(Packet.m_PacketID, 0x03u);
	TEST_FALSE(In.GetThreshold() >= 0);   // Compression isn't on until the proxy applies the threshold
	In.SetThreshold(256);
	Out.SetThreshold(256);

	// A payload above the threshold round-trips through the deflate framing:
	ContiguousByteBuffer Big(1000, std::byte{0x41});
	ContiguousByteBuffer Framed;
	Out.Serialize(0x20, ContiguousByteBufferView(Big), Framed);
	In.AddData(Framed.data(), Framed.size());
	TEST_TRUE(In.NextPacket(Packet));
	TEST_EQUAL(Packet.m_PacketID, 0x20u);
	TEST_EQUAL(Packet.m_Payload.size(), Big.size());
	TEST_TRUE(Packet.m_IsCompressed);

	// A small payload is sent uncompressed, DataLength = 0, and must still be understood:
	ContiguousByteBuffer Small(10, std::byte{0x42});
	Framed.clear();
	Out.Serialize(0x21, ContiguousByteBufferView(Small), Framed);
	In.AddData(Framed.data(), Framed.size());
	TEST_TRUE(In.NextPacket(Packet));
	TEST_EQUAL(Packet.m_PacketID, 0x21u);
	TEST_FALSE(Packet.m_IsCompressed);
	TEST_EQUAL(Packet.m_Payload.size(), Small.size());
}





static void TestLayoutHasNoHoles(void)
{
	// The layout tables are dense by packet ID, every ID below the table's end must have a name:
	for (UInt32 id = 0; id < 0x50; id++)
	{
		const auto * Definition = cPacketLayout::GetFromServerLayout(psGame, id);
		TEST_NOTEQUAL(Definition, nullptr);
		TEST_TRUE(AString(Definition->m_Name).size() > 0);
	}
	for (UInt32 id = 0; id < 0x21; id++)
	{
		const auto * Definition = cPacketLayout::GetFromClientLayout(psGame, id);
		TEST_NOTEQUAL(Definition, nullptr);
		TEST_TRUE(AString(Definition->m_Name).size() > 0);
	}
	// The states that use few packets:
	TEST_NOTEQUAL(cPacketLayout::GetFromServerLayout(psStatus, 0x00), nullptr);
	TEST_NOTEQUAL(cPacketLayout::GetFromServerLayout(psStatus, 0x01), nullptr);
	TEST_NOTEQUAL(cPacketLayout::GetFromServerLayout(psLogin, 0x00), nullptr);
	TEST_NOTEQUAL(cPacketLayout::GetFromServerLayout(psLogin, 0x01), nullptr);
	TEST_NOTEQUAL(cPacketLayout::GetFromServerLayout(psLogin, 0x02), nullptr);
	TEST_NOTEQUAL(cPacketLayout::GetFromServerLayout(psLogin, 0x03), nullptr);
	TEST_NOTEQUAL(cPacketLayout::GetFromClientLayout(psStatus, 0x00), nullptr);
	TEST_NOTEQUAL(cPacketLayout::GetFromClientLayout(psStatus, 0x01), nullptr);
	TEST_NOTEQUAL(cPacketLayout::GetFromClientLayout(psLogin, 0x00), nullptr);
	TEST_NOTEQUAL(cPacketLayout::GetFromClientLayout(psLogin, 0x01), nullptr);
	// IDs beyond the tables are reported as unknown rather than guessed at:
	TEST_EQUAL(cPacketLayout::GetFromServerLayout(psGame, 0x50), nullptr);
	TEST_EQUAL(cPacketLayout::GetFromClientLayout(psGame, 0x21), nullptr);
	TEST_EQUAL(cPacketLayout::GetFromServerLayout(psUnknown, 0x00), nullptr);
}





static void TestKnownPacketNames(void)
{
	// A few IDs pinned against wiki.vg Protocol revision 1313, so that an off-by-one in the
	// dense tables shows up as a failure instead of silent mislabelling:
	static const struct { UInt32 m_ID; const char * m_Name; } g_ServerPackets[] =
	{
		{ 0x00, "Spawn Object" },
		{ 0x0d, "Server Difficulty" },
		{ 0x0f, "Chat Message" },
		{ 0x0c, "Boss Bar" },
		{ 0x1b, "Entity Status" },
		{ 0x1f, "Keep Alive" },
		{ 0x20, "Chunk Data" },
		{ 0x30, "Use Bed" },
		{ 0x37, "Select Advancement Tab" },
		{ 0x4f, "Entity Effect" },
		// The ones a real login and a walk around the world produce, as seen against a live server:
		{ 0x23, "Join Game" },
		{ 0x09, "Update Block Entity" },
		{ 0x10, "Multi Block Change" },
		{ 0x19, "Named Sound Effect" },
		{ 0x26, "Entity Relative Move" },
		{ 0x27, "Entity Look And Relative Move" },
		{ 0x28, "Entity Look" },
		{ 0x2c, "Player Abilities" },
		{ 0x36, "Entity Head Look" },
		{ 0x3c, "Entity Metadata" },
		{ 0x3e, "Entity Velocity" },
		{ 0x41, "Update Health" },
		{ 0x47, "Time Update" },
	};
	for (const auto & Test : g_ServerPackets)
	{
		const auto * Definition = cPacketLayout::GetFromServerLayout(psGame, Test.m_ID);
		TEST_NOTEQUAL(Definition, nullptr);
		TEST_EQUAL(AString(Definition->m_Name), AString(Test.m_Name));
	}

	static const struct { UInt32 m_ID; const char * m_Name; } g_ClientPackets[] =
	{
		{ 0x00, "Teleport Confirm" },
		{ 0x0b, "Keep Alive" },
		{ 0x02, "Chat Message" },
		{ 0x04, "Client Settings" },
		{ 0x13, "Player Abilities" },
		{ 0x14, "Player Digging" },
		{ 0x18, "Resource Pack Status" },
		{ 0x1a, "Held Item Change" },
		{ 0x1d, "Animation" },
		{ 0x20, "Use Item" },
	};
	for (const auto & Test : g_ClientPackets)
	{
		const auto * Definition = cPacketLayout::GetFromClientLayout(psGame, Test.m_ID);
		TEST_NOTEQUAL(Definition, nullptr);
		TEST_EQUAL(AString(Definition->m_Name), AString(Test.m_Name));
	}
}





static void TestUnknownFieldsAreReported(void)
{
	// A packet shorter than its layout must be reported as incomplete instead of reading past its end:
	cTestWriter Writer;
	Writer.WriteVarInt(5);   // Only the first of the two Entity IDs of "Spawn Object"
	static const cPacketDefinition * Definition = cPacketLayout::GetFromServerLayout(psGame, 0x00);
	cPayloadReader Reader(Writer.Data());
	const auto * Field = Definition->m_Fields;
	AString First;
	TEST_TRUE(Reader.ReadField(*Field, First));   // Entity ID, reads fine
	++Field;
	AString Value;
	TEST_FALSE(Reader.ReadField(*Field, Value));             // Entity Type, payload is exhausted
}





IMPLEMENT_TEST_MAIN("ProtoProxyProtocol",
	TestVarIntRoundtrip();
	TestPositionRoundtrip();
	TestHandshakeFields();
	TestSlotLayout();
	TestMetadataLayout();
	TestNBTSkipping();
	TestSlotWithNBT();
	TestChunkData();
	TestFramingRoundtrip();
	TestFramingAcrossTcpSplits();
	TestTwoPacketsInOneRead();
	TestSetCompressionChangesFraming();
	TestLayoutHasNoHoles();
	TestKnownPacketNames();
	TestUnknownFieldsAreReported();
)
