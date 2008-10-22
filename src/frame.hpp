/// \file  frame.hpp
/// \brief BEEP frame
///
/// UNCLASSIFIED
#ifndef BEEP_FRAME_HEAD
#define BEEP_FRAME_HEAD 1
namespace beep {

namespace detail {

static size_t
buffer_size_accum(const size_t init, const_buffer aBuffer)
{
	return init + buffer_size(aBuffer);
}

// hacky until i get something better
/// \note this does not account for transport layer keywords.
static const char*
keyword_from_type(const int t)
{
	switch (t) {
	case 0:
		return "MSG";
		break;
	case 1:
		return "RPY";
		break;
	case 2:
		return "ANS";
		break;
	case 3:
		return "ERR";
		break;
	case 4:
		return "NUL";
		break;
	default:
		return "UNK";
	}
}

}      // namespace detail

class frame {
public:
	typedef char byte_type;

	static const char* terminator() { return "\r\n"; }

	enum frame_type {
		msg = 0,
		rpy = 1,
		ans = 2,
		err = 3,
		nul = 4,
	};

	struct header {
		static const char sp = ' ';
		static const char more_token = '*';
		static const char final_token = '.';

		uint32_t channel;  // range: 0 .. 2147483647 (2^31 - 1)
		uint32_t msgno;    // range: 0 .. 2147483647 (2^31 - 1)
		uint32_t seqno;    // range: 0 .. 4294967295 (2^32 - 1)
		uint32_t size;     // range: 0 .. 2147483647 (2^31 - 1) [payload size]
		uint32_t ansno;    // range: 0 .. 4294967295 (2^32 - 1)

		int      type;
		bool     final;

		header()
			: channel(0)
			, msgno(0)
			, seqno(0)
			, size(0)
			, ansno(0)
			, type(frame::msg)
			, final(true)
		{
		}
	};

	static bool parse(std::streambuf *sb, header& out)
	{
		bool isValid = false;
		istream is(sb);
		string frameHeader;
		if (getline(is, frameHeader)) {
			istringstream frameParser(frameHeader);
			string keyword;
			string continuation;
			frameParser >> keyword >> out.channel >> out.msgno
						>> continuation >> out.seqno >> out.size;

			if (keyword == "MSG") {
				out.type = frame::msg;
			} else if (keyword == "RPY") {
				out.type = frame::rpy;
			}

			if (frameParser || (!frameParser && frameParser.eof())) {
				isValid = true;
			}
		}
		return isValid;
	}

	struct trailer {
		const char *id;

		trailer()
			: id("END")
		{
		}
	};

	static bool parse(std::streambuf *sb, trailer &out)
	{
		bool isValid = false;
		istream is(sb);
		string frameTrailer;
		if (getline(is, frameTrailer) || (!is && is.eof())) {
			isValid = true;
		}
		return isValid;
	}

	// trick to forward declare operator<< for frame::header and frame::trailer
	friend std::ostream& operator<<(std::ostream &os, const frame::header&);
	friend std::ostream& operator<<(std::ostream &os, const frame::trailer&);

	typedef vector<const_buffer>                            buffers_container;
	typedef buffers_container::const_iterator               const_iterator;

	frame()
		: header_()
		, trailer_()
		, payload_()
	{
	}

	template <typename ConstBufferSequence>
	frame(const ConstBufferSequence &buffer)
		: header_()
		, trailer_()
		, payload_()
	{
		payload_.resize(distance(buffer.begin(), buffer.end()));
		copy(buffer.begin(), buffer.end(), payload_.begin());
		header_.size = accumulate(buffer.begin(), buffer.end(), 0,
								  detail::buffer_size_accum);
	}

	const_iterator begin() const { return payload_.begin(); }
	const_iterator end() const { return payload_.end(); }

	header &get_header() { return header_; }
	const header &get_header() const { return header_; }

	trailer &get_trailer() { return trailer_; }
	const trailer &get_trailer() const { return trailer_; }

private:
	header                    header_;
	trailer                   trailer_;
	buffers_container         payload_;
};     // class frame

inline
std::ostream&
operator<<(std::ostream &os, const frame::header& header)
{
	if (os) {
		const char contin_token =
			//header.final ? header.final_token : header.more_token;
			header.final ? '.' : '*';
		const char * const keyword = detail::keyword_from_type(header.type);

		os << keyword << header.sp << header.channel << header.sp
		   << header.msgno << header.sp << contin_token
		   << header.sp << header.seqno << header.sp << header.size
		   << frame::terminator();
	}
	return os;
}

inline
std::ostream&
operator<<(std::ostream &os, const frame::trailer& trailer)
{
	if (os) {
		os << trailer.id << frame::terminator();
	}
	return os;
}

}      // namespace beep
#endif // BEEP_FRAME_HEAD
