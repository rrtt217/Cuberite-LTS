
// PacketLayout.cpp

// Transcription of the protocol 340 (MC 1.12.2) packet definitions used when decoding a captured stream.
//
// Every layout below is taken from wiki.vg, "Protocol", revision 1313 - the last revision that describes
// protocol 340 (1.12.2) as the current stable one:
//   https://wiki.vg/index.php?title=Protocol&oldid=1313
// Field kinds are documented in PacketLayout.h, their decoding in PayloadReader.h.
//
// The tables are dense, indexed by packet id, and cover every packet documented for the protocol.
// A packet whose id is not covered is reported by cConnection as unknown and dumped as hex.





#include "Globals.h"
#include "PacketLayout.h"





/** Ends a field list. The field kind is irrelevant, only the null name is looked at. */
#define END_OF_FIELDS {nullptr, ftBool}

/** Ends a packet table. Same reason. */
#define END_OF_PACKETS {nullptr, nullptr}

/** The field list of a packet that carries no fields at all. */
static const cPacketField s_NoFields[] =
{
	END_OF_FIELDS
};

/** Declares an empty field list, for packets that carry no data at all. */
#define NO_FIELDS s_NoFields





////////////////////////////////////////////////////////////////////////////////
// Handshaking, packets sent by the client:

static const cPacketField g_fHandshake[] =
{
	{ "Protocol Version", ftVarInt },
	{ "Server Address", ftString },
	{ "Server Port", ftUShort },
	{ "Next State", ftVarInt },
	END_OF_FIELDS
};

static const cPacketField g_fLegacyServerListPing[] =
{
	{ "Payload", ftRawHex },
	END_OF_FIELDS
};

static const cPacketDefinition g_HandshakingFromClient[] =
{
	/* 0x00 */ { "Handshake", g_fHandshake },
	/* 0xfe */ { "Legacy Server List Ping", g_fLegacyServerListPing },
	END_OF_PACKETS
};





////////////////////////////////////////////////////////////////////////////////
// Status, packets sent by the server:

static const cPacketField g_fStatusResponse[] =
{
	{ "JSON Response", ftString },
	END_OF_FIELDS
};

static const cPacketField g_fStatusPong[] =
{
	{ "Payload", ftLong },
	END_OF_FIELDS
};

static const cPacketDefinition g_StatusFromServer[] =
{
	/* 0x00 */ { "Response", g_fStatusResponse },
	/* 0x01 */ { "Pong", g_fStatusPong },
	END_OF_PACKETS
};





////////////////////////////////////////////////////////////////////////////////
// Status, packets sent by the client:

static const cPacketDefinition g_StatusFromClient[] =
{
	/* 0x00 */ { "Request", NO_FIELDS },
	/* 0x01 */ { "Ping", g_fStatusPong },
	END_OF_PACKETS
};





////////////////////////////////////////////////////////////////////////////////
// Login, packets sent by the server:

static const cPacketField g_fLoginDisconnect[] =
{
	{ "Reason", ftString },
	END_OF_FIELDS
};

static const cPacketField g_fEncryptionRequest[] =
{
	{ "Server ID", ftString },
	{ "Public Key", ftByteArray },
	{ "Verify Token", ftByteArray },
	END_OF_FIELDS
};

static const cPacketField g_fLoginSuccess[] =
{
	{ "UUID", ftString },
	{ "Username", ftString },
	END_OF_FIELDS
};

static const cPacketField g_fSetCompression[] =
{
	{ "Threshold", ftVarInt },
	END_OF_FIELDS
};

static const cPacketDefinition g_LoginFromServer[] =
{
	/* 0x00 */ { "Disconnect", g_fLoginDisconnect },
	/* 0x01 */ { "Encryption Request", g_fEncryptionRequest },
	/* 0x02 */ { "Login Success", g_fLoginSuccess },
	/* 0x03 */ { "Set Compression", g_fSetCompression },
	END_OF_PACKETS
};





////////////////////////////////////////////////////////////////////////////////
// Login, packets sent by the client:

static const cPacketField g_fLoginStart[] =
{
	{ "Name", ftString },
	END_OF_FIELDS
};

static const cPacketField g_fEncryptionResponse[] =
{
	{ "Shared Secret", ftByteArray },
	{ "Verify Token", ftByteArray },
	END_OF_FIELDS
};

static const cPacketDefinition g_LoginFromClient[] =
{
	/* 0x00 */ { "Login Start", g_fLoginStart },
	/* 0x01 */ { "Encryption Response", g_fEncryptionResponse },
	END_OF_PACKETS
};





////////////////////////////////////////////////////////////////////////////////
// Game, packets sent by the server:

static const cPacketField g_fServerDifficulty[] =
{
	{ "Difficulty", ftUByte },
	END_OF_FIELDS
};

static const cPacketField g_fBossBar[] =
{
	{ "Boss Bar", ftBossBar },
	END_OF_FIELDS
};

