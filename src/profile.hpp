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

	// Ideally, I want a signature with ASIO details
	void on_init_transmitted(const boost::system::error_code &error,
							 size_t bytes_transferred)
	{
		if (!error) {
			cout << "Transmitted " << bytes_transferred << " bytes." << endl;
			this->handle_init_transmission();
		} else {
			cerr << "Could not send greeting: " << error.message() << endl;
		}
	}

	bool encode(const int code, void *raw, const size_t rawlen,
				char* dest, size_t *destlen)
	{
		return do_encode(code, raw, rawlen, dest, destlen);
	}

	void setup_message(const int code,
					   char* const enc, const size_t encLen,
					   message &msg)
	{
		do_setup_message(code, enc, encLen, msg);
	}

	bool handle(const message &in, message &reply)
	{
		return do_handle(in, reply);
	}
private:
	string                    uri_;
	string                    encoding_;
	string                    content_;

	virtual bool do_initialize(message &msg) = 0;
	virtual void handle_init_transmission() = 0;
	virtual bool do_encode(const int code, void *raw, const size_t rawlen,
						   char* dest, size_t *destlen) = 0;
	virtual void do_setup_message(const int code, char * const enc,
								  const size_t encLen, message &msg) = 0;
	virtual bool do_handle(const message &msg, message &reply) = 0;
};     // class profile

}      // namespace beep
#endif // BEEP_PROFILE_HEAD
