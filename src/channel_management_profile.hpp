/// \file  channel_management_profile.hpp
/// \brief BEEP channel management interface
///
/// UNCLASSIFIED
#ifndef BEEP_CHANNEL_MANAGEMENT_PROFILE_HEAD
#define BEEP_CHANNEL_MANAGEMENT_PROFILE_HEAD 1
namespace beep {

/// \brief Initial tuning profile
///
/// \note No profile identification is defined for this profile
class channel_management_profile : public profile {
public:
	channel_management_profile()
		: profile()
		, header_("Content-Type: application/beep+xml\r\n")
		, greeting_("<greeting />\r\n")
	{
		/// \todo Add supported profiles (if any)
	}

	virtual ~channel_management_profile()
	{
	}

	template <class ChannelType>
	bool add_channel(const ChannelType &channel, ostream &strm)
	{
		strm << "<start number='" << channel.number() << "'>\r\n"
			 << "\t<profile uri='" << channel.get_profile()->get_uri()
			 << "' />\r\n</start>\r\n";
		return strm;
	}

	void init_add_message(const string &myStr, message &msg)
	{
		msg.set_type(frame::msg);
		msg.add_header(buffer(header_));
		msg.add_content(buffer(myStr));
	}

private:
	string header_;
	string greeting_;

	virtual bool do_initialize(message &msg)
	{
		msg.set_type(frame::rpy);
		msg.add_header(buffer(header_));
		msg.add_content(buffer(greeting_));
		return true;
	}
};     // class channel_management_profile

}
#endif // BEEP_CHANNEL_MANAGEMENT_PROFILE_HEAD