static const cPacketField g_fSpawnObject[] =
{
	{ "Entity ID", ftVarInt },
	{ "Object UUID", ftUUID },
	{ "Type", ftByte },
	{ "X", ftDouble },
	{ "Y", ftDouble },
	{ "Z", ftDouble },
	{ "Pitch", ftAngle },
	{ "Yaw", ftAngle },
	{ "Data", ftInt },
	{ "Velocity X", ftShort },
	{ "Velocity Y", ftShort },
	{ "Velocity Z", ftShort },
	END_OF_FIELDS
};

static const cPacketField g_fSpawnExperienceOrb[] =
{
	{ "Entity ID", ftVarInt },
	{ "X", ftDouble },
	{ "Y", ftDouble },
	{ "Z", ftDouble },
	{ "Count", ftShort },
	END_OF_FIELDS
};

static const cPacketField g_fSpawnGlobalEntity[] =
{
	{ "Entity ID", ftVarInt },
	{ "Type", ftByte },
	{ "X", ftDouble },
	{ "Y", ftDouble },
	{ "Z", ftDouble },
	END_OF_FIELDS
};

static const cPacketField g_fSpawnMob[] =
{
	{ "Entity ID", ftVarInt },
	{ "Entity UUID", ftUUID },
	{ "Type", ftVarInt },
	{ "X", ftDouble },
	{ "Y", ftDouble },
	{ "Z", ftDouble },
	{ "Yaw", ftAngle },
	{ "Pitch", ftAngle },
	{ "Head Pitch", ftAngle },
	{ "Velocity X", ftShort },
	{ "Velocity Y", ftShort },
	{ "Velocity Z", ftShort },
	{ "Metadata", ftMetadata },
	END_OF_FIELDS
};

static const cPacketField g_fSpawnPainting[] =
{
	{ "Entity ID", ftVarInt },
	{ "Entity UUID", ftUUID },
	{ "Title", ftString },
	{ "Location", ftPosition },
	{ "Direction", ftByte },
	END_OF_FIELDS
};

static const cPacketField g_fSpawnPlayer[] =
{
	{ "Entity ID", ftVarInt },
	{ "Player UUID", ftUUID },
	{ "X", ftDouble },
	{ "Y", ftDouble },
	{ "Z", ftDouble },
	{ "Yaw", ftAngle },
	{ "Pitch", ftAngle },
	{ "Metadata", ftMetadata },
	END_OF_FIELDS
};

static const cPacketField g_fAnimation[] =
{
	{ "Entity ID", ftVarInt },
	{ "Animation", ftUByte },
	END_OF_FIELDS
};

static const cPacketField g_fStatistics[] =
{
	{ "Statistics", ftStatistics },
	END_OF_FIELDS
};

static const cPacketField g_fBlockBreakAnimation[] =
{
	{ "Entity ID", ftVarInt },
	{ "Location", ftPosition },
	{ "Destroy Stage", ftByte },
	END_OF_FIELDS
};

static const cPacketField g_fUpdateBlockEntity[] =
{
	{ "Location", ftPosition },
	{ "Action", ftUByte },
	{ "NBT Data", ftNBT },
	END_OF_FIELDS
};

static const cPacketField g_fBlockAction[] =
{
	{ "Location", ftPosition },
	{ "Action ID", ftUByte },
	{ "Action Param", ftUByte },
	{ "Block Type", ftVarInt },
	END_OF_FIELDS
};

static const cPacketField g_fBlockChange[] =
{
	{ "Location", ftPosition },
	{ "Block ID", ftVarInt },
	END_OF_FIELDS
};

static const cPacketField g_fTabComplete[] =
{
	{ "Matches", ftStringArray },
	END_OF_FIELDS
};

static const cPacketField g_fChatMessage[] =
{
	{ "JSON Data", ftString },
	{ "Position", ftByte },
	END_OF_FIELDS
};

static const cPacketField g_fMultiBlockChange[] =
{
	{ "Chunk X", ftInt },
	{ "Chunk Z", ftInt },
	{ "Records", ftMultiBlockRecords },
	END_OF_FIELDS
};

static const cPacketField g_fConfirmTransaction[] =
{
	{ "Window ID", ftByte },
	{ "Action Number", ftShort },
	{ "Accepted", ftBool },
	END_OF_FIELDS
};

static const cPacketField g_fCloseWindow[] =
{
	{ "Window ID", ftUByte },
	END_OF_FIELDS
};

static const cPacketField g_fOpenWindow[] =
{
	{ "Window", ftOpenWindow },
	END_OF_FIELDS
};

static const cPacketField g_fWindowItems[] =
{
	{ "Window ID", ftUByte },
	{ "Slot Data", ftSlotArray },
	END_OF_FIELDS
};

static const cPacketField g_fWindowProperty[] =
{
	{ "Window ID", ftUByte },
	{ "Property", ftShort },
	{ "Value", ftShort },
	END_OF_FIELDS
};

