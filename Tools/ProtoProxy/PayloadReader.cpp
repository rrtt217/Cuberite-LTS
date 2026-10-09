
// PayloadReader.cpp

// Implementation of the protocol-340 (MC 1.12.2) payload decoding, see PayloadReader.h for the sources.
//
// Field types documented in PacketLayout.h are decoded here; the wire formats follow
//   wiki.vg "Data types"          https://wikivg.booky.dev/Data_types
//   wiki.vg "Slot Data"           https://wikivg.booky.dev/index.php?title=Slot_Data&oldid=6528
//   wiki.vg "Entity metadata"     https://wikivg.booky.dev/index.php?title=Entity_metadata&oldid=4109
//   wiki.vg "Chunk Format"        https://wikivg.booky.dev/index.php?title=Chunk_Format&oldid=8203
//   wiki.vg "NBT"                 https://wikivg.booky.dev/NBT





#include "Globals.h"
#include "PayloadReader.h"
#include "StringUtils.h"
#include "UUID.h"
#include <cstdio>





/** Number of entries that a compound field lists individually before summarising the rest. */
static constexpr size_t MAX_LISTED_ITEMS = 32;

/** Number of bytes per chunk-section light array, wiki.vg "Chunk Format#Chunk Section structure". */
static constexpr size_t CHUNK_SECTION_LIGHT_SIZE = 2048;

/** Number of bytes in a Chunk Data packet's biome array, wiki.vg "Chunk Format#Biomes". */
static constexpr size_t CHUNK_BIOME_SIZE = 256;





////////////////////////////////////////////////////////////////////////////////
// Local helpers:

/** Returns the bytes as a compact hexadecimal string. */
static AString ToHex(const void * a_Data, size_t a_Size)
{
	static const char g_Digits[] = "0123456789abcdef";
	const auto * Bytes = static_cast<const unsigned char *>(a_Data);
	AString res;
	res.reserve(a_Size * 2);
	for (size_t i = 0; i < a_Size; i++)
	{
		res.push_back(g_Digits[Bytes[i] >> 4]);
		res.push_back(g_Digits[Bytes[i] & 0x0f]);
	}
	return res;
}





/** Returns an angle byte as degrees, wiki.vg "Data types#Angle". */
static double AngleToDegrees(unsigned char a_Byte)
{
	return static_cast<double>(a_Byte) * 360.0 / 256.0;
}





/** Names of the entity-metadata value types, protocol 340, wiki.vg "Entity metadata". */
static const char * MetadataTypeName(unsigned char a_Type)
{
	switch (a_Type)
	{
		case 0: return "Byte";
		case 1: return "VarInt";
		case 2: return "Float";
		case 3: return "String";
		case 4: return "Chat";
		case 5: return "Slot";
		case 6: return "Boolean";
		case 7: return "Rotation";
		case 8: return "Position";
		case 9: return "OptPosition";
		case 10: return "Direction";
		case 11: return "OptUUID";
		case 12: return "OptBlockID";
		case 13: return "NBT";
	}
	return "Unknown";
}





/** Names of the NBT tag types, wiki.vg "NBT". */
static const char * NBTTypeName(unsigned char a_Type)
{
	switch (a_Type)
	{
		case 0: return "End";
		case 1: return "Byte";
		case 2: return "Short";
		case 3: return "Int";
		case 4: return "Long";
		case 5: return "Float";
		case 6: return "Double";
		case 7: return "ByteArray";
		case 8: return "String";
		case 9: return "List";
		case 10: return "Compound";
		case 11: return "IntArray";
		case 12: return "LongArray";
	}
	return "Unknown";
}





/** Skips one named NBT tag in a raw byte array, the form used inside compounds and at the root.
Returns false if the array doesn't hold a complete tag. */
static bool SkipNBTRaw(const std::byte * a_Data, size_t a_Size, size_t & a_Consumed, unsigned char & a_TagType);

static bool SkipNBTBody(const std::byte * a_Data, size_t a_Size, size_t & a_Consumed, unsigned char a_TagType)
{
	// a_Data points behind the tag's type and name, at the tag's payload:
	switch (a_TagType)
	{
		case 0: a_Consumed = 0; return true;                                  // TAG_End
		case 1: a_Consumed = 1; break;                                        // TAG_Byte
		case 2: a_Consumed = 2; break;                                        // TAG_Short
		case 3: case 5: a_Consumed = 4; break;                                 // TAG_Int, TAG_Float
		case 4: case 6: a_Consumed = 8; break;                                 // TAG_Long, TAG_Double
		case 7:       // TAG_Byte_Array
		case 11:      // TAG_Int_Array
		case 12:      // TAG_Long_Array
		{
			if (a_Size < 4)
			{
				return false;
			}
			const auto Length = static_cast<UInt32>(
				(static_cast<unsigned char>(a_Data[0]) << 24) |
				(static_cast<unsigned char>(a_Data[1]) << 16) |
				(static_cast<unsigned char>(a_Data[2]) << 8) |
				(static_cast<unsigned char>(a_Data[3]))
			);
			const size_t ItemSize = (a_TagType == 11) ? 4 : ((a_TagType == 12) ? 8 : 1);
			a_Consumed = 4 + static_cast<size_t>(Length) * ItemSize;
			break;
		}
		case 8:  // TAG_String
		{
			if (a_Size < 2)
			{
				return false;
			}
			const auto Length = static_cast<UInt16>((static_cast<unsigned char>(a_Data[0]) << 8) | static_cast<unsigned char>(a_Data[1]));
			a_Consumed = 2u + Length;
			break;
		}
		case 9:  // TAG_List
		{
			if (a_Size < 5)
			{
				return false;
			}
			const auto ElementType = static_cast<unsigned char>(a_Data[0]);
			if (ElementType == 0)
			{
				// wiki.vg "NBT": the element type is TAG_End only for an empty list:
				a_Consumed = 5;
				return true;
			}
			const auto NumElements = static_cast<UInt32>(
				(static_cast<unsigned char>(a_Data[1]) << 24) |
				(static_cast<unsigned char>(a_Data[2]) << 16) |
				(static_cast<unsigned char>(a_Data[3]) << 8) |
				(static_cast<unsigned char>(a_Data[4]))
			);
			size_t Pos = 5;
			for (UInt32 i = 0; i < NumElements; i++)
			{
				// The list's elements carry neither a type nor a name, wiki.vg "NBT":
				size_t ElementSize;
				if (!SkipNBTBody(a_Data + Pos, a_Size - Pos, ElementSize, ElementType))
				{
					return false;
				}
				Pos += ElementSize;
			}
			a_Consumed = Pos;
			return true;  // a_Consumed is final here, the caller adds the header size
		}
		case 10:  // TAG_Compound
		{
			size_t Pos = 0;
			while (true)
			{
				size_t ChildSize;
				unsigned char ChildType;
				if (!SkipNBTRaw(a_Data + Pos, a_Size - Pos, ChildSize, ChildType))
				{
					return false;
				}
				Pos += ChildSize;
				if (ChildType == 0)
				{
					a_Consumed = Pos;
					return true;
				}
			}
		}
		default: return false;
	}  // switch (a_TagType)

	return a_Size >= a_Consumed;
}





static bool SkipNBTRaw(
	const std::byte * a_Data,
	size_t a_Size,
	size_t & a_Consumed,
	unsigned char & a_TagType
)
{
	if (a_Size < 1)
	{
		return false;
	}
	a_TagType = static_cast<unsigned char>(a_Data[0]);
	size_t Pos = 1;
	if (a_TagType == 0)
	{
		// TAG_End carries neither a name nor a payload:
		a_Consumed = Pos;
		return true;
	}
	// Named tags carry a 16-bit name length followed by the name:
	if (a_Size < 3)
	{
		return false;
	}
	const auto NameLen = static_cast<UInt16>((static_cast<unsigned char>(a_Data[1]) << 8) | static_cast<unsigned char>(a_Data[2]));
	Pos += 2u + NameLen;
	if (a_Size < Pos)
	{
		return false;
	}
	size_t BodySize;
	if (!SkipNBTBody(a_Data + Pos, a_Size - Pos, BodySize, a_TagType))
	{
		return false;
	}
	a_Consumed = Pos + BodySize;
	return a_Size >= a_Consumed;
}





