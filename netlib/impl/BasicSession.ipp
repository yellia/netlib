#ifndef BASICSESSION_IPP
#define BASICSESSION_IPP

namespace netlib
{
	template<typename Message, bool ReadDispatchThreadSafe>
	BasicSession<Message, ReadDispatchThreadSafe>::BasicSession(boost::asio::io_service& service, ISessionEvent<Message>* listener, bool traceLog, std::size_t writeSizeMax)
		:m_Socket(service)
		,m_WriteStrand(service)
		,m_ReadStrand(service)
		,m_ActiveTime(boost::posix_time::microsec_clock::local_time())
		,m_Writing(false)
		,m_Listener(listener)
		,m_SendBytes(0)
		,m_RecvBytes(0)
		,m_TraceLog(traceLog)
		,m_Closing(false)
	{
		WritingSizeMax(writeSizeMax);
	}

	template<typename Message, bool ReadDispatchThreadSafe>
	inline BasicSession<Message, ReadDispatchThreadSafe>::~BasicSession()
	{

	}

	template<typename Message, bool ReadDispatchThreadSafe>
	inline void  BasicSession<Message, ReadDispatchThreadSafe>::WritingSizeMax(std::size_t writingSizeMax)
	{
		m_WritingSizeMax = writingSizeMax;
		m_WritingBuffers.reserve(m_WritingSizeMax);
	}

	template<typename Message, bool ReadDispatchThreadSafe>
	void  BasicSession<Message, ReadDispatchThreadSafe>::StartSession(boost::system::error_code& ec)
	{
		ec = boost::system::error_code();
		
		do
		{
			m_RemoteEp  = m_Socket.remote_endpoint(ec);
			m_LocalEp   = m_Socket.local_endpoint(ec);

			NL_BREAK_IF(ec)
			
			PerformAsyncRead();
		} while (0);
	}

	template <typename Message, bool ReadDispatchThreadSafe>
	inline void BasicSession<Message, ReadDispatchThreadSafe>::CloseSession(boost::system::error_code& ec)
	{
		ec = boost::system::error_code();

		do
		{
			NL_BREAK_IF(m_bClosing)

			m_Closing = true;

			NL_BREAK_IF(!m_WriteBuffers.empty())

			NL_BREAK_IF(!m_Socket.is_open())
			
			//Why 为什么不用socket.shutdown??
			//m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			//NL_BREAK_IF(ec)

			m_Socket.close(ec);
		}while(0);
	}
	
	template<typename Message, bool ReadDispatchThreadSafe>
	void  BasicSession<Message, ReadDispatchThreadSafe>::AsyncWrite(const Message& message, bool bFlushImmediately/*=true*/)
	{
		std::string strMessage;
		MessageTrait::Serialize(message, strMessage);
		AsyncWrite(MessageTrait::InstanceId(message), strMessage, bFlushImmediately);
	}
	
	template <typename Message, bool ReadDispatchThreadSafe>
	void BasicSession<Message, ReadDispatchThreadSafe>::AsyncWrite(const std::string& strId, const std::string& strMessage, bool bFlushImmediately)
	{
		m_WriteBuffers.push(std::make_pair(strId, strMessage));

		// Notify writer
		if (bFlushImmediately)
			m_WriteStrand.post(boost::bind(&BasicSession<Message>::AsyncWriteImpl, shared_from_this()));
	}
	
	template <typename Message, bool ReadDispatchThreadSafe>
	void BasicSession<Message, ReadDispatchThreadSafe>::AsyncWriteImpl()
	{
		if(m_Writing)
			return;

		m_WritingMessageIdSize.clear();
		m_WritingBuffers.clear();

		if(!m_TempBuffer.second.empty())
		{
			m_WritingMessageIdSize.push_back(std::make_pair(m_TempBuffer.first, m_TempBuffer.second.size()));
			m_WritingBuffers.insert(m_WritingBuffers.end(), m_TempBuffer.second.begin(), m_TempBuffer.second.end());
			m_TempBuffer.second.clear();
		}

		while(m_WriteBuffers.try_pop(m_TempBuffer) && m_TempBuffer.second.size()+m_WritingBuffers.size() <= m_WritingSizeMax)
		{
			m_WritingMessageIdSize.push_back(std::make_pair(m_TempBuffer.first, m_TempBuffer.second.size()));
			m_WritingBuffers.insert(m_WritingBuffers.end(), m_TempBuffer.second.begin(), m_TempBuffer.second.end());
			m_TempBuffer.second.clear();
		}

		if(m_WriteBuffers.empty() && !m_TempBuffer.second.empty())
		{
			m_WritingMessageIdSize.push_back(std::make_pair(m_TempBuffer.first, m_TempBuffer.second.size()));
			m_WritingBuffers.insert(m_WritingBuffers.end(), m_TempBuffer.second.begin(), m_TempBuffer.second.end());
			m_TempBuffer.second.clear();
		}
		// The capacity of m_WritingBuffers may be very huge if every message exceeds m_WritingSizeMax

		if(m_WritingBuffers.empty())
		{
			m_Writing = false;

			if(m_Closing)
			{
				boost::system::error_code ec;
				m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
				m_Socket.close(ec);
			}
			return;
		}

		AsyncHandler handler = boost::bind(&BasicSession<Message>::HandlerAsyncWrite,
						shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred);

		boost::asio::async_write(m_Socket,
								   boost::asio::buffer(m_WritingBuffers),
								   m_WriteStrand.wrap(MakeCustomizeHandler(m_WriteAllocator, handler)));

		m_Writing = true;
	}