static const cPacketField g_fSetSlot[] =
{
	{ "Window ID", ftByte },
	{ "Slot", ftShort },
	{ "Slot Data", ftSlot },
	END_OF_FIELDS
};

static const cPacketField g_fSetCooldown[] =
{
	{ "Item ID", ftVarInt },
	{ "Cooldown Ticks", ftVarInt },
	END_OF_FIELDS
};

static const cPacketField g_fPluginMessage[] =
{
	{ "Channel", ftString },
	{ "Data", ftByteArray },
	END_OF_FIELDS
};

static const cPacketField g_fNamedSoundEffect[] =
{
	{ "Sound Name", ftString },
	{ "Sound Category", ftVarInt },
	{ "Effect Position X", ftInt },
	{ "Effect Position Y", ftInt },
	{ "Effect Position Z", ftInt },
	{ "Volume", ftFloat },
	{ "Pitch", ftFloat },
	END_OF_FIELDS
};

static const cPacketField g_fDisconnect[] =
{
	{ "Reason", ftString },
	END_OF_FIELDS
};

static const cPacketField g_fEntityStatus[] =
{
	{ "Entity ID", ftInt },
	{ "Entity Status", ftByte },
	END_OF_FIELDS
};

static const cPacketField g_fExplosion[] =
{
	{ "X", ftFloat },
	{ "Y", ftFloat },
	{ "Z", ftFloat },
	{ "Radius", ftFloat },
	{ "Records", ftExplosionRecords },
	{ "Player Motion X", ftFloat },
	{ "Player Motion Y", ftFloat },
	{ "Player Motion Z", ftFloat },
	END_OF_FIELDS
};

static const cPacketField g_fUnloadChunk[] =
{
	{ "Chunk X", ftInt },
	{ "Chunk Z", ftInt },
	END_OF_FIELDS
};

static const cPacketField g_fChangeGameState[] =
{
	{ "Reason", ftUByte },
	{ "Value", ftFloat },
	END_OF_FIELDS
};

static const cPacketField g_fKeepAlive[] =
{
	{ "Keep Alive ID", ftLong },
	END_OF_FIELDS
};

static const cPacketField g_fChunkData[] =
{
	{ "Chunk X", ftInt },
	{ "Chunk Z", ftInt },
	{ "Chunk Data", ftChunkData },
	END_OF_FIELDS
};

static const cPacketField g_fEffect[] =
{
	{ "Effect ID", ftInt },
	{ "Location", ftPosition },
	{ "Data", ftInt },
	{ "Disable Relative Volume", ftBool },
	END_OF_FIELDS
};

static const cPacketField g_fParticle[] =
{
	{ "Particle ID", ftInt },
	{ "Long Distance", ftBool },
	{ "X", ftFloat },
	{ "Y", ftFloat },
	{ "Z", ftFloat },
	{ "Offset X", ftFloat },
	{ "Offset Y", ftFloat },
	{ "Offset Z", ftFloat },
	{ "Particle Data", ftFloat },
	{ "Particle Count", ftInt },
	{ "Data", ftParticleData },
	END_OF_FIELDS
};

static const cPacketField g_fJoinGame[] =
{
	{ "Entity ID", ftInt },
	{ "Gamemode", ftUByte },
	{ "Dimension", ftInt },
	{ "Difficulty", ftUByte },
	{ "Max Players", ftUByte },
	{ "Level Type", ftString },
	{ "Reduced Debug Info", ftBool },
	END_OF_FIELDS
};

static const cPacketField g_fMap[] =
{
	{ "Map", ftMap },
	END_OF_FIELDS
};

static const cPacketField g_fEntity[] =
{
	{ "Entity ID", ftVarInt },
	END_OF_FIELDS
};

static const cPacketField g_fEntityRelMove[] =
{
	{ "Entity ID", ftVarInt },
	{ "Delta X", ftShort },
	{ "Delta Y", ftShort },
	{ "Delta Z", ftShort },
	{ "On Ground", ftBool },
	END_OF_FIELDS
};

static const cPacketField g_fEntityRelMoveLook[] =
{
	{ "Entity ID", ftVarInt },
	{ "Delta X", ftShort },
	{ "Delta Y", ftShort },
	{ "Delta Z", ftShort },
	{ "Yaw", ftAngle },
	{ "Pitch", ftAngle },
	{ "On Ground", ftBool },
	END_OF_FIELDS
};

static const cPacketField g_fEntityLook[] =
{
	{ "Entity ID", ftVarInt },
	{ "Yaw", ftAngle },
	{ "Pitch", ftAngle },
	{ "On Ground", ftBool },
	END_OF_FIELDS
};

static const cPacketField g_fVehicleMove[] =
{
	{ "X", ftDouble },
	{ "Y", ftDouble },
	{ "Z", ftDouble },
	{ "Yaw", ftFloat },
	{ "Pitch", ftFloat },
	END_OF_FIELDS
};