////////////////////////////////////////////////////////////////////////////////
// cPayloadReader - construction and primitives:

cPayloadReader::cPayloadReader(const std::byte * a_Data, size_t a_Size):
	m_Payload(a_Data, a_Size),
	m_Pos(0),
	m_NBTFileIdx(0)
{
}





cPayloadReader::cPayloadReader(const ContiguousByteBuffer & a_Payload):
	cPayloadReader(a_Payload.data(), a_Payload.size())
{
}





ContiguousByteBufferView cPayloadReader::GetRemaining(void) const
{
	return ContiguousByteBufferView(m_Payload).substr(m_Pos);
}





bool cPayloadReader::SkipBytes(size_t a_Count)
{
	if (GetRemainingBytes() < a_Count)
	{
		return false;
	}
	m_Pos += a_Count;
	return true;
}





bool cPayloadReader::ReadBytes(void * a_Dst, size_t a_Count)
{
	if (GetRemainingBytes() < a_Count)
	{
		return false;
	}
	memcpy(a_Dst, m_Payload.data() + m_Pos, a_Count);
	m_Pos += a_Count;
	return true;
}





bool cPayloadReader::PeekByte(unsigned char & a_Byte) const
{
	if (GetRemainingBytes() < 1)
	{
		return false;
	}
	a_Byte = static_cast<unsigned char>(m_Payload[m_Pos]);
	return true;
}





bool cPayloadReader::ReadBoolValue(bool & a_Value)
{
	unsigned char Byte;
	if (!ReadUByteValue(Byte))
	{
		return false;
	}
	a_Value = (Byte != 0);
	return true;
}





bool cPayloadReader::ReadByteValue(Int8 & a_Value)
{
	unsigned char Byte;
	if (!ReadUByteValue(Byte))
	{
		return false;
	}
	a_Value = static_cast<Int8>(Byte);
	return true;
}





bool cPayloadReader::ReadUByteValue(unsigned char & a_Byte)
{
	if (GetRemainingBytes() < 1)
	{
		return false;
	}
	a_Byte = static_cast<unsigned char>(m_Payload[m_Pos]);
	m_Pos += 1;
	return true;
}





bool cPayloadReader::ReadShortValue(Int16 & a_Value)
{
	UInt16 Value;
	if (!ReadUShortValue(Value))
	{
		return false;
	}
	a_Value = static_cast<Int16>(Value);
	return true;
}





bool cPayloadReader::ReadUShortValue(UInt16 & a_Value)
{
	if (GetRemainingBytes() < 2)
	{
		return false;
	}
	const auto * Bytes = reinterpret_cast<const unsigned char *>(m_Payload.data() + m_Pos);
	a_Value = static_cast<UInt16>((Bytes[0] << 8) | Bytes[1]);
	m_Pos += 2;
	return true;
}





bool cPayloadReader::ReadIntValue(Int32 & a_Value)
{
	if (GetRemainingBytes() < 4)
	{
		return false;
	}
	const auto * Bytes = reinterpret_cast<const unsigned char *>(m_Payload.data() + m_Pos);
	a_Value = static_cast<Int32>(
		(static_cast<UInt32>(Bytes[0]) << 24) |
		(static_cast<UInt32>(Bytes[1]) << 16) |
		(static_cast<UInt32>(Bytes[2]) << 8) |
		(static_cast<UInt32>(Bytes[3]))
	);
	m_Pos += 4;
	return true;
}





bool cPayloadReader::ReadLongValue(Int64 & a_Value)
{
	if (GetRemainingBytes() < 8)
	{
		return false;
	}
	const auto * Bytes = reinterpret_cast<const unsigned char *>(m_Payload.data() + m_Pos);
	auto Value = static_cast<UInt64>(0);
	for (int i = 0; i < 8; i++)
	{
		Value = (Value << 8) | static_cast<UInt64>(Bytes[i]);
	}
	a_Value = static_cast<Int64>(Value);
	m_Pos += 8;
	return true;
}





bool cPayloadReader::ReadFloatValue(float & a_Value)
{
	Int32 Bits;
	if (!ReadIntValue(Bits))
	{
		return false;
	}
	memcpy(&a_Value, &Bits, sizeof(a_Value));
	return true;
}





bool cPayloadReader::ReadDoubleValue(double & a_Value)
{
	Int64 Bits;
	if (!ReadLongValue(Bits))
	{
		return false;
	}
	memcpy(&a_Value, &Bits, sizeof(a_Value));
	return true;
}





bool cPayloadReader::ReadVarIntValue(Int64 & a_Value)
{
	// wiki.vg "Data types#VarInt and VarLong": up to 5 bytes holding a 32-bit signed value:
	UInt32 Result = 0;
	int Shift = 0;
	int NumBytes = 0;
	while (true)
	{
		unsigned char Byte;
		if (!ReadUByteValue(Byte))
		{
			return false;
		}
		Result |= static_cast<UInt32>(Byte & 0x7f) << Shift;
		Shift += 7;
		NumBytes += 1;
		if ((Byte & 0x80) == 0)
		{
			break;
		}
		if (NumBytes >= 5)
		{
			return false;
		}
	}
	a_Value = static_cast<Int32>(Result);
	return true;
}





bool cPayloadReader::ReadVarLongValue(Int64 & a_Value)
{
	// wiki.vg "Data types#VarInt and VarLong": up to 10 bytes, little-endian 7-bit groups:
	auto Result = static_cast<UInt64>(0);
	int Shift = 0;
	int NumBytes = 0;
	while (true)
	{
		unsigned char Byte;
		if (!ReadUByteValue(Byte))
		{
			return false;
		}
		Result |= static_cast<UInt64>(Byte & 0x7f) << Shift;
		Shift += 7;
		NumBytes += 1;
		if ((Byte & 0x80) == 0)
		{
			break;
		}
		if (NumBytes >= 10)
		{
			return false;
		}
	}
	a_Value = static_cast<Int64>(Result);
	return true;
}





bool cPayloadReader::ReadStringValue(AString & a_Value)
{
	Int64 Length;
	if (!ReadVarLongValue(Length) || (Length < 0) || (static_cast<size_t>(Length) > GetRemainingBytes()))
	{
		return false;
	}
	a_Value.assign(reinterpret_cast<const char *>(m_Payload.data() + m_Pos), static_cast<size_t>(Length));
	m_Pos += static_cast<size_t>(Length);
	return true;
}





bool cPayloadReader::ReadUUIDValue(AString & a_Value)
{
	unsigned char Bytes[16];
	if (!ReadBytes(Bytes, sizeof(Bytes)))
	{
		return false;
	}
	// Render the dashed form directly, wiki.vg "Data types#UUID":
	a_Value = fmt::format(
		FMT_STRING("{}-{}-{}-{}-{}"),
		ToHex(Bytes, 4),
		ToHex(Bytes + 4, 2),
		ToHex(Bytes + 6, 2),
		ToHex(Bytes + 8, 2),
		ToHex(Bytes + 10, 6)
	);
	return true;
}





bool cPayloadReader::ReadBlockPosition(Vector3i & a_Position)
{
	// wiki.vg "Data types#Position": 26 bits X, 26 bits Z, 12 bits Y, all big-endian within the 64-bit value:
	Int64 Raw;
	if (!ReadLongValue(Raw))
	{
		return false;
	}
	const auto Value = static_cast<UInt64>(Raw);
	auto X = static_cast<Int32>(Value >> 38);
	auto Z = static_cast<Int32>(Value & 0x03ffffff);
	const auto Y = static_cast<Int32>((Value >> 26) & 0x0fff);
	if (X >= (1 << 25))
	{
		X -= (1 << 26);
	}
	if (Z >= (1 << 25))
	{
		Z -= (1 << 26);
	}
	a_Position.x = X;
	a_Position.y = Y;
	a_Position.z = Z;
	return true;
}





