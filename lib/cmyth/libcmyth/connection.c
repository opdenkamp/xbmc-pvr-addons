/*
 *  Copyright (C) 2004-2012, Eric Lund, Jon Gettler
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file connection.c
 * Functions to handle creating connections to a MythTV backend and
 * interacting with those connections.
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <cmyth_local.h>

static char * cmyth_conn_get_setting_unlocked(cmyth_conn_t conn, const char* hostname, const char* setting);
static int cmyth_conn_set_setting_unlocked(cmyth_conn_t conn, const char* hostname, const char* setting, const char* value);

typedef struct {
	unsigned int version;
	char token[14]; // up to 13 chars used in v74 + the terminating NULL character
} myth_protomap_t;

static myth_protomap_t protomap[] = {
	{62, "78B5631E"},
	{63, "3875641D"},
	{64, "8675309J"},
	{65, "D2BB94C2"},
	{66, "0C0FFEE0"},
	{67, "0G0G0G0"},
	{68, "90094EAD"},
	{69, "63835135"},
	{70, "53153836"},
	{71, "05e82186"},
	{72, "D78EFD6F"},
	{73, "D7FE8D6F"},
	{74, "SingingPotato"},
	{75, "SweetRock"},
	{0, ""}
};

/*
 * cmyth_conn_destroy()
 *
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Tear down and release storage associated with a connection.  This
 * should only be called by cmyth_conn_release().  All others should
 * call ref_release() to release a connection.
 *
 * Return Value:
 *
 * None.
 */
static void
cmyth_conn_done(cmyth_conn_t conn)
{
	int err;
	char msg[5] = "DONE";

	if (!conn) {
		return;
	}
	if (!conn->conn_hang && conn->conn_ann != ANN_NONE) {
		pthread_mutex_lock(&conn->conn_mutex);

		/*
		 * Try to shut down the connection.  Can't do much
		 * if it fails other than log it.
		 */
		if ((err = cmyth_send_message(conn, msg)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_send_message() failed (%d)\n",
				  __FUNCTION__, err);
		}

		pthread_mutex_unlock(&conn->conn_mutex);
	}
}

static void
cmyth_conn_destroy(cmyth_conn_t conn)
{
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s } !\n", __FUNCTION__);
		return;
	}
	if (conn->conn_fd >= 0) {
		/* Try to close nicely before shutdown */
		cmyth_conn_done(conn);
		cmyth_dbg(CMYTH_DBG_PROTO,
			  "%s: shutdown and close connection fd = %d\n",
			  __FUNCTION__, conn->conn_fd);
		shutdown(conn->conn_fd, SHUT_RDWR);
		closesocket(conn->conn_fd);
	}
	if (conn->conn_buf) {
		free(conn->conn_buf);
	}
	if (conn->server)
	{
		free( conn->server );
	}
	pthread_mutex_destroy(&conn->conn_mutex);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);
}

/*
 * cmyth_conn_create()
 *
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Allocate and initialize a cmyth_conn_t structure.  This should only
 * be called by cmyth_connect(), which establishes a connection.
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_conn_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_conn_t
 */
static cmyth_conn_t
cmyth_conn_create(void)
{
	cmyth_conn_t ret = ref_alloc(sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!ret) {
			cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!\n", __FUNCTION__);
		return NULL;
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_conn_destroy);

	ret->conn_fd = -1;
	ret->conn_buf = NULL;
	ret->conn_len = 0;
	ret->conn_buflen = 0;
	ret->conn_pos = 0;
	ret->conn_hang = 0;
	ret->server = NULL;
	ret->port = 0;
	ret->conn_ann = ANN_NONE;
	pthread_mutex_init(&ret->conn_mutex, NULL);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);
	return ret;
}

/*
 * cmyth_connect()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Create a connection to the port specified by 'port' on the
 * server named 'server'.  This creates a data structure called a
 * cmyth_conn_t which contains the file descriptor for a socket, a
 * buffer for reading from the socket, and information used to manage
 * offsets in that buffer.  The buffer length is specified in 'buflen'.
 *
 * The returned connection has a single reference.  The connection
 * will be shut down and closed when the last reference is released
 * using ref_release().
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_conn_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_conn_t
 */
static char my_hostname[128];
static volatile cmyth_socket_t my_fd;

static void
sighandler(int sig)
{
	/*
	 * XXX: This is not thread safe...
	 */
	closesocket(my_fd);
	my_fd = -1;
}

static cmyth_conn_t
cmyth_connect_addr(struct addrinfo* addr, char *server, char *service,
		    int32_t buflen, int32_t tcp_rcvbuf)
{
	cmyth_conn_t ret = NULL;
	unsigned char *buf = NULL;
	cmyth_socket_t fd;
#ifndef _MSC_VER
	void (*old_sighandler)(int);
	int old_alarm;
#endif
	int temp;
	socklen_t size;

	fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (fd < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cannot create socket (%d)\n",
			  __FUNCTION__, errno);
		return NULL;
	}

	/*
	 * Set a 4kb tcp receive buffer on all myth protocol sockets,
	 * otherwise we risk the connection hanging.  Oddly, setting this
	 * on the data sockets causes stuttering during playback.
	 */
	if (tcp_rcvbuf == 0)
		tcp_rcvbuf = 4096;

	temp = tcp_rcvbuf;
	size = sizeof(temp);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: setting socket option SO_RCVBUF to %d", __FUNCTION__, tcp_rcvbuf);
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&temp, size)) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: could not set rcvbuf from socket(%d)\n",
			  __FUNCTION__, errno);
	}
	if(getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&temp, &size)) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: could not get rcvbuf from socket(%d)\n",
			  __FUNCTION__, errno);
	}

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting to %s:%s fd = %d\n",
			__FUNCTION__, server, service, fd);
#ifndef _MSC_VER
	old_sighandler = signal(SIGALRM, sighandler);
	old_alarm = alarm(5);
#endif
	my_fd = fd;
	if (connect(fd, addr->ai_addr, addr->ai_addrlen) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: connect failed on port %s to '%s' (%d)\n",
			  __FUNCTION__, server, service, errno);
		closesocket(fd);
#ifndef _MSC_VER
		signal(SIGALRM, old_sighandler);
		alarm(old_alarm);
#endif
		return NULL;
	}
	my_fd = -1;
#ifndef _MSC_VER
	signal(SIGALRM, old_sighandler);
	alarm(old_alarm);
