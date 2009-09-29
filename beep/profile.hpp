/// \file  profile.hpp
/// \brief BEEP profile
///
/// UNCLASSIFIED
#ifndef BEEP_PROFILE_HEAD
#define BEEP_PROFILE_HEAD 1

#include <string>
#include "message.hpp"

namespace beep {

class profile {
public:
	profile()
		: uri_()
		, initialization_()
		, has_initiailizer_(false)
	{
	}

	const std::string &get_uri() const { return uri_; }
	const std::string &uri() const { return uri_; }
	void set_uri(const std::string &u) { uri_ = u; }

	void set_initialization_message(const message &m)
	{
		initialization_ = m;
		has_initiailizer_ = true;
	}
private:
	std::string uri_;
	message     initialization_;
	bool        has_initiailizer_;
};     // class profile
}      // namespace beep
#endif // BEEP_PROFILE_HEAD
