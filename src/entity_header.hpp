/// \file  entity_header.hpp
/// \brief MIME entity header
///
/// UNCLASSIFIED
#ifndef BEEP_ENTITY_HEADER_HEAD
#define BEEP_ENTITY_HEADER_HEAD 1
namespace beep {

struct entity_header {
	string name;
	string value;
};

inline
std::ostream&
operator<<(std::ostream &os, const entity_header &header)
{
	if (os) {
		os << header.name << ": " << header.value << "\r\n";
	}
	return os;
}

}      // namespace beep
#endif // BEEP_ENTITY_HEADER_HEAD
