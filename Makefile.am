AUTOMAKE_OPTIONS := foreign dist-bzip2 subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = \
	-fvisibility=hidden
AM_CXXFLAGS = \
	-fvisibility=hidden -fvisibility-inlines-hidden \
	@BOOST_CPPFLAGS@
AM_LDFLAGS = @BOOST_LDFLAGS@

noinst_PROGRAMS = example/listener example/initiator example/beep-compat-test
noinst_LTLIBRARIES = gtest/src/libgtest.la beep/libbeepcmp.la beep/libbeepcore.la

check_PROGRAMS = \
	test/utframe \
	test/utmessage \
	test/utcmp \
	test/utsession \
	test/uterror \
	test/utsolotcp
TESTS = \
	test/utframe \
	test/utmessage \
	test/utcmp \
	test/utsession \
	test/uterror \
	test/utsolotcp

###############################################################################
# GTest Library
# -----------------------------------------------------------------------------
gtest_src_libgtest_la_SOURCES = \
	gtest/include/gtest/* \
	gtest/include/gtest/internal/* \
	gtest/src/gtest-internal-inl.h \
	gtest/src/gtest.cc \
	gtest/src/gtest-death-test.cc \
	gtest/src/gtest-filepath.cc \
	gtest/src/gtest-port.cc \
	gtest/src/gtest-printers.cc \
	gtest/src/gtest-test-part.cc \
	gtest/src/gtest-typed-test.cc
gtest_src_libgtest_la_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/gtest/include -I$(top_srcdir)/gtest

###############################################################################
# Header only library
# -----------------------------------------------------------------------------
nobase_include_HEADERS = \
	beep/frame.hpp \
	beep/frame-stream.hpp \
	beep/frame-parser.hpp \
	beep/transport-service/basic-solo-stream.hpp \
	beep/transport-service/solo-tcp.hpp

beep_libbeepcore_la_SOURCES = \
	beep/frame-parser.cpp \
	beep/frame-stream.cpp \
	beep/message.cpp \
	beep/message.hpp \
	beep/message-generator.hpp

beep_libbeepcmp_la_SOURCES = \
	beep/channel-management-protocol.hpp \
	beep/cmp-parse.cpp \
	beep/cmp-generate.cpp \
	beep/cmp-adapt.hpp \
	beep/channel-manager.hpp \
	beep/channel-manager.cpp \
	beep/channel.hpp \
	beep/channel.cpp

beep_libbeepcmp_la_LIBADD =
	$(top_builddir)/beep/libbeepcore.la

###############################################################################
# Test Programs
###############################################################################
###############################################################################
# Frames Unit Test
# -----------------------------------------------------------------------------
test_utframe_SOURCES = test/test-frame.cpp
test_utframe_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/gtest/include -I$(top_srcdir)/gtest
test_utframe_LDADD = $(AM_LDFLAGS) \
	$(top_builddir)/beep/libbeepcore.la \
	@BOOST_SYSTEM_LIB@ \
	$(top_builddir)/gtest/src/libgtest.la
# Message Unit Test
# -----------------------------------------------------------------------------
test_utmessage_SOURCES = test/test-message.cpp
test_utmessage_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/gtest/include -I$(top_srcdir)/gtest
test_utmessage_LDADD = $(AM_LDFLAGS) \
	$(top_builddir)/beep/libbeepcore.la \
	$(top_builddir)/beep/libbeepcmp.la \
	@BOOST_SYSTEM_LIB@ \
	$(top_builddir)/gtest/src/libgtest.la
# Channel Management Protocol Parsing and Generator Unit Test
# -----------------------------------------------------------------------------
test_utcmp_SOURCES = test/test-cmp.cpp
test_utcmp_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/gtest/include -I$(top_srcdir)/gtest
test_utcmp_LDADD = $(AM_LDFLAGS) \
	$(top_builddir)/beep/libbeepcore.la \
	$(top_builddir)/beep/libbeepcmp.la \
	@BOOST_SYSTEM_LIB@ \
	$(top_builddir)/gtest/src/libgtest.la
# BEEP session over single TCP/IP connection Unit Test
# -----------------------------------------------------------------------------
test_utsolotcp_SOURCES = test/test-solo-tcp-transport-service.cpp
test_utsolotcp_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/gtest/include -I$(top_srcdir)/gtest
test_utsolotcp_LDADD = $(AM_LDFLAGS) \
	$(top_builddir)/beep/libbeepcore.la \
	@BOOST_SYSTEM_LIB@ \
	$(top_builddir)/gtest/src/libgtest.la
# BEEP session over single TCP/IP connection Error Handling Unit Test
# -----------------------------------------------------------------------------
test_uterror_SOURCES = test/test-solo-tcp-error-handling.cpp
test_uterror_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/gtest/include -I$(top_srcdir)/gtest
test_uterror_LDADD = $(AM_LDFLAGS) \
	$(top_builddir)/beep/libbeepcore.la \
	@BOOST_SYSTEM_LIB@ \
	$(top_builddir)/gtest/src/libgtest.la
# BEEP session test
# -----------------------------------------------------------------------------
test_utsession_SOURCES = test/test-session.cpp
test_utsession_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/gtest/include -I$(top_srcdir)/gtest
test_utsession_LDADD = $(AM_LDFLAGS) \
	$(top_builddir)/beep/libbeepcore.la \
	$(top_builddir)/beep/libbeepcmp.la \
	@BOOST_SYSTEM_LIB@ \
	$(top_builddir)/gtest/src/libgtest.la

###############################################################################
# Examples
###############################################################################
###############################################################################
# Example Listner
# -----------------------------------------------------------------------------
example_listener_SOURCES = example/simple_listener.cpp
example_listener_LDADD = \
	$(top_builddir)/beep/libbeepcore.la \
	$(top_builddir)/beep/libbeepcmp.la \
	@BOOST_SYSTEM_LIB@
# Example Initiator
# -----------------------------------------------------------------------------
example_initiator_SOURCES = example/simple_initiator.cpp
example_initiator_LDADD = \
	$(top_builddir)/beep/libbeepcore.la \
	$(top_builddir)/beep/libbeepcmp.la \
	@BOOST_SYSTEM_LIB@
# Example compatibility test
# -----------------------------------------------------------------------------
example_beep_compat_test_SOURCES = example/compat_test.cpp
example_beep_compat_test_LDADD = \
	$(top_builddir)/beep/libbeepcore.la \
	$(top_builddir)/beep/libbeepcmp.la \
	@BOOST_SYSTEM_LIB@
