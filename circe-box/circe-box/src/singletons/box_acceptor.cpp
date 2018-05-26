// 这个文件是 Circe 服务器应用程序框架的一部分。
// Copyleft 2017 - 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "box_acceptor.hpp"
#include "servlet_container.hpp"
#include "common/interserver_acceptor.hpp"
#include "protocol/error_codes.hpp"
#include "../mmain.hpp"

namespace Circe {
namespace Box {

namespace {
	class Specialized_acceptor : public Common::Interserver_acceptor {
	public:
		Specialized_acceptor(std::string bind, boost::uint16_t port, std::string application_key)
			: Common::Interserver_acceptor(STD_MOVE(bind), port, STD_MOVE(application_key))
		{
			//
		}

	protected:
		boost::shared_ptr<const Common::Interserver_servlet_callback> sync_get_servlet(boost::uint16_t message_id) const OVERRIDE {
			return Servlet_container::get_servlet(message_id);
		}
	};

	Poseidon::Mutex g_mutex;
	boost::weak_ptr<Specialized_acceptor> g_weak_acceptor;
}

MODULE_RAII_PRIORITY(handles, INIT_PRIORITY_LOW){
	const Poseidon::Mutex::Unique_lock lock(g_mutex);
	const AUTO(bind, get_config<std::string>("box_acceptor_bind"));
	const AUTO(port, get_config<boost::uint16_t>("box_acceptor_port"));
	const AUTO(appkey, get_config<std::string>("box_acceptor_appkey"));
	LOG_CIRCE_INFO("Initializing Box_acceptor...");
	const AUTO(acceptor, boost::make_shared<Specialized_acceptor>(bind, port, appkey));
	acceptor->activate();
	handles.push(acceptor);
	g_weak_acceptor = acceptor;
}

boost::shared_ptr<Common::Interserver_connection> Box_acceptor::get_session(const Poseidon::Uuid &connection_uuid){
	PROFILE_ME;

	const Poseidon::Mutex::Unique_lock lock(g_mutex);
	const AUTO(acceptor, g_weak_acceptor.lock());
	if(!acceptor){
		LOG_CIRCE_WARNING("Box_acceptor has not been initialized.");
		return VAL_INIT;
	}
	return acceptor->get_session(connection_uuid);
}
std::size_t Box_acceptor::get_all_sessions(boost::container::vector<boost::shared_ptr<Common::Interserver_connection> > &sessions_ret){
	PROFILE_ME;

	const Poseidon::Mutex::Unique_lock lock(g_mutex);
	const AUTO(acceptor, g_weak_acceptor.lock());
	if(!acceptor){
		LOG_CIRCE_WARNING("Box_acceptor has not been initialized.");
		return 0;
	}
	return acceptor->get_all_sessions(sessions_ret);
}
std::size_t Box_acceptor::safe_broadcast_notification(const Poseidon::Cbpp::Message_base &msg) NOEXCEPT {
	PROFILE_ME;

	const Poseidon::Mutex::Unique_lock lock(g_mutex);
	const AUTO(acceptor, g_weak_acceptor.lock());
	if(!acceptor){
		LOG_CIRCE_WARNING("Box_acceptor has not been initialized.");
		return 0;
	}
	return acceptor->safe_broadcast_notification(msg);
}
std::size_t Box_acceptor::clear(long err_code, const char *err_msg) NOEXCEPT {
	PROFILE_ME;

	const Poseidon::Mutex::Unique_lock lock(g_mutex);
	const AUTO(acceptor, g_weak_acceptor.lock());
	if(!acceptor){
		LOG_CIRCE_WARNING("Box_acceptor has not been initialized.");
		return 0;
	}
	return acceptor->clear(err_code, err_msg);
}

}
}