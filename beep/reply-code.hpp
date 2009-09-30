/// \file  reply-code.hpp
/// \brief BEEP session reply codes
///
/// UNCLASSIFIED
#ifndef BEEP_REPLY_CODE_HEAD
#define BEEP_REPLY_CODE_HEAD 1
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
}      // namespace beep
#endif // BEEP_REPLY_CODE_HEAD