void cPayloadReader::RememberValue(const char * a_Name, Int64 a_Value)
{
	if (a_Name != nullptr)
	{
		m_LastValues[a_Name] = a_Value;
	}
}





Int64 cPayloadReader::GetFieldValue(const char * a_Name, Int64 a_Default) const
{
	auto itr = m_LastValues.find(a_Name);
	return (itr == m_LastValues.end()) ? a_Default : itr->second;
}





AString cPayloadReader::DescribeNBTFile(const std::byte * a_Data, size_t a_Size)
{
	if (m_NBTFilePrefix.empty())
	{
		return AString();
	}
	const auto fnam = fmt::format(FMT_STRING("{}_nbt_{}.bin"), m_NBTFilePrefix, m_NBTFileIdx++);
	FILE * f = fopen(fnam.c_str(), "wb");
	if (f == nullptr)
	{
		return fmt::format(FMT_STRING("; could not save to \"{}\""), fnam);
	}
	fwrite(a_Data, 1, a_Size, f);
	fclose(f);
	return fmt::format(FMT_STRING("; saved to \"{}\""), fnam);
}





////////////////////////////////////////////////////////////////////////////////
// cPayloadReader - protocol-340 data structures:

bool cPayloadReader::ReadSlot(AString & a_Value)
{
	// wiki.vg "Slot Data", the format used by protocol 340: Short Item ID, -1 meaning "empty":
	Int16 ItemID;
	if (!ReadShortValue(ItemID))
	{
		return false;
	}
	if (ItemID < 0)
	{
		a_Value = "<empty>";
		return true;
	}
	Int8 Count;
	Int16 Damage;
	if (!ReadByteValue(Count) || !ReadShortValue(Damage))
	{
		return false;
	}
	a_Value = fmt::format(FMT_STRING("item {} x {}, damage {}"), ItemID, static_cast<int>(Count), Damage);
	unsigned char FirstTag;
	if (!PeekByte(FirstTag))
	{
		return false;
	}
	if (FirstTag == 0)
	{
		// TAG_End = no NBT data:
		return SkipBytes(1);
	}
	AString NBTDesc;
	if (!ReadNBT(NBTDesc, false))
	{
		return false;
	}
	a_Value.append(fmt::format(FMT_STRING(", {}"), NBTDesc));
	return true;
}





bool cPayloadReader::ReadNBT(AString & a_Value, bool a_ShouldDumpHex)
{
	const auto Remaining = GetRemaining();
	size_t Consumed = 0;
	unsigned char TagType = 0;
	if (!SkipNBTRaw(Remaining.data(), Remaining.size(), Consumed, TagType))
	{
		return false;
	}
	a_Value = fmt::format(
		FMT_STRING("{} NBT, {} bytes{}"),
		NBTTypeName(TagType),
		static_cast<unsigned>(Consumed),
		DescribeNBTFile(Remaining.data(), Consumed)
	);
	if (a_ShouldDumpHex)
	{
		AString Hex;
		CreateHexDump(Hex, Remaining.data(), Consumed, 16);
		a_Value.append("\n" + Hex);
	}
	return SkipBytes(Consumed);
}





bool cPayloadReader::ReadMetadata(AString & a_Value)
{
	AString Lines;
	size_t NumEntries = 0;
	while (true)
	{
		unsigned char Index;
		if (!ReadUByteValue(Index))
		{
			return false;
		}
		if (Index == 0xff)
		{
			// The metadata array's terminator:
			break;
		}
		unsigned char Type;
		if (!ReadUByteValue(Type))
		{
			return false;
		}
		AString Text;
		switch (Type)
		{
			case 0:
			{
				Int8 Value;
				if (!ReadByteValue(Value))
				{
					return false;
				}
				Text = fmt::format(FMT_STRING("{}"), static_cast<int>(Value));
				break;
			}
			case 1:
			case 10:
			case 12:
			{
				Int64 Value;
				if (!ReadVarLongValue(Value))
				{
					return false;
				}
				if (Type == 12)
				{
					Text = fmt::format(FMT_STRING("{} (block {} meta {})"), Value, Value >> 4, Value & 15);
				}
				else
				{
					Text = fmt::format(FMT_STRING("{}"), Value);
				}
				break;
			}
			case 2:
			{
				float Value;
				if (!ReadFloatValue(Value))
				{
					return false;
				}
				Text = fmt::format(FMT_STRING("{}"), Value);
				break;
			}
			case 3:
			case 4:
			{
				if (!ReadStringValue(Text))
				{
					return false;
				}
				Text = "\"" + Text + "\"";
				break;
			}
			case 5:
			{
				if (!ReadSlot(Text))
				{
					return false;
				}
				break;
			}
			case 6:
			{
				bool Value;
				if (!ReadBoolValue(Value))
				{
					return false;
				}
				Text = Value ? "true" : "false";
				break;
			}
			case 7:
			{
				float Rot[3];
				for (float & R : Rot)
				{
					if (!ReadFloatValue(R))
					{
						return false;
					}
				}
				Text = fmt::format(FMT_STRING("<{}, {}, {}>"), Rot[0], Rot[1], Rot[2]);
				break;
			}
			case 8:
			case 9:
			{
				bool IsPresent = true;
				if (Type == 9)
				{
					if (!ReadBoolValue(IsPresent))
					{
						return false;
					}
				}
				if (!IsPresent)
				{
					Text = "<none>";
					break;
				}
				Vector3i Pos;
				if (!ReadBlockPosition(Pos))
				{
					return false;
				}
				Text = fmt::format(FMT_STRING("<{}, {}, {}>"), Pos.x, Pos.y, Pos.z);
				break;
			}
			case 11:
			{
				bool IsPresent;
				if (!ReadBoolValue(IsPresent))
				{
					return false;
				}
				if (!IsPresent)
				{
					Text = "<none>";
					break;
				}
				if (!ReadUUIDValue(Text))
				{
					return false;
				}
				break;
			}
			case 13:
			{
				if (!ReadNBT(Text, false))
				{
					return false;
				}
				break;
			}
			default:
			{
				// Unknown metadata type, cannot continue reading the array:
				return false;
			}
		}  // switch (Type)

		NumEntries += 1;
		if (NumEntries <= MAX_LISTED_ITEMS)
		{
			if (!Lines.empty())
			{
				Lines.push_back('\n');
			}
			Lines.append(fmt::format(
				FMT_STRING("  [{}] {} = {}"),
				static_cast<unsigned>(Index),
				MetadataTypeName(Type),
				Text
			));
		}
	}  // while (true)

	a_Value = fmt::format(FMT_STRING("{} entries"), static_cast<unsigned>(NumEntries));
	if (!Lines.empty())
	{
		a_Value.push_back('\n');
		a_Value += Lines;
	}
	return true;
}






