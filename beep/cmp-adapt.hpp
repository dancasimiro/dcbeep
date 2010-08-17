/// \file  cmp-adapt.hpp
/// \brief Implements parsers and generators for channel management
///
/// UNCLASSIFIED
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

BOOST_FUSION_ADAPT_STRUCT(beep::cmp::greeting_message,
						  (std::vector<std::string>, profile_uris)
						 )

BOOST_FUSION_ADAPT_STRUCT(beep::cmp::close_message,
						  (unsigned int, channel)
						  (unsigned int, code)
						  (std::string, language)
						  (std::string, diagnostic)
						  )

BOOST_FUSION_ADAPT_STRUCT(beep::cmp::error_message,
						  (unsigned int, code)
						  (std::string, language)
						  (std::string, diagnostic)
						  )
