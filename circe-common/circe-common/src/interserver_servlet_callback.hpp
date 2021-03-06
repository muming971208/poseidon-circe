// 这个文件是 Circe 服务器应用程序框架的一部分。
// Copyleft 2017 - 2018, LH_Mouse. All wrongs reserved.

#ifndef CIRCE_COMMON_INTERSERVER_SERVLET_CALLBACK_HPP_
#define CIRCE_COMMON_INTERSERVER_SERVLET_CALLBACK_HPP_

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <poseidon/stream_buffer.hpp>
#include "interserver_response.hpp"

namespace Circe {
namespace Common {

class Interserver_connection;

typedef boost::function<
	Interserver_response (const boost::shared_ptr<Interserver_connection> &connection, boost::uint16_t message_id, Poseidon::Stream_buffer payload)
	> Interserver_servlet_callback;

}
}

#endif
