// \file  channel.hpp
/// \brief BEEP channel built on Boost ASIO
///
/// UNCLASSIFIED
#ifndef BEEP_CHANNEL_HEAD
#define BEEP_CHANNEL_HEAD 1
namespace beep {

namespace detail {

}      // namespace detail

/// \brief Establish a new channel in the BEEP session
template <class SessionType>
class basic_channel {
public:
	typedef SessionType                                   session_type;
	typedef session_type&                                 session_reference;
	typedef shared_ptr<profile>                           profile_pointer;
	typedef typename session_type::connection_type        connection_type;
	typedef typename session_type::connection_reference   connection_reference;

	basic_channel()
		: num_(0)
		, msgno_(0)
		, seqno_(0)
		, profile_()
	{
	}

	basic_channel(session_reference mySession)
		: num_(mySession.next_channel_number())
		, msgno_(0)
		, seqno_(0)
		, profile_()
	{
	}

	basic_channel(const basic_channel& src)
		: num_(src.num_)
		, msgno_(src.msgno_)
		, seqno_(src.seqno_)
		, profile_(src.profile_)
	{
	}

	basic_channel &operator=(const basic_channel &src)
	{
		if (&src != this) {
			this->num_ = src.num_;
			this->msgno_ = src.msgno_;
			this->seqno_ = src.seqno_;
			this->profile_ = src.profile_;
		}
		return *this;
	}

	void set_profile(profile_pointer pp)
	{
		profile_ = pp;
	}
	profile_pointer get_profile() const { return profile_; }

	uint32_t number() const { return num_; }
	void set_number(uint32_t num) { num_ = num; }
	uint32_t next_message_number() const { return msgno_; }
	uint32_t next_sequence_number() const { return seqno_; }
	void increment_message_number() { ++msgno_; }
	void increase_sequence_number(const size_t octs) { seqno_ += octs; }

private:
	uint32_t                  num_, msgno_, seqno_;
	profile_pointer           profile_;  // active profile
};     // class basic_channel  

}      // namespace beep
#endif // BEEP_CHANNEL_HEAD