#endif

	if ((my_hostname[0] == '\0') &&
	    (gethostname(my_hostname, sizeof(my_hostname)) < 0)) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: gethostname failed (%d)\n",
			  __FUNCTION__, errno);
		goto shut;
	}

	/*
	 * Okay, we are connected. Now is a good time to allocate some
	 * resources.
	 */
	ret = cmyth_conn_create();
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_conn_create() failed\n",
			  __FUNCTION__);
		goto shut;
	}
	buf = malloc(buflen * sizeof(unsigned char));
	if (!buf) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s:- malloc(%d) failed allocating buf\n",
			  __FUNCTION__, buflen * sizeof(unsigned char *));
		goto shut;
	}
	ret->conn_fd = fd;
	ret->conn_buflen = buflen;
	ret->conn_buf = buf;
	ret->conn_len = 0;
	ret->conn_pos = 0;
	ret->conn_version = 8;
	ret->conn_tcp_rcvbuf = tcp_rcvbuf;
	return ret;

    shut:
	if (ret) {
		ref_release(ret);
	}

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: error connecting to "
		  "%s, shutdown and close fd = %d\n",
		  __FUNCTION__, server, fd);
	shutdown(fd, 2);
	closesocket(fd);
	return NULL;
}

static int
cmyth_reconnect_addr(cmyth_conn_t conn, struct addrinfo* addr)
{
	cmyth_socket_t fd;
#ifndef _MSC_VER
	void (*old_sighandler)(int);
	int old_alarm;
#endif
	int temp;
	socklen_t size;

	if (conn->conn_fd >= 0) {
		shutdown(conn->conn_fd, 2);
		closesocket(conn->conn_fd);
		conn->conn_fd = -1;
	}

	fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (fd < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cannot create socket (%d)\n",
		  __FUNCTION__, errno);
		return 0;
	}

	/*
	 * Set a 4kb tcp receive buffer on all myth protocol sockets,
	 * otherwise we risk the connection hanging.  Oddly, setting this
	 * on the data sockets causes stuttering during playback.
	 */
	if (conn->conn_tcp_rcvbuf == 0)
		conn->conn_tcp_rcvbuf = 4096;

	temp = conn->conn_tcp_rcvbuf;
	size = sizeof(temp);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: setting socket option SO_RCVBUF to %d", __FUNCTION__, conn->conn_tcp_rcvbuf);
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&temp, size)) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: could not set rcvbuf from socket(%d)\n",
			  __FUNCTION__, errno);
	}
	if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&temp, &size)) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: could not get rcvbuf from socket(%d)\n",
			  __FUNCTION__, errno);
	}

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting to %s:%"PRIu16" fd = %d\n",
		  __FUNCTION__, conn->server, conn->port, fd);
#ifndef _MSC_VER
	old_sighandler = signal(SIGALRM, sighandler);
	old_alarm = alarm(5);
#endif
	my_fd = fd;
	if (connect(fd, addr->ai_addr, addr->ai_addrlen) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: connect failed on port %"PRIu16" to '%s' (%d)\n",
			  __FUNCTION__, conn->server, conn->port, errno);
		closesocket(fd);
#ifndef _MSC_VER
		signal(SIGALRM, old_sighandler);
		alarm(old_alarm);
#endif
		return 0;
	}
	my_fd = -1;
#ifndef _MSC_VER
	signal(SIGALRM, old_sighandler);
	alarm(old_alarm);
#endif

	if ((my_hostname[0] == '\0') &&
		  (gethostname(my_hostname, sizeof(my_hostname)) < 0)) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: gethostname failed (%d)\n",
		  __FUNCTION__, errno);
		goto shut;
	}

	conn->conn_fd = fd;
	conn->conn_len = 0;
	conn->conn_pos = 0;
	return 1;

    shut:
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: error connecting to "
		  "%s, shutdown and close fd = %d\n",
		  __FUNCTION__, conn->server, fd);

	shutdown(fd, 2);
	closesocket(fd);
	return 0;
}

static cmyth_conn_t
cmyth_connect(char *server, uint16_t port, uint32_t buflen,
		    int32_t tcp_rcvbuf)
{
	struct   addrinfo hints;
	struct   addrinfo *result, *addr;
	char     service[33];
	int      res;
	cmyth_conn_t conn = NULL;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	sprintf(service, "%"PRIu16, port);

	res = getaddrinfo(server, service, &hints, &result);
	if(res) {
		switch(res) {
		case EAI_NONAME:
			cmyth_dbg(CMYTH_DBG_ERROR,"%s:- The specified host is unknown\n",
					__FUNCTION__);
			break;

		case EAI_FAIL:
			cmyth_dbg(CMYTH_DBG_ERROR,"%s:- A non-recoverable failure in name resolution occurred\n",
					__FUNCTION__);
			break;

		case EAI_MEMORY:
			cmyth_dbg(CMYTH_DBG_ERROR,"%s:- A memory allocation failure occurred\n",
					__FUNCTION__);
			break;

		case EAI_AGAIN:
			cmyth_dbg(CMYTH_DBG_ERROR,"%s:- A temporary error occurred on an authoritative name server\n",
					__FUNCTION__);
			break;

		default:
			cmyth_dbg(CMYTH_DBG_ERROR,"%s:- Unknown error %d\n",
					__FUNCTION__, res);
			break;
		}
		return NULL;
	}

	for (addr = result; addr; addr = addr->ai_next) {
		conn = cmyth_connect_addr(addr, server, service, buflen, tcp_rcvbuf);
		if (conn)
			break;
	}

	if (conn) {
		conn->server = strdup( server );
		conn->port   = port;
	}

	freeaddrinfo(result);
	return conn;
}

static int
cmyth_reconnect(cmyth_conn_t conn)
{
	struct   addrinfo hints;
	struct   addrinfo *result, *addr;
	char     service[33];
	int      res;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	sprintf(service, "%"PRIu16, conn->port);

	res = getaddrinfo(conn->server, service, &hints, &result);
	if (res) {
		switch(res) {
			case EAI_NONAME:
				cmyth_dbg(CMYTH_DBG_ERROR,"%s:- The specified host is unknown\n",
					  __FUNCTION__);
				break;

			case EAI_FAIL:
				cmyth_dbg(CMYTH_DBG_ERROR,"%s:- A non-recoverable failure in name resolution occurred\n",
					  __FUNCTION__);
				break;

			case EAI_MEMORY:
				cmyth_dbg(CMYTH_DBG_ERROR,"%s:- A memory allocation failure occurred\n",
					  __FUNCTION__);
				break;

			case EAI_AGAIN:
				cmyth_dbg(CMYTH_DBG_ERROR,"%s:- A temporary error occurred on an authoritative name server\n",
					  __FUNCTION__);
				break;

			default:
				cmyth_dbg(CMYTH_DBG_ERROR,"%s:- Unknown error %d\n",
					  __FUNCTION__, res);
				break;
			}
			return 0;
	}

	for (addr = result; addr; addr = addr->ai_next) {
		res = cmyth_reconnect_addr(conn, addr);
		if (res)
			break;
	}

	freeaddrinfo(result);
	return res;
}

