/*
 * Copyright (C) 2011 Sansar Choinyambuu
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

#include "tcg_pts_attr_req_func_comp_evid.h"

#include <pa_tnc/pa_tnc_msg.h>
#include <bio/bio_writer.h>
#include <bio/bio_reader.h>
#include <utils/linked_list.h>
#include <debug.h>

typedef struct private_tcg_pts_attr_req_func_comp_evid_t private_tcg_pts_attr_req_func_comp_evid_t;

/**
 * Request Functional Component Evidence
 * see section 3.14.1 of PTS Protocol: Binding to TNC IF-M Specification
 *
 *					   1				   2				   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     Flags     |     Sub-component Depth (for Component #1)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |					Component Functional Name #1                |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |					Component Functional Name #1                |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                           ........                            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     Flags     |    Sub-component Depth  (for Component #N)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                   Component Functional Name #N                |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                   Component Functional Name #N                |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * Component Functional Name Structure 
 * (see section 5.1 of PTS Protocol: Binding to TNC IF-M Specification)
 *
 *					   1				   2				   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |	 Component Functional Name Vendor ID        |Fam| Qualifier |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                   Component Functional Name                   |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
 
#define PTS_REQ_FUNC_COMP_EVID_SIZE		12
#define PTS_REQ_FUNC_COMP_FAMILY_MASK	0xC0

/**
 * Private data of an tcg_pts_attr_req_func_comp_evid_t object.
 */
struct private_tcg_pts_attr_req_func_comp_evid_t {

	/**
	 * Public members of tcg_pts_attr_req_func_comp_evid_t
	 */
	tcg_pts_attr_req_func_comp_evid_t public;

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
	 * List of Functional Components
	 */
	linked_list_t *list;

	/**
	 * Reference count
	 */
	refcount_t ref;
};

typedef struct entry_t entry_t;

/**
 * Functional component entry
 */
struct entry_t {
	u_int8_t flags;
	u_int32_t depth;
	pts_comp_func_name_t *name;
};

/**
 * Enumerate functional component entries
 */
static bool entry_filter(void *null, entry_t **entry, u_int8_t *flags,
						 void *i2, u_int32_t *depth, void *i3,
						 pts_comp_func_name_t **name)
{
	*flags = (*entry)->flags;
	*depth = (*entry)->depth;
	*name  = (*entry)->name;

	return TRUE;
}

/**
 * Free an entry_t object
 */
static void free_entry(entry_t *this)
{
	if (this)
	{
		this->name->destroy(this->name);
		free(this);
	}
}

METHOD(pa_tnc_attr_t, get_vendor_id, pen_t,
	private_tcg_pts_attr_req_func_comp_evid_t *this)
{
	return this->vendor_id;
}

METHOD(pa_tnc_attr_t, get_type, u_int32_t,
	private_tcg_pts_attr_req_func_comp_evid_t *this)
{
	return this->type;
}

METHOD(pa_tnc_attr_t, get_value, chunk_t,
	private_tcg_pts_attr_req_func_comp_evid_t *this)
{
	return this->value;
}

METHOD(pa_tnc_attr_t, get_noskip_flag, bool,
	private_tcg_pts_attr_req_func_comp_evid_t *this)
{
	return this->noskip_flag;
}

METHOD(pa_tnc_attr_t, set_noskip_flag,void,
	private_tcg_pts_attr_req_func_comp_evid_t *this, bool noskip)
{
	this->noskip_flag = noskip;
}

METHOD(pa_tnc_attr_t, build, void,
	private_tcg_pts_attr_req_func_comp_evid_t *this)
{
	bio_writer_t *writer;
	enumerator_t *enumerator;
	entry_t *entry;

	if (this->value.ptr)
	{
		return;
	}
	writer = bio_writer_create(PTS_REQ_FUNC_COMP_EVID_SIZE);

	enumerator = this->list->create_enumerator(this->list);
	while (enumerator->enumerate(enumerator, &entry))
	{
		writer->write_uint8 (writer, entry->flags);
		writer->write_uint24(writer, entry->depth);
		writer->write_uint24(writer, entry->name->get_vendor_id(entry->name));
		writer->write_uint8 (writer, entry->name->get_qualifier(entry->name));
		writer->write_uint32(writer, entry->name->get_name(entry->name));
	}
	enumerator->destroy(enumerator);

	this->value = chunk_clone(writer->get_buf(writer));
	writer->destroy(writer);
}

