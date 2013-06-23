#ifndef NETLIBMESSAGE_IPP
#define NETLIBMESSAGE_IPP
#include "NetLibError.hpp"

namespace netlib
{
	inline MessageWithHead::MessageWithHead()
		:m_MessageId(0)
	{
	}

	inline MessageWithHead::MessageWithHead(uint16_t MessageId, const std::string& strBody)
		:m_MessageId(MessageId)
		,m_strBody(strBody)
	{
	}

	inline MessageWithHead::MessageWithHead(uint16_t MessageId, const char* pBody, std::size_t uSize)
		:m_MessageId(MessageId)
		,m_strBody(pBody, uSize)
	{
	}

	inline MessageWithHead::MessageWithHead(uint16_t MessageId, const char* pBody)
		:m_MessageId(MessageId)
		,m_strBody(pBody)
	{
	}

	inline void  MessageWithHead::GetSerializedString(std::string& strTemp) const
	{
		uint16_t bodySize = m_strBody.size();
		if (bodySize > ((std::numeric_limits<uint16_t>::max)() - 4))
			throw MessageTooLargeException();

		strTemp.reserve(bodySize + 4);

		char szTemp[4];

		uint16_t NetMessageId = ::htons(m_MessageId);
		uint16_t NetBodySize  = ::htons(bodySize);

		memcpy(szTemp, &NetMessageId, 2);
		memcpy(szTemp+2, &NetBodySize, 2);

		strTemp.assign(szTemp, sizeof(szTemp));
		strTemp.append(m_strBody);
	}

	inline void   MessageTrait<MessageWithHead>::Serialize(const MessageWithHead& message, std::string& strBuffer)
	{
		return message.GetSerializedString(strBuffer);
	}

	inline std::string MessageTrait<MessageWithHead>::InstanceId(const MessageWithHead& message)
	{
		char szTemp[8];
		snprintf(szTemp, sizeof(szTemp), "%04x", message.GetMessageId());

		return std::string(szTemp, 4);
	}

	inline void   MessageTrait<MessageWithHead>::Consum(const void* buffer, std::size_t uSize, boost::system::error_code& ec) {
		ec = boost::system::error_code();

		const char* bufferTmp = static_cast<const char*>(buffer);
		std::size_t consumBytes = 1;
		while (consumBytes++ <= uSize && !ec)
			Consum(*bufferTmp++, ec);
	}

	inline bool  MessageTrait<MessageWithHead>::TryPopMessage(MessageWithHead& message, std::size_t& uSize)
	{
		if (m_Messages.empty())
			return false;

		message = m_Messages.front().first;
		uSize   = m_Messages.front().second;
		m_Messages.pop_front();
		return true;
	}

	inline void        MessageTrait<MessageWithHead>::Consum(char chInput, boost::system::error_code& ec) {   
		if (m_ParseState == PARSE_BEGIN)
		{
			m_strBuffer.clear();
			m_strBuffer.push_back(chInput);
			m_ParseState = PARSE_HEAD;
			m_uCompleteSize = 0;
			return;
		}

		m_uCompleteSize++;

		if (m_ParseState == PARSE_HEAD)
		{
			m_strBuffer.push_back(chInput);

			if (m_strBuffer.size() == 2)
			{
				 uint16_t usMessageId = 0;
				 memcpy(&usMessageId, m_strBuffer.c_str(), 2);
				 m_Sample.SetMessageId(::ntohs(usMessageId));
			}

			if (m_strBuffer.size() == 4)
			{
				 memcpy(&m_usBodySize, m_strBuffer.c_str() + 2, 2);
				 m_usBodySize = ::ntohs(m_usBodySize);
				 m_ParseState = PARSE_BODY;

				 if (m_usBodySize == 0)
				 {
					 m_ParseState = PARSE_BEGIN;
					 m_Sample.GetMessageBody().clear();
					 m_Messages.push_back(std::make_pair(m_Sample, m_uCompleteSize));
					 return;
				 }

				 return;
			}

			return;
		}

		if (m_ParseState == PARSE_BODY) 
		{
			m_strBuffer.push_back(chInput);

			if (m_strBuffer.size() == m_usBodySize + 4)
			{
				 m_ParseState = PARSE_BEGIN;
				 m_Sample.GetMessageBody().assign(m_strBuffer.c_str()+4, m_usBodySize);
				 m_Messages.push_back(std::make_pair(m_Sample, m_uCompleteSize));
				 return;
			}
	        
			return;
		}

		m_uCompleteSize--;
		ec = error::make_error_code(error::invalid_parse_status);
	}
}
 
#endif