static cmyth_conn_t
cmyth_conn_connect(char *server, uint16_t port, uint32_t buflen,
		   int32_t tcp_rcvbuf, int event, cmyth_conn_ann_t ann)
{
	cmyth_conn_t conn;
	char msg[256];
	uint32_t tmp_ver;
	int attempt = 0;

    top:
	conn = cmyth_connect(server, port, buflen, tcp_rcvbuf);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_connect(%s, %"PRIu16", %"PRIu32") failed\n",
			  __FUNCTION__, server, port, buflen);
		return NULL;
	}

	/*
	 * Find out what the Myth Protocol Version is for this connection.
	 * Loop around until we get agreement from the server.
	 */
	if (attempt == 0)
		tmp_ver = conn->conn_version;
	conn->conn_version = tmp_ver;

	/*
	 * Myth 0.23.1 (Myth 0.23 + fixes) introduced an out of sequence protocol version number (23056)
	 * due to the next protocol version number having already been bumped in trunk.
	 *
	 * http://www.mythtv.org/wiki/Myth_Protocol
	 */
	if (tmp_ver >= 62 && tmp_ver != 23056) { // Treat protocol version number 23056 the same as protocol 56
		myth_protomap_t *map = protomap;
		while (map->version != 0 && map->version != tmp_ver)
			map++;
		if (map->version == 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: failed to connect with any version\n",
				  __FUNCTION__);
			goto shut;
		}
		sprintf(msg, "MYTH_PROTO_VERSION %"PRIu32" %s", conn->conn_version, map->token);
	} else {
		sprintf(msg, "MYTH_PROTO_VERSION %"PRIu32, conn->conn_version);
	}
	if (cmyth_send_message(conn, msg) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message('%s') failed\n",
			  __FUNCTION__, msg);
		goto shut;
	}
	if (cmyth_rcv_version(conn, &tmp_ver) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_version() failed\n",
			  __FUNCTION__);
		goto shut;
	}
	cmyth_dbg(CMYTH_DBG_ERROR,
		  "%s: asked for version %"PRIu32", got version %"PRIu32"\n",
		  __FUNCTION__, conn->conn_version, tmp_ver);
	if (conn->conn_version != tmp_ver) {
		if (attempt == 1) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: failed to connect with any version\n",
				  __FUNCTION__);
			goto shut;
		}
		attempt = 1;
		ref_release(conn);
		goto top;
	}
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: agreed on Version %"PRIu32" protocol\n",
		  __FUNCTION__, conn->conn_version);

	sprintf(msg, "ANN %s %s %d", (ann == ANN_MONITOR ? "Monitor" : "Playback"), my_hostname, event);
	if (cmyth_send_message(conn, msg) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message('%s') failed\n",
			  __FUNCTION__, msg);
		goto shut;
	}
	if (cmyth_rcv_okay(conn) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_okay() failed\n",
			  __FUNCTION__);
		goto shut;
	}
	conn->conn_ann = ann;

	/*
	 * All of the downstream code in libcmyth assumes a monotonically increasing version number.
	 * This was not the case for Myth 0.23.1 (0.23 + fixes) where protocol version number 23056
	 * was used since 57 had already been used in trunk.
	 *
	 * Convert from protocol version number 23056 to version number 56 so subsequent code within
	 * libcmyth uses the same logic for the 23056 protocol as would be used for protocol version 56.
	 */
	if (conn->conn_version == 23056) {
		conn->conn_version = 56;
	}

	return conn;

    shut:
	ref_release(conn);
	return NULL;
}

static int
cmyth_conn_reconnect(cmyth_conn_t conn, int event, cmyth_conn_ann_t ann)
{
	char msg[256];
	uint32_t tmp_ver;
	int attempt = 0;
	int ret = 0;

    top:
	ret = cmyth_reconnect(conn);
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_reconnect() failed\n",
			  __FUNCTION__);
		return ret;
	}

	/*
	 * Find out what the Myth Protocol Version is for this connection.
	 * Loop around until we get agreement from the server.
	 */
	if (attempt == 0)
		tmp_ver = conn->conn_version;
	conn->conn_version = tmp_ver;

	/*
	 * Myth 0.23.1 (Myth 0.23 + fixes) introduced an out of sequence protocol version number (23056)
	 * due to the next protocol version number having already been bumped in trunk.
	 *
	 * http://www.mythtv.org/wiki/Myth_Protocol
	 */
	if (tmp_ver >= 62 && tmp_ver != 23056) { // Treat protocol version number 23056 the same as protocol 56
		myth_protomap_t *map = protomap;
		while (map->version != 0 && map->version != tmp_ver)
			map++;
		if (map->version == 0) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: failed to connect with any version\n",
				  __FUNCTION__);
			goto shut;
		}
		sprintf(msg, "MYTH_PROTO_VERSION %"PRIu32" %s", conn->conn_version, map->token);
	} else {
		sprintf(msg, "MYTH_PROTO_VERSION %"PRIu32, conn->conn_version);
	}
	if (cmyth_send_message(conn, msg) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_send_message('%s') failed\n",
			  __FUNCTION__, msg);
		goto shut;
	}
	if (cmyth_rcv_version(conn, &tmp_ver) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_version() failed\n",
			  __FUNCTION__);
		goto shut;
	}
	cmyth_dbg(CMYTH_DBG_ERROR, "%s: asked for version %"PRIu32", got version %"PRIu32"\n",
		  __FUNCTION__, conn->conn_version, tmp_ver);
	if (conn->conn_version != tmp_ver) {
		if (attempt == 1) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: failed to connect with any version\n",
			  __FUNCTION__);
			goto shut;
		}
		attempt = 1;
		goto top;
	}
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: agreed on Version %"PRIu32" protocol\n",
		  __FUNCTION__, conn->conn_version);

	sprintf(msg, "ANN %s %s %d", (ann == ANN_MONITOR ? "Monitor" : "Playback"), my_hostname, event);
	if (cmyth_send_message(conn, msg) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_send_message('%s') failed\n",
			  __FUNCTION__, msg);
		goto shut;
	}
	if (cmyth_rcv_okay(conn) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_okay() failed\n",
			  __FUNCTION__);
		goto shut;
	}
	conn->conn_ann = ann;

	/*
	 * All of the downstream code in libcmyth assumes a monotonically increasing version number.
	 * This was not the case for Myth 0.23.1 (0.23 + fixes) where protocol version number 23056
	 * was used since 57 had already been used in trunk.
	 *
	 * Convert from protocol version number 23056 to version number 56 so subsequent code within
	 * libcmyth uses the same logic for the 23056 protocol as would be used for protocol version 56.
	 */
	if (conn->conn_version == 23056) {
		conn->conn_version = 56;
	}

	return ret;

    shut:
	return ret;
}

