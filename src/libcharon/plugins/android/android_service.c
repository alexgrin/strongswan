/*
 * Copyright (C) 2010 Tobias Brunner
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

#include <unistd.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>

#include "android_service.h"

#include <daemon.h>
#include <threading/thread.h>
#include <processing/jobs/callback_job.h>

typedef struct private_android_service_t private_android_service_t;

/**
 * private data of Android service
 */
struct private_android_service_t {

	/**
	 * public interface
	 */
	android_service_t public;

	/**
	 * listener to track progress
	 */
	listener_t listener;

	/**
	 * job that handles requests from the Android control socket
	 */
	callback_job_t *job;

	/**
	 * android credentials
	 */
	android_creds_t *creds;

	/**
	 * android control socket
	 */
	int control;

};

/**
 * Read a string argument from the Android control socket
 */
static char *read_argument(int fd, u_char length)
{
	int offset = 0;
	char *data = malloc(length + 1);
	while (offset < length)
	{
		int n = recv(fd, &data[offset], length - offset, 0);
		if (n < 0)
		{
			DBG1(DBG_CFG, "failed to read argument from Android"
				 " control socket: %s", strerror(errno));
			free(data);
			return NULL;
		}
		offset += n;
	}
	data[length] = '\0';
	DBG1(DBG_CFG, "received argument from Android control socket: %s", data);
	return data;
}

/**
 * handle the request received from the Android control socket
 */
static job_requeue_t initiate(private_android_service_t *this)
{
	bool oldstate;
	int fd, i = 0;
	char *hostname = NULL, *cacert = NULL, *username = NULL, *password = NULL;
	identification_t *gateway = NULL, *user = NULL;
	ike_cfg_t *ike_cfg;
	peer_cfg_t *peer_cfg;
	child_cfg_t *child_cfg;
	traffic_selector_t *ts;
	ike_sa_t *ike_sa;
	auth_cfg_t *auth;
	lifetime_cfg_t lifetime = {
		.time = {
			.life = 10800, /* 3h */
			.rekey = 10200, /* 2h50min */
			.jitter = 300 /* 5min */
		}
	};

	fd = accept(this->control, NULL, 0);
	if (fd < 0)
	{
		DBG1(DBG_CFG, "accept on Android control socket failed: %s",
			 strerror(errno));
		return JOB_REQUEUE_NONE;
	}
	close(this->control);

	while (TRUE)
	{
		u_char length;
		if (recv(fd, &length, 1, 0) != 1)
		{
			DBG1(DBG_CFG, "failed to read from Android control socket: %s",
				 strerror(errno));
			return JOB_REQUEUE_NONE;
		}

		if (length == 0xFF)
		{	/* last argument */
			break;
		}
		else
		{
			switch (i++)
			{
				case 0: /* gateway */
					hostname = read_argument(fd, length);
					break;
				case 1: /* CA certificate name */
					cacert = read_argument(fd, length);
					break;
				case 2: /* username */
					username = read_argument(fd, length);
					break;
				case 3: /* password */
					password = read_argument(fd, length);
					break;
			}
		}
	}

	if (cacert)
	{
		if (!this->creds->add_certificate(this->creds, cacert))
		{
			DBG1(DBG_CFG, "failed to load CA certificate");
		}
		/* if this is a server cert we could use the cert subject as id
		 * but we have to test first if that possible to configure */
	}

	gateway = identification_create_from_string(hostname);
	DBG1(DBG_CFG, "using CA certificate, gateway identitiy '%Y'", gateway);

	if (username)
	{
		user = identification_create_from_string(username);
		this->creds->set_username_password(this->creds, user, password);
	}

	ike_cfg = ike_cfg_create(TRUE, FALSE, "0.0.0.0", IKEV2_UDP_PORT,
							 hostname, IKEV2_UDP_PORT);
	ike_cfg->add_proposal(ike_cfg, proposal_create_default(PROTO_IKE));

	peer_cfg = peer_cfg_create("android", 2, ike_cfg, CERT_SEND_IF_ASKED,
							   UNIQUE_REPLACE, 1, /* keyingtries */
							   36000, 0, /* rekey 10h, reauth none */
							   600, 600, /* jitter, over 10min */
							   TRUE, 0, /* mobike, DPD */
							   host_create_from_string("0.0.0.0", 0) /* virt */,
							   NULL, FALSE, NULL, NULL); /* pool, mediation */

	auth = auth_cfg_create();
	auth->add(auth, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_EAP);
	auth->add(auth, AUTH_RULE_IDENTITY, user);
	peer_cfg->add_auth_cfg(peer_cfg, auth, TRUE);
	auth = auth_cfg_create();
	auth->add(auth, AUTH_RULE_AUTH_CLASS, AUTH_CLASS_PUBKEY);
	auth->add(auth, AUTH_RULE_IDENTITY, gateway);
	peer_cfg->add_auth_cfg(peer_cfg, auth, FALSE);

	child_cfg = child_cfg_create("android", &lifetime, NULL, TRUE, MODE_TUNNEL,
								 ACTION_NONE, ACTION_NONE, FALSE, 0, 0);
	child_cfg->add_proposal(child_cfg, proposal_create_default(PROTO_ESP));
	ts = traffic_selector_create_dynamic(0, 0, 65535);
	child_cfg->add_traffic_selector(child_cfg, TRUE, ts);
	ts = traffic_selector_create_from_string(0, TS_IPV4_ADDR_RANGE, "0.0.0.0",
											 0, "255.255.255.255", 65535);
	child_cfg->add_traffic_selector(child_cfg, FALSE, ts);
	peer_cfg->add_child_cfg(peer_cfg, child_cfg);

	/*this->listener.ike_up_down = ike_up_down;
	this->listener.child_up_down = child_up_down;
	charon->bus->add_listener(charon->bus, &this->listener);*/

	/* confirm that we received the request */
	u_char code = i;
	send(fd, &code, 1, 0);

	if (charon->controller->initiate(charon->controller, peer_cfg, child_cfg,
									 controller_cb_empty, NULL) != SUCCESS)
	{
		DBG1(DBG_CFG, "failed to initiate tunnel");
		code = 0x33; /* FIXME: this indicates an AUTH error, which might not be the case */
		send(fd, &code, 1, 0);
		return JOB_REQUEUE_NONE;
	}
	property_set("vpn.status", "ok");
	return JOB_REQUEUE_NONE;
}

METHOD(android_service_t, destroy, void,
	   private_android_service_t *this)
{
	free(this);
}

/**
 * See header
 */
android_service_t *android_service_create(android_creds_t *creds)
{
	private_android_service_t *this;

	INIT(this,
		.public = {
			.destroy = _destroy,
		},
		.creds = creds,
	);

	this->control = android_get_control_socket("charon");
	if (this->control == -1)
	{
		DBG1(DBG_CFG, "failed to get Android control socket");
		free(this);
		return NULL;
	}

	if (listen(this->control, 1) < 0)
	{
		DBG1(DBG_CFG, "failed to listen on Android control socket: %s",
			 strerror(errno));
		close(this->control);
		free(this);
		return NULL;
	}

	this->job = callback_job_create((callback_job_cb_t)initiate, this,
									NULL, NULL);
	charon->processor->queue_job(charon->processor, (job_t*)this->job);

	return &this->public;
}
