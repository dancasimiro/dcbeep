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

			assert(frameParser);
			if (frameParser) {
				cout << frameHeader << "\n";
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
		if (getline(is, frameTrailer)) {
			cout << frameTrailer << "\n*********" << endl;
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
		, hdbuf_()
		, trbuf_()
		, payload_()
		, buffers_()
	{
	}

	template <typename ConstBufferSequence>
	frame(const ConstBufferSequence &buffer)
		: header_()
		, trailer_()
		, hdbuf_()
		, trbuf_()
		, payload_()
		, buffers_()
	{
		payload_.resize(distance(buffer.begin(), buffer.end()));
		copy(buffer.begin(), buffer.end(), payload_.begin());
		header_.size = accumulate(buffer.begin(), buffer.end(), 0,
								  detail::buffer_size_accum);
	}

	const_iterator begin() const { return buffers_.begin(); }
	const_iterator end() const { return buffers_.end(); }

	// this is temporary... (i hope)
	void refresh()
	{
		ostringstream hdstrm;
		hdstrm << header_;
		hdbuf_ = hdstrm.str();

		ostringstream trstrm;
		trstrm << trailer_;
		trbuf_ = trstrm.str();

		buffers_.resize(payload_.size() + 2);
		buffers_.front() = buffer(hdbuf_);
		copy(payload_.begin(), payload_.end(), buffers_.begin() + 1);
		buffers_.back() = buffer(trbuf_);
		cout << "OUT header: " << hdbuf_ << endl;
	}

	header &get_header() { return header_; }
	trailer &get_trailer() { return trailer_; }

	void set_header(const header &hd) { header_ = hd; }

private:
	header                    header_;
	trailer                   trailer_;
	string                    hdbuf_, trbuf_;
	buffers_container         payload_;
	buffers_container         buffers_;
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
