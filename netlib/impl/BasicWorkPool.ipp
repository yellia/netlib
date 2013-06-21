#ifndef BASIC_WORKPOOL_IPP
#define BASIC_WORKPOOL_IPP

namespace netlib
{
	BasicWorkPool::BasicWorkPool(std::size_t core/*=1*/)
		:m_CoreCount(core)
		,m_NextIoService(0)
	{
		if (m_CoreCount == 0)
			throw std::runtime_error("io_service_pool size is 0");

		// Give all the io_services work to do so that their run() functions will not
		// exit until they are explicitly stopped.
		for (std::size_t i = 0; i < m_CoreCount; ++i)
		{
			IoServicePtr io_service(new boost::asio::io_service);
			WorkPtr work(new boost::asio::io_service::work(*io_service));
			m_Services.push_back(io_service);
			m_Works.push_back(work);
		}
	}

	boost::asio::io_service&  BasicWorkPool::GetIoService()
	{
		// Use a round-robin scheme to choose the next io_service to use.
		boost::asio::io_service& service = *m_Services[m_NextIoService++];

		if (m_NextIoService == m_Services.size())
			m_NextIoService = 0;

		return service;
	}

	void  BasicWorkPool::StartWorkPool()
	{
		for (std::size_t i=0; i<m_CoreCount; i++)
			m_Threads.create_thread(boost::bind(&boost::asio::io_service::run, m_Services[i]));
	}

	void  BasicWorkPool::StopWorkPool()
	{
		m_Works.clear();
		m_Threads.join_all();
		m_Services.clear();
	}
}

#endif