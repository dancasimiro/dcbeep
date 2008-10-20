HEADERS = session.hpp channel.hpp tcp.hpp entity_header.hpp frame.hpp message.hpp profile.hpp channel_management_profile.hpp beep.hpp connection.hpp test_profile.hpp

.PHONY = all

all: listener initiator


listener: simple_listener.cpp $(HEADERS) listener.hpp
	g++ -o listener simple_listener.cpp -lboost_system-xgcc40-mt-1_36

initiator: simple_initiator.cpp $(HEADERS) initiator.hpp
	g++ -o initiator simple_initiator.cpp -lboost_system-xgcc40-mt-1_36
