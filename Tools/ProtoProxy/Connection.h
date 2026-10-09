
// Connection.h

// Interfaces to the cConnection class representing a single pair of connected sockets,
// understanding the Minecraft protocol version 340 (MC 1.12.2).





#pragma once

#include "ByteBuffer.h"
#include "PacketFraming.h"
#include "PacketLayout.h"
#include "PayloadReader.h"
#include "mbedTLS++/AesCfb128Decryptor.h"
#include "mbedTLS++/AesCfb128Encryptor.h"

#ifndef _WIN32
	typedef int SOCKET;
#endif





class cServer;





/** Handles one client - server connection pair: relays both directions, decoding every packet for the log.
The connection to the client is kept unencrypted, while the connection to the server uses whatever
encryption the server requests - the proxy feeds the server an encryption response of its own. */
class cConnection
{
public:
	cConnection(SOCKET a_ClientSocket, cServer & a_Server);
	~cConnection();

	/** Runs the relay until either side closes the connection. */
	void Run(void);

	/** Logs a_Format formatted with a_ArgList, prefixed with the time relative to the connection start. */
	void vLog(const char * a_Format, fmt::format_args a_ArgList);

	template <typename... Args>
	void Log(const char * a_Format, const Args & ... a_Args)
	{
		vLog(a_Format, fmt::make_format_args(a_Args...));
	}

	/** Logs a_Format formatted with a_ArgList, followed by a hexdump of a_Data. */
	void vDataLog(const void * a_Data, size_t a_Size, const char * a_Format, fmt::format_args a_ArgList);

	template <typename... Args>
	void DataLog(const void * a_Data, size_t a_Size, const char * a_Format, const Args & ... a_Args)
	{
		vDataLog(a_Data, a_Size, a_Format, fmt::make_format_args(a_Args...));
	}

	void LogFlush(void);

protected:

	/** Which way the packet that is being handled went. */
	enum eDirection : unsigned char
	{
		dirFromClient,  // Client -> proxy -> server
		dirFromServer,  // Server -> proxy -> client
	} ;

	/** Base for the log filename and all files connected to this log */
	AString m_LogNameBase;

	cCriticalSection m_CSLog;
	FILE * m_LogFile;

	cServer & m_Server;
	SOCKET m_ClientSocket;
	SOCKET m_ServerSocket;

	std::chrono::steady_clock::time_point m_BeginTick;  // Tick when the relative time was first retrieved (used for GetRelativeTime())

	/*
	The protocol states are those of the Handshake packet's Next State field:
	-1: no initial handshake received yet
	1: status
	2: login
	3: game
	*/
	/** State the to-server protocol is in (as defined by the initial handshake / login), -1 if no initial handshake received yet */
	eProtocolState m_ServerProtocolState;

	/** State the to-client protocol is in (as defined by the initial handshake / login), -1 if no initial handshake received yet */
	eProtocolState m_ClientProtocolState;

	/** True if the server connection has provided encryption keys */
	bool m_IsServerEncrypted;

	cAesCfb128Decryptor m_ServerDecryptor;
	cAesCfb128Encryptor m_ServerEncryptor;

	ContiguousByteBuffer m_ServerEncryptionBuffer;  // Buffer for the data to be sent to the server once encryption is established

	cInboundPackets m_PacketsFromClient;
	cInboundPackets m_PacketsFromServer;

	/** Serialisation of the packets that the proxy generates itself (handshake, encryption response). */
	cOutboundPackets m_ToServer;

	/** Index of the next file into which an NBT blob found in a packet should be written. */
	int m_NBTFileIdx;

	bool ConnectToServer(void);

	/** Receives from the server and processes everything that was received; returns false if the connection aborted */
	bool RelayFromServer(void);

	/** Receives from the client and processes everything that was received; returns false if the connection aborted */
	bool RelayFromClient(void);

	/** Returns the time relative to the first call of this function, in the fractional seconds elapsed */
	double GetRelativeTime(void);

	/** Sends data to the specified socket. If sending fails, prints a fail message using a_Peer and returns false. */
	bool SendData(SOCKET a_Socket, const ContiguousByteBufferView a_Data, const char * a_Peer);

	/** Encrypts the data using a_Encryptor and sends them to the specified socket. */
	bool SendEncryptedData(SOCKET a_Socket, cAesCfb128Encryptor & a_Encryptor, ContiguousByteBuffer & a_Data, const char * a_Peer);

	/** Feeds freshly received bytes into the packet stream of the given direction and handles all complete packets.
	Returns false if the connection should be closed. */
	bool ProcessData(eDirection a_Direction, const char * a_Data, size_t a_Size);

	/** Decodes, logs and relays a single packet. Returns false if the connection should be closed. */
	bool HandlePacket(eDirection a_Direction, const sCapturedPacket & a_Packet);

	/** Logs one packet's fields, using the layout matching its state and direction. */
	void LogPacket(
		eDirection a_Direction,
		const sCapturedPacket & a_Packet,
		eProtocolState a_State,
		const cPacketDefinition * a_Definition
	);

	/** Rewrites the port in the client's handshake and sends it to the server.
	Returns false if the connection should be closed. */
	bool HandleHandshake(const sCapturedPacket & a_Packet);

	/** Intercepts the server's Encryption Request, replies with a response of the proxy's own and
	starts decrypting the server's stream. The request itself is not forwarded to the client. */
	bool HandleEncryptionRequest(const sCapturedPacket & a_Packet);

	/** Sends an Encryption Response, generated by the proxy, to the server, and starts encryption. */
	bool SendEncryptionKeyResponse(const ContiguousByteBufferView a_ServerPublicKey, const ContiguousByteBufferView a_VerifyToken);

	/** Sets both protocol states to Game and remembers that the encryption phase is over. */
	void HandleLoginSuccess(void);

	/** Applies the threshold of a Set Compression packet to both directions. */
	void HandleSetCompression(const ContiguousByteBufferView a_Payload);
} ;
