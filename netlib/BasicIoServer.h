#ifndef BASIC_IO_SERVER_H
#define BASIC_IO_SERVER_H

#include "BasicSession.hpp"
#include "BasicWorkPool.hpp"

namespace netlib
{
	template<typename Message>
	class BasicIoServer
		:public ISessionEvent<Message>
		,private boost::noncopyable
	{
	public:
		//TypeDefine
		typedef BasicSession<Message>                   Session;
		typedef boost::shared_ptr<Session>              SessionPtr;
		typedef std::vector<SessionPtr>                 SessionSet;
		typedef boost::shared_ptr<Message>              MessagePtr;
	
	public:
		BasicIoServer(BasicWorkPool& workPool, uint32_t connectTimeout = 0, uint32_t connectMaxLimit = 1024);
		virtual ~BasicIoServer();

		inline uint32_t   ConnectTimeout() const { return m_ConnectTimeout;}
		//void              ConnectTimeout(uint32_t);

	public:
		virtual Session*  HandleSessionCreate(boost::asio::io_service&) = 0;
		virtual void      HandleSessionConnected(SessionPtr);

		virtual void HandleSessionDisconnected(SessionPtr, const boost::system::error_code&);

	public:
		void  StartServer(const boost::asio::ip::tcp::endpoint&, boost::system::error_code&);
		void  StopServer(boost::system::error_code&);

	private:
		inline void  Initialize();
		void  HandleAccepted(const boost::system::error_code&);
		void  PerformAsyncAccept();
		void  PerformSessionCheck(const boost::system::error_code&);
		void  RemoveAllSessions();
		void  RemoveAllSessionsImpl();
		void  HandleSessionConnectedImpl(SessionPtr);

	private:
		BasicIoServer& m_WorkPool;
		uint32_t m_ConnectTimeout;
		uint32_t m_ConnectionMaxLimit;

		boost::asio::ip::tcp::acceptor      m_Acceptor;
		boost::asio::ip::tcp::endpoint      m_EndPoint;

		SessionPtr                       m_sptr;
		HandlerAllocator                  m_AcceptAllocator;
		HandlerAllocator                  m_TimerAllocator;

		SessionSet                        m_Sessions;
		boost::asio::deadline_timer        m_UpdateTimer;
		boost::asio::io_service::strand    m_UpdateStrand;

		boost::posix_time::ptime           m_ConnectMaxTime;
		uint64_t                            m_ConnectTotal;
		uint64_t                            m_ConnectNow;
		uint64_t                            m_ConnectMax;
		uint64_t                            m_KickedSessions;

		bool                               m_Stopping;
	};
}

#include "impl/BasicIoServer.ipp"

#endif