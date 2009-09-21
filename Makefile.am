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
	test/utframe
TESTS = \
	test/utframe

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