static const cPacketField g_fOpenSignEditor[] =
{
	{ "Location", ftPosition },
	END_OF_FIELDS
};

static const cPacketField g_fCraftRecipeResponse[] =
{
	{ "Window ID", ftByte },
	{ "Recipe", ftVarInt },
	END_OF_FIELDS
};

static const cPacketField g_fPlayerAbilities[] =
{
	{ "Flags", ftByte },
	{ "Flying Speed", ftFloat },
	{ "Field of View Modifier", ftFloat },
	END_OF_FIELDS
};

static const cPacketField g_fCombatEvent[] =
{
	{ "Combat Event", ftCombatEvent },
	END_OF_FIELDS
};

static const cPacketField g_fPlayerListItem[] =
{
	{ "Player List Item", ftPlayerListItem },
	END_OF_FIELDS
};

static const cPacketField g_fPlayerPositionLook[] =
{
	{ "X", ftDouble },
	{ "Y", ftDouble },
	{ "Z", ftDouble },
	{ "Yaw", ftFloat },
	{ "Pitch", ftFloat },
	{ "Flags", ftByte },
	{ "Teleport ID", ftVarInt },
	END_OF_FIELDS
};

static const cPacketField g_fUseBed[] =
{
	{ "Entity ID", ftVarInt },
	{ "Location", ftPosition },
	END_OF_FIELDS
};

static const cPacketField g_fUnlockRecipes[] =
{
	{ "Unlock Recipes", ftUnlockRecipes },
	END_OF_FIELDS
};

static const cPacketField g_fDestroyEntities[] =
{
	{ "Entity IDs", ftVarIntArray },
	END_OF_FIELDS
};

static const cPacketField g_fRemoveEntityEffect[] =
{
	{ "Entity ID", ftVarInt },
	{ "Effect ID", ftByte },
	END_OF_FIELDS
};

static const cPacketField g_fResourcePackSend[] =
{
	{ "URL", ftString },
	{ "Hash", ftString },
	END_OF_FIELDS
};

static const cPacketField g_fRespawn[] =
{
	{ "Dimension", ftInt },
	{ "Difficulty", ftUByte },
	{ "Gamemode", ftUByte },
	{ "Level Type", ftString },
	END_OF_FIELDS
};

static const cPacketField g_fEntityHeadLook[] =
{
	{ "Entity ID", ftVarInt },
	{ "Head Yaw", ftAngle },
	END_OF_FIELDS
};

static const cPacketField g_fSelectAdvancementTab[] =
{
	{ "Select Advancement Tab", ftSelectAdvancementTab },
	END_OF_FIELDS
};

static const cPacketField g_fWorldBorder[] =
{
	{ "World Border", ftWorldBorder },
	END_OF_FIELDS
};

static const cPacketField g_fCamera[] =
{
	{ "Camera ID", ftVarInt },
	END_OF_FIELDS
};

static const cPacketField g_fHeldItemChange[] =
{
	{ "Slot", ftByte },
	END_OF_FIELDS
};

static const cPacketField g_fDisplayScoreboard[] =
{
	{ "Position", ftByte },
	{ "Score Name", ftString },
	END_OF_FIELDS
};

static const cPacketField g_fEntityMetadata[] =
{
	{ "Entity ID", ftVarInt },
	{ "Metadata", ftMetadata },
	END_OF_FIELDS
};

static const cPacketField g_fAttachEntity[] =
{
	{ "Attached Entity ID", ftInt },
	{ "Holding Entity ID", ftInt },
	END_OF_FIELDS
};

static const cPacketField g_fEntityVelocity[] =
{
	{ "Entity ID", ftVarInt },
	{ "Velocity X", ftShort },
	{ "Velocity Y", ftShort },
	{ "Velocity Z", ftShort },
	END_OF_FIELDS
};

static const cPacketField g_fEntityEquipment[] =
{
	{ "Entity ID", ftVarInt },
	{ "Slot", ftVarInt },
	{ "Item", ftSlot },
	END_OF_FIELDS
};

static const cPacketField g_fSetExperience[] =
{
	{ "Experience Bar", ftFloat },
	{ "Level", ftVarInt },
	{ "Total Experience", ftVarInt },
	END_OF_FIELDS
};

static const cPacketField g_fUpdateHealth[] =
{
	{ "Health", ftFloat },
	{ "Food", ftVarInt },
	{ "Food Saturation", ftFloat },
	END_OF_FIELDS
};

static const cPacketField g_fScoreboardObjective[] =
{
	{ "Objective Name", ftString },
	{ "Mode", ftByte },
	{ "Objective Data", ftOptObjectiveData },
	END_OF_FIELDS
};

static const cPacketField g_fSetPassengers[] =
{
	{ "Entity ID", ftVarInt },
	{ "Passengers", ftVarIntArray },
	END_OF_FIELDS
};

