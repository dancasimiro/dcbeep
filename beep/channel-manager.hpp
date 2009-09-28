/// \file  channel-manager.hpp
/// \brief Add and remove channels from a session
///
/// UNCLASSIFIED
#ifndef BEEP_CHANNEL_MANAGER_HEAD
#define BEEP_CHANNEL_MANAGER_HEAD 1

#include <string>
#include "message.hpp"
#include "channel.hpp"

namespace beep {

class channel_manager {
public:

	channel_manager()
		: zero_(0)
		, greeting_()
	{
		greeting_.set_mime(mime::beep_xml());
		greeting_.set_type(message::RPY);
		greeting_.set_content("<greeting />");
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
