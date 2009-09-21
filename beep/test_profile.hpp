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
		, init_("Application Specific Message!\r\n")
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
};
#endif