/*
 * cmyth_conn_connect_ctrl()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Create a connection for use as a control connection within the
 * MythTV protocol.  Return a pointer to the newly created connection.
 * The connection is returned held, and may be released using
 * ref_release().
 *
 * Return Value:
 *
 * Success: Non-NULL cmyth_conn_t (this is a pointer type)
 *
 * Failure: NULL cmyth_conn_t
 */
cmyth_conn_t
cmyth_conn_connect_ctrl(char *server, uint16_t port, uint32_t buflen,
			int32_t tcp_rcvbuf)
{
	cmyth_conn_t ret;

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting control connection\n",
		  __FUNCTION__);
	ret = cmyth_conn_connect(server, port, buflen, tcp_rcvbuf, 0, ANN_PLAYBACK);
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: done connecting control connection ret = %p\n",
		  __FUNCTION__, ret);
	return ret;
}

/*
 * cmyth_conn_reconnect_ctrl()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Reconnect connection for use as a control connection within the
 * MythTV protocol.
 *
 * Return Value:
 *
 * Success: 1
 *
 * Failure: 0
 */
int
cmyth_conn_reconnect_ctrl(cmyth_conn_t control)
{
	int ret;

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: reconnecting control connection\n",
		  __FUNCTION__);
	if (control)
		ret = cmyth_conn_reconnect(control, 0, ANN_PLAYBACK);
	else
		ret = 0;
	if (ret)
		control->conn_hang = 0;
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: done reconnecting control connection ret = %d\n",
		  __FUNCTION__, ret);
	return ret;
}

/*
 * cmyth_conn_connect_event()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Create a connection for use as a event connection within the
 * MythTV protocol.  Return a pointer to the newly created connection.
 * The connection is returned held, and may be released using
 * ref_release().
 *
 * Return Value:
 *
 * Success: Non-NULL cmyth_conn_t (this is a pointer type)
 *
 * Failure: NULL cmyth_conn_t
 */
cmyth_conn_t
cmyth_conn_connect_event(char *server, uint16_t port, uint32_t buflen,
			 int32_t tcp_rcvbuf)
{
	cmyth_conn_t ret;
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting event channel connection\n",
		  __FUNCTION__);
	ret = cmyth_conn_connect(server, port, buflen, tcp_rcvbuf, 1, ANN_MONITOR);
	cmyth_dbg(CMYTH_DBG_PROTO,
		  "%s: done connecting event channel connection ret = %p\n",
		  __FUNCTION__, ret);
	return ret;
}

/*
 * cmyth_conn_reconnect_event()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Reconnect connection for use as a event connection within the
 * MythTV protocol.
 *
 * Return Value:
 *
 * Success: 1
 *
 * Failure: 0
 */
int
cmyth_conn_reconnect_event(cmyth_conn_t conn)
{
	int ret;
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: re-connecting event channel connection\n",
		  __FUNCTION__);
	if (conn)
		ret = cmyth_conn_reconnect(conn, 1, ANN_MONITOR);
	else
		ret = 0;
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: done re-connecting event channel connection ret = %d\n",
		  __FUNCTION__, ret);
	return ret;
}

/*
 * cmyth_conn_connect_monitor()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Create a connection for use as a monitor connection within the
 * MythTV protocol.  Return a pointer to the newly created connection.
 * The connection is returned held, and may be released using
 * ref_release().
 *
 * Return Value:
 *
 * Success: Non-NULL cmyth_conn_t (this is a pointer type)
 *
 * Failure: NULL cmyth_conn_t
 */
cmyth_conn_t
cmyth_conn_connect_monitor(char *server, uint16_t port, uint32_t buflen,
			int32_t tcp_rcvbuf)
{
	cmyth_conn_t ret;

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting monitor connection\n",
		  __FUNCTION__);
	ret = cmyth_conn_connect(server, port, buflen, tcp_rcvbuf, 0, ANN_MONITOR);
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: done connecting monitor connection ret = %p\n",
		  __FUNCTION__, ret);
	return ret;
}

/*
 * cmyth_conn_reconnect_monitor()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Reconnect connection for use as a monitor connection within the
 * MythTV protocol.
 *
 * Return Value:
 *
 * Success: 1
 *
 * Failure: 0
 */
int
cmyth_conn_reconnect_monitor(cmyth_conn_t control)
{
	int ret;

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: reconnecting monitor connection\n",
		  __FUNCTION__);
	if (control)
		ret = cmyth_conn_reconnect(control, 0, ANN_MONITOR);
	else
		ret = 0;
	if (ret)
		control->conn_hang = 0;
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: done reconnecting monitor connection ret = %d\n",
		  __FUNCTION__, ret);
	return ret;
}

/*
 * cmyth_conn_connect_playback()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Create a connection for use as a playback connection within the
 * MythTV protocol.  Return a pointer to the newly created connection.
 * The connection is returned held, and may be released using
 * ref_release().
 *
 * Return Value:
 *
 * Success: Non-NULL cmyth_conn_t (this is a pointer type)
 *
 * Failure: NULL cmyth_conn_t
 */
cmyth_conn_t
cmyth_conn_connect_playback(char *server, uint16_t port, uint32_t buflen,
			int32_t tcp_rcvbuf)
{
	cmyth_conn_t ret;

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting playback connection\n",
		  __FUNCTION__);
	ret = cmyth_conn_connect(server, port, buflen, tcp_rcvbuf, 0, ANN_PLAYBACK);
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: done connecting playback connection ret = %p\n",
		  __FUNCTION__, ret);
	return ret;
}

/*
 * cmyth_conn_reconnect_playback()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Reconnect connection for use as a playback connection within the
 * MythTV protocol.
 *
 * Return Value:
 *
 * Success: 1
 *
 * Failure: 0
 */
int
cmyth_conn_reconnect_playback(cmyth_conn_t control)
{
	int ret;

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: reconnecting playback connection\n",
		  __FUNCTION__);
	if (control)
		ret = cmyth_conn_reconnect(control, 0, ANN_PLAYBACK);
	else
		ret = 0;
	if (ret)
		control->conn_hang = 0;
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: done reconnecting playback connection ret = %d\n",
		  __FUNCTION__, ret);
	return ret;
}

/*
 * cmyth_conn_connect_file()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Create a file structure containing a data connection for use
 * transfering a file within the MythTV protocol.  Return a pointer to
 * the newly created file structure.  The connection in the file
 * structure is returned held as is the file structure itself.  The
 * connection will be released when the file structure is released.
 * The file structure can be released using ref_release().
 *
 * Return Value:
 *
 * Success: Non-NULL cmyth_file_t (this is a pointer type)
 *
 * Failure: NULL cmyth_file_t
 */