static const cPacketField g_fTeams[] =
{
	{ "Teams", ftTeams },
	END_OF_FIELDS
};

static const cPacketField g_fUpdateScore[] =
{
	{ "Entity Name", ftString },
	{ "Action", ftByte },
	{ "Objective Name", ftString },
	{ "Value", ftOptScore },
	END_OF_FIELDS
};

static const cPacketField g_fSpawnPosition[] =
{
	{ "Location", ftPosition },
	END_OF_FIELDS
};

static const cPacketField g_fTimeUpdate[] =
{
	{ "World Age", ftLong },
	{ "Time Of Day", ftLong },
	END_OF_FIELDS
};

static const cPacketField g_fTitle[] =
{
	{ "Title", ftTitle },
	END_OF_FIELDS
};

static const cPacketField g_fSoundEffect[] =
{
	{ "Sound ID", ftVarInt },
	{ "Sound Category", ftVarInt },
	{ "Effect Position X", ftInt },
	{ "Effect Position Y", ftInt },
	{ "Effect Position Z", ftInt },
	{ "Volume", ftFloat },
	{ "Pitch", ftFloat },
	END_OF_FIELDS
};

static const cPacketField g_fPlayerListHeaderFooter[] =
{
	{ "Header", ftString },
	{ "Footer", ftString },
	END_OF_FIELDS
};

static const cPacketField g_fCollectItem[] =
{
	{ "Collected Entity ID", ftVarInt },
	{ "Collector Entity ID", ftVarInt },
	{ "Pickup Item Count", ftVarInt },
	END_OF_FIELDS
};

static const cPacketField g_fEntityTeleport[] =
{
	{ "Entity ID", ftVarInt },
	{ "X", ftDouble },
	{ "Y", ftDouble },
	{ "Z", ftDouble },
	{ "Yaw", ftAngle },
	{ "Pitch", ftAngle },
	{ "On Ground", ftBool },
	END_OF_FIELDS
};

static const cPacketField g_fAdvancements[] =
{
	{ "Advancements", ftRawHex },
	END_OF_FIELDS
};

static const cPacketField g_fEntityProperties[] =
{
	{ "Entity ID", ftVarInt },
	{ "Properties", ftEntityProperties },
	END_OF_FIELDS
};

static const cPacketField g_fEntityEffect[] =
{
	{ "Entity ID", ftVarInt },
	{ "Effect ID", ftByte },
	{ "Amplifier", ftByte },
	{ "Duration", ftVarInt },
	{ "Flags", ftByte },
	END_OF_FIELDS
};

