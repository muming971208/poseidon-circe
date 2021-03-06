// 这个文件是 Circe 服务器应用程序框架的一部分。
// Copyleft 2017 - 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "singletons/servlet_container.hpp"
#include "common/interserver_connection.hpp"
#include "common/define_interserver_servlet_for.hpp"
#include "protocol/exception.hpp"
#include "protocol/messages_pilot.hpp"
#include "singletons/compass_repository.hpp"

#define DEFINE_SERVLET_FOR(...)   CIRCE_DEFINE_INTERSERVER_SERVLET_FOR(::Circe::Pilot::Servlet_container::insert_servlet, __VA_ARGS__)

namespace Circe {
namespace Pilot {

namespace {
	void parse_lock_disposition(int *shared_lock_disposition_ret, int *exclusive_lock_disposition_ret, const boost::shared_ptr<Compass> &compass, const boost::shared_ptr<Common::Interserver_connection> &connection, unsigned lock_disposition){
		POSEIDON_PROFILE_ME;

		int shared_lock_disposition;
		int exclusive_lock_disposition;
		switch(lock_disposition){
		case Protocol::Pilot::lock_leave_alone:
			shared_lock_disposition = 0;
			exclusive_lock_disposition = 0;
			break;
		case Protocol::Pilot::lock_try_acquire_shared:
			shared_lock_disposition = 1;
			exclusive_lock_disposition = 0;
			break;
		case Protocol::Pilot::lock_try_acquire_exclusive:
			shared_lock_disposition = 0;
			exclusive_lock_disposition = 1;
			break;
		case Protocol::Pilot::lock_release_shared:
			shared_lock_disposition = -1;
			exclusive_lock_disposition = 0;
			break;
		case Protocol::Pilot::lock_release_exclusive:
			shared_lock_disposition = 0;
			exclusive_lock_disposition = -1;
			break;
		default:
			CIRCE_LOG_ERROR("Unknown lock_disposition: ", lock_disposition);
			POSEIDON_THROW(Poseidon::Exception, Poseidon::Rcnts::view("Unknown lock_disposition"));
		}
		if(shared_lock_disposition < 0){
			POSEIDON_THROW_ASSERT(compass->is_locked_shared_by(connection->get_connection_uuid()));
		}
		if(exclusive_lock_disposition < 0){
			POSEIDON_THROW_ASSERT(compass->is_locked_exclusive_by(connection->get_connection_uuid()));
		}
		*shared_lock_disposition_ret = shared_lock_disposition;
		*exclusive_lock_disposition_ret = exclusive_lock_disposition;
	}
	void commit_lock_disposition(const boost::shared_ptr<Compass> &compass, const boost::shared_ptr<Common::Interserver_connection> &connection, int shared_lock_disposition, int exclusive_lock_disposition){
		POSEIDON_PROFILE_ME;

		for(int i = exclusive_lock_disposition; i > 0; --i){
			POSEIDON_THROW_ASSERT(compass->try_lock_exclusive(connection));
		}
		for(int i = shared_lock_disposition; i > 0; --i){
			POSEIDON_THROW_ASSERT(compass->try_lock_shared(connection));
		}

		for(int i = exclusive_lock_disposition; i < 0; ++i){
			POSEIDON_THROW_ASSERT(compass->release_lock_exclusive(connection));
		}
		for(int i = shared_lock_disposition; i < 0; ++i){
			POSEIDON_THROW_ASSERT(compass->release_lock_shared(connection));
		}
	}

