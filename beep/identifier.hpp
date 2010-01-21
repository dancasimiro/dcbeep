/// \file identifier.hpp
///
/// UNCLASSIFIED
#ifndef BEEP_IDENTIFIER_HEAD
#define BEEP_IDENTIFIER_HEAD 1

#include <istream>
#include <string>
#include <uuid/uuid.h>

namespace beep {
class identifier;
}      // namespace beep

namespace std {
ostream& operator<<(ostream &os, const beep::identifier &ident);
istream& operator>>(istream &is, beep::identifier &ident);
}      // namespace std

namespace beep {

class identifier {
public:
	identifier()
		: uuid_()
	{
		uuid_generate(uuid_);
	}

	identifier(const identifier &src)
		: uuid_()
	{
		uuid_copy(uuid_, src.uuid_);
	}

	identifier &operator=(const identifier &rhs)
	{
		// don't worry about self-assignment. depend on uuid library to do the
		// right thing.
		uuid_copy(uuid_, rhs.uuid_);
		return *this;
	}

	bool operator<(const identifier &rhs) const
	{
		return uuid_compare(uuid_, rhs.uuid_) < 0;
	}

	bool operator==(const identifier &rhs) const
	{
		return uuid_compare(uuid_, rhs.uuid_) == 0;
	}

	bool operator!=(const identifier &rhs) const { 	return !(*this == rhs); }

	friend std::ostream &std::operator<<(std::ostream &os, const identifier&);
	friend std::istream &std::operator>>(std::istream &is, identifier&);
private:
	uuid_t uuid_;
};     // class identifier

}      // namespace beep

namespace std {
inline
ostream& operator<<(ostream &os, const beep::identifier &ident)
{
	if (os) {
		// is there a safer way? what if uuid doesn't fill its contract?
		// I got "36" out of the man page for uuid_unparse.
		static const size_t uuid_str_len = 36;
		char out[64] = { '\0' };
		uuid_unparse(ident.uuid_, out);
		// "string" adds an extra copy, but it guarantees that I don't write
		// out more than uuid_str_len characters.
		os << string(out, uuid_str_len);
	}
	return os;
}

inline
istream& operator>>(istream &is, beep::identifier &ident)
{
	if (is) {
		string uuid_str;
		uuid_t uu;
		if (is >> uuid_str) {
			if (!uuid_parse(uuid_str.c_str(), uu)) {
				uuid_copy(ident.uuid_, uu);
			} else {
				is.setstate(istream::badbit);
			}
		}
	}
	return is;
}

}      // namespace std
#endif // BEEP_IDENTIFIER_HEAD