static const cPacketDefinition g_GameFromServer[] =
{
	/* 0x00 */ { "Spawn Object", g_fSpawnObject },
	/* 0x01 */ { "Spawn Experience Orb", g_fSpawnExperienceOrb },
	/* 0x02 */ { "Spawn Global Entity", g_fSpawnGlobalEntity },
	/* 0x03 */ { "Spawn Mob", g_fSpawnMob },
	/* 0x04 */ { "Spawn Painting", g_fSpawnPainting },
	/* 0x05 */ { "Spawn Player", g_fSpawnPlayer },
	/* 0x06 */ { "Animation", g_fAnimation },
	/* 0x07 */ { "Statistics", g_fStatistics },
	/* 0x08 */ { "Block Break Animation", g_fBlockBreakAnimation },
	/* 0x09 */ { "Update Block Entity", g_fUpdateBlockEntity },
	/* 0x0a */ { "Block Action", g_fBlockAction },
	/* 0x0b */ { "Block Change", g_fBlockChange },
	/* 0x0c */ { "Boss Bar", g_fBossBar },
	/* 0x0d */ { "Server Difficulty", g_fServerDifficulty },
	/* 0x0e */ { "Tab-Complete", g_fTabComplete },
	/* 0x0f */ { "Chat Message", g_fChatMessage },
	/* 0x10 */ { "Multi Block Change", g_fMultiBlockChange },
	/* 0x11 */ { "Confirm Transaction", g_fConfirmTransaction },
	/* 0x12 */ { "Close Window", g_fCloseWindow },
	/* 0x13 */ { "Open Window", g_fOpenWindow },
	/* 0x14 */ { "Window Items", g_fWindowItems },
	/* 0x15 */ { "Window Property", g_fWindowProperty },
	/* 0x16 */ { "Set Slot", g_fSetSlot },
	/* 0x17 */ { "Set Cooldown", g_fSetCooldown },
	/* 0x18 */ { "Plugin Message", g_fPluginMessage },
	/* 0x19 */ { "Named Sound Effect", g_fNamedSoundEffect },
	/* 0x1a */ { "Disconnect", g_fDisconnect },
	/* 0x1b */ { "Entity Status", g_fEntityStatus },
	/* 0x1c */ { "Explosion", g_fExplosion },
	/* 0x1d */ { "Unload Chunk", g_fUnloadChunk },
	/* 0x1e */ { "Change Game State", g_fChangeGameState },
	/* 0x1f */ { "Keep Alive", g_fKeepAlive },
	/* 0x20 */ { "Chunk Data", g_fChunkData },
	/* 0x21 */ { "Effect", g_fEffect },
	/* 0x22 */ { "Particle", g_fParticle },
	/* 0x23 */ { "Join Game", g_fJoinGame },
	/* 0x24 */ { "Map", g_fMap },
	/* 0x25 */ { "Entity", g_fEntity },
	/* 0x26 */ { "Entity Relative Move", g_fEntityRelMove },
	/* 0x27 */ { "Entity Look And Relative Move", g_fEntityRelMoveLook },
	/* 0x28 */ { "Entity Look", g_fEntityLook },
	/* 0x29 */ { "Vehicle Move", g_fVehicleMove },
	/* 0x2a */ { "Open Sign Editor", g_fOpenSignEditor },
	/* 0x2b */ { "Craft Recipe Response", g_fCraftRecipeResponse },
	/* 0x2c */ { "Player Abilities", g_fPlayerAbilities },
	/* 0x2d */ { "Combat Event", g_fCombatEvent },
	/* 0x2e */ { "Player List Item", g_fPlayerListItem },
	/* 0x2f */ { "Player Position And Look", g_fPlayerPositionLook },
	/* 0x30 */ { "Use Bed", g_fUseBed },
	/* 0x31 */ { "Unlock Recipes", g_fUnlockRecipes },
	/* 0x32 */ { "Destroy Entities", g_fDestroyEntities },
	/* 0x33 */ { "Remove Entity Effect", g_fRemoveEntityEffect },
	/* 0x34 */ { "Resource Pack Send", g_fResourcePackSend },
	/* 0x35 */ { "Respawn", g_fRespawn },
	/* 0x36 */ { "Entity Head Look", g_fEntityHeadLook },
	/* 0x37 */ { "Select Advancement Tab", g_fSelectAdvancementTab },
	/* 0x38 */ { "World Border", g_fWorldBorder },
	/* 0x39 */ { "Camera", g_fCamera },
	/* 0x3a */ { "Held Item Change", g_fHeldItemChange },
	/* 0x3b */ { "Display Scoreboard", g_fDisplayScoreboard },
	/* 0x3c */ { "Entity Metadata", g_fEntityMetadata },
	/* 0x3d */ { "Attach Entity", g_fAttachEntity },
	/* 0x3e */ { "Entity Velocity", g_fEntityVelocity },
	/* 0x3f */ { "Entity Equipment", g_fEntityEquipment },
	/* 0x40 */ { "Set Experience", g_fSetExperience },
	/* 0x41 */ { "Update Health", g_fUpdateHealth },
	/* 0x42 */ { "Scoreboard Objective", g_fScoreboardObjective },
	/* 0x43 */ { "Set Passengers", g_fSetPassengers },
	/* 0x44 */ { "Teams", g_fTeams },
	/* 0x45 */ { "Update Score", g_fUpdateScore },
	/* 0x46 */ { "Spawn Position", g_fSpawnPosition },
	/* 0x47 */ { "Time Update", g_fTimeUpdate },
	/* 0x48 */ { "Title", g_fTitle },
	/* 0x49 */ { "Sound Effect", g_fSoundEffect },
	/* 0x4a */ { "Player List Header And Footer", g_fPlayerListHeaderFooter },
	/* 0x4b */ { "Collect Item", g_fCollectItem },
	/* 0x4c */ { "Entity Teleport", g_fEntityTeleport },
	/* 0x4d */ { "Advancements", g_fAdvancements },
	/* 0x4e */ { "Entity Properties", g_fEntityProperties },
	/* 0x4f */ { "Entity Effect", g_fEntityEffect },
	END_OF_PACKETS
};





////////////////////////////////////////////////////////////////////////////////
// Game, packets sent by the client:

static const cPacketField g_fTeleportConfirm[] =
{
	{ "Teleport ID", ftVarInt },
	END_OF_FIELDS
};

static const cPacketField g_fClientTabComplete[] =
{
	{ "Text", ftString },
	{ "Assume Command", ftBool },
	{ "Looked At Block", ftTabComplete },
	END_OF_FIELDS
};

static const cPacketField g_fChatMessageClient[] =
{
	{ "Message", ftString },
	END_OF_FIELDS
};

static const cPacketField g_fClientStatus[] =
{
	{ "Action ID", ftVarInt },
	END_OF_FIELDS
};

static const cPacketField g_fClientSettings[] =
{
	{ "Locale", ftString },
	{ "View Distance", ftByte },
	{ "Chat Mode", ftVarInt },
	{ "Chat Colors", ftBool },
	{ "Displayed Skin Parts", ftUByte },
	{ "Main Hand", ftVarInt },
	END_OF_FIELDS
};