/** Walks the chunk sections inside a chunk data array, assuming the given sky-light presence.
a_Sections receives the sections found.
Returns false if the array ends before all sections in a_Bitmask are read, or if a section header
carries an implausible number of bits per block - which means the walk is out of sync. */
static bool ParseChunkSections(
	const std::byte * a_Data,
	size_t a_Size,
	UInt32 a_Bitmask,
	bool a_HasSkyLight,
	size_t a_BiomeBytes,
	std::vector<cPayloadReader::sChunkSection> & a_Sections
)
{
	// wiki.vg "Chunk Format", "Chunk Section structure", the form used by protocol 340:
	cPayloadReader Sections(a_Data, a_Size);
	auto RemainingMask = a_Bitmask;
	while (RemainingMask != 0)
	{
		UInt32 SectionIndex = 0;
		for (UInt32 Bit = 0; Bit < 32; Bit++)
		{
			if ((RemainingMask & (1u << Bit)) != 0)
			{
				SectionIndex = Bit;
				RemainingMask &= ~(1u << Bit);
				break;
			}
		}
		unsigned char Bits = 0;
		Int64 PaletteLength = 0;
		Int64 NumLongs = 0;
		if (
			!Sections.ReadUByteValue(Bits) ||
			!Sections.ReadVarIntValue(PaletteLength) ||
			!Sections.ReadVarIntValue(NumLongs) ||
			(PaletteLength < 0) ||
			(NumLongs < 0)
		)
		{
			return false;
		}
		// A section's palette is either the global one (13 bits per block) or a per-section one of 4 - 8 bits:
		if ((Bits < 4) || (Bits > 16))
		{
			return false;
		}
		const size_t Body =
			static_cast<size_t>(NumLongs) * 8 +
			CHUNK_SECTION_LIGHT_SIZE +
			(a_HasSkyLight ? CHUNK_SECTION_LIGHT_SIZE : 0);
		if (Sections.GetRemainingBytes() < Body)
		{
			return false;
		}
		Sections.SkipBytes(Body);
		cPayloadReader::sChunkSection Section;
		Section.m_Index = static_cast<int>(SectionIndex);
		Section.m_BitsPerBlock = Bits;
		Section.m_PaletteLength = static_cast<UInt32>(PaletteLength);
		Section.m_NumDataLongs = static_cast<UInt32>(NumLongs);
		Section.m_HasSkyLight = a_HasSkyLight;
		a_Sections.push_back(Section);
	}
	// The biome array follows the sections, only when the data came ground-up continuous:
	if (Sections.GetRemainingBytes() != a_BiomeBytes)
	{
		return false;
	}
	return true;
}





bool cPayloadReader::ReadChunkData(AString & a_Value)
{
	// wiki.vg "Chunk Format", the "Chunk Data Structure" of protocol 340:
	sChunkData Data;
	Data.m_Bitmask = 0;
	Data.m_DataSize = 0;
	Data.m_NumBlockEntities = 0;
	Data.m_HasBiomeData = false;
	Data.m_HasSkyLight = false;
	Data.m_SizeMatches = false;
	if (!ReadBoolValue(Data.m_IsGroundUpContinuous))
	{
		return false;
	}
	Data.m_HasBiomeData = Data.m_IsGroundUpContinuous;
	Int64 Bitmask = 0;
	Int64 Size = 0;
	if (!ReadVarIntValue(Bitmask) || !ReadVarIntValue(Size))
	{
		return false;
	}
	Data.m_Bitmask = static_cast<UInt32>(Bitmask);
	if ((Size < 0) || (static_cast<size_t>(Size) > GetRemainingBytes()))
	{
		return false;
	}
	Data.m_DataSize = static_cast<UInt32>(Size);
	const size_t BiomeBytes = Data.m_HasBiomeData ? CHUNK_BIOME_SIZE : 0;
	const auto * DataArray = m_Payload.data() + m_Pos;

	// Sky light is sent only for overworld chunks, and the packet doesn't say which dimension it came
	// from, so walk the sections twice - once as if sky light was sent, once as if it wasn't - and keep
	// whichever walk accounts for the data array exactly:
	std::vector<sChunkSection> SkySections, NoSkySections;
	const bool SkyWalks = ParseChunkSections(DataArray, static_cast<size_t>(Size), Data.m_Bitmask, true, BiomeBytes, SkySections);
	const bool NoSkyWalks = ParseChunkSections(DataArray, static_cast<size_t>(Size), Data.m_Bitmask, false, BiomeBytes, NoSkySections);
	if (SkyWalks)
	{
		Data.m_HasSkyLight = true;
		Data.m_SizeMatches = true;
		Data.m_Sections = std::move(SkySections);
	}
	else if (NoSkyWalks)
	{
		Data.m_HasSkyLight = false;
		Data.m_SizeMatches = true;
		Data.m_Sections = std::move(NoSkySections);
	}
	else if (SkySections.size() >= NoSkySections.size())
	{
		// Neither assumption fits, report the sections we did manage to read:
		Data.m_HasSkyLight = true;
		Data.m_Sections = std::move(SkySections);
	}
	else
	{
		Data.m_HasSkyLight = false;
		Data.m_Sections = std::move(NoSkySections);
	}

	if (!SkipBytes(static_cast<size_t>(Size)))
	{
		return false;
	}
	if (!ReadVarIntValue(Bitmask))   // Re-use the local for the block-entity count:
	{
		return false;
	}
	Data.m_NumBlockEntities = static_cast<UInt32>(Bitmask);

	a_Value = fmt::format(
		FMT_STRING("ground-up {}, bitmask 0x{:02x}, {} sections, {} bytes of data"),
		Data.m_IsGroundUpContinuous ? "continuous" : "columns",
		Data.m_Bitmask,
		static_cast<unsigned>(Data.m_Sections.size()),
		Data.m_DataSize
	);
	for (const auto & Section : Data.m_Sections)
	{
		a_Value.append(fmt::format(
			FMT_STRING("\n  section {}: {} bits/block, {} palette entries, {} longs, block light{}"),
			Section.m_Index,
			Section.m_BitsPerBlock,
			Section.m_PaletteLength,
			Section.m_NumDataLongs,
			(Data.m_HasSkyLight ? ", sky light" : "")
		));
	}
	if (Data.m_HasBiomeData)
	{
		a_Value.append(fmt::format(FMT_STRING("\n  {} bytes of biome data"), CHUNK_BIOME_SIZE));
	}
	a_Value.append(fmt::format(
		FMT_STRING("\n  {} block entities, size check: {}"),
		Data.m_NumBlockEntities,
		Data.m_SizeMatches ? "matches" : "does not match the data size"
	));
	return true;
}





////////////////////////////////////////////////////////////////////////////////
// cPayloadReader - compound fields:

bool cPayloadReader::ReadParticleData(AString & a_Value)
{
	// wiki.vg Protocol#Particle: the trailing array's length depends on the particle:
	const auto ParticleID = GetFieldValue("Particle ID", -1);
	int NumValues = 0;
	switch (ParticleID)
	{
		case 36: NumValues = 2; break;         // iconcrack
		case 37: case 38: case 46: NumValues = 1; break;  // blockcrack, blockdust, fallingdust
	}
	if (NumValues == 0)
	{
		a_Value = "<none>";
		return true;
	}
	AString Values;
	for (int i = 0; i < NumValues; i++)
	{
		Int64 Value;
		if (!ReadVarIntValue(Value))
		{
			return false;
		}
		if (!Values.empty())
		{
			Values += ", ";
		}
		Values.append(fmt::format(FMT_STRING("{}"), Value));
	}
	a_Value = Values;
	return true;
}





bool cPayloadReader::ReadStatistics(AString & a_Value)
{
	Int64 Count;
	if (!ReadVarIntValue(Count) || (Count < 0))
	{
		return false;
	}
	a_Value = fmt::format(FMT_STRING("{} entries"), Count);
	for (Int64 i = 0; i < Count; i++)
	{
		AString Name;
		Int64 Value;
		if (!ReadStringValue(Name) || !ReadVarIntValue(Value))
		{
			return false;
		}
		if (i < static_cast<Int64>(MAX_LISTED_ITEMS))
		{
			a_Value.append(fmt::format(FMT_STRING("\n  {} = {}"), Name, Value));
		}
	}
	return true;
}





bool cPayloadReader::ReadExplosionRecords(AString & a_Value)
{
	Int32 Count;
	if (!ReadIntValue(Count) || (Count < 0))
	{
		return false;
	}
	a_Value = fmt::format(FMT_STRING("{} records"), Count);
	for (Int32 i = 0; i < Count; i++)
	{
		Int8 Dx, Dy, Dz;
		if (!ReadByteValue(Dx) || !ReadByteValue(Dy) || !ReadByteValue(Dz))
		{
			return false;
		}
		if (i < static_cast<Int32>(MAX_LISTED_ITEMS))
		{
			a_Value.append(fmt::format(
				FMT_STRING("\n  <{}, {}, {}>"),
				static_cast<int>(Dx), static_cast<int>(Dy), static_cast<int>(Dz)
			));
		}
	}
	return true;
}