	template <typename Message, bool ReadDispatchThreadSafe>
	void BasicSession<Message, ReadDispatchThreadSafe>::HandlerAsyncWrite(
	const boost::system::error_code& ec, std::size_t bytes_transffered)
	{

		if(ec) {
			OnErrorWrite(ec);
			return;
		}

		m_SendBytes += bytes_transffered;

		if(m_TraceLog)
		{
			for(std::vector<std::pair<std::string, std::size_t> >::const_iterator iter=m_WritingMessageIdSize.begin();
				iter!=m_WritingMessageIdSize.end(); iter++)
			{
				std::size_t size = iter->second;

				MessageLogMapIter j = m_WriteLogs.find(iter->first);
				if(j == m_WriteLogs.end())
					m_WriteLogs.insert(std::make_pair(iter->first, boost::make_tuple(size, size, size, 1)));
				else
				{
					MessageLog& log = j->second;
					boost::get<0>(log) += size;

					if(boost::get<1>(log) > size)
						boost::get<1>(log) = size;
					if(boost::get<2>(log) < size)
						boost::get<2>(log) = size;

					boost::get<3>(log) += 1;
				}
			}
		}

		m_Writing = false;
		AsyncWriteImpl();
	}
	
	template <typename Message, bool ReadDispatchThreadSafe>
	void BasicSession<Message, ReadDispatchThreadSafe>::FlushWriteBuffers()
	{
		if(!m_WriteBuffers.empty())
			m_WriteStrand.post(boost::bind(&BasicSession<Message>::AsyncWriteImpl, shared_from_this()));
	}
	
	template <typename Message, bool ReadDispatchThreadSafe>
	void BasicSession<Message, ReadDispatchThreadSafe>::OnErrorWrite(const boost::system::error_code& ec)
	{
		std::cout << "Write Error: " << ec.message().c_str() << std::endl;
		//if (m_pEvent)
		//m_pEventSink->HandleSessionDisconnected(shared_from_this(), ec);
	}

	template<typename Message, bool ReadDispatchThreadSafe>
	void BasicSession<Message, ReadDispatchThreadSafe>::PerformAsyncRead()
	{
		AsyncHandler handler = boost::bind(&BasicSession<Message>::HandleAsyncRead, shared_from_this(),
												boost::asio::placeholders::error,
												boost::asio::placeholders::bytes_transferred);

		if(!ReadDispatchThreadSafe)
			m_Socket.async_read_some(boost::asio::buffer(m_RecvStream),
						m_ReadStrand.wrap(MakeCustomizeHandler(m_ReadAllocator, handler)));
		else
			m_Socket.async_read_some(boost::asio::buffer(m_RecvStream),
						MakeCustomizeHandler(m_ReadAllocator, handler));
	}

	template<typename Message, bool ReadDispatchThreadSafe>
	void BasicSession<Message, ReadDispatchThreadSafe>::HandleAsyncRead(const boost::system::error_code& ec, std::size_t bytes_transferred)
	{
		if(ec)
		{
			OnErrorReceived(ec);
			return;
		}

		BOOST_ASSERT(bytes_transferred <= m_RecvStream.size());
		OnBufferReceived(m_RecvStream.data(), bytes_transferred);
	}

	template <typename Message, bool ReadDispatchThreadSafe>
	void BasicSession<Message, ReadDispatchThreadSafe>::OnErrorReceived(const boost::system::error_code& ec)
	{
		if(MessageTrait::consum_if_disconnected)
			OnBufferReceived(NULL, 0, false);

		if(m_Listener)
			m_Listener->HandleSessionDisconnected(shared_from_this(), ec);
	}

	template <typename Message, bool ReadDispatchThreadSafe>
	void BasicSession<Message, ReadDispatchThreadSafe>::OnBufferReceived(const char* pBuffer, std::size_t size, bool performRecv)
	{
		boost::system::error_code ec;
		m_Trait.Consum(pBuffer, size, ec);

		if(ec)
		{
			HandleErrorReceived(ec);
			return;
		}

		T message;
		std::size_t sizeTemp;
		while(m_Trait.TryPopMessage(message, sizeTemp))
		{
			HandleMessageReceived(message);

			if(m_TraceLog)
			{
				std::string strId(m_Trait.InstanceId(message));

				MessageLogMapIter j = m_RecvLogs.find(strId);
				if(j == m_RecvLogs.end())
					m_RecvLogs.insert(std::make_pair(strId, boost::make_tuple(sizeTemp, sizeTemp, sizeTemp, 1)));
				else
				{
					MessageLog& log = j->second;
					boost::get<0>(log) += sizeTemp;

					if(boost::get<1>(log) > sizeTemp)
						boost::get<1>(log) = sizeTemp;
					if(boost::get<2>(log) < sizeTemp)
						boost::get<2>(log) = sizeTemp;

					boost::get<3>(log) += 1;
				}
			}
		}

		m_RecvBytes += size;
		
		if(performRecv)
			PerformAsyncRead();
	}

}

#endif