cmyth_file_t
cmyth_conn_connect_file(cmyth_proginfo_t prog,  cmyth_conn_t control,
			uint32_t buflen, int32_t tcp_rcvbuf)
{
	cmyth_conn_t conn = NULL;
	char *announcement = NULL;
	char reply[16];
	int err = 0;
	int count = 0;
	int r;
	int ann_size = sizeof("ANN FileTransfer  0 0 0000[]:[][]:[]");
	cmyth_file_t ret = NULL;
	uint32_t file_id;
	int64_t file_length;

	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: prog is NULL\n", __FUNCTION__);
		goto shut;
	}
	if (!prog->proginfo_host) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: prog host is NULL\n",
			  __FUNCTION__);
		goto shut;
	}
	if (!prog->proginfo_pathname) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: prog has no pathname in it\n",
			  __FUNCTION__);
		goto shut;
	}
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting data connection\n", __FUNCTION__);
	conn = cmyth_connect(control->server, control->port, buflen, tcp_rcvbuf);
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: done connecting data connection, conn = %d\n", __FUNCTION__, conn);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_connect(%s, %"PRIu16", %"PRIu32") failed\n", __FUNCTION__, control->server, control->port, buflen);
		goto shut;
	}
	/*
	 * Explicitly set the conn version to the control version as cmyth_connect() doesn't and some of
	 * the cmyth_rcv_* functions expect it to be the same as the protocol version used by mythbackend.
	 */
	conn->conn_version = control->conn_version;

	ann_size += strlen(prog->proginfo_pathname) + strlen(my_hostname);
	announcement = malloc(ann_size);
	if (!announcement) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: malloc(%d) failed for announcement\n",
			  __FUNCTION__, ann_size);
		goto shut;
	}
	if (control->conn_version >= 44) {
		sprintf(announcement, "ANN FileTransfer %s 0 0 1000[]:[]%s[]:[]", // write = false
			  my_hostname, prog->proginfo_pathname);
	}
	else {
		sprintf(announcement, "ANN FileTransfer %s[]:[]%s",
			  my_hostname, prog->proginfo_pathname);
	}

	if (cmyth_send_message(conn, announcement) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message('%s') failed\n",
			  __FUNCTION__, announcement);
		free(announcement);
		goto shut;
	}
	free(announcement);
	count = cmyth_rcv_length(conn);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		goto shut;
	}
	reply[sizeof(reply) - 1] = '\0';
	r = cmyth_rcv_string(conn, &err, reply, sizeof(reply) - 1, count);
	if (err != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_string() failed (%d)\n",
			  __FUNCTION__, err);
		goto shut;
	}
	if (strcmp(reply, "OK") != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: reply ('%s') is not 'OK'\n",
			  __FUNCTION__, reply);
		goto shut;
	}
	count -= r;
	r = cmyth_rcv_uint32(conn, &err, &file_id, count);
	if (err) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: (id) cmyth_rcv_long() failed (%d)\n",
			  __FUNCTION__, err);
		goto shut;
	}
	count -= r;
	r = cmyth_rcv_int64(conn, &err, &file_length, count);
	if (err) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: (length) cmyth_rcv_int64() failed (%d)\n",
			  __FUNCTION__, err);
		goto shut;
	}
	count -= r;
	if (count != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: %d leftover bytes\n",
			  __FUNCTION__, count);
	}

	ret = cmyth_file_create(control);
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_file_create() failed\n",
			  __FUNCTION__);
		goto shut;
	}
	ret->file_data = conn;
	ret->file_id = file_id;
	ret->file_length = file_length;
	return ret;

    shut:
	ref_release(conn);
	return NULL;
}

/*
 * cmyth_conn_connect_path()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Create a file structure containing a data connection for use
 * transfering a file within the MythTV protocol.  Return a pointer to
 * the newly created file structure.  The connection in the file
 * structure is returned held as is the file structure itself.  The
 * connection will be released when the file structure is released.
 * The file structure can be released using ref_release().
 *
 * Return Value:
 *
 * Success: Non-NULL cmyth_file_t (this is a pointer type)
 *
 * Failure: NULL cmyth_file_t
 */
cmyth_file_t
cmyth_conn_connect_path(char* path, cmyth_conn_t control,
			uint32_t buflen, int32_t tcp_rcvbuf, char* storage_group)
{
	cmyth_conn_t conn = NULL;
	char *announcement = NULL;
	char reply[16];
	int err = 0;
	int count = 0;
	int r;
	int ann_size = sizeof("ANN FileTransfer  0 0 0000[]:[][]:[]");
	cmyth_file_t ret = NULL;
	uint32_t file_id;
	int64_t file_length;

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting data connection\n",
		  __FUNCTION__);
	conn = cmyth_connect(control->server, control->port, buflen, tcp_rcvbuf);
	cmyth_dbg(CMYTH_DBG_PROTO,
		  "%s: done connecting data connection, conn = %p\n",
		  __FUNCTION__, conn);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_connect(%s, %"PRIu16", %"PRIu32") failed\n",
			  __FUNCTION__, control->server, control->port, buflen);
		goto shut;
	}
	/*
	 * Explicitly set the conn version to the control version as cmyth_connect() doesn't and some of
	 * the cmyth_rcv_* functions expect it to be the same as the protocol version used by mythbackend.
	 */
	conn->conn_version = control->conn_version;

	ann_size += strlen(path) + strlen(my_hostname) + strlen(storage_group);
	announcement = malloc(ann_size);
	if (!announcement) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: malloc(%d) failed for announcement\n",
			  __FUNCTION__, ann_size);
		goto shut;
	}
	if (control->conn_version >= 44) { /*TSP: from version 44 according to the source code*/
		if (strlen(storage_group) > 1) {
			sprintf(announcement, "ANN FileTransfer %s 0 0 100[]:[]%s[]:[]%s",
				  my_hostname, path, storage_group);
		} else {
			sprintf(announcement, "ANN FileTransfer %s 0 0 100[]:[]%s[]:[]",  // write = false
				  my_hostname, path);
		}
	} else {
		sprintf(announcement, "ANN FileTransfer %s[]:[]%s",
			  my_hostname, path);
	}
	if (cmyth_send_message(conn, announcement) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message('%s') failed\n",
			  __FUNCTION__, announcement);
		free(announcement);
		goto shut;
	}
	free(announcement);
	count = cmyth_rcv_length(conn);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		goto shut;
	}
	reply[sizeof(reply) - 1] = '\0';
	r = cmyth_rcv_string(conn, &err, reply, sizeof(reply) - 1, count);
	if (err != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_string() failed (%d)\n",
			  __FUNCTION__, err);
		goto shut;
	}
	if (strcmp(reply, "OK") != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: reply ('%s') is not 'OK'\n",
			  __FUNCTION__, reply);
		goto shut;
	}
	count -= r;
	r = cmyth_rcv_uint32(conn, &err, &file_id, count);
	if (err) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: (id) cmyth_rcv_long() failed (%d)\n",
			  __FUNCTION__, err);
		goto shut;
	}
	count -= r;
	r = cmyth_rcv_int64(conn, &err, &file_length, count);
	if (err) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: (length) cmyth_rcv_int64() failed (%d)\n",
			  __FUNCTION__, err);
		goto shut;
	}
	count -= r;
	if (count != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: %d leftover bytes\n",
			  __FUNCTION__, count);
	}

	ret = cmyth_file_create(control);
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_file_create() failed\n",
			  __FUNCTION__);
		goto shut;
	}
	ret->file_data = conn;
	ret->file_id = file_id;
	ret->file_length = file_length;
	return ret;

    shut:
	ref_release(conn);
	return NULL;
}

