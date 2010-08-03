/*
 * Copyright (C) 2010 Martin Willi
 * Copyright (C) 2010 revosec AG
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

/**
 * @defgroup libtls libtls
 *
 * @addtogroup libtls
 * TLS implementation on top of libstrongswan
 *
 * @defgroup tls tls
 * @{ @ingroup libtls
 */

#ifndef TLS_H_
#define TLS_H_

typedef enum tls_version_t tls_version_t;
typedef enum tls_content_type_t tls_content_type_t;
typedef enum tls_handshake_type_t tls_handshake_type_t;
typedef struct tls_t tls_t;

#include <library.h>

/**
 * TLS/SSL version numbers
 */
enum tls_version_t {
	SSL_2_0 = 0x0200,
	SSL_3_0 = 0x0300,
	TLS_1_0 = 0x0301,
	TLS_1_1 = 0x0302,
	TLS_1_2 = 0x0303,
};

/**
 * Enum names for tls_version_t
 */
extern enum_name_t *tls_version_names;

/**
 * TLS higher level content type
 */
enum tls_content_type_t {
	TLS_CHANGE_CIPHER_SPEC = 20,
	TLS_ALERT = 21,
	TLS_HANDSHAKE = 22,
	TLS_APPLICATION_DATA = 23,
};

/**
 * Enum names for tls_content_type_t
 */
extern enum_name_t *tls_content_type_names;

/**
 * TLS handshake subtype
 */
enum tls_handshake_type_t {
	TLS_HELLO_REQUEST = 0,
	TLS_CLIENT_HELLO = 1,
	TLS_SERVER_HELLO = 2,
	TLS_CERTIFICATE = 11,
	TLS_SERVER_KEY_EXCHANGE = 12,
	TLS_CERTIFICATE_REQUEST = 13,
	TLS_SERVER_HELLO_DONE = 14,
	TLS_CERTIFICATE_VERIFY = 15,
	TLS_CLIENT_KEY_EXCHANGE = 16,
	TLS_FINISHED = 20,
};

/**
 * Enum names for tls_handshake_type_t
 */
extern enum_name_t *tls_handshake_type_names;

/**
 * A bottom-up driven TLS stack, suitable for EAP implementations.
 */
struct tls_t {

	/**
	 * Process a TLS record, pass it to upper layers.
	 *
	 * @param type		type of the TLS record to process
	 * @param data		associated TLS record data
	 * @return
	 *					- SUCCESS if TLS negotiation complete
	 *					- FAILED if TLS handshake failed
	 *					- NEED_MORE if more invocations to process/build needed
	 */
	status_t (*process)(tls_t *this, tls_content_type_t type, chunk_t data);

	/**
	 * Query upper layer for TLS record, build protected record.
	 *
	 * @param type		type of the built TLS record
	 * @param data		allocated data of the built TLS record
	 * @return
	 *					- SUCCESS if TLS negotiation complete
	 *					- FAILED if TLS handshake failed
	 *					- NEED_MORE if upper layers have more records to send
	 *					- INVALID_STATE if more input records required
	 */
	status_t (*build)(tls_t *this, tls_content_type_t *type, chunk_t *data);

	/**
	 * Check if TLS stack is acting as a server.
	 *
	 * @return			TRUE if server, FALSE if peer
	 */
	bool (*is_server)(tls_t *this);

	/**
	 * Get the negotiated TLS/SSL version.
	 *
	 * @return			negotiated TLS version
	 */
	tls_version_t (*get_version)(tls_t *this);

	/**
	 * Set the negotiated TLS/SSL version.
	 *
	 * @param version	negotiated TLS version
	 */
	void (*set_version)(tls_t *this, tls_version_t version);

	/**
	 * Check if TLS negotiation completed successfully.
	 *
	 * @return			TRUE if TLS negotation and authentication complete
	 */
	bool (*is_complete)(tls_t *this);

	/**
	 * Get the MSK for EAP-TLS.
	 *
	 * @return			MSK, internal data
	 */
	chunk_t (*get_eap_msk)(tls_t *this);

	/**
	 * Destroy a tls_t.
	 */
	void (*destroy)(tls_t *this);
};

/**
 * Create a tls instance.
 *
 * @param is_server		TRUE to act as server, FALSE for client
 * @param server		server identity
 * @param peer			peer identity
 * @return				TLS stack
 */
tls_t *tls_create(bool is_server, identification_t *server,
				  identification_t *peer);

#endif /** TLS_H_ @}*/