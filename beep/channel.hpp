// \file  channel.hpp
/// \brief BEEP channel built on Boost ASIO
///
/// UNCLASSIFIED
#ifndef BEEP_CHANNEL_HEAD
#define BEEP_CHANNEL_HEAD 1

#include <string>

namespace beep {

class channel {
public:
	typedef std::string profile_type;

	channel();
	channel(const unsigned int c);
	channel(const unsigned int c, const profile_type &p);
	channel(const unsigned int c, const unsigned int m);
	channel(const channel& src);
	channel &operator=(const channel &src);

	/// The channel number ("channel") must be a non-negative integer (in the
	/// range 0..2147483647).
	unsigned int get_number() const { return num_; }

	void set_profile(const profile_type &p) { profile_ = p; }
	const profile_type &get_profile() const { return profile_; }

	/// The message number ("msgno") must be a non-negative integer (in the
	/// range 0..2147483647) and have a different value than all other "MSG"
	/// messages on the same channel for which a reply has not been
	/// completely received.
	unsigned int get_message_number() const { return msgno_; }

	/// The sequence number ("seqno") must be a non-negative integer (in the
	/// range 0..4294967295) and specifies the sequence number of the first
	/// octet in the payload, for the associated channel (c.f., Section
	/// 2.2.1.2).
	unsigned int get_sequence_number() const { return seqno_; }

	/// The answer number ("ansno") must be a non-negative integer (in the
	/// range 0..4294967295) and must have a different value than all other
	/// answers in progress for the message being replied to.
	unsigned int get_answer_number() const { return ansno_; }

	/// The frame payload consists of zero or more octets.
	///
	/// Every payload octet sent in each direction on a channel has an
	/// associated sequence number.  Numbering of payload octets within a
	/// frame is such that the first payload octet is the lowest numbered,
	/// and the following payload octets are numbered consecutively.  (When a
	/// channel is created, the sequence number associated with the first
	/// payload octet of the first frame is 0.)
	///
	/// The actual sequence number space is finite, though very large,
	/// ranging from 0..4294967295 (2**32 - 1).  Since the space is finite,
	/// all arithmetic dealing with sequence numbers is performed modulo
	/// 2**32.  This unsigned arithmetic preserves the relationship of
	/// sequence numbers as they cycle from 2**32 - 1 to 0 again.  Consult
	/// Sections 2 through 5 of [8] for a discussion of the arithmetic
	/// properties of sequence numbers.
	///
	/// When receiving a frame, the sum of its sequence number and payload
	/// size, modulo 4294967296 (2**32), gives the expected sequence number
	/// associated with the first payload octet of the next frame received.
	/// Accordingly, when receiving a frame if the sequence number isn't the
	/// expected value for this channel, then the BEEP peers have lost
	/// synchronization, then the session is terminated without generating a
	/// response, and it is recommended that a diagnostic entry be logged.
	void update(const size_t msg_size);
private:
	unsigned int num_;
	unsigned int msgno_;
	unsigned int seqno_;
	unsigned int ansno_;
	profile_type profile_; // URI identifier
};     // class channel

bool operator<(const channel &lhs, const channel &rhs);
bool operator!=(const channel &lhs, const channel &rhs);

}      // namespace beep
#endif // BEEP_CHANNEL_HEAD
