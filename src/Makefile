HEADERS = session.hpp channel.hpp tcp.hpp entity_header.hpp frame.hpp message.hpp profile.hpp channel_management_profile.hpp beep.hpp connection.hpp test_profile.hpp

.PHONY = all clean

all: listener initiator
clean:
	rm -f listener initiator

listener: simple_listener.cpp $(HEADERS) listener.hpp
	g++ -g -O0 -o listener simple_listener.cpp -lboost_system-xgcc40-mt-1_39

initiator: simple_initiator.cpp $(HEADERS) initiator.hpp
	g++ -g -O0 -o initiator simple_initiator.cpp -lboost_system-xgcc40-mt-1_39
