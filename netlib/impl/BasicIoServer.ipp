#ifndef BASICIOSERVER_IPP
#define BASICIOSERVER_IPP

namespace netlib
{
	template<typename Message>
	BasicIoServer<Message>::BasicIoServer(BasicWorkPool& workPool, uint32_t connectTimeout/*= 0*/, uint32_t connectMaxLimit/*= 1024*/)
		:m_WorkPool(workPool)
		,m_ConnectTimeout(connectTimeout)
		,m_ConnectionMaxLimit(connectMaxLimit)
		,m_Acceptor(m_WorkPool.GetIoService())
		,m_UpdateTimer(m_Acceptor.get_io_service())
		,m_UpdateStrand(m_Acceptor.get_io_service())
	{
		
	}

	template<typename Message>
	BasicIoServer<Message>::~BasicIoServer()
	{

	}

	template<typename Message>
	void  BasicIoServer<Message>::StartServer(const boost::asio::ip::tcp::endpoint& endPoint, boost::system::error_code& ec)
	{
		ec = boost::system::error_code();

		do 
		{
			NL_BREAK_IF(m_Acceptor.is_open())

			Initialize();

			m_EndPoint = endPoint;
			m_Acceptor.open(endPoint.protocol(), ec);
			NL_BREAK_IF(ec)

			m_Acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
			NL_BREAK_IF(ec)

			m_Acceptor.bind(m_EndPoint, ec);
			NL_BREAK_IF(ec)

			m_Acceptor.listen(boost::asio::socket_base::max_connections, ec);
			NL_BREAK_IF(ec)

			PerformAsyncAccept();

			if (m_uConnectTimeout > 0)
			{
				m_UpdateTimer.expires_from_now(boost::posix_time::seconds(1));
				m_UpdateTimer.async_wait(m_UpdateStrand.wrap(
					MakeCustomizeHandler(m_TimerAllocator, boost::bind(&BasicIoServer<Message>::PerformSessionCheck, this, _1))
				));
			}
		}while (0);
	}

	template<typename Message>
	void  BasicIoServer<Message>::StopServer(boost::system::error_code&)
	{

	}

	/*template<typename Message>
	void BasicIoServer<Message>::ConnectTimeout(uint32_t connectTimeout)
	{
		//Why m_ConnectTimeout == 0
		if (connectTimeout > 0 && m_ConnectTimeout == 0)
		{
			m_UpdateTimer.expires_from_now(boost::posix_time::seconds(1));
			m_UpdateTimer.async_wait(m_UpdateStrand.wrap(
				MakeCustomizeHandler(m_TimerAllocator, boost::bind(&BasicIoServer<Message>::PerformSessionCheck, this, _1))
				));
		}

		m_ConnectTimeout = connectTimeout;
	}*/

	template<typename Message>
	inline void  BasicIoServer<Message>::Initialize() 
	{
		m_ConnectMaxTime        = boost::posix_time::microsec_clock::local_time();

		m_ConnectTotal       = 0;
		m_ConnectNow         = 0;
		m_ConnectMax         = 0;

		m_KickedSessions     = 0;
	}

	template<typename Message>
	void  BasicIoServer<Message>::PerformSessionCheck(const boost::system::error_code& ec)
	{
		if (ec) {
			if (ec != boost::asio::error::make_error_code(boost::asio::error::operation_aborted))
				std::printf("Error %s.", ec.message().c_str());
			return;
		}

		boost::posix_time::ptime now             = boost::posix_time::microsec_clock::local_time();
		boost::posix_time::time_duration timeout = boost::posix_time::seconds(m_ConnectTimeout);

		for (typename SessionSet::iterator j=m_Sessions.begin(); j!=m_Sessions.end(); j++) {
			SessionPtr sptr = *j;

			boost::posix_time::time_duration duration = now - sptr->ActiveTime();
			if (sptr->Socket().is_open() && duration > timeout) {
				boost::system::error_code tmp;
				sptr->CloseSession(tmp);
				m_KickedSessions++;
			}
		}

		m_UpdateTimer.expires_from_now(boost::posix_time::seconds(1));
		m_UpdateTimer.async_wait(m_UpdateStrand.wrap(
			MakeCustomizeHandler(m_TimerAllocator,boost::bind(&BasicIoServer<Message>::PerformSessionCheck,this,_1))
		));
	}
}

#endif