// \file  channel.hpp
/// \brief BEEP channel built on Boost ASIO
///
/// UNCLASSIFIED
#ifndef BEEP_CHANNEL_HEAD
#define BEEP_CHANNEL_HEAD 1
namespace beep {

/// \brief Establish a new channel in the BEEP session
class channel {
public:
	channel()
		: num_(0)
		, msgno_(0)
		, seqno_(0)
		, profile_()
	{
	}

	channel(const channel& src)
		: num_(src.num_)
		, msgno_(src.msgno_)
		, seqno_(src.seqno_)
		, profile_(src.profile_)
	{
	}

	channel &operator=(const channel &src)
	{
		if (&src != this) {
			this->num_ = src.num_;
			this->msgno_ = src.msgno_;
			this->seqno_ = src.seqno_;
			this->profile_ = src.profile_;
		}
		return *this;
	}

	void set_profile(const string &p) { profile_ = p; }
	const string &profile() const { return profile_; }

	uint32_t number() const { return num_; }
	void set_number(uint32_t num) { num_ = num; }
	uint32_t next_message_number() const { return msgno_; }
	uint32_t next_sequence_number() const { return seqno_; }
	void increment_message_number() { ++msgno_; }
	void increase_sequence_number(const size_t octs) { seqno_ += octs; }

private:
	uint32_t                  num_, msgno_, seqno_;
	string                    profile_; // URI identifier
};     // class channel  

}      // namespace beep
#endif // BEEP_CHANNEL_HEAD
