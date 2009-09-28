/// \file  channel-manager.hpp
/// \brief Add and remove channels from a session
///
/// UNCLASSIFIED
#ifndef BEEP_CHANNEL_MANAGER_HEAD
#define BEEP_CHANNEL_MANAGER_HEAD 1

#include <istream>
#include <sstream>
#include <string>

#include "message.hpp"
#include "channel.hpp"

#include "tinyxml.h"

namespace beep {

namespace profile {
namespace channel_management {

class greeting {
public:
};

}      // namespace channel_management
}      // namespace profile
}      // namespace beep

namespace std {

// for now, always use tinyxml to encode the greeting
ostream&
operator<<(ostream &strm,
		   const beep::profile::channel_management::greeting&)
{
	if (strm) {
		TiXmlElement root("greeting");
		/// \todo add a "profile" element for each profile in g
		/// \todo add a "features" attribute for optional feature
		/// \todo add a "localize" attribute for each language token
		strm << root;
	}
	return strm;
}

}      // namespace std

namespace beep {

class channel_manager {
public:

	channel_manager()
		: zero_(0)
		, greeting_()
	{
		using std::ostringstream;

		greeting_.set_mime(mime::beep_xml());
		greeting_.set_type(message::RPY);
		profile::channel_management::greeting g;
		ostringstream strm;
		strm << g;
		greeting_.set_content(strm.str());
	}

	const channel &tuning_channel() const { return zero_; }
	channel &tuning_channel() { return zero_; }

	const message &greeting() const { return greeting_; }
private:
	channel zero_; ///< used for adding/removing subsequent channels
	message greeting_;
};

}      // namespace beep
#endif // BEEP_CHANNEL_MANAGER_HEAD
