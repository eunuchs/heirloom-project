/*
 * Heirloom mailx - a mail user agent derived from Berkeley Mail.
 *
 * Copyright (c) 2000-2005 Gunnar Ritter, Freiburg i. Br., Germany.
 */
/*
 * Generated from
 * <http://www.mozilla.org/projects/nspr/reference/html/prerr.html#26127>.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License
 * at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 */

/*	"@(#)nsserr.c	1.3 (gritter) 3/4/06"	*/

#include <sslerr.h>
#include <secerr.h>

static const char *
nss_strerror(int ec)
{
	const char	*cp;
	static char	eb[30];

	switch (ec) {
	default:
		snprintf(eb, sizeof eb, "Unknown error %d", ec);
		cp = eb;
		break;
	case SSL_ERROR_EXPORT_ONLY_SERVER:
		cp = "Unable to communicate securely. Peer does not support high-grade encryption";
		break;
	case SSL_ERROR_US_ONLY_SERVER:
		cp = "Unable to communicate securely. Peer requires high-grade encryption which is not supported";
		break;
	case SSL_ERROR_NO_CYPHER_OVERLAP:
		cp = "Cannot communicate securely with peer: no common encryption algorithm(s)";
		break;
	case SSL_ERROR_NO_CERTIFICATE:
		cp = "Unable to find the certificate or key necessary for authentication";
		break;
	case SSL_ERROR_BAD_CERTIFICATE:
		cp = "Unable to communicate securely with peer: peers's certificate was rejected";
		break;
	case SSL_ERROR_BAD_CLIENT:
		cp = "The server has encountered bad data from the client";
		break;
	case SSL_ERROR_BAD_SERVER:
		cp = "The client has encountered bad data from the server";
		break;
	case SSL_ERROR_UNSUPPORTED_CERTIFICATE_TYPE:
		cp = "Unsupported certificate type";
		break;
	case SSL_ERROR_UNSUPPORTED_VERSION:
		cp = "Peer using unsupported version of security protocol";
		break;
	case SSL_ERROR_WRONG_CERTIFICATE:
		cp = "Client authentication failed: private key in key database does not correspond to public key in certificate database";
		break;
	case SSL_ERROR_BAD_CERT_DOMAIN:
		cp = "Unable to communicate securely with peer: requested domain name does not match the server's certificate";
		break;
	case SSL_ERROR_POST_WARNING:
		cp = "(unused)";
		break;
	case SSL_ERROR_SSL2_DISABLED:
		cp = "Peer only supports SSL version 2, which is locally disabled";
		break;
	case SSL_ERROR_BAD_MAC_READ:
		cp = "SSL received a record with an incorrect Message Authentication Code";
		break;
	case SSL_ERROR_BAD_MAC_ALERT:
		cp = "SSL peer reports incorrect Message Authentication Code";
		break;
	case SSL_ERROR_BAD_CERT_ALERT:
		cp = "SSL peer cannot verify your certificate";
		break;
	case SSL_ERROR_REVOKED_CERT_ALERT:
		cp = "SSL peer rejected your certificate as revoked";
		break;
	case SSL_ERROR_EXPIRED_CERT_ALERT:
		cp = "SSL peer rejected your certificate as expired";
		break;
	case SSL_ERROR_SSL_DISABLED:
		cp = "Cannot connect: SSL is disabled";
		break;
	case SSL_ERROR_FORTEZZA_PQG:
		cp = "Cannot connect: SSL peer is in another FORTEZZA domain";
		break;
	case SSL_ERROR_UNKNOWN_CIPHER_SUITE:
		cp = "An unknown SSL cipher suite has been requested";
		break;
	case SSL_ERROR_NO_CIPHERS_SUPPORTED:
		cp = "No cipher suites are present and enabled in this program";
		break;
	case SSL_ERROR_BAD_BLOCK_PADDING:
		cp = "SSL received a record with bad block padding";
		break;
	case SSL_ERROR_RX_RECORD_TOO_LONG:
		cp = "SSL received a record that exceeded the maximum permissible length";
		break;
	case SSL_ERROR_TX_RECORD_TOO_LONG:
		cp = "SSL attempted to send a record that exceeded the maximum permissible length";
		break;
	case SSL_ERROR_CLOSE_NOTIFY_ALERT:
		cp = "SSL peer has closed this connection";
		break;
	case SSL_ERROR_PUB_KEY_SIZE_LIMIT_EXCEEDED:
		cp = "SSL Server attempted to use domestic-grade public key with export cipher suite";
		break;
	case SSL_ERROR_NO_SERVER_KEY_FOR_ALG:
		cp = "Server has no key for the attempted key exchange algorithm";
		break;
	case SSL_ERROR_TOKEN_INSERTION_REMOVAL:
		cp = "PKCS #11 token was inserted or removed while operation was in progress";
		break;
	case SSL_ERROR_TOKEN_SLOT_NOT_FOUND:
		cp = "No PKCS#11 token could be found to do a required operation";
		break;
	case SSL_ERROR_NO_COMPRESSION_OVERLAP:
		cp = "Cannot communicate securely with peer: no common compression algorithm(s)";
		break;
	case SSL_ERROR_HANDSHAKE_NOT_COMPLETED:
		cp = "Cannot initiate another SSL handshake until current handshake is complete";
		break;
	case SSL_ERROR_BAD_HANDSHAKE_HASH_VALUE:
		cp = "Received incorrect handshakes hash values from peer";
		break;
	case SSL_ERROR_CERT_KEA_MISMATCH:
		cp = "The certificate provided cannot be used with the selected key exchange algorithm";
		break;
	case SSL_ERROR_NO_TRUSTED_SSL_CLIENT_CA:
		cp = "No certificate authority is trusted for SSL client authentication";
		break;
	case SSL_ERROR_SESSION_NOT_FOUND:
		cp = "Client's SSL session ID not found in server's session cache";
		break;
	case SSL_ERROR_RX_MALFORMED_HELLO_REQUEST:
		cp = "SSL received a malformed Hello Request handshake message";
		break;
	case SSL_ERROR_RX_MALFORMED_CLIENT_HELLO:
		cp = "SSL received a malformed Client Hello handshake message";
		break;
	case SSL_ERROR_RX_MALFORMED_SERVER_HELLO:
		cp = "SSL received a malformed Server Hello handshake message";
		break;
	case SSL_ERROR_RX_MALFORMED_CERTIFICATE:
		cp = "SSL received a malformed Certificate handshake message";
		break;
	case SSL_ERROR_RX_MALFORMED_SERVER_KEY_EXCH:
		cp = "SSL received a malformed Server Key Exchange handshake message";
		break;
	case SSL_ERROR_RX_MALFORMED_CERT_REQUEST:
		cp = "SSL received a malformed Certificate Request handshake message";
		break;
	case SSL_ERROR_RX_MALFORMED_HELLO_DONE:
		cp = "SSL received a malformed Server Hello Done handshake message";
		break;
	case SSL_ERROR_RX_MALFORMED_CERT_VERIFY:
		cp = "SSL received a malformed Certificate Verify handshake message";
		break;
	case SSL_ERROR_RX_MALFORMED_CLIENT_KEY_EXCH:
		cp = "SSL received a malformed Client Key Exchange handshake message";
		break;
	case SSL_ERROR_RX_MALFORMED_FINISHED:
		cp = "SSL received a malformed Finished handshake message";
		break;
	case SSL_ERROR_RX_MALFORMED_CHANGE_CIPHER:
		cp = "SSL received a malformed Change Cipher Spec record";
		break;
	case SSL_ERROR_RX_MALFORMED_ALERT:
		cp = "SSL received a malformed Alert record";
		break;
	case SSL_ERROR_RX_MALFORMED_HANDSHAKE:
		cp = "SSL received a malformed Handshake record";
		break;
	case SSL_ERROR_RX_MALFORMED_APPLICATION_DATA:
		cp = "SSL received a malformed Application Data record";
		break;
	case SSL_ERROR_RX_UNEXPECTED_HELLO_REQUEST:
		cp = "SSL received an unexpected Hello Request handshake message";
		break;
	case SSL_ERROR_RX_UNEXPECTED_CLIENT_HELLO:
		cp = "SSL received an unexpected Client Hello handshake message";
		break;
	case SSL_ERROR_RX_UNEXPECTED_SERVER_HELLO:
		cp = "SSL received an unexpected Server Hello handshake message";
		break;
	case SSL_ERROR_RX_UNEXPECTED_CERTIFICATE:
		cp = "SSL received an unexpected Certificate handshake message";
		break;
	case SSL_ERROR_RX_UNEXPECTED_SERVER_KEY_EXCH:
		cp = "SSL received an unexpected Server Key Exchange handshake message";
		break;
	case SSL_ERROR_RX_UNEXPECTED_CERT_REQUEST:
		cp = "SSL received an unexpected Certificate Request handshake message";
		break;
	case SSL_ERROR_RX_UNEXPECTED_HELLO_DONE:
		cp = "SSL received an unexpected Server Hello Done handshake message";
		break;
	case SSL_ERROR_RX_UNEXPECTED_CERT_VERIFY:
		cp = "SSL received an unexpected Certificate Verify handshake message";
		break;
	case SSL_ERROR_RX_UNEXPECTED_CLIENT_KEY_EXCH:
		cp = "SSL received an unexpected Client Key Exchange handshake message";
		break;
	case SSL_ERROR_RX_UNEXPECTED_FINISHED:
		cp = "SSL received an unexpected Finished handshake message";
		break;
	case SSL_ERROR_RX_UNEXPECTED_CHANGE_CIPHER:
		cp = "SSL received an unexpected Change Cipher Spec record";
		break;
	case SSL_ERROR_RX_UNEXPECTED_ALERT:
		cp = "SSL received an unexpected Alert record";
		break;
	case SSL_ERROR_RX_UNEXPECTED_HANDSHAKE:
		cp = "SSL received an unexpected Handshake record";
		break;
	case SSL_ERROR_RX_UNEXPECTED_APPLICATION_DATA:
		cp = "SSL received an unexpected Application Data record";
		break;
	case SSL_ERROR_RX_UNKNOWN_RECORD_TYPE:
		cp = "SSL received a record with an unknown content type";
		break;
	case SSL_ERROR_RX_UNKNOWN_HANDSHAKE:
		cp = "SSL received a handshake message with an unknown message type";
		break;
	case SSL_ERROR_RX_UNKNOWN_ALERT:
		cp = "SSL received an alert record with an unknown alert description";
		break;
	case SSL_ERROR_HANDSHAKE_UNEXPECTED_ALERT:
		cp = "SSL peer was not expecting a handshake message it received";
		break;
	case SSL_ERROR_DECOMPRESSION_FAILURE_ALERT:
		cp = "SSL peer was unable to successfully decompress an SSL record it received";
		break;
	case SSL_ERROR_HANDSHAKE_FAILURE_ALERT:
		cp = "SSL peer was unable to negotiate an acceptable set of security parameters";
		break;
	case SSL_ERROR_ILLEGAL_PARAMETER_ALERT:
		cp = "SSL peer rejected a handshake message for unacceptable content";
		break;
	case SSL_ERROR_UNSUPPORTED_CERT_ALERT:
		cp = "SSL peer does not support certificates of the type it received";
		break;
	case SSL_ERROR_CERTIFICATE_UNKNOWN_ALERT:
		cp = "SSL peer had some unspecified issue with the certificate it received";
		break;
	case SSL_ERROR_DECRYPTION_FAILED_ALERT:
		cp = "Peer was unable to decrypt an SSL record it received";
		break;
	case SSL_ERROR_RECORD_OVERFLOW_ALERT:
		cp = "Peer received an SSL record that was longer than is permitted";
		break;
	case SSL_ERROR_UNKNOWN_CA_ALERT:
		cp = "Peer does not recognize and trust the CA that issued your certificate";
		break;
	case SSL_ERROR_ACCESS_DENIED_ALERT:
		cp = "Peer received a valid certificate, but access was denied";
		break;
	case SSL_ERROR_DECODE_ERROR_ALERT:
		cp = "Peer could not decode an SSL handshake message";
		break;
	case SSL_ERROR_DECRYPT_ERROR_ALERT:
		cp = "Peer reports failure of signature verification or key exchange";
		break;
	case SSL_ERROR_EXPORT_RESTRICTION_ALERT:
		cp = "Peer reports negotiation not in compliance with export regulations";
		break;
	case SSL_ERROR_PROTOCOL_VERSION_ALERT:
		cp = "Peer reports incompatible or unsupported protocol version";
		break;
	case SSL_ERROR_INSUFFICIENT_SECURITY_ALERT:
		cp = "Server requires ciphers more secure than those supported by client";
		break;
	case SSL_ERROR_INTERNAL_ERROR_ALERT:
		cp = "Peer reports it experienced an internal error";
		break;
	case SSL_ERROR_USER_CANCELED_ALERT:
		cp = "Peer user canceled handshake";
		break;
	case SSL_ERROR_NO_RENEGOTIATION_ALERT:
		cp = "Peer does not permit renegotiation of SSL security parameters";
		break;
	case SSL_ERROR_GENERATE_RANDOM_FAILURE:
		cp = "SSL experienced a failure of its random number generator";
		break;
	case SSL_ERROR_SIGN_HASHES_FAILURE:
		cp = "Unable to digitally sign data required to verify your certificate";
		break;
	case SSL_ERROR_EXTRACT_PUBLIC_KEY_FAILURE:
		cp = "SSL was unable to extract the public key from the peer's certificate";
		break;
	case SSL_ERROR_SERVER_KEY_EXCHANGE_FAILURE:
		cp = "Unspecified failure while processing SSL Server Key Exchange handshake";
		break;
	case SSL_ERROR_CLIENT_KEY_EXCHANGE_FAILURE:
		cp = "Unspecified failure while processing SSL Client Key Exchange handshake";
		break;
	case SSL_ERROR_ENCRYPTION_FAILURE:
		cp = "Bulk data encryption algorithm failed in selected cipher suite";
		break;
	case SSL_ERROR_DECRYPTION_FAILURE:
		cp = "Bulk data decryption algorithm failed in selected cipher suite";
		break;
	case SSL_ERROR_MD5_DIGEST_FAILURE:
		cp = "MD5 digest function failed";
		break;
	case SSL_ERROR_SHA_DIGEST_FAILURE:
		cp = "SHA-1 digest function failed";
		break;
	case SSL_ERROR_MAC_COMPUTATION_FAILURE:
		cp = "Message Authentication Code computation failed";
		break;
	case SSL_ERROR_SYM_KEY_CONTEXT_FAILURE:
		cp = "Failure to create Symmetric Key context";
		break;
	case SSL_ERROR_SYM_KEY_UNWRAP_FAILURE:
		cp = "Failure to unwrap the Symmetric key in Client Key Exchange message";
		break;
	case SSL_ERROR_IV_PARAM_FAILURE:
		cp = "PKCS11 code failed to translate an IV into a param";
		break;
	case SSL_ERROR_INIT_CIPHER_SUITE_FAILURE:
		cp = "Failed to initialize the selected cipher suite";
		break;
	case SSL_ERROR_SOCKET_WRITE_FAILURE:
		cp = "Attempt to write encrypted data to underlying socket failed";
		break;
	case SSL_ERROR_SESSION_KEY_GEN_FAILURE:
		cp = "Failed to generate session keys for SSL session";
		break;
	case SEC_ERROR_IO:
		cp = "An I/O error occurred during authentication";
		break;
	case SEC_ERROR_LIBRARY_FAILURE:
		cp = "Security library failure";
		break;
	case SEC_ERROR_BAD_DATA:
		cp = "Security library: received bad data";
		break;
	case SEC_ERROR_OUTPUT_LEN:
		cp = "Security library: output length error";
		break;
	case SEC_ERROR_INPUT_LEN:
		cp = "Security library: input length error";
		break;
	case SEC_ERROR_INVALID_ARGS:
		cp = "Security library: invalid arguments";
		break;
	case SEC_ERROR_INVALID_ALGORITHM:
		cp = "Security library: invalid algorithm";
		break;
	case SEC_ERROR_INVALID_AVA:
		cp = "Security library: invalid AVA";
		break;
	case SEC_ERROR_INVALID_TIME:
		cp = "Security library: invalid time";
		break;
	case SEC_ERROR_BAD_DER:
		cp = "Security library: improperly formatted DER-encoded message";
		break;
	case SEC_ERROR_BAD_SIGNATURE:
		cp = "Peer's certificate has an invalid signature";
		break;
	case SEC_ERROR_EXPIRED_CERTIFICATE:
		cp = "Peer's certificate has expired";
		break;
	case SEC_ERROR_REVOKED_CERTIFICATE:
		cp = "Peer's certificate has been revoked";
		break;
	case SEC_ERROR_UNKNOWN_ISSUER:
		cp = "Peer's certificate issuer is not recognized";
		break;
	case SEC_ERROR_BAD_KEY:
		cp = "Peer's public key is invalid";
		break;
	case SEC_ERROR_BAD_PASSWORD:
		cp = "The password entered is incorrect";
		break;
	case SEC_ERROR_RETRY_PASSWORD:
		cp = "New password entered incorrectly";
		break;
	case SEC_ERROR_NO_NODELOCK:
		cp = "Security library: no nodelock";
		break;
	case SEC_ERROR_BAD_DATABASE:
		cp = "Security library: bad database";
		break;
	case SEC_ERROR_NO_MEMORY:
		cp = "Security library: memory allocation failure";
		break;
	case SEC_ERROR_UNTRUSTED_ISSUER:
		cp = "Peer's certificate issuer has been marked as not trusted by the";
		break;
	case SEC_ERROR_UNTRUSTED_CERT:
		cp = "Peer's certificate has been marked as not trusted by the user";
		break;
	case SEC_ERROR_DUPLICATE_CERT:
		cp = "Certificate already exists in your database";
		break;
	case SEC_ERROR_DUPLICATE_CERT_NAME:
		cp = "Downloaded certificate's name duplicates one already in your";
		break;
	case SEC_ERROR_ADDING_CERT:
		cp = "Error adding certificate to database";
		break;
	case SEC_ERROR_FILING_KEY:
		cp = "Error refiling the key for this certificate";
		break;
	case SEC_ERROR_NO_KEY:
		cp = "The private key for this certificate cannot be found in key";
		break;
	case SEC_ERROR_CERT_VALID:
		cp = "This certificate is valid";
		break;
	case SEC_ERROR_CERT_NOT_VALID:
		cp = "This certificate is not valid";
		break;
	case SEC_ERROR_CERT_NO_RESPONSE:
		cp = "Certificate library: no response";
		break;
	case SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE:
		cp = "The certificate issuer's certificate has expired";
		break;
	case SEC_ERROR_CRL_EXPIRED:
		cp = "The CRL for the certificate's issuer has expired";
		break;
	case SEC_ERROR_CRL_BAD_SIGNATURE:
		cp = "The CRL for the certificate's issuer has an invalid signature";
		break;
	case SEC_ERROR_CRL_INVALID:
		cp = "New CRL has an invalid format";
		break;
	case SEC_ERROR_EXTENSION_VALUE_INVALID:
		cp = "Certificate extension value is invalid";
		break;
	case SEC_ERROR_EXTENSION_NOT_FOUND:
		cp = "Certificate extension not found";
		break;
	case SEC_ERROR_CA_CERT_INVALID:
		cp = "Issuer certificate is invalid";
		break;
	case SEC_ERROR_PATH_LEN_CONSTRAINT_INVALID:
		cp = "Certificate path length constraint is invalid";
		break;
	case SEC_ERROR_CERT_USAGES_INVALID:
		cp = "Certificate usages field is invalid";
		break;
	case SEC_INTERNAL_ONLY:
		cp = "Internal-only module";
		break;
	case SEC_ERROR_INVALID_KEY:
		cp = "The key does not support the requested operation";
		break;
	case SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION:
		cp = "Certificate contains unknown critical extension";
		break;
	case SEC_ERROR_OLD_CRL:
		cp = "New CRL is not later than the current one";
		break;
	case SEC_ERROR_NO_EMAIL_CERT:
		cp = "Not encrypted or signed: you do not yet have an email certificate";
		break;
	case SEC_ERROR_NO_RECIPIENT_CERTS_QUERY:
		cp = "Not encrypted: you do not have certificates for each of the";
		break;
	case SEC_ERROR_NOT_A_RECIPIENT:
		cp = "Cannot decrypt: you are not a recipient, or matching certificate";
		break;
	case SEC_ERROR_PKCS7_KEYALG_MISMATCH:
		cp = "Cannot decrypt: key encryption algorithm does not match your";
		break;
	case SEC_ERROR_PKCS7_BAD_SIGNATURE:
		cp = "Signature verification failed: no signer found, too many signers";
		break;
	case SEC_ERROR_UNSUPPORTED_KEYALG:
		cp = "Unsupported or unknown key algorithm";
		break;
	case SEC_ERROR_DECRYPTION_DISALLOWED:
		cp = "Cannot decrypt: encrypted using a disallowed algorithm or key size";
		break;
#ifdef	notdef
	case XP_EC_FORTEZZA_BAD_CARD:
		cp = "FORTEZZA card has not been properly initialized";
		break;
	case XP_EC_FORTEZZA_NO_CARD:
		cp = "No FORTEZZA cards found";
		break;
	case XP_EC_FORTEZZA_NONE_SELECTED:
		cp = "No FORTEZZA card selected";
		break;
	case XP_EC_FORTEZZA_MORE_INFO:
		cp = "Please select a personality to get more info on";
		break;
	case XP_EC_FORTEZZA_PERSON_NOT_FOUND:
		cp = "Personality not found";
		break;
	case XP_EC_FORTEZZA_NO_MORE_INFO:
		cp = "No more information on that personality";
		break;
	case XP_EC_FORTEZZA_BAD_PIN:
		cp = "Invalid PIN";
		break;
	case XP_EC_FORTEZZA_PERSON_ERROR:
		cp = "Couldn't initialize FORTEZZA personalities";
		break;
#endif	/* notdef */
	case SEC_ERROR_NO_KRL:
		cp = "No KRL for this site's certificate has been found";
		break;
	case SEC_ERROR_KRL_EXPIRED:
		cp = "The KRL for this site's certificate has expired";
		break;
	case SEC_ERROR_KRL_BAD_SIGNATURE:
		cp = "The KRL for this site's certificate has an invalid signature";
		break;
	case SEC_ERROR_REVOKED_KEY:
		cp = "The key for this site's certificate has been revoked";
		break;
	case SEC_ERROR_KRL_INVALID:
		cp = "New KRL has an invalid format";
		break;
	case SEC_ERROR_NEED_RANDOM:
		cp = "Security library: need random data";
		break;
	case SEC_ERROR_NO_MODULE:
		cp = "Security library: no security module can perform the requested";
		break;
	case SEC_ERROR_NO_TOKEN:
		cp = "The security card or token does not exist, needs to be";
		break;
	case SEC_ERROR_READ_ONLY:
		cp = "Security library: read-only database";
		break;
	case SEC_ERROR_NO_SLOT_SELECTED:
		cp = "No slot or token was selected";
		break;
	case SEC_ERROR_CERT_NICKNAME_COLLISION:
		cp = "A certificate with the same nickname already exists";
		break;
	case SEC_ERROR_KEY_NICKNAME_COLLISION:
		cp = "A key with the same nickname already exists";
		break;
	case SEC_ERROR_SAFE_NOT_CREATED:
		cp = "Error while creating safe object";
		break;
	case SEC_ERROR_BAGGAGE_NOT_CREATED:
		cp = "Error while creating baggage object";
		break;
#ifdef	notdef
	case XP_AVA_REMOVE_PRINCIPAL_ERROR:
		cp = "Couldn't remove the principal";
		break;
	case XP_AVA_DELETE_PRIVILEGE_ERROR:
		cp = "Couldn't delete the privilege";
		break;
	case XP_AVA_CERT_NOT_EXISTS_ERROR:
		cp = "This principal doesn't have a certificate";
		break;
#endif	/* notdef */
	case SEC_ERROR_BAD_EXPORT_ALGORITHM:
		cp = "Required algorithm is not allowed";
		break;
	case SEC_ERROR_EXPORTING_CERTIFICATES:
		cp = "Error attempting to export certificates";
		break;
	case SEC_ERROR_IMPORTING_CERTIFICATES:
		cp = "Error attempting to import certificates";
		break;
	case SEC_ERROR_PKCS12_DECODING_PFX:
		cp = "Unable to import. Decoding error. File not valid";
		break;
	case SEC_ERROR_PKCS12_INVALID_MAC:
		cp = "Unable to import. Invalid MAC. Incorrect password or corrupt file";
		break;
	case SEC_ERROR_PKCS12_UNSUPPORTED_MAC_ALGORITHM:
		cp = "Unable to import. MAC algorithm not supported";
		break;
	case SEC_ERROR_PKCS12_UNSUPPORTED_TRANSPORT_MODE:
		cp = "Unable to import. Only password integrity and privacy modes";
		break;
	case SEC_ERROR_PKCS12_CORRUPT_PFX_STRUCTURE:
		cp = "Unable to import. File structure is corrupt";
		break;
	case SEC_ERROR_PKCS12_UNSUPPORTED_PBE_ALGORITHM:
		cp = "Unable to import. Encryption algorithm not supported";
		break;
	case SEC_ERROR_PKCS12_UNSUPPORTED_VERSION:
		cp = "Unable to import. File version not supported";
		break;
	case SEC_ERROR_PKCS12_PRIVACY_PASSWORD_INCORRECT:
		cp = "Unable to import. Incorrect privacy password";
		break;
	case SEC_ERROR_PKCS12_CERT_COLLISION:
		cp = "Unable to import. Same nickname already exists in database";
		break;
	case SEC_ERROR_USER_CANCELLED:
		cp = "The user clicked cancel";
		break;
	case SEC_ERROR_PKCS12_DUPLICATE_DATA:
		cp = "Not imported, already in database";
		break;
	case SEC_ERROR_MESSAGE_SEND_ABORTED:
		cp = "Message not sent";
		break;
	case SEC_ERROR_INADEQUATE_KEY_USAGE:
		cp = "Certificate key usage inadequate for attempted operation";
		break;
	case SEC_ERROR_INADEQUATE_CERT_TYPE:
		cp = "Certificate type not approved for application";
		break;
	case SEC_ERROR_CERT_ADDR_MISMATCH:
		cp = "Address in signing certificate does not match address in message";
		break;
	case SEC_ERROR_PKCS12_UNABLE_TO_IMPORT_KEY:
		cp = "Unable to import. Error attempting to import private key";
		break;
	case SEC_ERROR_PKCS12_IMPORTING_CERT_CHAIN:
		cp = "Unable to import. Error attempting to import certificate chain";
		break;
	case SEC_ERROR_PKCS12_UNABLE_TO_LOCATE_OBJECT_BY_NAME:
		cp = "Unable to export. Unable to locate certificate or key by nickname";
		break;
	case SEC_ERROR_PKCS12_UNABLE_TO_EXPORT_KEY:
		cp = "Unable to export. Private key could not be located and exported";
		break;
	case SEC_ERROR_PKCS12_UNABLE_TO_WRITE:
		cp = "Unable to export. Unable to write the export file";
		break;
	case SEC_ERROR_PKCS12_UNABLE_TO_READ:
		cp = "Unable to import. Unable to read the import file";
		break;
	case SEC_ERROR_PKCS12_KEY_DATABASE_NOT_INITIALIZED:
		cp = "Unable to export. Key database corrupt or deleted";
		break;
	case SEC_ERROR_KEYGEN_FAIL:
		cp = "Unable to generate public-private key pair";
		break;
	case SEC_ERROR_INVALID_PASSWORD:
		cp = "Password entered is invalid";
		break;
	case SEC_ERROR_RETRY_OLD_PASSWORD:
		cp = "Old password entered incorrectly";
		break;
	case SEC_ERROR_BAD_NICKNAME:
		cp = "Certificate nickname already in use";
		break;
	case SEC_ERROR_NOT_FORTEZZA_ISSUER:
		cp = "Peer FORTEZZA chain has a non-FORTEZZA Certificate";
		break;
	case SEC_ERROR_CANNOT_MOVE_SENSITIVE_KEY:
		cp = "A sensitive key cannot be moved to the slot where it is needed";
		break;
	case SEC_ERROR_JS_INVALID_MODULE_NAME:
		cp = "Invalid module name";
		break;
	case SEC_ERROR_JS_INVALID_DLL:
		cp = "Invalid module path/filename";
		break;
	case SEC_ERROR_JS_ADD_MOD_FAILURE:
		cp = "Unable to add module";
		break;
	case SEC_ERROR_JS_DEL_MOD_FAILURE:
		cp = "Unable to delete module";
		break;
	case SEC_ERROR_OLD_KRL:
		cp = "New KRL is not later than the current one";
		break;
	case SEC_ERROR_CKL_CONFLICT:
		cp = "New CKL has different issuer than current CKL";
		break;
	case SEC_ERROR_CERT_NOT_IN_NAME_SPACE:
		cp = "Certificate issuer is not permitted to issue a certificate with";
		break;
	case SEC_ERROR_KRL_NOT_YET_VALID:
		cp = "The key revocation list for this certificate is not yet valid";
		break;
	case SEC_ERROR_CRL_NOT_YET_VALID:
		cp = "The certificate revocation list for this certificate is not yet valid";
		break;
	case SEC_ERROR_UNKNOWN_CERT:
		cp = "The requested certificate could not be found";
		break;
	case SEC_ERROR_UNKNOWN_SIGNER:
		cp = "The signer's certificate could not be found";
		break;
	case SEC_ERROR_CERT_BAD_ACCESS_LOCATION:
		cp = "The location for the certificate status server has invalid format";
		break;
	case SEC_ERROR_OCSP_UNKNOWN_RESPONSE_TYPE:
		cp = "The OCSP response cannot be fully decoded; it is of an unknown type";
		break;
	case SEC_ERROR_OCSP_BAD_HTTP_RESPONSE:
		cp = "The OCSP server returned unexpected/invalid HTTP data";
		break;
	case SEC_ERROR_OCSP_MALFORMED_REQUEST:
		cp = "The OCSP server found the request to be corrupted or improperly formed";
		break;
	case SEC_ERROR_OCSP_SERVER_ERROR:
		cp = "The OCSP server experienced an internal error";
		break;
	case SEC_ERROR_OCSP_TRY_SERVER_LATER:
		cp = "The OCSP server suggests trying again later";
		break;
	case SEC_ERROR_OCSP_REQUEST_NEEDS_SIG:
		cp = "The OCSP server requires a signature on this request";
		break;
	case SEC_ERROR_OCSP_UNAUTHORIZED_REQUEST:
		cp = "The OCSP server has refused this request as unauthorized";
		break;
	case SEC_ERROR_OCSP_UNKNOWN_RESPONSE_STATUS:
		cp = "The OCSP server returned an unrecognizable status";
		break;
	case SEC_ERROR_OCSP_UNKNOWN_CERT:
		cp = "The OCSP server has no status for the certificate";
		break;
	case SEC_ERROR_OCSP_NOT_ENABLED:
		cp = "You must enable OCSP before performing this operation";
		break;
	case SEC_ERROR_OCSP_NO_DEFAULT_RESPONDER:
		cp = "You must set the OCSP default responder before performing this operation";
		break;
	case SEC_ERROR_OCSP_MALFORMED_RESPONSE:
		cp = "The response from the OCSP server was corrupted or improperly formed";
		break;
	case SEC_ERROR_OCSP_UNAUTHORIZED_RESPONSE:
		cp = "The signer of the OCSP response is not authorized to give status for this certificate";
		break;
	case SEC_ERROR_OCSP_FUTURE_RESPONSE:
		cp = "The OCSP response is not yet valid (contains a date in the future)";
		break;
	case SEC_ERROR_OCSP_OLD_RESPONSE:
		cp = "The OCSP response contains out-of-date information";
		break;
	case SEC_ERROR_DIGEST_NOT_FOUND:
		cp = "The CMS or PKCS #7 Digest was not found in signed message";
		break;
	case SEC_ERROR_UNSUPPORTED_MESSAGE_TYPE:
		cp = "The CMS or PKCS #7 Message type is unsupported";
		break;
	case SEC_ERROR_MODULE_STUCK:
		cp = "PKCS #11 module could not be removed because it is still in use";
		break;
	case SEC_ERROR_BAD_TEMPLATE:
		cp = "Could not decode ASN.1 data. Specified template was invalid";
		break;
	case SEC_ERROR_CRL_NOT_FOUND:
		cp = "No matching CRL was found";
		break;
	case SEC_ERROR_REUSED_ISSUER_AND_SERIAL:
		cp = "You are attempting to import a cert with the same issuer/serial as an existing cert, but that is not the same cert";
		break;
	case SEC_ERROR_BUSY:
		cp = "NSS could not shutdown. Objects are still in use";
		break;
	case SEC_ERROR_EXTRA_INPUT:
		cp = "DER-encoded message contained extra usused data";
		break;
	case SEC_ERROR_UNSUPPORTED_ELLIPTIC_CURVE:
		cp = "Unsupported elliptic curve";
		break;
	case SEC_ERROR_UNSUPPORTED_EC_POINT_FORM:
		cp = "Unsupported elliptic curve point form";
		break;
	case SEC_ERROR_UNRECOGNIZED_OID:
		cp = "Unrecognized Object IDentifier";
		break;
	case SEC_ERROR_OCSP_INVALID_SIGNING_CERT:
		cp = "Invalid OCSP signing certificate in OCSP response";
		break;
#ifdef	notdef
	case SEC_ERROR_REVOKED_CERTIFICATE_CRL:
		cp = "Certificate is revoked in issuer's certificate revocation list";
		break;
	case SEC_ERROR_REVOKED_CERTIFICATE_OCSP:
		cp = "Issuer's OCSP responder reports certificate is revoked";
		break;
	case SEC_ERROR_CRL_INVALID_VERSION:
		cp = "Issuer's Certificate Revocation List has an unknown version number";
		break;
	case SEC_ERROR_CRL_V1_CRITICAL_EXTENSION:
		cp = "Issuer's V1 Certificate Revocation List has a critical extension";
		break;
	case SEC_ERROR_CRL_UNKNOWN_CRITICAL_EXTENSION:
		cp = "Issuer's V2 Certificate Revocation List has an unknown critical extension";
		break;
#endif	/* notdef */
	}
	return cp;
}
