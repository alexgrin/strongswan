/*
 * Copyright (C) 2011 Andreas Steffen, HSR Hochschule fuer Technik Rapperswil
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

#include "ietf_attr_product_info.h"

#include <pa_tnc/pa_tnc_msg.h>
#include <bio/bio_writer.h>
#include <bio/bio_reader.h>
#include <debug.h>

typedef struct private_ietf_attr_product_info_t private_ietf_attr_product_info_t;

/**
 * PA-TNC Product Information type  (see section 4.2.2 of RFC 5792)
 *
 *                       1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |               Product Vendor ID               |  Product ID   |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |  Product ID   |         Product Name (Variable Length)        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

#define PRODUCT_INFO_MIN_SIZE	5

/**
 * Private data of an ietf_attr_product_info_t object.
 */
struct private_ietf_attr_product_info_t {

	/**
	 * Public members of ietf_attr_product_info_t
	 */
	ietf_attr_product_info_t public;

	/**
	 * Attribute vendor ID
	 */
	pen_t vendor_id;

	/**
	 * Attribute type
	 */
	u_int32_t type;

	/**
	 * Attribute value
	 */
	chunk_t value;

	/**
	 * Noskip flag
	 */
	bool noskip_flag;

	/**
	 * Product vendor ID
	 */
	pen_t product_vendor_id;

	/**
	 * Product ID
	 */
	u_int16_t product_id;

	/**
	 * Product Name
	 */
	char *product_name;

	/**
	 * Reference count
	 */
	refcount_t ref;
};

METHOD(pa_tnc_attr_t, get_vendor_id, pen_t,
	private_ietf_attr_product_info_t *this)
{
	return this->vendor_id;
}

METHOD(pa_tnc_attr_t, get_type, u_int32_t,
	private_ietf_attr_product_info_t *this)
{
	return this->type;
}

METHOD(pa_tnc_attr_t, get_value, chunk_t,
	private_ietf_attr_product_info_t *this)
{
	return this->value;
}

METHOD(pa_tnc_attr_t, get_noskip_flag, bool,
	private_ietf_attr_product_info_t *this)
{
	return this->noskip_flag;
}

METHOD(pa_tnc_attr_t, set_noskip_flag,void,
	private_ietf_attr_product_info_t *this, bool noskip)
{
	this->noskip_flag = noskip;
}

METHOD(pa_tnc_attr_t, build, void,
	private_ietf_attr_product_info_t *this)
{
	bio_writer_t *writer;
	chunk_t product_name;

	if (this->value.ptr)
	{
		return;
	}
	product_name = chunk_create(this->product_name, strlen(this->product_name));

	writer = bio_writer_create(PRODUCT_INFO_MIN_SIZE);
	writer->write_uint24(writer, this->product_vendor_id);
	writer->write_uint16(writer, this->product_id);
	writer->write_data  (writer, product_name);

	this->value = chunk_clone(writer->get_buf(writer));
	writer->destroy(writer);
}

METHOD(pa_tnc_attr_t, process, status_t,
	private_ietf_attr_product_info_t *this, u_int32_t *offset)
{
	bio_reader_t *reader;
	chunk_t product_name;

	if (this->value.len < PRODUCT_INFO_MIN_SIZE)
	{
		DBG1(DBG_TNC, "insufficient data for IETF product information");
		*offset = 0;
		return FAILED;
	}
	reader = bio_reader_create(this->value);
	reader->read_uint24(reader, &this->product_vendor_id);
	reader->read_uint16(reader, &this->product_id);
	reader->read_data  (reader, reader->remaining(reader), &product_name);
	reader->destroy(reader);

	this->product_name = malloc(product_name.len + 1);
	memcpy(this->product_name, product_name.ptr, product_name.len);
	this->product_name[product_name.len] = '\0';

	return SUCCESS;
}

METHOD(pa_tnc_attr_t, get_ref, pa_tnc_attr_t*,
	private_ietf_attr_product_info_t *this)
{
	ref_get(&this->ref);
	return &this->public.pa_tnc_attribute;
}

METHOD(pa_tnc_attr_t, destroy, void,
	private_ietf_attr_product_info_t *this)
{
	if (ref_put(&this->ref))
	{
		free(this->product_name);
		free(this->value.ptr);
		free(this);
	}
}

METHOD(ietf_attr_product_info_t, get_info, char*,
	private_ietf_attr_product_info_t *this, pen_t *vendor_id, u_int16_t *id)
{
	if (vendor_id)
	{
		*vendor_id = this->product_vendor_id;
	}
	if (id)
	{
		*id = this->product_id;
	}
	return this->product_name;
}

/**
 * Described in header.
 */
pa_tnc_attr_t *ietf_attr_product_info_create(pen_t vendor_id, u_int16_t id,
											 char *name)
{
	private_ietf_attr_product_info_t *this;

	INIT(this,
		.public = {
			.pa_tnc_attribute = {
				.get_vendor_id = _get_vendor_id,
				.get_type = _get_type,
				.get_value = _get_value,
				.get_noskip_flag = _get_noskip_flag,
				.set_noskip_flag = _set_noskip_flag,
				.build = _build,
				.process = _process,
				.get_ref = _get_ref,
				.destroy = _destroy,
			},
			.get_info = _get_info,
		},
		.vendor_id = PEN_IETF,
		.type = IETF_ATTR_PRODUCT_INFORMATION,
		.product_vendor_id = vendor_id,
		.product_id = id,
		.product_name = strdup(name),
		.ref = 1,
	);

	return &this->public.pa_tnc_attribute;
}

/**
 * Described in header.
 */
pa_tnc_attr_t *ietf_attr_product_info_create_from_data(chunk_t data)
{
	private_ietf_attr_product_info_t *this;

	INIT(this,
		.public = {
			.pa_tnc_attribute = {
				.get_vendor_id = _get_vendor_id,
				.get_type = _get_type,
				.get_value = _get_value,
				.build = _build,
				.process = _process,
				.get_ref = _get_ref,
				.destroy = _destroy,
			},
			.get_info = _get_info,
		},
		.vendor_id = PEN_IETF,
		.type = IETF_ATTR_PRODUCT_INFORMATION,
		.value = chunk_clone(data),
		.ref = 1,
	);

	return &this->public.pa_tnc_attribute;
}

