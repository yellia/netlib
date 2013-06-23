#ifndef BASIC_SESSION_H
#define BASIC_SESSION_H

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/noncopyable.hpp>
#include <tbb/concurrent_queue.h>
#include <map>

#include "NetLibMessage.hpp"
#include "BasicWorkPool.hpp"

namespace netlib
{

	template<typename Message>
	class ISessionEvent;

	typedef boost::tuple
	<
		uint64_t /*Total bytes*/,
		std::size_t /* Min bytes*/, 
		std::size_t /* Max bytes*/,
		uint64_t /* Count*/
	> MessageLog;

	typedef std::map<std::string,  MessageLog> MessageLogMap;
	typedef MessageLogMap::const_iterator      MessageLogMapConstIter;
	typedef MessageLogMap::iterator           MessageLogMapIter;

	template<typename Message, bool ReadDispatchThreadSafe=true>
	class BasicSession
		:public boost::enable_shared_from_this<BasicSession<Message> >
		,private boost::noncopyable
	{
	public:
		typedef BasicSession<Message>   Session;
		typedef boost::function<void(const boost::system::error_code&, std::size_t)>  AsyncHandler;
		typedef boost::shared_ptr<Session>                  SessionPtr;
		typedef boost::shared_ptr<Message>                  MessagePtr;
		typedef MessageTrait<Message>                      MessageTrait;

	public:
		BasicSession(boost::asio::io_service& ioService, ISessionEvent<Message> *, bool=false, std::size_t=8192);
		virtual ~BasicSession();

		inline void  WritingSizeMax(std::size_t writingSizeMax);
		inline std::size_t WritingSizeMax()const{return m_WritingSizeMax;}

		void   ActiveTime(const boost::posix_time::ptime& now) { m_ActiveTime = now; }
		const boost::posix_time::ptime& ActiveTime() const{ return m_ActiveTime; }

		boost::asio::ip::tcp::socket&  Socket() { return m_Socket;}
		boost::asio::ip::tcp::endpoint&  RemoteEndPoint() { return m_RemoteEp;}
		boost::asio::ip::tcp::endpoint&  LocalEndPoint() { return m_LocalEp;}

	public:
		virtual void    HandleMessageReceived(const Message&) = 0;
		virtual void    HandleErrorReceived(const boost::system::error_code& ec) = 0;

	public:
		void  StartSession(boost::system::error_code&);
		void  CloseSession(boost::system::error_code&);
		void  AsyncWrite(const Message&, bool bFlushImmediately=true);
		void  FlushWriteBuffers();

	private:
		void  PerformAsyncRead();
		void  HandleAsyncRead(const boost::system::error_code&, std::size_t);
		void  OnErrorReceived(const boost::system::error_code& ec);
		void  OnBufferReceived(const char* pBuffer, std::size_t size, bool performRecv);
		void  OnErrorWrite(const boost::system::error_code& ec);
		void  AsyncWrite(const std::string& strId, const std::string& strMessage, bool bFlushImmediately);
		void  AsyncWriteImpl();
		void  HandlerAsyncWrite(const boost::system::error_code& ec, std::size_t bytes_transffered);

	private:

		boost::asio::ip::tcp::socket            m_Socket;

		boost::asio::strand                    m_WriteStrand;
		boost::asio::strand                    m_ReadStrand;

		boost::posix_time::ptime               m_ActiveTime;

		boost::array<char, 1024>               m_RecvStream;

		// Low efficiency for huge messages.
		typedef std::pair<std::string, std::string> OutMessage;
		typedef tbb::concurrent_queue<OutMessage>   WriteMessageQueue;
		WriteMessageQueue                         m_WriteBuffers;
		bool                                     m_Writing;
		std::vector<std::pair<std::string, std::size_t> > m_WritingMessageIdSize;
		std::vector<char>                         m_WritingBuffers;
		OutMessage                               m_TempBuffer;
		std::size_t                              m_WritingSizeMax;

		ISessionEvent<Message>*                   m_Listener;

		boost::asio::ip::tcp::socket::endpoint_type m_RemoteEp;
		boost::asio::ip::tcp::socket::endpoint_type m_LocalEp;

		uint64_t                                 m_SendBytes;
		uint64_t                                 m_RecvBytes;

		HandlerAllocator                          m_ReadAllocator;
		HandlerAllocator                          m_WriteAllocator;

		MessageTrait                              m_Trait;

		MessageLogMap                             m_RecvLogs;
		MessageLogMap                             m_WriteLogs;

		bool                                     m_TraceLog;
		bool                                     m_Closing;
	};

	template <typename Message>
	class ISessionEvent
	{
	public:
		typedef BasicSession<Message>               Session;
		typedef boost::shared_ptr<Session>          SessionPtr;

	public:
		virtual void HandleSessionDisconnected(SessionPtr, const boost::system::error_code&) = 0;
	};
}

#include "impl/BasicSession.ipp"
#endif