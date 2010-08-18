// \file  channel.cpp
/// \brief BEEP channel built on Boost ASIO
///
/// UNCLASSIFIED
#if HAVE_CONFIG_H
# include "config.h"
#endif
#include "channel.hpp"

namespace beep {

channel::channel()
	: num_(0)
	, msgno_(0)
	, seqno_(0)
	, ansno_(0)
	, profile_()
{
}

channel::channel(const unsigned int c)
	: num_(c)
	, msgno_(0)
	, seqno_(0)
	, ansno_(0)
	, profile_()
{
}

channel::channel(const unsigned int c, const profile_type &p)
	: num_(c)
	, msgno_(0)
	, seqno_(0)
	, ansno_(0)
	, profile_(p)
{
}

channel::channel(const unsigned int c, const unsigned int m)
	: num_(c)
	, msgno_(m)
	, seqno_(0)
	, ansno_(0)
	, profile_()
{
}

channel::channel(const channel& src)
	: num_(src.num_)
	, msgno_(src.msgno_)
	, seqno_(src.seqno_)
	, ansno_(src.ansno_)
	, profile_(src.profile_)
{
}

channel&
channel::operator=(const channel &src)
{
	if (&src != this) {
		this->num_ = src.num_;
		this->msgno_ = src.msgno_;
		this->seqno_ = src.seqno_;
		this->ansno_ = src.ansno_;
		this->profile_ = src.profile_;
	}
	return *this;
}

void
channel::update(const size_t msg_size)
{
	msgno_ += 1;
	msgno_ %= 2147483647u;

	// maximum value of seqno is < 4294967296
	//seqno_ = (seqno_ + msg.payload_size()) % 4294967296ll;
	seqno_ += msg_size;
}

bool
operator<(const channel &lhs, const channel &rhs)
{
	return
		lhs.get_number() < rhs.get_number()
		&& lhs.get_message_number() < rhs.get_message_number()
		&& lhs.get_answer_number() < rhs.get_answer_number()
		;
}

bool operator!=(const channel &lhs, const channel &rhs)
{
	return
		lhs.get_number() != rhs.get_number()
		|| lhs.get_message_number() != rhs.get_message_number()
		|| lhs.get_answer_number() != rhs.get_answer_number()
		;
}

} // namespace beep