static const cPacketField g_fEnchantItem[] =
{
	{ "Window ID", ftByte },
	{ "Enchantment", ftByte },
	END_OF_FIELDS
};

static const cPacketField g_fClickWindow[] =
{
	{ "Window ID", ftUByte },
	{ "Slot", ftShort },
	{ "Button", ftByte },
	{ "Action Number", ftShort },
	{ "Mode", ftVarInt },
	{ "Clicked Item", ftSlot },
	END_OF_FIELDS
};

static const cPacketField g_fUseEntity[] =
{
	{ "Target", ftVarInt },
	{ "Type", ftVarInt },
	{ "Target Data", ftUseEntity },
	END_OF_FIELDS
};

static const cPacketField g_fPlayer[] =
{
	{ "On Ground", ftBool },
	END_OF_FIELDS
};

static const cPacketField g_fPlayerPosition[] =
{
	{ "X", ftDouble },
	{ "Feet Y", ftDouble },
	{ "Z", ftDouble },
	{ "On Ground", ftBool },
	END_OF_FIELDS
};

static const cPacketField g_fPlayerPositionLookClient[] =
{
	{ "X", ftDouble },
	{ "Feet Y", ftDouble },
	{ "Z", ftDouble },
	{ "Yaw", ftFloat },
	{ "Pitch", ftFloat },
	{ "On Ground", ftBool },
	END_OF_FIELDS
};

static const cPacketField g_fPlayerLook[] =
{
	{ "Yaw", ftFloat },
	{ "Pitch", ftFloat },
	{ "On Ground", ftBool },
	END_OF_FIELDS
};

static const cPacketField g_fSteerBoat[] =
{
	{ "Right Paddle Turning", ftBool },
	{ "Left Paddle Turning", ftBool },
	END_OF_FIELDS
};

static const cPacketField g_fCraftRecipeRequest[] =
{
	{ "Window ID", ftByte },
	{ "Recipe", ftVarInt },
	{ "Make All", ftBool },
	END_OF_FIELDS
};

static const cPacketField g_fPlayerAbilitiesClient[] =
{
	{ "Flags", ftByte },
	{ "Flying Speed", ftFloat },
	{ "Walking Speed", ftFloat },
	END_OF_FIELDS
};

static const cPacketField g_fPlayerDigging[] =
{
	{ "Status", ftVarInt },
	{ "Location", ftPosition },
	{ "Face", ftByte },
	END_OF_FIELDS
};

static const cPacketField g_fEntityAction[] =
{
	{ "Entity ID", ftVarInt },
	{ "Action ID", ftVarInt },
	{ "Jump Boost", ftVarInt },
	END_OF_FIELDS
};

static const cPacketField g_fSteerVehicle[] =
{
	{ "Sideways", ftFloat },
	{ "Forward", ftFloat },
	{ "Flags", ftUByte },
	END_OF_FIELDS
};

static const cPacketField g_fCraftingBookData[] =
{
	{ "Crafting Book Data", ftCraftingBookData },
	END_OF_FIELDS
};

static const cPacketField g_fResourcePackStatus[] =
{
	{ "Result", ftVarInt },
	END_OF_FIELDS
};

static const cPacketField g_fAdvancementTab[] =
{
	{ "Advancement Tab", ftAdvancementTab },
	END_OF_FIELDS
};

static const cPacketField g_fHeldItemChangeClient[] =
{
	{ "Slot", ftShort },
	END_OF_FIELDS
};

static const cPacketField g_fCreativeInventoryAction[] =
{
	{ "Slot", ftShort },
	{ "Clicked Item", ftSlot },
	END_OF_FIELDS
};

static const cPacketField g_fUpdateSign[] =
{
	{ "Location", ftPosition },
	{ "Line 1", ftString },
	{ "Line 2", ftString },
	{ "Line 3", ftString },
	{ "Line 4", ftString },
	END_OF_FIELDS
};

static const cPacketField g_fAnimationClient[] =
{
	{ "Hand", ftVarInt },
	END_OF_FIELDS
};

static const cPacketField g_fSpectate[] =
{
	{ "Target Player", ftUUID },
	END_OF_FIELDS
};

static const cPacketField g_fPlayerBlockPlacement[] =
{
	{ "Location", ftPosition },
	{ "Face", ftVarInt },
	{ "Hand", ftVarInt },
	{ "Cursor Position X", ftFloat },
	{ "Cursor Position Y", ftFloat },
	{ "Cursor Position Z", ftFloat },
	END_OF_FIELDS
};

static const cPacketField g_fUseItem[] =
{
	{ "Hand", ftVarInt },
	END_OF_FIELDS
};