bool cPayloadReader::ReadMultiBlockRecords(AString & a_Value)
{
	// wiki.vg Protocol#Multi Block Change, the record layout of protocol 340:
	Int64 Count;
	if (!ReadVarIntValue(Count) || (Count < 0))
	{
		return false;
	}
	const auto ChunkX = GetFieldValue("Chunk X", 0);
	const auto ChunkZ = GetFieldValue("Chunk Z", 0);
	a_Value = fmt::format(FMT_STRING("{} records"), Count);
	for (Int64 i = 0; i < Count; i++)
	{
		unsigned char Horiz, Y;
		Int64 BlockID;
		if (!ReadUByteValue(Horiz) || !ReadUByteValue(Y) || !ReadVarIntValue(BlockID))
		{
			return false;
		}
		if (i < static_cast<Int64>(MAX_LISTED_ITEMS))
		{
			a_Value.append(fmt::format(
				FMT_STRING("\n  <{}, {}, {}> block {} (type {}, meta {})"),
				ChunkX * 16 + ((Horiz >> 4) & 15), Y, ChunkZ * 16 + (Horiz & 15),
				BlockID, BlockID >> 4, BlockID & 15
			));
		}
	}
	return true;
}





bool cPayloadReader::ReadEntityProperties(AString & a_Value)
{
	Int32 NumProperties;
	if (!ReadIntValue(NumProperties) || (NumProperties < 0))
	{
		return false;
	}
	a_Value = fmt::format(FMT_STRING("{} properties"), NumProperties);
	for (Int32 i = 0; i < NumProperties; i++)
	{
		AString Key, Uuid;
		double Value;
		Int64 NumModifiers;
		if (!ReadStringValue(Key) || !ReadDoubleValue(Value) || !ReadVarIntValue(NumModifiers) || (NumModifiers < 0))
		{
			return false;
		}
		a_Value.append(fmt::format(FMT_STRING("\n  {} = {}"), Key, Value));
		for (Int64 j = 0; j < NumModifiers; j++)
		{
			// wiki.vg Protocol#Entity Properties, "Modifier Data structure":
			if (!ReadUUIDValue(Uuid) || !ReadDoubleValue(Value))
			{
				return false;
			}
			Int8 Operation;
			if (!ReadByteValue(Operation))
			{
				return false;
			}
			a_Value.append(fmt::format(FMT_STRING("\n    modifier {} amount {} operation {}"), Uuid, Value, static_cast<int>(Operation)));
		}
	}
	return true;
}





bool cPayloadReader::ReadMap(AString & a_Value)
{
	// wiki.vg Protocol#Map:
	Int64 ItemDamage;
	Int8 Scale;
	bool TrackingPosition;
	Int64 IconCount;
	if (
		!ReadVarIntValue(ItemDamage) ||
		!ReadByteValue(Scale) ||
		!ReadBoolValue(TrackingPosition) ||
		!ReadVarIntValue(IconCount) ||
		(IconCount < 0)
	)
	{
		return false;
	}
	a_Value = fmt::format(
		FMT_STRING("map {}, scale {}, tracking {}, {} icons"),
		ItemDamage, static_cast<int>(Scale), TrackingPosition ? "yes" : "no", IconCount
	);
	for (Int64 i = 0; i < IconCount; i++)
	{
		Int8 TypeAndDirection, X, Z;
		if (!ReadByteValue(TypeAndDirection) || !ReadByteValue(X) || !ReadByteValue(Z))
		{
			return false;
		}
		if (i < static_cast<Int64>(MAX_LISTED_ITEMS))
		{
			a_Value.append(fmt::format(
				FMT_STRING("\n  icon type {}, direction {}, at ({}, {})"),
				(TypeAndDirection >> 4) & 15, TypeAndDirection & 15, static_cast<int>(X), static_cast<int>(Z)
			));
		}
	}
	Int8 Columns;
	if (!ReadByteValue(Columns))
	{
		return false;
	}
	if (Columns <= 0)
	{
		a_Value += ", no columns";
		return true;
	}
	Int8 Rows, X, Z;
	Int64 Length;
	if (!ReadByteValue(Rows) || !ReadByteValue(X) || !ReadByteValue(Z) || !ReadVarIntValue(Length) || (Length < 0))
	{
		return false;
	}
	a_Value.append(fmt::format(
		FMT_STRING("\n  {} columns x {} rows at ({}, {}), {} bytes of pixels"),
		static_cast<int>(Columns), static_cast<int>(Rows), static_cast<int>(X), static_cast<int>(Z), Length
	));
	return SkipBytes(static_cast<size_t>(Length));
}









bool cPayloadReader::ReadTeamInfo(AString & a_Value, bool a_IsCreate)
{
	// wiki.vg Protocol#Teams, the layout shared by mode 0 (create) and mode 2 (update info):
	AString DisplayName, Prefix, Suffix, NameTagVisibility, CollisionRule;
	Int8 FriendlyFlags, Color;
	if (
		!ReadStringValue(DisplayName) ||
		!ReadStringValue(Prefix) ||
		!ReadStringValue(Suffix) ||
		!ReadByteValue(FriendlyFlags) ||
		!ReadStringValue(NameTagVisibility) ||
		!ReadStringValue(CollisionRule) ||
		!ReadByteValue(Color)
	)
	{
		return false;
	}
	a_Value = fmt::format(
		FMT_STRING("\n  display name \"{}\", prefix \"{}\", suffix \"{}\""
		"\n  friendly flags 0x{:02x}, name tag visibility \"{}\", collision rule \"{}\", color {}"),
		DisplayName, Prefix, Suffix,
		static_cast<unsigned>(FriendlyFlags) & 0xff, NameTagVisibility, CollisionRule,
		static_cast<int>(Color)
	);
	if (!a_IsCreate)
	{
		return true;
	}
	Int64 NumEntities;
	if (!ReadVarIntValue(NumEntities) || (NumEntities < 0))
	{
		return false;
	}
	a_Value.append(fmt::format(FMT_STRING("\n  {} entities"), NumEntities));
	for (Int64 i = 0; i < NumEntities; i++)
	{
		AString Name;
		if (!ReadStringValue(Name))
		{
			return false;
		}
		if (i < static_cast<Int64>(MAX_LISTED_ITEMS))
		{
			a_Value.append(fmt::format(FMT_STRING("\n    {}"), Name));
		}
	}
	return true;
}





bool cPayloadReader::ReadTeams(AString & a_Value)
{
	// wiki.vg Protocol#Teams:
	AString TeamName;
	Int8 Mode;
	if (!ReadStringValue(TeamName) || !ReadByteValue(Mode))
	{
		return false;
	}
	a_Value = fmt::format(FMT_STRING("team \"{}\", mode {}"), TeamName, static_cast<int>(Mode));
	switch (Mode)
	{
		case 0: return ReadTeamInfo(a_Value, true);
		case 1: return true;  // remove team, no further fields
		case 2: return ReadTeamInfo(a_Value, false);
		case 3: case 4:
		{
			Int64 NumEntities;
			if (!ReadVarIntValue(NumEntities) || (NumEntities < 0))
			{
				return false;
			}
			a_Value.append(fmt::format(FMT_STRING("\n  {} entities"), NumEntities));
			for (Int64 i = 0; i < NumEntities; i++)
			{
				AString Name;
				if (!ReadStringValue(Name))
				{
					return false;
				}
				if (i < static_cast<Int64>(MAX_LISTED_ITEMS))
				{
					a_Value.append(fmt::format(FMT_STRING("\n    {}"), Name));
				}
			}
			return true;
		}
	}
	return false;
}