/*
 * cmyth_conn_connect_ring()
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Create a new ring buffer connection for use transferring live-tv
 * using the MythTV protocol.  Return a pointer to the newly created
 * ring buffer connection.  The ring buffer connection is returned
 * held, and may be released using ref_release().
 *
 * Return Value:
 *
 * Success: Non-NULL cmyth_conn_t (this is a pointer type)
 *
 * Failure: NULL cmyth_conn_t
 */
int
cmyth_conn_connect_ring(cmyth_recorder_t rec, uint32_t buflen, int32_t tcp_rcvbuf)
{
	cmyth_conn_t conn;
	char *announcement;
	int ann_size = sizeof("ANN RingBuffer  ");
	char *server;
	uint16_t port;

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: rec is NULL\n", __FUNCTION__);
		return -1;
	}

	server = rec->rec_server;
	port = rec->rec_port;

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting ringbuffer\n",
		  __FUNCTION__);
	conn = cmyth_connect(server, port, buflen, tcp_rcvbuf);
	cmyth_dbg(CMYTH_DBG_PROTO,
		  "%s: connecting ringbuffer, conn = %p\n",
		  __FUNCTION__, conn);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_connect(%s, %"PRIu16", %"PRIu32") failed\n",
			  __FUNCTION__, server, port, buflen);
		return -1;
	}

	ann_size += CMYTH_INT32_LEN + strlen(my_hostname);
	announcement = malloc(ann_size);
	if (!announcement) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: malloc(%d) failed for announcement\n",
			  __FUNCTION__, ann_size);
		goto shut;
	}
	sprintf(announcement,
		"ANN RingBuffer %s %"PRIu32, my_hostname, rec->rec_id);
	if (cmyth_send_message(conn, announcement) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message('%s') failed\n",
			  __FUNCTION__, announcement);
		free(announcement);
		goto shut;
	}
	free(announcement);
	if (cmyth_rcv_okay(conn) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_okay() failed\n",
			  __FUNCTION__);
		goto shut;
	}

        rec->rec_ring->conn_data = conn;
	return 0;

    shut:
	ref_release(conn);
	return -1;
}

int
cmyth_conn_connect_recorder(cmyth_recorder_t rec, uint32_t buflen,
			    int32_t tcp_rcvbuf)
{
	cmyth_conn_t conn;
	char *server;
	uint16_t port;

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: rec is NULL\n", __FUNCTION__);
		return -1;
	}

	server = rec->rec_server;
	port = rec->rec_port;

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting recorder control\n",
		  __FUNCTION__);
	conn = cmyth_conn_connect(server, port, buflen, tcp_rcvbuf, 0, ANN_PLAYBACK);
	cmyth_dbg(CMYTH_DBG_PROTO,
		  "%s: done connecting recorder control, conn = %p\n",
		  __FUNCTION__, conn);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_connect(%s, %"PRIu16", %"PRIu32") failed\n",
			  __FUNCTION__, server, port, buflen);
		return -1;
	}

	if (rec->rec_conn)
		ref_release(rec->rec_conn);
	rec->rec_conn = conn;

	return 0;
}

/*
 * cmyth_conn_check_block()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Check whether a block has finished transfering from a backend
 * server. This non-blocking check looks for a response from the
 * server indicating that a block has been entirely sent to on a data
 * socket.
 *
 * Return Value:
 *
 * Success: 0 for not complete, 1 for complete
 *
 * Failure: -(errno)
 */
int
cmyth_conn_check_block(cmyth_conn_t conn, uint32_t size)
{
	fd_set check;
	struct timeval timeout;
	int length;
	int err = 0;
	uint32_t sent;

	if (!conn) {
		return -EINVAL;
	}
	timeout.tv_sec = timeout.tv_usec = 0;
	FD_ZERO(&check);
	FD_SET(conn->conn_fd, &check);
	if (select((int)conn->conn_fd + 1, &check, NULL, NULL, &timeout) < 0) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: select failed (%d)\n",
			  __FUNCTION__, errno);
		return -(errno);
	}
	if (FD_ISSET(conn->conn_fd, &check)) {
		/*
		 * We have a bite, reel it in.
		 */
		length = cmyth_rcv_length(conn);
		if (length < 0) {
			return length;
		}
		cmyth_rcv_uint32(conn, &err, &sent, length);
		if (err) {
			return -err;
		}
		if (sent == size) {
			/*
			 * This block has been sent, return TRUE.
			 */
			cmyth_dbg(CMYTH_DBG_DEBUG,
				  "%s: block finished (%"PRIu32" bytes)\n",
				  __FUNCTION__, sent);
			return 1;
		} else {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: block finished short (%"PRIu32" bytes)\n",
				  __FUNCTION__, sent);
			return -ECANCELED;
		}
	}
	return 0;
}

/*
 * cmyth_conn_get_recorder_from_num()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Obtain a recorder from a connection by its recorder number.  The
 * recorder structure created by this describes how to set up a data
 * connection and play media streamed from a particular back-end recorder.
 *
 * This fills out the recorder structure specified by 'rec'.
 *
 * Return Value:
 *
 * Success: non-NULL cmyth_recorder_t (this type is a pointer)
 *
 * Failure: NULL cmyth_recorder_t
 */
