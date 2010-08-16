/// \file  cmp-adapt.hpp
/// \brief Implements parsers and generators for channel management
///
/// UNCLASSIFIED
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

BOOST_FUSION_ADAPT_STRUCT(beep::cmp::greeting_message,
						  (std::vector<std::string>, profile_uris)
						 )
