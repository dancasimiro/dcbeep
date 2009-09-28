AUTOMAKE_OPTIONS := foreign dist-bzip2 subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = \
	-fvisibility=hidden
AM_CXXFLAGS = \
	-fvisibility=hidden -fvisibility-inlines-hidden \
	@BOOST_CPPFLAGS@
AM_LDFLAGS = @BOOST_LDFLAGS@

noinst_PROGRAMS = example/listener example/initiator

check_PROGRAMS = \
	test/utframe \
	test/utmessage \
	test/utsession \
	test/utsolotcp
TESTS = \
	test/utframe \
	test/utmessage \
	test/utsession \
	test/utsolotcp

###############################################################################
# Header only library
# -----------------------------------------------------------------------------
nobase_include_HEADERS = \
	beep/frame.hpp \
	beep/frame-stream.hpp \
	beep/identifier.hpp \
	beep/transport-service/basic-solo-stream.hpp \
	beep/transport-service/solo-tcp.hpp

###############################################################################
# Test Programs
###############################################################################
###############################################################################
# Frames Unit Test
# -----------------------------------------------------------------------------
test_utframe_SOURCES = test/test-frame.cpp
test_utframe_CPPFLAGS = $(AM_CPPFLAGS) @GTEST_CPPFLAGS@
test_utframe_CXXFLAGS = $(AM_CXXFLAGS) @GTEST_CXXFLAGS@
test_utframe_LDADD = $(AM_LDFLAGS) @BOOST_SYSTEM_LIB@ @GTEST_LIBS@
# Message Unit Test
# -----------------------------------------------------------------------------
test_utmessage_SOURCES = test/test-message.cpp
test_utmessage_CPPFLAGS = $(AM_CPPFLAGS) @GTEST_CPPFLAGS@
test_utmessage_CXXFLAGS = $(AM_CXXFLAGS) @GTEST_CXXFLAGS@
test_utmessage_LDADD = $(AM_LDFLAGS) @BOOST_SYSTEM_LIB@ @GTEST_LIBS@
# BEEP session over single TCP/IP connection Unit Test
# -----------------------------------------------------------------------------
test_utsolotcp_SOURCES = test/test-solo-tcp-transport-service.cpp
test_utsolotcp_CPPFLAGS = $(AM_CPPFLAGS) @GTEST_CPPFLAGS@
test_utsolotcp_CXXFLAGS = $(AM_CXXFLAGS) @GTEST_CXXFLAGS@
test_utsolotcp_LDADD = $(AM_LDFLAGS) @BOOST_SYSTEM_LIB@ @GTEST_LIBS@
# BEEP session test
# -----------------------------------------------------------------------------
test_utsession_SOURCES = test/test-session.cpp
test_utsession_CPPFLAGS = $(AM_CPPFLAGS) @GTEST_CPPFLAGS@
test_utsession_CXXFLAGS = $(AM_CXXFLAGS) @GTEST_CXXFLAGS@
test_utsession_LDADD = $(AM_LDFLAGS) @BOOST_SYSTEM_LIB@ @GTEST_LIBS@

###############################################################################
# Examples
###############################################################################
###############################################################################
# Example Listner
# -----------------------------------------------------------------------------
example_listener_SOURCES = example/simple_listener.cpp
example_listener_LDADD = @BOOST_SYSTEM_LIB@
# Example Initiator
# -----------------------------------------------------------------------------
example_initiator_SOURCES = example/simple_initiator.cpp
example_initiator_LDADD = @BOOST_SYSTEM_LIB@
