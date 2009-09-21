#include <gtest/gtest.h>

#include <vector>
using namespace std;
#include <boost/asio.hpp>
using namespace boost::asio;
#include "beep/frame.hpp"

class FrameMessageHeaderTest : public testing::Test {
public:
	~FrameMessageHeaderTest() { }
protected:
	virtual void SetUp()
	{
	}

	virtual void TearDown()
	{
	}
};

TEST_F(FrameMessageHeaderTest, ParseKeyword)
{
	beep::frame frm;
}

int
main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