bool cPayloadReader::ReadBossBar(AString & a_Value)
{
	// wiki.vg Protocol#Boss Bar:
	AString Uuid, Title;
	Int64 Action, Color, Dividers;
	float Health;
	unsigned char Flags;
	if (!ReadUUIDValue(Uuid) || !ReadVarIntValue(Action))
	{
		return false;
	}
	a_Value = fmt::format(FMT_STRING("bar {}, action {}"), Uuid, Action);
	switch (Action)
	{
		case 0:  // add
		{
			if (
				!ReadStringValue(Title) ||
				!ReadFloatValue(Health) ||
				!ReadVarIntValue(Color) ||
				!ReadVarIntValue(Dividers) ||
				!ReadUByteValue(Flags)
			)
			{
				return false;
			}
			a_Value.append(fmt::format(
				FMT_STRING("\n  title \"{}\", health {}, color {}, division {}, flags 0x{:02x}"),
				Title, Health, Color, Dividers, Flags
			));
			return true;
		}
		case 1: return true;  // remove
		case 2:
		{
			if (!ReadFloatValue(Health))
			{
				return false;
			}
			a_Value.append(fmt::format(FMT_STRING(", health {}"), Health));
			return true;
		}
		case 3:
		{
			if (!ReadStringValue(Title))
			{
				return false;
			}
			a_Value.append(fmt::format(FMT_STRING("\n  title \"{}\""), Title));
			return true;
		}
		case 4:
		{
			if (!ReadVarIntValue(Color) || !ReadVarIntValue(Dividers))
			{
				return false;
			}
			a_Value.append(fmt::format(FMT_STRING(", color {}, division {}"), Color, Dividers));
			return true;
		}
		case 5:
		{
			if (!ReadUByteValue(Flags))
			{
				return false;
			}
			a_Value.append(fmt::format(FMT_STRING(", flags 0x{:02x}"), Flags));
			return true;
		}
	}
	return false;
}





bool cPayloadReader::ReadTitle(AString & a_Value)
{
	// wiki.vg Protocol#Title:
	Int64 Action;
	if (!ReadVarIntValue(Action))
	{
		return false;
	}
	static const char * const g_ActionNames[] = { "set title", "set subtitle", "set action bar", "set times", "hide", "reset" };
	a_Value = (Action >= 0) && (Action < 6) ? g_ActionNames[Action] : fmt::format(FMT_STRING("action {}"), Action);
	switch (Action)
	{
		case 0: case 1: case 2:
		{
			AString Text;
			if (!ReadStringValue(Text))
			{
				return false;
			}
			a_Value.append(fmt::format(FMT_STRING(": {}"), Text));
			return true;
		}
		case 3:
		{
			Int32 FadeIn, Stay, FadeOut;
			if (!ReadIntValue(FadeIn) || !ReadIntValue(Stay) || !ReadIntValue(FadeOut))
			{
				return false;
			}
			a_Value.append(fmt::format(FMT_STRING(", fade in {}, stay {}, fade out {}"), FadeIn, Stay, FadeOut));
			return true;
		}
	}
	return true;
}





bool cPayloadReader::ReadWorldBorder(AString & a_Value)
{
	// wiki.vg Protocol#World Border:
	Int64 Action;
	if (!ReadVarIntValue(Action))
	{
		return false;
	}
	static const char * const g_ActionNames[] =
	{
		"set size", "lerp size", "set center", "initialize", "set warning time", "set warning blocks"
	};
	a_Value = (Action >= 0) && (Action < 6) ? g_ActionNames[Action] : fmt::format(FMT_STRING("action {}"), Action);
	double X = 0, Z = 0, OldDiameter = 0, NewDiameter = 0;
	Int64 Speed = 0, WarningTime = 0, WarningBlocks = 0, TeleportBoundary = 0;
	switch (Action)
	{
		case 0: if (!ReadDoubleValue(NewDiameter)) { return false; } a_Value.append(fmt::format(FMT_STRING(", diameter {}"), NewDiameter)); return true;
		case 1:
		{
			if (!ReadDoubleValue(OldDiameter) || !ReadDoubleValue(NewDiameter) || !ReadVarLongValue(Speed))
			{
				return false;
			}
			a_Value.append(fmt::format(FMT_STRING(", {} -> {} over {} ms"), OldDiameter, NewDiameter, Speed));
			return true;
		}
		case 2: case 3:
		{
			if (!ReadDoubleValue(X) || !ReadDoubleValue(Z))
			{
				return false;
			}
			a_Value.append(fmt::format(FMT_STRING(", center ({}, {})"), X, Z));
			if (Action == 2)
			{
				return true;
			}
			if (
				!ReadDoubleValue(OldDiameter) ||
				!ReadDoubleValue(NewDiameter) ||
				!ReadVarLongValue(Speed) ||
				!ReadVarIntValue(TeleportBoundary) ||
				!ReadVarIntValue(WarningTime) ||
				!ReadVarIntValue(WarningBlocks)
			)
			{
				return false;
			}
			a_Value.append(fmt::format(
				FMT_STRING("\n  diameter {} -> {} over {} ms, teleport boundary {}, warning {} s / {} m"),
				OldDiameter, NewDiameter, Speed, TeleportBoundary, WarningTime, WarningBlocks
			));
			return true;
		}
		case 4: case 5:
		{
			Int64 & Value = (Action == 4) ? WarningTime : WarningBlocks;
			if (!ReadVarIntValue(Value))
			{
				return false;
			}
			a_Value.append(fmt::format(FMT_STRING(", {}"), Value));
			return true;
		}
	}
	return false;
}





bool cPayloadReader::ReadCombatEvent(AString & a_Value)
{
	// wiki.vg Protocol#Combat Event:
	Int64 Event;
	if (!ReadVarIntValue(Event))
	{
		return false;
	}
	switch (Event)
	{
		case 0: a_Value = "enter combat"; return true;
		case 1:
		{
			Int64 Duration;
			Int32 EntityID;
			if (!ReadVarIntValue(Duration) || !ReadIntValue(EntityID))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("end combat, {} ticks, entity {}"), Duration, EntityID);
			return true;
		}
		case 2:
		{
			Int64 PlayerID;
			Int32 EntityID;
			AString Message;
			if (!ReadVarIntValue(PlayerID) || !ReadIntValue(EntityID) || !ReadStringValue(Message))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("entity dead, player {}, entity {}, message {}"), PlayerID, EntityID, Message);
			return true;
		}
	}
	return false;
}





