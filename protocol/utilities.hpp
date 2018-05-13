// 这个文件是 Circe 服务器应用程序框架的一部分。
// Copyleft 2017 - 2018, LH_Mouse. All wrongs reserved.

#ifndef CIRCE_PROTOCOL_UTILITIES_HPP_
#define CIRCE_PROTOCOL_UTILITIES_HPP_

#include <poseidon/cxx_ver.hpp>
#include <poseidon/option_map.hpp>
#include <boost/container/vector.hpp>

namespace Circe {
namespace Protocol {

template<typename DestinationT>
void copy_key_values(boost::container::vector<DestinationT> &dst, const Poseidon::Option_map &src){
	dst.reserve(dst.size() + src.size());
	for(AUTO(it, src.begin()); it != src.end(); ++it){
		dst.emplace_back();
		dst.back().key = it->first.get();
		dst.back().value = it->second;
	}
}
#ifdef POSEIDON_CXX11
template<typename DestinationT>
void copy_key_values(boost::container::vector<DestinationT> &dst, Poseidon::Option_map &&src){
	dst.reserve(dst.size() + src.size());
	for(AUTO(it, src.begin()); it != src.end(); ++it){
		dst.emplace_back();
		dst.back().key = it->first.get();
		dst.back().value = STD_MOVE(it->second);
	}
}
#endif

template<typename SourceT>
void copy_key_values(Poseidon::Option_map &dst, const boost::container::vector<SourceT> &src){
	for(AUTO(it, src.begin()); it != src.end(); ++it){
		dst.append(Poseidon::Rcnts(it->key), it->value);
	}
}
#ifdef POSEIDON_CXX11
template<typename SourceT>
void copy_key_values(Poseidon::Option_map &dst, boost::container::vector<SourceT> &&src){
	for(AUTO(it, src.begin()); it != src.end(); ++it){
		dst.append(Poseidon::Rcnts(it->key), STD_MOVE(it->value));
	}
}
#endif

template<typename DestinationT, typename SourceT>
void copy_key_values(boost::container::vector<DestinationT> &dst, const boost::container::vector<SourceT> &src){
	dst.reserve(dst.size() + src.size());
	for(AUTO(it, src.begin()); it != src.end(); ++it){
		dst.emplace_back();
		dst.back().key = it->key;
		dst.back().value = it->value;
	}
}
#ifdef POSEIDON_CXX11
template<typename DestinationT, typename SourceT>
void copy_key_values(boost::container::vector<DestinationT> &dst, boost::container::vector<SourceT> &&src){
	dst.reserve(dst.size() + src.size());
	for(AUTO(it, src.begin()); it != src.end(); ++it){
		dst.emplace_back();
		dst.back().key = STD_MOVE(it->key);
		dst.back().value = STD_MOVE(it->value);
	}
}
#endif

template<typename SourceT>
Poseidon::Option_map copy_key_values(const boost::container::vector<SourceT> &src){
	Poseidon::Option_map dst;
	copy_key_values<>(dst, src);
	return dst;
}
#ifdef POSEIDON_CXX11
template<typename SourceT>
Poseidon::Option_map copy_key_values(boost::container::vector<SourceT> &&src){
	Poseidon::Option_map dst;
	copy_key_values<>(dst, STD_MOVE(src));
	return dst;
}
#endif

template<typename ContainerT>
typename ContainerT::iterator emplace_at_end(ContainerT &dst){
	return dst.emplace(dst.end());
}

}
}

#endif
