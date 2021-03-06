#ifndef NTCP_SESSION_H__
#define NTCP_SESSION_H__

#include <inttypes.h>
#include <list>
#include <memory>
#include <cryptopp/modes.h>
#include <cryptopp/aes.h>
#include <cryptopp/adler32.h>
#include "aes.h"
#include "Identity.h"
#include "RouterInfo.h"
#include "I2NPProtocol.h"
#include "TransportSession.h"

namespace i2p
{
namespace transport
{

#pragma pack(1)
	struct NTCPPhase1
	{
		uint8_t pubKey[256];
		uint8_t HXxorHI[32];
	};	
	
	struct NTCPPhase2
	{
		uint8_t pubKey[256];
		struct
		{
			uint8_t hxy[32];
			uint32_t timestamp;
			uint8_t filler[12];
		} encrypted;	
	};	
	
#pragma pack()	

	const size_t NTCP_MAX_MESSAGE_SIZE = 16384; 
	const size_t NTCP_BUFFER_SIZE = 1040; // fits one tunnel message (1028)
	const int NTCP_TERMINATION_TIMEOUT = 120; // 2 minutes
	const size_t NTCP_DEFAULT_PHASE3_SIZE = 2/*size*/ + i2p::data::DEFAULT_IDENTITY_SIZE/*387*/ + 4/*ts*/ + 15/*padding*/ + 40/*signature*/; // 448 	

	class NTCPSession: public TransportSession, public std::enable_shared_from_this<NTCPSession>
	{
		public:

			NTCPSession (boost::asio::io_service& service, std::shared_ptr<const i2p::data::RouterInfo> in_RemoteRouter = nullptr);
			~NTCPSession ();
			void Terminate ();

			boost::asio::ip::tcp::socket& GetSocket () { return m_Socket; };
			bool IsEstablished () const { return m_IsEstablished; };
			
			void ClientLogin ();
			void ServerLogin ();
			void SendI2NPMessage (I2NPMessage * msg);

			size_t GetNumSentBytes () const { return m_NumSentBytes; };
			size_t GetNumReceivedBytes () const { return m_NumReceivedBytes; };
			
		protected:

			void Connected ();
			void SendTimeSyncMessage ();
			void SetIsEstablished (bool isEstablished) { m_IsEstablished = isEstablished; }
			
		private:

			void CreateAESKey (uint8_t * pubKey, i2p::crypto::AESKey& key);
				
			// client
			void SendPhase3 ();
			void HandlePhase1Sent (const boost::system::error_code& ecode,  std::size_t bytes_transferred);
			void HandlePhase2Received (const boost::system::error_code& ecode, std::size_t bytes_transferred);
			void HandlePhase3Sent (const boost::system::error_code& ecode, std::size_t bytes_transferred, uint32_t tsA);
			void HandlePhase4Received (const boost::system::error_code& ecode, std::size_t bytes_transferred, uint32_t tsA);

			//server
			void SendPhase2 ();
			void SendPhase4 (uint32_t tsA, uint32_t tsB);
			void HandlePhase1Received (const boost::system::error_code& ecode, std::size_t bytes_transferred);
			void HandlePhase2Sent (const boost::system::error_code& ecode, std::size_t bytes_transferred, uint32_t tsB);
			void HandlePhase3Received (const boost::system::error_code& ecode, std::size_t bytes_transferred, uint32_t tsB);
			void HandlePhase3ExtraReceived (const boost::system::error_code& ecode, std::size_t bytes_transferred, uint32_t tsB, size_t paddingLen);
			void HandlePhase3 (uint32_t tsB, size_t paddingLen);
			void HandlePhase4Sent (const boost::system::error_code& ecode,  std::size_t bytes_transferred);
			
			// common
			void Receive ();
			void HandleReceived (const boost::system::error_code& ecode, std::size_t bytes_transferred);
			bool DecryptNextBlock (const uint8_t * encrypted);	
		
			void Send (i2p::I2NPMessage * msg);
			void HandleSent (const boost::system::error_code& ecode, std::size_t bytes_transferred, i2p::I2NPMessage * msg);


			// timer
			void ScheduleTermination ();
			void HandleTerminationTimer (const boost::system::error_code& ecode);
			
		private:

			boost::asio::ip::tcp::socket m_Socket;
			boost::asio::deadline_timer m_TerminationTimer;
			bool m_IsEstablished;
			
			i2p::crypto::CBCDecryption m_Decryption;
			i2p::crypto::CBCEncryption m_Encryption;
			CryptoPP::Adler32 m_Adler;

			struct Establisher
			{	
				NTCPPhase1 phase1;
				NTCPPhase2 phase2;
			} * m_Establisher;	
			
			i2p::crypto::AESAlignedBuffer<NTCP_BUFFER_SIZE + 16> m_ReceiveBuffer;
			i2p::crypto::AESAlignedBuffer<16> m_TimeSyncBuffer;
			int m_ReceiveBufferOffset; 

			i2p::I2NPMessage * m_NextMessage;
			std::list<i2p::I2NPMessage *> m_DelayedMessages;
			size_t m_NextMessageOffset;

			size_t m_NumSentBytes, m_NumReceivedBytes;
	};	
}	
}	

#endif