	unsigned get_compass_lock_state(const boost::shared_ptr<Compass> &compass, const boost::shared_ptr<Common::Interserver_connection> &connection){
		POSEIDON_PROFILE_ME;

		if(compass->is_locked_exclusive_by(connection->get_connection_uuid())){
			return Protocol::Pilot::lock_exclusive_by_me;
		} else if(compass->is_locked_exclusive()){
			return Protocol::Pilot::lock_exclusive_by_others;
		} else if(compass->is_locked_shared_by(connection->get_connection_uuid())){
			return Protocol::Pilot::lock_shared_by_me;
		} else if(compass->is_locked_shared()){
			return Protocol::Pilot::lock_shared_by_others;
		} else {
			return Protocol::Pilot::lock_free_for_acquisition;
		}
	}
}

DEFINE_SERVLET_FOR(Protocol::Pilot::Compare_exchange_request, connection, req){
	const AUTO(compass, Compass_repository::open_compass(Compass_key::from_hash_of(req.key.data(), req.key.size())));
	POSEIDON_THROW_ASSERT(compass);
	CIRCE_LOG_DEBUG("Opened compass: ", compass->get_compass_key());

	int shared_lock_disposition;
	int exclusive_lock_disposition;
	parse_lock_disposition(&shared_lock_disposition, &exclusive_lock_disposition, compass, connection, boost::numeric_cast<unsigned>(req.lock_disposition));

	std::string value_old = compass->get_value();
	boost::uint64_t version_old = compass->get_version();
	bool succeeded = false;
	std::size_t criterion_index = 0;
	unsigned lock_state = Protocol::Pilot::lock_free_for_acquisition;

	// Search for the first match.
	while((criterion_index < req.criteria.size()) && (req.criteria.at(criterion_index).value_cmp != value_old)){
		++criterion_index;
	}
	CIRCE_LOG_DEBUG("Compass comparison result: criterion_index = ", criterion_index);

	bool locked;
	if(!req.criteria.empty() && (criterion_index >= req.criteria.size())){
		// Don't lock the value if the comparison fails.
		locked = false;
	} else if(req.criteria.empty() && (exclusive_lock_disposition <= 0)){
		// Ask for shared locking if the value isn't to be modified.
		locked = compass->try_lock_shared(connection);
		shared_lock_disposition -= locked;
	} else {
		// Otherwise, ask for exclusive locking.
		locked = compass->try_lock_exclusive(connection);
		exclusive_lock_disposition -= locked;
	}
	// Update the value only if the value has been locked successfully.
	if(locked){
		if(criterion_index < req.criteria.size()){
			compass->set_value(STD_MOVE(req.criteria.at(criterion_index).value_new));
		}
		commit_lock_disposition(compass, connection, shared_lock_disposition, exclusive_lock_disposition);
		succeeded = true;
	}
	lock_state = get_compass_lock_state(compass, connection);
	compass->update_last_access_time();

	Protocol::Pilot::Compare_exchange_response resp;
	resp.value_old       = STD_MOVE(value_old);
	resp.version_old     = version_old;
	resp.succeeded       = succeeded;
	resp.criterion_index = criterion_index;
	resp.lock_state      = lock_state;
	return resp;
}

DEFINE_SERVLET_FOR(Protocol::Pilot::Exchange_request, connection, req){
	const AUTO(compass, Compass_repository::open_compass(Compass_key::from_hash_of(req.key.data(), req.key.size())));
	POSEIDON_THROW_ASSERT(compass);
	CIRCE_LOG_DEBUG("Opened compass: ", compass->get_compass_key());

	int shared_lock_disposition;
	int exclusive_lock_disposition;
	parse_lock_disposition(&shared_lock_disposition, &exclusive_lock_disposition, compass, connection, boost::numeric_cast<unsigned>(req.lock_disposition));

	std::string value_old = compass->get_value();
	boost::uint64_t version_old = compass->get_version();
	bool succeeded = false;
	unsigned lock_state = Protocol::Pilot::lock_free_for_acquisition;

	// Ask for exclusive locking.
	bool locked = compass->try_lock_exclusive(connection);
	exclusive_lock_disposition -= locked;
	// Update the value only if the value has been locked successfully.
	if(locked){
		// Update the value safely.
		compass->set_value(STD_MOVE(req.value_new));
		commit_lock_disposition(compass, connection, shared_lock_disposition, exclusive_lock_disposition);
		succeeded = true;
	}
	lock_state = get_compass_lock_state(compass, connection);
	compass->update_last_access_time();

	Protocol::Pilot::Exchange_response resp;
	resp.value_old   = STD_MOVE(value_old);
	resp.version_old = version_old;
	resp.succeeded   = succeeded;
	resp.lock_state  = lock_state;
	return resp;
}

DEFINE_SERVLET_FOR(Protocol::Pilot::Add_watch_request, connection, req){
	const AUTO(compass, Compass_repository::open_compass(Compass_key::from_hash_of(req.key.data(), req.key.size())));
	POSEIDON_THROW_ASSERT(compass);
	CIRCE_LOG_DEBUG("Opened compass: ", compass->get_compass_key());

	const AUTO(watcher_uuid, compass->add_watcher(connection));

	Protocol::Pilot::Add_watch_response resp;
	resp.watcher_uuid = watcher_uuid;
	return resp;
}

DEFINE_SERVLET_FOR(Protocol::Pilot::Remove_watch_notification, /*connection*/, ntfy){
	const AUTO(compass, Compass_repository::open_compass(Compass_key::from_hash_of(ntfy.key.data(), ntfy.key.size())));
	POSEIDON_THROW_ASSERT(compass);
	CIRCE_LOG_DEBUG("Opened compass: ", compass->get_compass_key());

	const AUTO(watcher_uuid, Poseidon::Uuid(ntfy.watcher_uuid));
	compass->remove_watcher(watcher_uuid);

	return Protocol::error_success;
}

}
}
