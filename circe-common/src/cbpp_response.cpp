// 这个文件是 Circe 服务器应用程序框架的一部分。
// Copyleft 2017, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "cbpp_response.hpp"

namespace Circe {
namespace Common {

CbppResponse::CbppResponse(long err_code, std::string err_msg)
	: m_err_code(err_code), m_err_msg(STD_MOVE(err_msg))
{
	LOG_CIRCE_DEBUG("CbppResponse constructor: err_code = ", get_err_code(), ", err_msg = ", get_err_msg());
}
CbppResponse::~CbppResponse(){
	LOG_CIRCE_DEBUG("CbppResponse destructor: err_code = ", get_err_code(), ", err_msg = ", get_err_msg());
}

}
}