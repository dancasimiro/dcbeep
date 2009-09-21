/// \file  profile.hpp
/// \brief BEEP profile
///
/// UNCLASSIFIED
#ifndef BEEP_PROFILE_HEAD
#define BEEP_PROFILE_HEAD 1
namespace beep {

class profile {
public:
	profile()
		: uri_()
		, encoding_()
		, content_()
	{
	}

	profile(const string &uri)
		: uri_(uri)
		, encoding_()
		, content_()
	{
	}

	virtual ~profile()
	{
	}

	const string &get_uri() const { return uri_; }
	void set_uri(const string &n) { uri_ = n; }

	/// \brief Does this profile need some sort of initialization?
	/// \param msg (out) Insert the initialization message here
	/// \return True if this profile sends an initialization message
	bool initialize(message &msg)
	{ return do_initialize(msg); }

private:
	string                    uri_;
	string                    encoding_;
	string                    content_;

	virtual bool do_initialize(message &msg) = 0;
};     // class profile

}      // namespace beep
#endif // BEEP_PROFILE_HEAD