bool cPayloadReader::ReadPlayerListItem(AString & a_Value)
{
	// wiki.vg Protocol#Player List Item:
	static const char * const g_ActionNames[] = { "add player", "initialize chat", "update game mode", "update latency", "remove player" };
	Int64 Action, NumPlayers;
	if (!ReadVarIntValue(Action) || !ReadVarIntValue(NumPlayers) || (NumPlayers < 0))
	{
		return false;
	}
	a_Value = fmt::format(
		FMT_STRING("{}, {} players"),
		(Action >= 0) && (Action < 5) ? g_ActionNames[Action] : fmt::format(FMT_STRING("action {}"), Action),
		NumPlayers
	);
	for (Int64 i = 0; i < NumPlayers; i++)
	{
		AString Uuid, Name;
		if (!ReadUUIDValue(Uuid))
		{
			return false;
		}
		a_Value.append(fmt::format(FMT_STRING("\n  {}"), Uuid));
		if (Action == 0)
		{
			Int64 NumProperties;
			if (!ReadStringValue(Name) || !ReadVarIntValue(NumProperties) || (NumProperties < 0))
			{
				return false;
			}
			Int64 Gamemode, Ping;
			bool HasDisplayName;
			AString DisplayName;
			if (
				!ReadVarIntValue(Gamemode) ||
				!ReadVarIntValue(Ping) ||
				!ReadBoolValue(HasDisplayName)
			)
			{
				return false;
			}
			if (HasDisplayName && !ReadStringValue(DisplayName))
			{
				return false;
			}
			a_Value.append(fmt::format(
				FMT_STRING("\n    name \"{}\", gamemode {}, ping {} ms, display name {}"),
				Name, Gamemode, Ping, HasDisplayName ? DisplayName : AString("<none>")
			));
			for (Int64 j = 0; j < NumProperties; j++)
			{
				AString Key, Value, Signature;
				if (!ReadStringValue(Key) || !ReadStringValue(Value) || !ReadBoolValue(HasDisplayName))
				{
					return false;
				}
				if (HasDisplayName && !ReadStringValue(Signature))
				{
					return false;
				}
				a_Value.append(fmt::format(
					FMT_STRING("\n      property {} = {}{}"),
					Key, Value, HasDisplayName ? " (signed)" : ""
				));
			}
		}
		else if (Action == 2)
		{
			Int64 Gamemode;
			if (!ReadVarIntValue(Gamemode))
			{
				return false;
			}
			a_Value.append(fmt::format(FMT_STRING("\n    gamemode {}"), Gamemode));
		}
		else if (Action == 3)
		{
			Int64 Ping;
			if (!ReadVarIntValue(Ping))
			{
				return false;
			}
			a_Value.append(fmt::format(FMT_STRING("\n    ping {} ms"), Ping));
		}
	}
	return true;
}





bool cPayloadReader::ReadUnlockRecipes(AString & a_Value)
{
	// wiki.vg Protocol#Unlock Recipes, the layout of protocol 340:
	Int64 Action;
	bool IsBookOpen, IsFiltering;
	Int64 NumIDs;
	if (
		!ReadVarIntValue(Action) ||
		!ReadBoolValue(IsBookOpen) ||
		!ReadBoolValue(IsFiltering) ||
		!ReadVarIntValue(NumIDs) ||
		(NumIDs < 0)
	)
	{
		return false;
	}
	a_Value = fmt::format(
		FMT_STRING("action {}, book {}, filter {}, {} recipes"),
		Action, IsBookOpen ? "open" : "closed", IsFiltering ? "on" : "off", NumIDs
	);
	for (Int64 i = 0; i < NumIDs; i++)
	{
		Int64 ID;
		if (!ReadVarIntValue(ID))
		{
			return false;
		}
		if (i < static_cast<Int64>(MAX_LISTED_ITEMS))
		{
			a_Value.append(fmt::format(FMT_STRING("\n  recipe {}"), ID));
		}
	}
	if (Action != 0)
	{
		return true;
	}
	if (!ReadVarIntValue(NumIDs) || (NumIDs < 0))
	{
		return false;
	}
	a_Value.append(fmt::format(FMT_STRING(", {} known recipes"), NumIDs));
	for (Int64 i = 0; i < NumIDs; i++)
	{
		Int64 ID;
		if (!ReadVarIntValue(ID))
		{
			return false;
		}
		if (i < static_cast<Int64>(MAX_LISTED_ITEMS))
		{
			a_Value.append(fmt::format(FMT_STRING("\n  known {}"), ID));
		}
	}
	return true;
}





bool cPayloadReader::ReadOpenWindow(AString & a_Value)
{
	// wiki.vg Protocol#Open Window:
	unsigned char WindowID, NumSlots;
	AString WindowType, WindowTitle;
	if (
		!ReadUByteValue(WindowID) ||
		!ReadStringValue(WindowType) ||
		!ReadStringValue(WindowTitle) ||
		!ReadUByteValue(NumSlots)
	)
	{
		return false;
	}
	a_Value = fmt::format(
		FMT_STRING("window {}, type \"{}\", title \"{}\", {} slots"),
		static_cast<unsigned>(WindowID), WindowType, WindowTitle, static_cast<unsigned>(NumSlots)
	);
	if (WindowType == "EntityHorse")
	{
		// The horse window carries the EntityHorse's EID, per wiki.vg:
		Int32 EntityID;
		if (!ReadIntValue(EntityID))
		{
			return false;
		}
		a_Value.append(fmt::format(FMT_STRING(", horse entity {}"), EntityID));
	}
	return true;
}





bool cPayloadReader::ReadTabComplete(AString & a_Value)
{
	// wiki.vg Protocol#Tab-Complete (serverbound):
	bool HasPosition;
	if (!ReadBoolValue(HasPosition))
	{
		return false;
	}
	if (!HasPosition)
	{
		a_Value = "no position";
		return true;
	}
	Vector3i Pos;
	if (!ReadBlockPosition(Pos))
	{
		return false;
	}
	a_Value = fmt::format(FMT_STRING("looking at <{}, {}, {}>"), Pos.x, Pos.y, Pos.z);
	return true;
}





bool cPayloadReader::ReadUseEntity(AString & a_Value)
{
	// wiki.vg Protocol#Use Entity:
	const auto Type = GetFieldValue("Type", -1);
	switch (Type)
	{
		case 0:  // interact
		case 2:   // interact at
		{
			if (Type == 2)
			{
				float X, Y, Z;
				if (!ReadFloatValue(X) || !ReadFloatValue(Y) || !ReadFloatValue(Z))
				{
					return false;
				}
				a_Value = fmt::format(FMT_STRING("at ({}, {}, {}), hand "), X, Y, Z);
			}
			Int64 Hand;
			if (!ReadVarIntValue(Hand))
			{
				return false;
			}
			if (Type == 0)
			{
				a_Value = "interact, hand ";
			}
			a_Value.append(fmt::format(FMT_STRING("{}"), Hand));
			return true;
		}
		case 1:
		{
			a_Value = "attack";
			return true;
		}
	}
	a_Value = "<nothing>";
	return true;
}





bool cPayloadReader::ReadCraftingBookData(AString & a_Value)
{
	// wiki.vg Protocol#Crafting Book Data:
	Int64 Type;
	if (!ReadVarIntValue(Type))
	{
		return false;
	}
	switch (Type)
	{
		case 0:
		{
			Int32 RecipeID;
			if (!ReadIntValue(RecipeID))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("displayed recipe {}"), RecipeID);
			return true;
		}
		case 1:
		{
			bool IsOpen, IsFiltering;
			if (!ReadBoolValue(IsOpen) || !ReadBoolValue(IsFiltering))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("book {}, filter {}"), IsOpen ? "open" : "closed", IsFiltering ? "on" : "off");
			return true;
		}
	}
	return false;
}





bool cPayloadReader::ReadAdvancementTab(AString & a_Value)
{
	// wiki.vg Protocol#Advancement Tab (serverbound):
	Int64 Action;
	if (!ReadVarIntValue(Action))
	{
		return false;
	}
	if (Action != 0)
	{
		a_Value = "closed tab";
		return true;
	}
	AString TabID;
	if (!ReadStringValue(TabID))
	{
		return false;
	}
	a_Value = fmt::format(FMT_STRING("opened tab \"{}\""), TabID);
	return true;
}





bool cPayloadReader::ReadSelectAdvancementTab(AString & a_Value)
{
	// wiki.vg Protocol#Select Advancement Tab (clientbound):
	bool HasID;
	if (!ReadBoolValue(HasID))
	{
		return false;
	}
	if (!HasID)
	{
		a_Value = "no tab";
		return true;
	}
	AString TabID;
	if (!ReadStringValue(TabID))
	{
		return false;
	}
	a_Value = fmt::format(FMT_STRING("tab \"{}\""), TabID);
	return true;
}





bool cPayloadReader::ReadOptionalScore(AString & a_Value)
{
	// wiki.vg Protocol#Update Score: the value is absent when the entry was removed:
	if (GetFieldValue("Action", 0) == 1)
	{
		a_Value = "<none>";
		return true;
	}
	Int64 Value;
	if (!ReadVarIntValue(Value))
	{
		return false;
	}
	a_Value = fmt::format(FMT_STRING("{}"), Value);
	return true;
}





