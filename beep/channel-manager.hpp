/// \file  channel-manager.hpp
/// \brief Add and remove channels from a session
///
/// UNCLASSIFIED
#ifndef BEEP_CHANNEL_MANAGER_HEAD
#define BEEP_CHANNEL_MANAGER_HEAD 1

#include <cassert>
#include <stdexcept>
#include <istream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <iterator>
#include <utility>
#include <functional>
#include <limits>

#include <boost/function.hpp>
#include <boost/system/error_code.hpp>

#include "role.hpp"
#include "frame.hpp" // for core_message_types definition
#include "message.hpp"
#include "channel.hpp"
#include "channel-management-protocol.hpp"
#include "reply-code.hpp"

namespace beep {

#if 0
inline
boost::system::system_error
make_error(const message &msg)
{
	using std::runtime_error;
	using std::istringstream;
	using boost::system::error_code;
	using boost::system::system_error;

	if (msg.get_type() != ERR) {
		throw runtime_error("An error code cannot be created from the given message.");
	}
	cmp::error myError;
	istringstream strm(msg.get_content());
	if (strm >> myError) {
		const error_code ec(myError.code(), beep::beep_category);
		return system_error(ec, myError.description());
	} else {
		throw runtime_error("could not decode the error message.");
	}
}
#endif
class channel_manager {
public:
	channel_manager()
		: profiles_()
		, zero_(0)
		, chnum_()
		, guess_(0)
	{
		chnum_.insert(0u);
	}

	const channel &tuning_channel() const { return zero_; }
	channel &tuning_channel() { return zero_; }

	template <class Handler>
	void install_profile(const std::string &profile_uri, Handler handler)
	{
		using std::make_pair;
		const std::pair<prof_map::iterator, bool> result =
			profiles_.insert(make_pair(profile_uri, handler));
		if (!result.second) {
			throw std::runtime_error("The profile was not installed.");
		}
	}

	template <typename OutputIterator>
	OutputIterator get_profiles(OutputIterator out) const
	{
		using std::transform;
		return transform(profiles_.begin(), profiles_.end(), out, profile_pair_to_uri());
	}

	template <typename FwdIterator>
	void
	greeting_message(const FwdIterator first_profile, const FwdIterator last_profile,
					 message &greeting_msg) const
	{
#if 0
		using std::ostringstream;
		using std::copy;
		using std::back_inserter;

		cmp::greeting_message g_protocol;
		copy(first_profile, last_profile, back_inserter(g_protocol.profile_uris));
		greeting_msg = cmp::generate(g_protocol);
		//greeting_msg.set_mime(mime::beep_xml());
		greeting_msg.set_type(RPY);
#endif
	}

	/// \brief test if a channel is active
	///
	/// First, test if the channel number represents the tuning channel. If not,
	/// check if the number is currently in use.
	///
	/// \return True if the channel number is currently active.
	bool channel_in_use(const unsigned int channel) const
	{
		assert(zero_.get_number() == 0u);
		return (channel == zero_.get_number()) || chnum_.count(channel) > 0;
	}

	/// To avoid conflict in assigning channel numbers when requesting the
	/// creation of a channel, BEEP peers acting in the initiating role use
	/// only positive integers that are odd-numbered; similarly, BEEP peers
	/// acting in the listening role use only positive integers that are
	/// even-numbered.
	unsigned int start_channel(const role r, const std::string &name,
							   const std::string &profile_uri, message &msg)
	{
#if 0
		using std::ostringstream;
		if (!profiles_.count(profile_uri)) return 0;

		unsigned int number = guess_;
		if (!chnum_.insert(number).second) {
			number = get_next_channel(r);
			if (!chnum_.insert(number).second) {
				throw std::runtime_error("could not find a channel number!");
			}
		}
		msg.set_mime(mime::beep_xml());
		msg.set_type(MSG);
		cmp::start start;
		start.set_number(number);
		start.set_server_name(name);
		start.push_back_profile(profile_uri);
		ostringstream strm;
		strm << start;
		msg.set_content(strm.str());
		guess_ = number + 2;
		return number;
#else
		return 0;
#endif
	}

	void close_channel(const unsigned int number, const reply_code::rc_enum rc,
					   message &msg)
	{
#if 0
		using std::ostringstream;
		msg.set_mime(mime::beep_xml());
		if (number > 0 && !chnum_.erase(number)) {
			throw std::runtime_error("The requested channel was not in use.");
		}
		msg.set_type(MSG);
		cmp::close close;
		close.set_number(number);
		close.set_code(rc);
		ostringstream strm;
		strm << close;
		msg.set_content(strm.str());
#endif
	}

