/// \file  channel_management_profile.hpp
/// \brief BEEP channel management interface
///
/// UNCLASSIFIED
#ifndef BEEP_CHANNEL_MANAGEMENT_PROFILE_HEAD
#define BEEP_CHANNEL_MANAGEMENT_PROFILE_HEAD 1
namespace beep {

struct start_message {
	static int code() { return 0; }

	int    request_num;
	string profile_uri;
};

class channel_management_profile : public profile {
public:
	enum data_type {
		start_message_type = 0,
	};

	channel_management_profile()
		: profile("http://dan/raw/tuning")
		, header_("Content-Type: application/beep+xml\r\n")
		, greeting_("<greeting />\r\n")
		, startReply_()
	{
		/// \todo Add supported profiles (if any)
	}

	virtual ~channel_management_profile()
	{
	}

private:
	string header_;
	string greeting_;
	string startReply_;

	virtual bool do_initialize(message &msg)
	{
		msg.set_type(frame::rpy);
		msg.add_header(buffer(header_));
		msg.add_content(buffer(greeting_));
		return true;
	}

	virtual void handle_init_transmission()
	{
		cout << "Sent the greeting message successfully!" << endl;
	}

	virtual bool do_encode(const int code, void *raw, const size_t rawlen,
						   char* dest, size_t *destlen)
	{
		switch (code) {
		case 0: // start_message
			assert(rawlen >= sizeof(start_message));
			start_message *msg = reinterpret_cast<start_message*>(raw);
			return encode_start_message(*msg, dest, destlen);
			break;
		default:
			assert(false);
		}
	}

	virtual bool encode_start_message(const start_message& msg,
									  char *dest, size_t *destlen)
	{
		bool success = false;
		ostringstream strm;

		strm << "<start number='" << msg.request_num << "'>\r\n"
			 << "\t<profile uri='" << msg.profile_uri
			 << "' />\r\n</start>\r\n";
		if (size_t strmLen = strm.str().length()) {
			if (strmLen < *destlen) {
				cout << "Copy stream bytes to destination." << endl;
				strm.str().copy(dest, *destlen);
				success = true;
			}
			*destlen = strmLen;
		} else {
			*destlen = 0;
		}
		return success;
	}

	virtual void do_setup_message(const int code, char * const enc,
								  const size_t encLen, message &msg)
	{
		msg.set_type(frame::msg);
		msg.add_header(buffer(header_));
		msg.add_content(buffer(enc, encLen));
	}

	virtual bool do_handle(const message &in, message &reply)
	{
		bool sendReply = false;
		// only start messages are supported right now.
		cout << "Input message type is " << in.type() << endl;
		if (in.type() == frame::msg) {
			cout << "tuning channel do_handle :\n";
			typedef message::const_iterator const_iterator;
			list<string> headers;

			// start message should only have one content buffer in message
			for (const_iterator i = in.begin(); i != in.end(); ++i) {
				cout << "Const Buffer start...\n";
				const string allContent(buffer_cast<const char*>(*i),
										buffer_size(*i));
				assert(allContent.length() == buffer_size(*i));
				size_t bodyStart = 0;
				// strip out the headers...
				string aHeader;
				istringstream istrm(allContent);
				do {
					aHeader.clear();
					getline(istrm, aHeader);
					bodyStart += aHeader.length();
					// strip out the final \r
					aHeader = aHeader.substr(0, aHeader.length() - 1);
					cout << "Header: " << aHeader << "\n";
					headers.push_back(aHeader);
				} while (aHeader.length() > 0); // look for blank line
				string msgBody(istrm.str(), bodyStart);

				cout << "contents (" << msgBody.length() << " bytes): "
					 << msgBody << "\n";
				cout << "const buffer end...\n";
			}
			cout << endl;

			startReply_ = "<profile uri='http://test/profile/usage' />";

			reply.set_type(frame::rpy);
			reply.add_header(buffer(header_));
			reply.add_content(buffer(startReply_));
			sendReply = true;
		}
		return sendReply;
	}

	void strip_headers()
	{
	}
};     // class channel_management_profile

}
#endif // BEEP_CHANNEL_MANAGEMENT_PROFILE_HEAD
