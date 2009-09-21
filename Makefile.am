AUTOMAKE_OPTIONS := foreign dist-bzip2 subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = \
	-fvisibility=hidden
AM_CXXFLAGS = \
	-fvisibility=hidden -fvisibility-inlines-hidden \
	@BOOST_CPPFLAGS@
AM_LDFLAGS = @BOOST_LDFLAGS@

noinst_PROGRAMS = example/listener example/initiator

#check_PROGRAMS =
#TESTS =
#BUILT_SOURCES =
#CLEANFILES += $(BUILT_SOURCES)

example_listener_SOURCES = example/simple_listener.cpp
example_listener_LDADD = @BOOST_SYSTEM_LIB@

example_initiator_SOURCES = example/simple_initiator.cpp
example_initiator_LDADD = @BOOST_SYSTEM_LIB@