	/// \return Accepted channel number, zero indicates an error
	unsigned int accept_start(const cmp::start_message &start_msg, message &response)
	{
#if 0
		using std::istringstream;
		using std::ostringstream;
		using std::find;
		using std::copy;
		using std::ostream_iterator;
		using std::iterator_traits;

		unsigned int channel = 0;
		cmp::start start;
		istringstream strm(start_msg.get_content());
		ostringstream ostrm;
		response.set_mime(mime::beep_xml());
		if (strm >> start) {
			channel = start.number();
			bool match_profile = false;
			if (chnum_.insert(channel).second) {
				typedef cmp::start::profile_const_iterator const_iterator;
				for (const_iterator i = start.profiles_begin(); i != start.profiles_end() && !match_profile; ++i) {
					if (find(first_profile, last_profile, *i) != last_profile) {
						acceptedProfile = *i;
						match_profile = true;
					}
				}
				if (match_profile) {
					response.set_type(RPY);
					TiXmlElement aProfile("profile");
					aProfile.SetAttribute("uri", acceptedProfile.uri());
					/// \todo Set the profile "encoding" (if required/allowed)
					/// \todo add a "features" attribute for optional feature
					/// \todo add a "localize" attribute for each language token
					ostrm << aProfile;
				} else {
					chnum_.erase(channel);
					channel = 0;
					response.set_type(ERR);
					ostringstream estrm;
					estrm << "The specified profile(s) are not supported. "
						"This listener supports the following profiles: ";
					typedef typename iterator_traits<FwdIterator>::value_type value_type;
					copy(first_profile, last_profile, ostream_iterator<value_type>(estrm, ", "));
					cmp::error error(reply_code::requested_action_not_accepted,
									 estrm.str());
					ostrm << error;
				}
			} else {
				response.set_type(ERR);
				ostringstream estrm;
				estrm << "The requested channel (" << channel
					  << ") is already in use.";
				cmp::error error(reply_code::requested_action_not_accepted,
								 estrm.str());
				ostrm << error;
			}
		} else {
			response.set_type(ERR);
			cmp::error error(reply_code::general_syntax_error,
							 "The 'start' message could not be decoded.");
			ostrm << error;
		}
		response.set_content(ostrm.str());
		return channel;
#else
		return 0;
#endif
	}

	unsigned close_channel(const message &close_msg, message &response)
	{
#if 0
		using std::istringstream;
		using std::ostringstream;
		using std::numeric_limits;

		response.set_mime(mime::beep_xml());

		cmp::close close;
		istringstream strm(close_msg.get_content());
		if (!(strm >> close)) {
			response.set_type(ERR);
			cmp::error error(reply_code::general_syntax_error,
							 "The 'close' message could not be decoded.");
			ostringstream ostrm;
			ostrm << error;
			response.set_content(ostrm.str());
			return numeric_limits<unsigned>::max();
		}
		const unsigned ch_num = close.number();
		if ((ch_num == zero_.get_number()) || chnum_.erase(ch_num)) {
			response.set_type(RPY);
			cmp::ok ok;
			ostringstream ostrm;
			ostrm << ok;
			response.set_content(ostrm.str());
		} else {
			response.set_type(ERR);
			cmp::error error(reply_code::parameter_invalid,
							 "Channel closure failed because the referenced channel number is not recognized.");
			ostringstream ostrm;
			ostrm << error;
			response.set_content(ostrm.str());
			return numeric_limits<unsigned>::max();
		}
		return ch_num;
#else
		return 0;
#endif
	}
private:
	typedef std::set<unsigned int> ch_set;
	typedef boost::system::error_code error_code;
	typedef boost::function<void (const error_code&, unsigned, bool, const message&)> profile_callback_type;
	typedef std::map<std::string, profile_callback_type> prof_map;

	prof_map     profiles_;
	channel      zero_; ///< used for adding/removing subsequent channels
	ch_set       chnum_;
	unsigned int guess_; // guess at next channel number

	static unsigned int get_first_number(const role r)
	{
		unsigned int number = 0;
		switch (r) {
		case initiating_role:
			number = 1;
			break;
		case listening_role:
			number = 2;
			break;
		default:
			throw std::runtime_error("Unknown beep role in the channel manager");
		}
		return number;
	}

	unsigned int get_next_channel(const role r)
	{
		unsigned int number = 0;
		if (chnum_.empty()) {
			number = get_first_number(r);
		} else {
			number = *chnum_.rbegin();
			if (!number) {
				number = get_first_number(r);
			} else {
				number += 2;
			}
		}
		return number;
	}

	struct profile_pair_to_uri : public std::unary_function<prof_map::value_type, std::string> {
		std::string operator()(const prof_map::value_type &pair) const
		{
			return pair.first;
		}
	};
};

}      // namespace beep
#endif // BEEP_CHANNEL_MANAGER_HEAD