static const cPacketDefinition g_GameFromClient[] =
{
	/* 0x00 */ { "Teleport Confirm", g_fTeleportConfirm },
	/* 0x01 */ { "Tab-Complete", g_fClientTabComplete },
	/* 0x02 */ { "Chat Message", g_fChatMessageClient },
	/* 0x03 */ { "Client Status", g_fClientStatus },
	/* 0x04 */ { "Client Settings", g_fClientSettings },
	/* 0x05 */ { "Confirm Transaction", g_fConfirmTransaction },
	/* 0x06 */ { "Enchant Item", g_fEnchantItem },
	/* 0x07 */ { "Click Window", g_fClickWindow },
	/* 0x08 */ { "Close Window", g_fCloseWindow },
	/* 0x09 */ { "Plugin Message", g_fPluginMessage },
	/* 0x0a */ { "Use Entity", g_fUseEntity },
	/* 0x0b */ { "Keep Alive", g_fKeepAlive },
	/* 0x0c */ { "Player", g_fPlayer },
	/* 0x0d */ { "Player Position", g_fPlayerPosition },
	/* 0x0e */ { "Player Position And Look", g_fPlayerPositionLookClient },
	/* 0x0f */ { "Player Look", g_fPlayerLook },
	/* 0x10 */ { "Vehicle Move", g_fVehicleMove },
	/* 0x11 */ { "Steer Boat", g_fSteerBoat },
	/* 0x12 */ { "Craft Recipe Request", g_fCraftRecipeRequest },
	/* 0x13 */ { "Player Abilities", g_fPlayerAbilitiesClient },
	/* 0x14 */ { "Player Digging", g_fPlayerDigging },
	/* 0x15 */ { "Entity Action", g_fEntityAction },
	/* 0x16 */ { "Steer Vehicle", g_fSteerVehicle },
	/* 0x17 */ { "Crafting Book Data", g_fCraftingBookData },
	/* 0x18 */ { "Resource Pack Status", g_fResourcePackStatus },
	/* 0x19 */ { "Advancement Tab", g_fAdvancementTab },
	/* 0x1a */ { "Held Item Change", g_fHeldItemChangeClient },
	/* 0x1b */ { "Creative Inventory Action", g_fCreativeInventoryAction },
	/* 0x1c */ { "Update Sign", g_fUpdateSign },
	/* 0x1d */ { "Animation", g_fAnimationClient },
	/* 0x1e */ { "Spectate", g_fSpectate },
	/* 0x1f */ { "Player Block Placement", g_fPlayerBlockPlacement },
	/* 0x20 */ { "Use Item", g_fUseItem },
	END_OF_PACKETS
};





////////////////////////////////////////////////////////////////////////////////
// cPacketLayout:

/** Returns the definition of the given packet ID in a_Table, or nullptr if the table doesn't define one. */
static const cPacketDefinition * GetDefinition(const cPacketDefinition * a_Table, size_t a_NumEntries, UInt32 a_PacketID)
{
	if (a_PacketID >= a_NumEntries)
	{
		return nullptr;
	}
	const auto & Def = a_Table[a_PacketID];
	return (Def.m_Name == nullptr) ? nullptr : &Def;
}





const cPacketDefinition * cPacketLayout::GetFromServerLayout(eProtocolState a_State, UInt32 a_PacketID)
{
	switch (a_State)
	{
		case psStatus:
		{
			return GetDefinition(g_StatusFromServer, ARRAYCOUNT(g_StatusFromServer), a_PacketID);
		}
		case psLogin:
		{
			return GetDefinition(g_LoginFromServer, ARRAYCOUNT(g_LoginFromServer), a_PacketID);
		}
		case psGame:
		{
			return GetDefinition(g_GameFromServer, ARRAYCOUNT(g_GameFromServer), a_PacketID);
		}
		case psUnknown:
		{
			return nullptr;
		}
	}
	return nullptr;
}





const cPacketDefinition * cPacketLayout::GetFromClientLayout(eProtocolState a_State, UInt32 a_PacketID)
{
	switch (a_State)
	{
		case psUnknown:
		{
			// The initial handshake, the only packet understood in this state:
			if (a_PacketID == 0x00)
			{
				return &g_HandshakingFromClient[0];
			}
			if (a_PacketID == 0xfe)
			{
				return &g_HandshakingFromClient[1];
			}
			return nullptr;
		}
		case psStatus:
		{
			return GetDefinition(g_StatusFromClient, ARRAYCOUNT(g_StatusFromClient), a_PacketID);
		}
		case psLogin:
		{
			return GetDefinition(g_LoginFromClient, ARRAYCOUNT(g_LoginFromClient), a_PacketID);
		}
		case psGame:
		{
			return GetDefinition(g_GameFromClient, ARRAYCOUNT(g_GameFromClient), a_PacketID);
		}
	}
	return nullptr;
}





const char * cPacketLayout::GetStateName(eProtocolState a_State)
{
	switch (a_State)
	{
		case psUnknown: return "handshaking";
		case psStatus: return "status";
		case psLogin: return "login";
		case psGame: return "game";
	}
	return "unknown";
}
