/// \file  test_profile.hpp
/// \brief Test proflie header
///
/// UNCLASSIFIED
#ifndef BEEP_TEST_PROFILE_HEAD
#define BEEP_TEST_PROFILE_HEAD 1

class test_profile : public beep::profile {
public:
	test_profile()
		: profile("http://test/profile/usage")
		, init_("Application Specific Message!")
	{
	}

	virtual ~test_profile()
	{
	}

private:
	string init_;
	virtual bool do_initialize(beep::message &msg)
	{
		msg.add_content(buffer(init_));
		return true;
	}

	virtual void handle_init_transmission()
	{
		cout << "THE TEST PROTOCOL IS INITIALIZED." << endl;
	}

	virtual bool do_encode(const int code, void *raw, const size_t rawlen,
						   char* dest, size_t *destlen)
	{
		cout << "test_profile: do_encode" << endl;
		return true;
	}

	virtual void do_setup_message(const int code, char * const enc,
								  const size_t encLen, beep::message &msg)
	{
		cout << "test_profile do_setup_message." << endl;
	}

	virtual bool do_handle(const beep::message &in, beep::message &reply)
	{
		typedef beep::message::const_iterator const_iterator;

		cout << "The Test Profile is handling a message:\n";
		for (const_iterator i = in.begin() ; i != in.end(); ++i) {
			cout << "Content: " << buffer_cast<const char*>(*i);
		}
		cout << endl;
		return false;
	}
};
#endif
