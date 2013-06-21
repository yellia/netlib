#ifndef BASIC_SESSION_H
#define BASIC_SESSION_H

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>

namespace netlib
{
	template<typename Message>
	class BasicSession
		:public boost::enable_shared_from_this<BasicSession<Message> >
		,private boost::noncopyable
	{

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
#endif