#ifndef NETLIBMESSAGE_HPP
#define NETLIBMESSAGE_HPP
#include "NetLibTypes.hpp"
#include <deque>

namespace netlib
{
	template<typename Message>
	class MessageTrait
	{
	public:
		static void         Serialize(const Message&, std::string&){}

		static std::string  InstanceId(const Message&) { return std::string();}

		void                Consum(const void*, std::size_t, boost::system::error_code&);
		bool                TryPopMessage(Message&, std::size_t&) { return false;}

		static const bool   consum_if_disconnected = false;
	};

	class MessageTooLargeException : public std::logic_error
	{
	public:
		MessageTooLargeException() : std::logic_error("message body size is too large.") {}
	};

	class MessageWithHead
	{
	public:
		MessageWithHead();
		MessageWithHead(uint16_t, const std::string&);
		MessageWithHead(uint16_t, const char*, std::size_t);
		MessageWithHead(uint16_t, const char*);

		uint16_t            GetMessageId() const { return m_MessageId;}
		void                SetMessageId(uint16_t MessageId) { m_MessageId = MessageId;}
		uint16_t            GetMessageBodySize() const { return m_strBody.size();}
		const std::string&  GetMessageBody() const { return m_strBody;}
		std::string&        GetMessageBody() { return m_strBody;}
		std::size_t         GetMessageSize() const { return 4 + GetMessageBodySize();}

		void                GetSerializedString(std::string&)const;

	private:
		uint16_t        m_MessageId;
		std::string     m_strBody;
	};

	template <>
	class MessageTrait<MessageWithHead>
	{
	public:
		MessageTrait() { m_ParseState = PARSE_BEGIN; m_usBodySize = 0; }

		static void   Serialize(const MessageWithHead&, std::string&);

		static std::string  InstanceId(const MessageWithHead&);

		void  Consum(const void*, std::size_t, boost::system::error_code&);
		bool  TryPopMessage(MessageWithHead&, std::size_t&);

		static const bool  consum_if_disconnected = false;

	private:
		// append 1 char and try to recogonize a completed message 
		void  Consum(char, boost::system::error_code&);

	private:
		MessageWithHead   m_Sample;
		std::string      m_strBuffer;
		uint16_t      m_usBodySize;
		std::deque<std::pair<MessageWithHead, std::size_t> > m_Messages;

		enum PARSE_STATE {
			PARSE_BEGIN,
			PARSE_HEAD,
			PARSE_BODY,
			PARSE_END
		}m_ParseState;

		std::size_t                 m_uCompleteSize;
	};
}

#include "impl/NetLibMessage.ipp"

#endif