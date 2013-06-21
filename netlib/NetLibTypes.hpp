#ifndef NETLIBTYPES_H
#define NETLIBTYPES_H

#include <boost/cstdint.hpp>

namespace netlib
{
	using boost::uint64_t;
	using boost::uint32_t;
	using boost::uint16_t;
	using boost::uint8_t;

	using boost::int64_t;
	using boost::int32_t;
	using boost::int16_t;
	using boost::int8_t;

	#define NL_BREAK_IF(cond) if (cond) break;
	#define NL_BREAK_IF_DO(cond, stat) if (cond) { stat; break; }
}

#endif