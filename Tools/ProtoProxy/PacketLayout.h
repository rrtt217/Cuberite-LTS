
// PacketLayout.h

// Declares the description of all packets understood by ProtoProxy (protocol 340, MC 1.12.2),
// and the lookup functions used when decoding a captured stream.
//
// The layouts are a direct transcription of the protocol documentation:
//   wiki.vg, "Protocol", revision 1313 (documents protocol 340 / 1.12.2):
//   https://wiki.vg/index.php?title=Protocol&oldid=1313
// Behavioural spec for every field type lives in PayloadReader.h.





#pragma once





/** The kind of a single packet field; determines both how many bytes are consumed and how the value is printed.
Compound kinds (ftChunkData, ftTeams, ...) consume the whole remaining packet and print multiple lines. */
enum eFieldType : unsigned char
{
	// Simple scalars:
	ftBool,             ///< One byte, 0 = false, everything else = true
	ftByte,             ///< Signed byte
	ftUByte,            ///< Unsigned byte
	ftShort,            ///< Signed big-endian 16-bit
	ftUShort,           ///< Unsigned big-endian 16-bit
	ftInt,              ///< Signed big-endian 32-bit
	ftLong,             ///< Signed big-endian 64-bit
	ftFloat,            ///< Big-endian 32-bit float
	ftDouble,           ///< Big-endian 64-bit double
	ftVarInt,           ///< VarInt, printed signed
	ftVarLong,          ///< VarLong, printed signed
	ftAngle,            ///< Unsigned byte, printed as degrees (value * 360 / 256)
	ftString,           ///< VarInt-length-prefixed UTF-8 string
	ftByteArray,        ///< VarInt-length-prefixed raw bytes, printed as a byte count
	ftUUID,             ///< 16 raw bytes, printed as the dashed hexadecimal form
	ftPosition,         ///< 64-bit packed block position, printed as <x, y, z>
	ftSlot,             ///< Item stack ("Slot Data" structure)
	ftMetadata,         ///< Entity metadata array, terminated by a 0xFF index
	ftNBT,              ///< A single NBT tag, printed as its size plus a hex dump

	// Arrays whose element count is stored inside the field itself:
	ftVarIntArray,      ///< VarInt count, then that many VarInts
	ftStringArray,      ///< VarInt count, then that many strings
	ftSlotArray,        ///< Unsigned Short count, then that many item stacks

	// Compound fields, each covering everything that follows in the packet:
	ftStatistics,       ///< Statistics packet: VarInt count, then (VarInt name, VarInt value) pairs
	ftExplosionRecords, ///< Explosion packet: Int count, then (Byte dx, Byte dy, Byte dz) records
	ftMultiBlockRecords,  ///< Multi Block Change records: VarInt count, then (UByte horiz, UByte y, VarInt block)
	ftEntityProperties, ///< Entity Properties packet: Int count, then the property records
	ftChunkData,        ///< Chunk Data packet, everything after Chunk Z
	ftParticleData,     ///< Particle packet's additional data, present only for some particles
	ftMap,              ///< Map packet's conditional icon / columns layout
	ftTeams,            ///< Teams packet, layout depends on Mode
	ftBossBar,          ///< Boss Bar packet, layout depends on Action
	ftTitle,            ///< Title packet, layout depends on Action
	ftWorldBorder,      ///< World Border packet, layout depends on Action
	ftCombatEvent,      ///< Combat Event packet, layout depends on Event
	ftPlayerListItem,   ///< Player List Item packet, layout depends on Action
	ftUnlockRecipes,    ///< Unlock Recipes packet, layout depends on Action
	ftOpenWindow,       ///< Open Window packet, the trailing Entity ID depends on Window Type
	ftTabComplete,      ///< Tab-Complete (serverbound), the trailing position depends on Has Position
	ftUseEntity,        ///< Use Entity packet, the trailing fields depend on Type
	ftCraftingBookData,  ///< Crafting Book Data packet (serverbound), layout depends on Type
	ftAdvancementTab,   ///< Advancement Tab (serverbound), the trailing identifier depends on Action
	ftSelectAdvancementTab,  ///< Select Advancement Tab (clientbound), the trailing identifier is optional
	ftOptScore,         ///< Update Score packet's trailing value, present unless Action == 1
	ftOptObjectiveData, ///< Scoreboard Objective packet's trailing fields, present for modes 0 and 2
	ftRawHex,           ///< The rest of the packet, printed as a hex dump
} ;





/** One named field of a packet layout.
Layouts are terminated by an entry whose name is nullptr. */
struct cPacketField
{
	const char * m_Name;
	eFieldType m_Type;
} ;





/** A single packet: its human-readable name and the list of its fields. */
struct cPacketDefinition
{
	const char * m_Name;
	const cPacketField * m_Fields;
} ;





/** The protocol states, using the same numbering as the Handshake packet's Next State field. */
enum eProtocolState : int
{
	psUnknown = -1,
	psStatus = 1,
	psLogin = 2,
	psGame = 3,
} ;





namespace cPacketLayout
{
	/** Returns the layout of the packet with the given id, as sent by the server in the given state.
	Returns nullptr if the proxy has no layout for that packet. */
	const cPacketDefinition * GetFromServerLayout(eProtocolState a_State, UInt32 a_PacketID);

	/** Returns the layout of the packet with the given id, as sent by the client in the given state.
	Returns nullptr if the proxy has no layout for that packet. */
	const cPacketDefinition * GetFromClientLayout(eProtocolState a_State, UInt32 a_PacketID);

	/** Returns the name of the protocol state, for logging. */
	const char * GetStateName(eProtocolState a_State);
} ;
