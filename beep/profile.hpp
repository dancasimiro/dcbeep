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

	profile(const std::string &uri)
		: uri_(uri)
		, initialization_()
		, has_initiailizer_(false)
	{
	}

	virtual ~profile() {}

	const std::string &uri() const { return uri_; }
	void set_uri(const std::string &u) { uri_ = u; }

	void set_initialization_message(const message &m)
	{
		initialization_ = m;
		has_initiailizer_ = true;
	}

	bool has_initialization_message() const { return has_initiailizer_; }
	const message &initial_message() const { return initialization_; }
private:
	std::string uri_;
	message     initialization_;
	bool        has_initiailizer_;
};     // class profile

bool operator==(const profile &lhs, const profile &rhs)
{
	return lhs.uri() == rhs.uri();
}

}      // namespace beep
#endif // BEEP_PROFILE_HEAD