cmyth_recorder_t
cmyth_conn_get_recorder_from_num(cmyth_conn_t conn, int32_t id)
{
	int err, count;
	int r;
	uint16_t port;
	char msg[256];
	char reply[256];
	cmyth_recorder_t rec = NULL;

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return NULL;
	}

	pthread_mutex_lock(&conn->conn_mutex);

	if ((rec=cmyth_recorder_create()) == NULL)
		goto fail;

	snprintf(msg, sizeof(msg), "GET_RECORDER_FROM_NUM[]:[]%"PRId32, id);

	if ((err = cmyth_send_message(conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto fail;
	}

	count = cmyth_rcv_length(conn);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		goto fail;
	}

	if ((r=cmyth_rcv_string(conn, &err,
				reply, sizeof(reply)-1, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_string() failed (%d)\n",
			  __FUNCTION__, r);
		goto fail;
	}
	count -= r;

	if ((r=cmyth_rcv_uint16(conn, &err, &port, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_long() failed (%d)\n",
			  __FUNCTION__, r);
		goto fail;
	}

	rec->rec_id = id;
	rec->rec_server = ref_strdup(reply);
	rec->rec_port = port;

	if (cmyth_conn_connect_recorder(rec, conn->conn_buflen,
					conn->conn_tcp_rcvbuf) < 0)
		goto fail;

	pthread_mutex_unlock(&conn->conn_mutex);

	return rec;

    fail:
	if (rec)
		ref_release(rec);

	pthread_mutex_unlock(&conn->conn_mutex);

	return NULL;
}

/*
 * cmyth_conn_get_free_recorder()
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Obtain the next available free recorder the connection specified by
 * 'control'.  This fills out the recorder structure specified by 'rec'.
 *
 * Return Value:
 *
 * Success: non-NULL cmyth_recorder_t (this type is a pointer)
 *
 * Failure: NULL cmyth_recorder_t
 */
cmyth_recorder_t
cmyth_conn_get_free_recorder(cmyth_conn_t conn)
{
	int err, count;
	int r;
	uint16_t port;
	uint32_t id;
	char msg[256];
	char reply[256];
	cmyth_recorder_t rec = NULL;

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return NULL;
	}

	pthread_mutex_lock(&conn->conn_mutex);

	if ((rec=cmyth_recorder_create()) == NULL)
		goto fail;

	snprintf(msg, sizeof(msg), "GET_FREE_RECORDER");

	if ((err = cmyth_send_message(conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto fail;
	}

	count = cmyth_rcv_length(conn);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		goto fail;
	}
	if ((r=cmyth_rcv_uint32(conn, &err, &id, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_long() failed (%d)\n",
			  __FUNCTION__, r);
		goto fail;
	}
	count -= r;
	if ((r=cmyth_rcv_string(conn, &err,
				reply, sizeof(reply)-1, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_string() failed (%d)\n",
			  __FUNCTION__, r);
		goto fail;
	}
	count -= r;
	if ((r=cmyth_rcv_uint16(conn, &err, &port, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_long() failed (%d)\n",
			  __FUNCTION__, r);
		goto fail;
	}

	rec->rec_id = id;
	rec->rec_server = ref_strdup(reply);
	rec->rec_port = port;

	if (cmyth_conn_connect_recorder(rec, conn->conn_buflen,
					conn->conn_tcp_rcvbuf) < 0)
		goto fail;

	pthread_mutex_unlock(&conn->conn_mutex);

	return rec;

    fail:
	if (rec)
		ref_release(rec);

	pthread_mutex_unlock(&conn->conn_mutex);

	return NULL;
}

int
cmyth_conn_get_freespace(cmyth_conn_t control,
			 int64_t *total, int64_t *used)
{
	int err, count, ret = 0;
	int r;
	char msg[256];
	char reply[256];
	int64_t lreply;

	if (control == NULL)
		return -EINVAL;

	if ((total == NULL) || (used == NULL))
		return -EINVAL;

	pthread_mutex_lock(&control->conn_mutex);

	if (control->conn_version >= 32)
		{ snprintf(msg, sizeof(msg), "QUERY_FREE_SPACE_SUMMARY"); }
	else if (control->conn_version >= 17)
		{ snprintf(msg, sizeof(msg), "QUERY_FREE_SPACE"); }
	else
		{ snprintf(msg, sizeof(msg), "QUERY_FREESPACE"); }

	if ((err = cmyth_send_message(control, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		ret = err;
		goto out;
	}

	if ((count=cmyth_rcv_length(control)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		ret = count;
		goto out;
	}

	if (control->conn_version >= 17) {
		if ((r=cmyth_rcv_int64(control, &err, &lreply, count)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_rcv_int64() failed (%d)\n",
				  __FUNCTION__, err);
			ret = err;
			goto out;
		}
		*total = lreply;
		if ((r=cmyth_rcv_int64(control, &err, &lreply, count - r)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_rcv_int64() failed (%d)\n",
				  __FUNCTION__, err);
			ret = err;
			goto out;
		}
		*used = lreply;
	}
	else
		{
			if ((r=cmyth_rcv_string(control, &err, reply,
						sizeof(reply)-1, count)) < 0) {
				cmyth_dbg(CMYTH_DBG_ERROR,
					  "%s: cmyth_rcv_string() failed (%d)\n",
					  __FUNCTION__, err);
				ret = err;
				goto out;
			}
			*total = atol(reply);
			if ((r=cmyth_rcv_string(control, &err, reply,
						sizeof(reply)-1,
						count-r)) < 0) {
				cmyth_dbg(CMYTH_DBG_ERROR,
					  "%s: cmyth_rcv_string() failed (%d)\n",
					  __FUNCTION__, err);
				ret = err;
				goto out;
			}
			*used = atol(reply);

			*used *= 1024;
			*total *= 1024;
		}

    out:
	pthread_mutex_unlock(&control->conn_mutex);

	return ret;
}

int
cmyth_conn_hung(cmyth_conn_t control)
{
	if (control == NULL)
		return -EINVAL;

	return control->conn_hang;
}

int32_t
cmyth_conn_get_protocol_version(cmyth_conn_t conn)
{
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			__FUNCTION__);
		return -EINVAL;
	}

	return conn->conn_version;
}


int32_t
cmyth_conn_get_free_recorder_count(cmyth_conn_t conn)
{
	char msg[256];
	int count, err;
	uint16_t c;
	int r;
	int ret;

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}

	pthread_mutex_lock(&conn->conn_mutex);

	snprintf(msg, sizeof(msg), "GET_FREE_RECORDER_COUNT");
	if ((r = cmyth_send_message(conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, r);
		ret = r;
		goto err;
	}

	if ((count = cmyth_rcv_length(conn)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		ret = count;
		goto err;
	}
	if ((r = cmyth_rcv_uint16(conn, &err, &c, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_long() failed (%d)\n",
			  __FUNCTION__, r);
		ret = err;
		goto err;
	}

	ret = (int)c;

    err:
	pthread_mutex_unlock(&conn->conn_mutex);

	return ret;
}

char *
cmyth_conn_get_backend_hostname(cmyth_conn_t conn)
{
	int count, err;
	char* result = NULL;

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return NULL;
	}

	pthread_mutex_lock(&conn->conn_mutex);
	if(conn->conn_version < 17) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: protocol version doesn't support QUERY_HOSTNAME\n",
			  __FUNCTION__);
		goto err;
	}

	if ((err = cmyth_send_message(conn,  "QUERY_HOSTNAME")) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto err;
	}

	if ((count = cmyth_rcv_length(conn)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		goto err;
	}

	result = ref_alloc(count+1);
	count -= cmyth_rcv_string(conn, &err,
				    result, count, count);
	if (err < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_string() failed (%d)\n",
			  __FUNCTION__, err);
		goto err;
	}

	while(count > 0 && !err) {
		char buffer[100];
		count -= cmyth_rcv_string(conn, &err, buffer, sizeof(buffer)-1, count);
		buffer[sizeof(buffer)-1] = 0;
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: odd left over data %s\n", __FUNCTION__, buffer);
	}
	pthread_mutex_unlock(&conn->conn_mutex);

	if(!strcmp("-1",result))  {
		cmyth_dbg(CMYTH_DBG_PROTO, "%s: Failed to retrieve backend hostname.\n",
			  __FUNCTION__);
	return NULL;
	}
	return result;

err:
	pthread_mutex_unlock(&conn->conn_mutex);
	if(result)
		ref_release(result);

	return NULL;
}

char *
cmyth_conn_get_client_hostname(cmyth_conn_t conn)
{
	char* result=NULL;
	result = ref_strdup(my_hostname);
	return result;
}

static char *
cmyth_conn_get_setting_unlocked(cmyth_conn_t conn, const char* hostname, const char* setting)
{
	char msg[256];
	int count, err;
	char* result = NULL;

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return NULL;
	}

	if(conn->conn_version < 17) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: protocol version doesn't support QUERY_SETTING\n",
			  __FUNCTION__);
		return NULL;
	}

	snprintf(msg, sizeof(msg), "QUERY_SETTING %s %s", hostname, setting);
	if ((err = cmyth_send_message(conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto err;
	}

	if ((count=cmyth_rcv_length(conn)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		goto err;
	}

	result = ref_alloc(count+1);
	count -= cmyth_rcv_string(conn, &err,
				    result, count, count);
	if (err < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_string() failed (%d)\n",
			  __FUNCTION__, err);
		goto err;
	}

	while(count > 0 && !err) {
		char buffer[100];
		count -= cmyth_rcv_string(conn, &err, buffer, sizeof(buffer)-1, count);
		buffer[sizeof(buffer)-1] = 0;
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: odd left over data %s\n", __FUNCTION__, buffer);
	}

	if(!strcmp("-1",result))  {
		cmyth_dbg(CMYTH_DBG_PROTO, "%s: Setting: %s or hostname: %s not found.\n",
			  __FUNCTION__, setting,hostname);
		return NULL;
	}
	return result;

err:
	if(result)
		ref_release(result);

	return NULL;
}

char *
cmyth_conn_get_setting(cmyth_conn_t conn, const char* hostname, const char* setting)
{
	char* result = NULL;

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return NULL;
	}

	pthread_mutex_lock(&conn->conn_mutex);
	result = cmyth_conn_get_setting_unlocked(conn, hostname, setting);
	pthread_mutex_unlock(&conn->conn_mutex);

	return result;
}

static int cmyth_conn_set_setting_unlocked(cmyth_conn_t conn,
               const char* hostname, const char* setting, const char* value)
{
	char msg[1024];
	int err = 0;

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}

	if(conn->conn_version < 17) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: protocol version doesn't support SET_SETTING\n",
			  __FUNCTION__);
		return -EPERM;
	}

	snprintf(msg, sizeof(msg), "SET_SETTING %s %s %s", hostname, setting, value);
	if ((err = cmyth_send_message(conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		return err;
	}

	if ((err = cmyth_rcv_okay(conn)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_okay() failed\n",
			  __FUNCTION__);
		return err;
	}

	return 0;
}

int cmyth_conn_set_setting(cmyth_conn_t conn,
               const char* hostname, const char* setting, const char* value)
{
	int result;

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
		  __FUNCTION__);
		return -EINVAL;
	}

	pthread_mutex_lock(&conn->conn_mutex);
	result = cmyth_conn_set_setting_unlocked(conn, hostname, setting, value);
	pthread_mutex_unlock(&conn->conn_mutex);

	return result;
}

