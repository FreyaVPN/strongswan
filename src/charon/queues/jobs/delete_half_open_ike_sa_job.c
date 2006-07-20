/**
 * @file delete_half_open_ike_sa_job.c
 * 
 * @brief Implementation of delete_half_open_ike_sa_job_t.
 * 
 */

/*
 * Copyright (C) 2005-2006 Martin Willi
 * Copyright (C) 2005 Jan Hutter
 * Hochschule fuer Technik Rapperswil
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

#include "delete_half_open_ike_sa_job.h"

#include <daemon.h>

typedef struct private_delete_half_open_ike_sa_job_t private_delete_half_open_ike_sa_job_t;

/**
 * Private data of an delete_half_open_ike_sa_job_t Object
 */
struct private_delete_half_open_ike_sa_job_t {
	/**
	 * public delete_half_open_ike_sa_job_t interface
	 */
	delete_half_open_ike_sa_job_t public;
	
	/**
	 * ID of the ike_sa to delete
	 */
	ike_sa_id_t *ike_sa_id;
	
	/**
	 * logger ref
	 */
	logger_t *logger;
};

/**
 * Implements job_t.get_type.
 */
static job_type_t get_type(private_delete_half_open_ike_sa_job_t *this)
{
	return DELETE_HALF_OPEN_IKE_SA;
}

/**
 * Implementation of job_t.execute.
 */
static status_t execute(private_delete_half_open_ike_sa_job_t *this)
{
	ike_sa_t *ike_sa;
	
	ike_sa = charon->ike_sa_manager->checkout(charon->ike_sa_manager, this->ike_sa_id);
	if (ike_sa == NULL)
	{
		/* hm, somebody was faster ;-) */
		return DESTROY_ME;
	}
	
	switch (ike_sa->get_state(ike_sa))
	{
		case IKE_ESTABLISHED:
		{
			/* IKE_SA is established and so is not getting destroyed */
			charon->ike_sa_manager->checkin(charon->ike_sa_manager, ike_sa);
			return DESTROY_ME;
		}
		default:
		{
			/* IKE_SA is half open and gets destroyed */
			this->logger->log(this->logger, AUDIT,
							  "deleting half open IKE_SA after timeout");
			charon->ike_sa_manager->checkin_and_destroy(charon->ike_sa_manager, ike_sa);
			return DESTROY_ME;
		}
	}
}

/**
 * Implements job_t.destroy.
 */
static void destroy(private_delete_half_open_ike_sa_job_t *this)
{
	this->ike_sa_id->destroy(this->ike_sa_id);
	free(this);
}

/*
 * Described in header
 */
delete_half_open_ike_sa_job_t *delete_half_open_ike_sa_job_create(ike_sa_id_t *ike_sa_id)
{
	private_delete_half_open_ike_sa_job_t *this = malloc_thing(private_delete_half_open_ike_sa_job_t);
	
	/* interface functions */
	this->public.job_interface.get_type = (job_type_t (*) (job_t *)) get_type;
	this->public.job_interface.execute = (status_t (*) (job_t *)) execute;
	this->public.job_interface.destroy = (void (*)(job_t *)) destroy;;
	
	/* private variables */
	this->ike_sa_id = ike_sa_id->clone(ike_sa_id);
	this->logger = logger_manager->get_logger(logger_manager, WORKER);
	
	return &(this->public);
}
