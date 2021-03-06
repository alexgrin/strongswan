/*
 * Copyright (C) 2011 Andreas Steffen
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
 * @defgroup pts_ita_comp_func_name pts_ita_comp_func_name
 * @{ @ingroup pts
 */

#ifndef PTS_ITA_COMP_IMA_H_
#define PTS_ITA_COMP_IMA_H_

#include "pts/components/pts_component.h"

/**
 * Create a PTS ITS Functional Component object
 *
 * @param qualifier		PTS Component Functional Name Qualifier
 * @param depth			Sub-component depth
 * @param pts_db		PTS measurement database
 */
pts_component_t* pts_ita_comp_ima_create(u_int8_t qualifier, u_int32_t depth,
										 pts_database_t *pts_db);

#endif /** PTS_ITA_COMP_IMA_H_ @}*/