/*
 * cmyth_conn_reschedule_recordings()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Issues a run of the re-scheduler.
 * Takes an optional recordid, or -1 (0xFFFFFFFF) performs a full run.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_conn_reschedule_recordings(cmyth_conn_t conn, uint32_t recordid)
{
	int err = 0;
	char msg[256];

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}

	if (conn->conn_version < 15) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: protocol version doesn't support RESCHEDULE_RECORDINGS\n",
			  __FUNCTION__);
		return -EPERM;
	}

	/*
	 * RESCHEDULE_RECORDINGS changed in protocol version 73:
	 *
	 * MATCH reschedule requests should be used when the guide data or a
	 * specific recording rule is changed. The syntax is as follows.
	 *
	 *    MATCH <recordid> <sourceid> <mplexid> <maxstarttime> <reason>
	 *
	 * CHECK reschedule requests should be used when the status of a
	 * specific episode is affected such as when "never record" or "allow
	 * re-record" are selected or a recording finishes or is deleted. The
	 * syntax is as follows.
	 *
	 *    CHECK <recstatus> <recordid> <findid> <reason>
	 *    <title>
	 *    <subtitle>
	 *    <description>
	 *    <programid>
	 */
	if (conn->conn_version < 73) {
		if (recordid > 0 && recordid < UINT32_MAX)
			snprintf(msg, sizeof(msg), "RESCHEDULE_RECORDINGS %"PRIu32, recordid);
		else
			snprintf(msg, sizeof(msg), "RESCHEDULE_RECORDINGS -1");
	} else {
		if (recordid == 0) {
			strncpy(msg, "RESCHEDULE_RECORDINGS []:[]CHECK 0 0 0 cmyth[]:[][]:[][]:[][]:[]**any**", sizeof(msg));
		} else {
			if (recordid > 0 && recordid < UINT32_MAX)
				snprintf(msg, sizeof(msg), "RESCHEDULE_RECORDINGS []:[]MATCH %"PRIu32" 0 0 - cmyth", recordid);
			else
				snprintf(msg, sizeof(msg), "RESCHEDULE_RECORDINGS []:[]MATCH 0 0 0 - cmyth");
		}
	}

	pthread_mutex_lock(&conn->conn_mutex);

	if ((err = cmyth_send_message(conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto out;
	}

	if ((err=cmyth_rcv_feedback(conn, "1", 1)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_feedback() failed (%d)\n",
			  __FUNCTION__, err);
		goto out;
	}

out:
	pthread_mutex_unlock(&conn->conn_mutex);
	return err;
}
