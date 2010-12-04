/*
 * Copyright (C) 2010 Sansar Choinyambuu
 * HSR Hochschule fuer Technik Rapperswil
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
 * @defgroup pb_language_preference_message pb_language_preference_message
 * @{ @ingroup tnccs_20
 */

#ifndef PB_LANGUAGE_PREFERENCE_MESSAGE_H_
#define PB_LANGUAGE_PREFERENCE_MESSAGE_H_

#include "pb_tnc_message.h"

typedef struct pb_language_preference_message_t pb_language_preference_message_t;

/**
 * Classs representing the PB-Language-Preference message type.
 */
struct pb_language_preference_message_t {

	/**
	 * PB-TNC Message interface
	 */
	pb_tnc_message_t pb_interface;

	/**
	 * Get PB Language Preference
	 *
	 * @return			Language preference
	 */
	chunk_t (*get_language_preference)(pb_language_preference_message_t *this);
};

/**
 * Create a PB-Language-Preference message from parameters
 *
 * @param language_preference		Preferred language(s)
 */
pb_tnc_message_t* pb_language_preference_message_create(chunk_t language_preference);

/**
 * Create an unprocessed PB-Language-Preference message from raw data
 *
  * @param data		PB-Language-Preference message data
 */
pb_tnc_message_t* pb_language_preference_message_create_from_data(chunk_t data);

#endif /** PB_PA_MESSAGE_H_ @}*/