bool cPayloadReader::ReadOptionalObjectiveData(AString & a_Value)
{
	// wiki.vg Protocol#Scoreboard Objective: the two trailing strings are sent for modes 0 and 2 only:
	const auto Mode = GetFieldValue("Mode", -1);
	if ((Mode != 0) && (Mode != 2))
	{
		a_Value = "<none>";
		return true;
	}
	AString Value, Type;
	if (!ReadStringValue(Value) || !ReadStringValue(Type))
	{
		return false;
	}
	a_Value = fmt::format(FMT_STRING("\"{}\", type \"{}\""), Value, Type);
	return true;
}





bool cPayloadReader::ReadRawHex(AString & a_Value)
{
	const auto Remaining = GetRemaining();
	a_Value = fmt::format(FMT_STRING("{} bytes"), static_cast<unsigned>(Remaining.size()));
	if (Remaining.empty())
	{
		return true;
	}
	AString Hex;
	CreateHexDump(Hex, Remaining.data(), Remaining.size(), 16);
	a_Value.push_back('\n');
	a_Value += Hex;
	return SkipBytes(Remaining.size());
}





////////////////////////////////////////////////////////////////////////////////
// cPayloadReader - the field dispatcher:

bool cPayloadReader::ReadField(const cPacketField & a_Field, AString & a_Value)
{
	switch (a_Field.m_Type)
	{
		case ftBool:
		{
			bool Value;
			if (!ReadBoolValue(Value))
			{
				return false;
			}
			a_Value = Value ? "true" : "false";
			RememberValue(a_Field.m_Name, Value ? 1 : 0);
			return true;
		}
		case ftByte:
		{
			Int8 Value;
			if (!ReadByteValue(Value))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("{}"), static_cast<int>(Value));
			RememberValue(a_Field.m_Name, Value);
			return true;
		}
		case ftUByte:
		{
			unsigned char Value;
			if (!ReadUByteValue(Value))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("{}"), static_cast<unsigned>(Value));
			RememberValue(a_Field.m_Name, Value);
			return true;
		}
		case ftShort:
		{
			Int16 Value;
			if (!ReadShortValue(Value))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("{}"), Value);
			RememberValue(a_Field.m_Name, Value);
			return true;
		}
		case ftUShort:
		{
			UInt16 Value;
			if (!ReadUShortValue(Value))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("{}"), Value);
			RememberValue(a_Field.m_Name, Value);
			return true;
		}
		case ftInt:
		{
			Int32 Value;
			if (!ReadIntValue(Value))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("{}"), Value);
			RememberValue(a_Field.m_Name, Value);
			return true;
		}
		case ftLong:
		{
			Int64 Value;
			if (!ReadLongValue(Value))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("{}"), Value);
			RememberValue(a_Field.m_Name, Value);
			return true;
		}
		case ftFloat:
		{
			float Value;
			if (!ReadFloatValue(Value))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("{}"), Value);
			return true;
		}
		case ftDouble:
		{
			double Value;
			if (!ReadDoubleValue(Value))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("{}"), Value);
			return true;
		}
		case ftVarInt:
		{
			Int64 Value;
			if (!ReadVarIntValue(Value))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("{}"), Value);
			RememberValue(a_Field.m_Name, Value);
			return true;
		}
		case ftVarLong:
		{
			Int64 Value;
			if (!ReadVarLongValue(Value))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("{}"), Value);
			RememberValue(a_Field.m_Name, Value);
			return true;
		}
		case ftAngle:
		{
			unsigned char Value;
			if (!ReadUByteValue(Value))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("{} ({} degrees)"), static_cast<unsigned>(Value), AngleToDegrees(Value));
			RememberValue(a_Field.m_Name, Value);
			return true;
		}
		case ftString:
		{
			if (!ReadStringValue(a_Value))
			{
				return false;
			}
			a_Value = "\"" + a_Value + "\"";
			return true;
		}
		case ftByteArray:
		{
			Int64 Length;
			if (!ReadVarLongValue(Length) || (Length < 0) || (static_cast<size_t>(Length) > GetRemainingBytes()))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("{} bytes"), Length);
			return SkipBytes(static_cast<size_t>(Length));
		}
		case ftUUID:
		{
			return ReadUUIDValue(a_Value);
		}
		case ftPosition:
		{
			Vector3i Pos;
			if (!ReadBlockPosition(Pos))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("<{}, {}, {}>"), Pos.x, Pos.y, Pos.z);
			return true;
		}
		case ftSlot:
		{
			return ReadSlot(a_Value);
		}
		case ftMetadata:
		{
			return ReadMetadata(a_Value);
		}
		case ftNBT:
		{
			return ReadNBT(a_Value);
		}
		case ftVarIntArray:
		{
			Int64 Count;
			if (!ReadVarIntValue(Count) || (Count < 0))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("{} values"), Count);
			for (Int64 i = 0; i < Count; i++)
			{
				Int64 Value;
				if (!ReadVarIntValue(Value))
				{
					return false;
				}
				if (i < static_cast<Int64>(MAX_LISTED_ITEMS))
				{
					a_Value.append(fmt::format(FMT_STRING("\n  {}"), Value));
				}
			}
			return true;
		}
		case ftStringArray:
		{
			Int64 Count;
			if (!ReadVarIntValue(Count) || (Count < 0))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("{} strings"), Count);
			for (Int64 i = 0; i < Count; i++)
			{
				AString Value;
				if (!ReadStringValue(Value))
				{
					return false;
				}
				if (i < static_cast<Int64>(MAX_LISTED_ITEMS))
				{
					a_Value.append(fmt::format(FMT_STRING("\n  \"{}\""), Value));
				}
			}
			return true;
		}
		case ftSlotArray:
		{
			UInt16 Count;
			if (!ReadUShortValue(Count))
			{
				return false;
			}
			a_Value = fmt::format(FMT_STRING("{} slots"), Count);
			for (UInt16 i = 0; i < Count; i++)
			{
				AString Slot;
				if (!ReadSlot(Slot))
				{
					return false;
				}
				if (i < MAX_LISTED_ITEMS)
				{
					a_Value.append(fmt::format(FMT_STRING("\n  [{}] {}"), i, Slot));
				}
			}
			return true;
		}
		case ftStatistics:           return ReadStatistics(a_Value);
		case ftExplosionRecords:     return ReadExplosionRecords(a_Value);
		case ftMultiBlockRecords:    return ReadMultiBlockRecords(a_Value);
		case ftEntityProperties:     return ReadEntityProperties(a_Value);
		case ftChunkData:            return ReadChunkData(a_Value);
		case ftParticleData:         return ReadParticleData(a_Value);
		case ftMap:                  return ReadMap(a_Value);
		case ftTeams:                return ReadTeams(a_Value);
		case ftBossBar:              return ReadBossBar(a_Value);
		case ftTitle:                return ReadTitle(a_Value);
		case ftWorldBorder:          return ReadWorldBorder(a_Value);
		case ftCombatEvent:          return ReadCombatEvent(a_Value);
		case ftPlayerListItem:       return ReadPlayerListItem(a_Value);
		case ftUnlockRecipes:        return ReadUnlockRecipes(a_Value);
		case ftOpenWindow:           return ReadOpenWindow(a_Value);
		case ftTabComplete:          return ReadTabComplete(a_Value);
		case ftUseEntity:            return ReadUseEntity(a_Value);
		case ftCraftingBookData:     return ReadCraftingBookData(a_Value);
		case ftAdvancementTab:       return ReadAdvancementTab(a_Value);
		case ftSelectAdvancementTab: return ReadSelectAdvancementTab(a_Value);
		case ftOptScore:             return ReadOptionalScore(a_Value);
		case ftOptObjectiveData:     return ReadOptionalObjectiveData(a_Value);
		case ftRawHex:               return ReadRawHex(a_Value);
	}  // switch (a_Field.m_Type)

	return false;
}