METHOD(pa_tnc_attr_t, process, status_t,
	private_tcg_pts_attr_req_func_comp_evid_t *this, u_int32_t *offset)
{
	bio_reader_t *reader;
	u_int32_t depth, vendor_id, name;
	u_int8_t flags, fam_and_qualifier, qualifier;
	status_t status = FAILED;
	entry_t *entry = NULL;

	if (this->value.len < PTS_REQ_FUNC_COMP_EVID_SIZE)
	{
		DBG1(DBG_TNC, "insufficient data for Request Functional "
					  "Component Evidence");
		*offset = 0;
		return FAILED;
	}
	reader = bio_reader_create(this->value);

	while (reader->remaining(reader))
	{
		if (!reader->read_uint8(reader, &flags))
		{
			DBG1(DBG_TNC, "insufficient data for PTS Request Functional "
						  "Component Evidence Flags");
			goto end;
		}
		if (!reader->read_uint24(reader, &depth))
		{
			DBG1(DBG_TNC, "insufficient data for PTS Request Functional "
						  "Component Evidence Sub Component Depth");
			goto end;
		}
		if (!reader->read_uint24(reader, &vendor_id))
		{
			DBG1(DBG_TNC, "insufficient data for PTS Request Functional "
						  "Component Evidence Component Name Vendor ID");
			goto end;
		}
		if (!reader->read_uint8(reader, &fam_and_qualifier))
		{
			DBG1(DBG_TNC, "insufficient data for PTS Request Functional "
						  "Component Evidence Family and Qualifier");
			goto end;
		}
		if (fam_and_qualifier & PTS_REQ_FUNC_COMP_FAMILY_MASK)
		{
			DBG1(DBG_TNC, "the Functional Name Encoding Family "
						  "is not Binary Enumeration");
			goto end;
		}
		if (!reader->read_uint32(reader, &name))
		{
			DBG1(DBG_TNC, "insufficient data for PTS Request Functional "
						  "Component Evidence Component Functional Name");
			goto end;
		}
		qualifier = fam_and_qualifier & ~PTS_REQ_FUNC_COMP_FAMILY_MASK;

		entry = malloc_thing(entry_t);
		entry->flags = flags;
		entry->depth = depth;
		entry->name = pts_comp_func_name_create(vendor_id, name, qualifier);
		
		this->list->insert_last(this->list, entry);
	}
	status = SUCCESS;

end:
	reader->destroy(reader);
	return status;
}

METHOD(pa_tnc_attr_t, get_ref, pa_tnc_attr_t*,
	private_tcg_pts_attr_req_func_comp_evid_t *this)
{
	ref_get(&this->ref);
	return &this->public.pa_tnc_attribute;
}

METHOD(pa_tnc_attr_t, destroy, void,
	private_tcg_pts_attr_req_func_comp_evid_t *this)
{
	if (ref_put(&this->ref))
	{
		this->list->destroy_function(this->list, (void *)free_entry);
		free(this->value.ptr);
		free(this);
	}
}

METHOD(tcg_pts_attr_req_func_comp_evid_t, add_component, void,
	private_tcg_pts_attr_req_func_comp_evid_t *this, u_int8_t flags,
	u_int32_t depth, pts_comp_func_name_t *name)
{
	entry_t *entry;

	entry = malloc_thing(entry_t);
	entry->flags = flags;
	entry->depth = depth;
	entry->name = name;
	this->list->insert_last(this->list, entry);
}

METHOD(tcg_pts_attr_req_func_comp_evid_t, get_count, int,
	private_tcg_pts_attr_req_func_comp_evid_t *this)
{
	return this->list->get_count(this->list);
}

METHOD(tcg_pts_attr_req_func_comp_evid_t, create_enumerator, enumerator_t*,
	private_tcg_pts_attr_req_func_comp_evid_t *this)
{
	return enumerator_create_filter(this->list->create_enumerator(this->list),
								   (void*)entry_filter, NULL, NULL);
}

/**
 * Described in header.
 */
pa_tnc_attr_t *tcg_pts_attr_req_func_comp_evid_create(void)
{
	private_tcg_pts_attr_req_func_comp_evid_t *this;

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
			.add_component = _add_component,
			.get_count = _get_count,
			.create_enumerator = _create_enumerator,
		},
		.vendor_id = PEN_TCG,
		.type = TCG_PTS_REQ_FUNC_COMP_EVID,
		.list = linked_list_create(),
		.ref = 1,
	);

	return &this->public.pa_tnc_attribute;
}

/**
 * Described in header.
 */
pa_tnc_attr_t *tcg_pts_attr_req_func_comp_evid_create_from_data(chunk_t data)
{
	private_tcg_pts_attr_req_func_comp_evid_t *this;

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
			.add_component = _add_component,
			.get_count = _get_count,
			.create_enumerator = _create_enumerator,
		},
		.vendor_id = PEN_TCG,
		.type = TCG_PTS_REQ_FUNC_COMP_EVID,
		.list = linked_list_create(),
		.value = chunk_clone(data),
		.ref = 1,
	);

	return &this->public.pa_tnc_attribute;
}
