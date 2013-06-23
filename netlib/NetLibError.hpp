#ifndef NETLIBERROR_HPP
#define NETLIBERROR_HPP
#include <boost/system/error_code.hpp>

namespace netlib
{
	namespace error
	{
		enum head_message_errors
		{
			invalid_parse_status   = 999
		};

		namespace detail
		{
			class head_message_category : public boost::system::error_category
			{
			public:
				virtual const char* name () const
				{
					return "htnetlib_head_message";
				}

				virtual std::string message(int ev) const
				{
					switch (ev) {
			case invalid_parse_status:
				return "invalid parsing state of head message trait.";
			default:
				return "unknown error.";        
					}
				}
			};
		}

		inline boost::system::error_category& get_head_message_category()
		{
			static detail::head_message_category category;
			return category;
		}
	}
}

namespace boost
{
	namespace system
	{
		template<>
		struct is_error_code_enum<netlib::error::head_message_errors>
		{
			static const bool value = true;
		};
	}
}

namespace netlib
{
	namespace error
	{
		inline boost::system::error_code make_error_code(head_message_errors error) {
			return boost::system::error_code(error, get_head_message_category());
		}
	}
}

#endif