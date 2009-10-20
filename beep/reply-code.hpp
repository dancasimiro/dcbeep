/// \file  reply-code.hpp
/// \brief BEEP session reply codes
///
/// UNCLASSIFIED
#ifndef BEEP_REPLY_CODE_HEAD
#define BEEP_REPLY_CODE_HEAD 1

#include <cassert>
#include <string>
#include <boost/system/error_code.hpp>

namespace beep {
namespace reply_code {

enum reply_code_type {
	success                                      = 200,
	service_not_available                        = 421,
	requested_action_not_taken                   = 450, ///< \note e.g., lock already in use
	requested_action_aborted                     = 451, ///< \note e.g., local error in processing
	temporary_authentication_failure             = 454,
	general_syntax_error                         = 500, ///< \note e.g., poorly-formed XML
	syntax_error_in_parameters                   = 501, ///< \note e.g., non-valid XML
	parameter_not_implemented                    = 504,
	authentication_required                      = 530,
	authentication_mechanism_insufficient        = 534, ///< \note e.g., too weak, sequence exhausted, etc.
	authentication_failure                       = 535,
	action_not_authorized_for_user               = 537,
	authentication_mechanism_requires_encryption = 538,
	requested_action_not_accepted                = 550, ///< \note e.g., no requested profiles are acceptable
	parameter_invalid                            = 553,
	transaction_failed                           = 554, ///< \note e.g., policy violation
};

typedef reply_code_type rc_enum;

}      // namespace reply_code

class error_category : public boost::system::error_category {
public:
	typedef std::string string;
	virtual ~error_category() { }

	virtual const char *name() const { return "BEEP Error Category"; }
	virtual string message(int ev) const
	{
		using namespace beep::reply_code;
		switch (ev) {
		case service_not_available:
			return string("BEEP service is not available");
		case requested_action_not_taken:
			return string("BEEP requested action was not taken");
		case requested_action_aborted:
			return string("BEEP requested action aborted");
		case temporary_authentication_failure:
			return string("BEEP temporary authentication failure");
		case general_syntax_error:
			return string("BEEP general syntax error (Badly formed XML)");
		case syntax_error_in_parameters:
			return string("BEEP syntax error in parameters (Bad XML)");
		case parameter_not_implemented:
			return string("BEEP parameter not implemented");
		case authentication_required:
			return string("BEEP authentication required");
		case authentication_mechanism_insufficient:
			return string("BEEP authentication mechanism is insufficient");
		case authentication_failure:
			return string("BEEP authentication failure");
		case action_not_authorized_for_user:
			return string("BEEP action is not authorized for this user");
		case authentication_mechanism_requires_encryption:
			return string("BEEP authentication mechanism requires encryption");
		case requested_action_not_accepted:
			return string("BEEP requested action was not accepted");
		case parameter_invalid:
			return string("BEEP parameter is invalid");
		case transaction_failed:
			return string("BEEP transaction failed");
		case success:
			return string("BEEP success code");
		default:
			assert(false);
			return string("Unknown BEEP error");
		}
	}
};

inline
const error_category & get_beep_category()
{
	static const error_category ecat;
	return ecat;
}

static const error_category &beep_category = get_beep_category();

}      // namespace beep
#endif // BEEP_REPLY_CODE_HEAD
