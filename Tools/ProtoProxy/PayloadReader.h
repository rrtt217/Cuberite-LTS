
// PayloadReader.h

// Decodes the fields of a single protocol-340 (MC 1.12.2) packet payload into human-readable text.
//
// The decoders follow the packet definitions in PacketLayout.cpp (wiki.vg revision 1313), the data types
// follow wiki.vg "Data types", "Slot Data", "Entity metadata", "Chunk Format" and "NBT"; the links are
// listed in ProtoProxy.txt.
//
// ReadField() returns the value as text; scalars are single-line, compound kinds may span several lines
// separated by '\n'. The caller (cConnection) handles indentation.
//
// The class is deliberately free of any socket or logging dependency, so that it can be unit-tested
// directly (see tests/ProtoProxy).





#pragma once

#include "Globals.h"
#include "PacketLayout.h"
#include <map>





class cPayloadReader
{
public:
	cPayloadReader(const std::byte * a_Data, size_t a_Size);
	explicit cPayloadReader(const ContiguousByteBuffer & a_Payload);

	/** Number of payload bytes that weren't consumed by the fields read so far. */
	size_t GetRemainingBytes(void) const { return m_Payload.size() - m_Pos; }

	/** If set, NBT blobs encountered in the payload are additionally saved to "<a_Prefix>_nbt_<index>.bin". */
	void SetNBTFilePrefix(const AString & a_Prefix) { m_NBTFilePrefix = a_Prefix; }

	/** Decodes a single field, as described by a_Field, and returns its textual representation.
	Returns false if the payload doesn't contain the whole field - the packet is then only partially decoded. */
	bool ReadField(const cPacketField & a_Field, AString & a_Value);

	/** Describes a single item stack ("Slot Data"), consuming it from the payload. */
	bool ReadSlot(AString & a_Value);

	/** Describes an entity-metadata array (index / type / value, terminated by index 0xFF). */
	bool ReadMetadata(AString & a_Value);

	/** One chunk section, as described inside a Chunk Data packet's data array. */
	struct sChunkSection
	{
		int m_Index;                ///< Bottom-up index of the section, taken from the primary bitmask
		unsigned m_BitsPerBlock;    ///< Wiki: "Bits Per Block"
		UInt32 m_PaletteLength;     ///< 0 = global palette in use
		UInt32 m_NumDataLongs;      ///< Number of 8-byte longs in the section's data array
		bool m_HasSkyLight;
	} ;

	/** The parsed data array of a Chunk Data packet. */
	struct sChunkData
	{
		bool m_IsGroundUpContinuous;
		UInt32 m_Bitmask;
		UInt32 m_DataSize;
		UInt32 m_NumBlockEntities;
		std::vector<sChunkSection> m_Sections;
		bool m_HasBiomeData;
		bool m_HasSkyLight;
		bool m_SizeMatches;         ///< Whether the parsed contents add up to the Size field exactly
	} ;

	/** Describes a Chunk Data packet's payload, starting at the Ground-Up-Continuous field. */
	bool ReadChunkData(AString & a_Value);

	/** Skips one NBT tag in the payload and describes it (type, size, and optionally a hex dump).
	Returns false if the payload doesn't hold a complete tag. Public for testing. */
	bool ReadNBT(AString & a_Value, bool a_ShouldDumpHex = true);

	/** Decodes a 64-bit packed block position, wiki.vg "Data types#Position". Public for testing. */
	bool ReadBlockPosition(Vector3i & a_Position);

	/** Decodes a VarInt, returning it sign-extended. Public for testing. */
	bool ReadVarIntValue(Int64 & a_Value);

	// Primitives, all operating on the payload cursor:
	bool ReadBytes(void * a_Dst, size_t a_Count);
	bool PeekByte(unsigned char & a_Byte) const;
	bool ReadBoolValue(bool & a_Value);
	bool ReadByteValue(Int8 & a_Value);
	bool ReadUByteValue(unsigned char & a_Value);
	bool ReadShortValue(Int16 & a_Value);
	bool ReadUShortValue(UInt16 & a_Value);
	bool ReadIntValue(Int32 & a_Value);
	bool ReadLongValue(Int64 & a_Value);
	bool ReadFloatValue(float & a_Value);
	bool ReadDoubleValue(double & a_Value);
	bool ReadVarLongValue(Int64 & a_Value);
	bool ReadStringValue(AString & a_Value);
	bool ReadUUIDValue(AString & a_Value);

	/** Returns a view of the not-yet-consumed part of the payload. */
	ContiguousByteBufferView GetRemaining(void) const;

	/** Skips n bytes; returns false if the payload is shorter. */
	bool SkipBytes(size_t a_Count);

protected:

	/** Records an integer field value, so that a later compound field can depend on it. */
	void RememberValue(const char * a_Name, Int64 a_Value);

	/** Returns the value of an earlier integer field, or a_Default if that field wasn't read. */
	Int64 GetFieldValue(const char * a_Name, Int64 a_Default) const;

	/** Saves an NBT blob to a file, if a file prefix was set; returns the text to append to the description. */
	AString DescribeNBTFile(const std::byte * a_Data, size_t a_Size);

	// Compound decoders:
	bool ReadTeamInfo(AString & a_Value, bool a_IsCreate);
	bool ReadParticleData(AString & a_Value);
	bool ReadStatistics(AString & a_Value);
	bool ReadExplosionRecords(AString & a_Value);
	bool ReadMultiBlockRecords(AString & a_Value);
	bool ReadEntityProperties(AString & a_Value);
	bool ReadMap(AString & a_Value);
	bool ReadTeams(AString & a_Value);
	bool ReadBossBar(AString & a_Value);
	bool ReadTitle(AString & a_Value);
	bool ReadWorldBorder(AString & a_Value);
	bool ReadCombatEvent(AString & a_Value);
	bool ReadPlayerListItem(AString & a_Value);
	bool ReadUnlockRecipes(AString & a_Value);
	bool ReadOpenWindow(AString & a_Value);
	bool ReadTabComplete(AString & a_Value);
	bool ReadUseEntity(AString & a_Value);
	bool ReadCraftingBookData(AString & a_Value);
	bool ReadAdvancementTab(AString & a_Value);
	bool ReadSelectAdvancementTab(AString & a_Value);
	bool ReadOptionalScore(AString & a_Value);
	bool ReadOptionalObjectiveData(AString & a_Value);
	bool ReadRawHex(AString & a_Value);

	/** The full payload being read. Owned by the instance so that NBT sub-buffers stay valid. */
	ContiguousByteBuffer m_Payload;
	size_t m_Pos;

	/** Index of the next NBT file to write. */
	int m_NBTFileIdx;

	/** Prefix for files into which NBT blobs are saved; empty = don't save. */
	AString m_NBTFilePrefix;

	/** Values of the integer fields read so far, so that a compound field can depend on an earlier scalar. */
	std::map<AString, Int64> m_LastValues;
} ;

