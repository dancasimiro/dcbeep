HEADERS = \
	beep/session.hpp \
	beep/channel.hpp \
	beep/tcp.hpp \
	beep/entity_header.hpp \
	beep/frame.hpp \
	beep/message.hpp \
	beep/profile.hpp \
	beep/channel_management_profile.hpp \
	beep/beep.hpp \
	beep/connection.hpp \
	test_profile.hpp

.PHONY = all clean

all: listener initiator
clean:
	rm -f listener initiator

listener: simple_listener.cpp $(HEADERS) beep/listener.hpp
	g++ -g -O0 -Wall -Wextra -o listener simple_listener.cpp -lboost_system-xgcc40-mt-1_39

initiator: simple_initiator.cpp $(HEADERS) beep/initiator.hpp
	g++ -g -O0 -Wall -Wextra -o initiator simple_initiator.cpp -lboost_system-xgcc40-mt-1_39
