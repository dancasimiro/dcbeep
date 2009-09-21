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
		, greeting_()
		, profiles_()
	{
		/// \todo Add supported profiles (if any)
	}

	virtual ~channel_management_profile()
	{
	}
	
	void add_profile(const string &prof)
	{
		profiles_.push_back(prof);
	}

	ostream& add_channel(const channel &chan, ostream &strm) const
	{
		strm << "<start number='" << chan.number() << "'>\r\n"
			 << "\t<profile uri='" << chan.profile()
			 << "' />\r\n</start>\r\n";
		return strm;
	}

	ostream& remove_channel(const channel &chan, ostream &strm) const
	{
		strm << "<close number='" << chan.number() << "' code='200' />\r\n";
		return strm;
	}

	void make_message(frame::frame_type t, const string &myStr, message &msg) const
	{
		msg.set_type(t);
		msg.add_header(buffer(header_));
		msg.add_content(buffer(myStr));
	}


	ostream& accept_profile(const string &uri, ostream &strm) const
	{
		strm << "<profile uri='" << uri << "' />\r\n";
		return strm;
	}

	ostream& acknowledge(ostream &strm) const
	{
		strm << "<ok />\r\n";
		return strm;
	}

	ostream& encode_error(const int rc, ostream &strm) const
	{
		/// \todo the error code must be 3 digits...
		strm << "<error code='" << rc << "'>"
			 << "details are coming soon..."
			 << "</error>\r\n";
		return strm;
	}

private:
	typedef list<string>                profile_container;

	string                    header_;
	string                    greeting_;
	profile_container         profiles_;

	virtual bool do_initialize(message &msg)
	{
		ostringstream strm;
		strm << "<greeting";
		if (profiles_.empty()) {
			strm << " /> \r\n";
		} else {
			strm << ">\n";
			typedef profile_container::const_iterator const_iterator;
			for (const_iterator i = profiles_.begin(); i != profiles_.end(); ++i) {
				strm << "\t<profile uri='" << *i << "' />\n";
			}
			strm << "</greeting>\r\n";
		}
		greeting_ = strm.str();
		msg.set_type(frame::rpy);
		msg.add_header(buffer(header_));
		msg.add_content(buffer(greeting_));
		return true;
	}
};     // class channel_management_profile

}
#endif // BEEP_CHANNEL_MANAGEMENT_PROFILE_HEAD
