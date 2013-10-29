/*
 *  Copyright (C) 2004-2012, Eric Lund
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

#include <sys/types.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <string.h>
#ifdef _MSC_VER
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <cmyth_local.h>
#include <safe_string.h>

void
cmyth_database_close(cmyth_database_t db)
{
	if (db->mysql != NULL) {
		mysql_close(db->mysql);
		db->mysql = NULL;
		db->db_setup = 0;
		db->db_version = 0;
		db->db_tz_utc = 0;
	}
}

static void
cmyth_database_destroy(cmyth_database_t db)
{
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	cmyth_database_close(db);

	if (db->db_host)
		ref_release(db->db_host);
	if (db->db_user)
		ref_release(db->db_user);
	if (db->db_pass)
		ref_release(db->db_pass);
	if (db->db_name)
		ref_release(db->db_name);
}

cmyth_database_t
cmyth_database_init(char *host, char *db_name, char *user, char *pass, unsigned short port)
{
	cmyth_database_t rtrn = ref_alloc(sizeof(*rtrn));
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);

	ref_set_destroy(rtrn, (ref_destroy_t)cmyth_database_destroy);

	if (rtrn != NULL) {
		rtrn->db_host = ref_strdup(host);
		rtrn->db_user = ref_strdup(user);
		rtrn->db_pass = ref_strdup(pass);
		rtrn->db_name = ref_strdup(db_name);
		rtrn->db_port = port;
		rtrn->db_setup = 0;
		rtrn->db_version = 0;
		rtrn->db_tz_utc = 0;
		rtrn->db_tz_name[0] = '\0';
	}

	return rtrn;
}

int
cmyth_database_setup(cmyth_database_t db)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	uint32_t dbversion;
	const char *query_str = "SELECT data FROM settings WHERE value = 'DBSchemaVer' AND hostname IS NULL LIMIT 1;";
	cmyth_mysql_query_t *query;

	// db setup done
	db->db_setup = 1;
	db->db_version = 0;
	// Get db version from settings
	query = cmyth_mysql_query_create(db, query_str);
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}
	row = mysql_fetch_row(res);
	dbversion = safe_atol(row[0]);
	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s, database version is: %"PRIu32, __FUNCTION__, dbversion);
	if (dbversion > 0) {
		db->db_version = dbversion;
		if (dbversion >= 1307) {
			db->db_tz_utc = 1;
			strcpy(db->db_tz_name, "UTC");
		} else {
			db->db_tz_utc = 0;
			strcpy(db->db_tz_name, "SYSTEM");
		}
		return 1;
	}

	return 0;
}

int
cmyth_database_set_host(cmyth_database_t db, char *host)
{
	cmyth_database_close(db);
	ref_release(db->db_host);
	db->db_host = ref_strdup(host);
	if (!db->db_host)
		return 0;
	else
		return 1;
}

int
cmyth_database_set_user(cmyth_database_t db, char *user)
{
	cmyth_database_close(db);
	ref_release(db->db_user);
	db->db_user = ref_strdup(user);
	if (!db->db_user)
		return 0;
	else
		return 1;
}

int
cmyth_database_set_pass(cmyth_database_t db, char *pass)
{
	cmyth_database_close(db);
	ref_release(db->db_user);
	db->db_pass = ref_strdup(pass);
	if (!db->db_pass)
		return 0;
	else
		return 1;
}

int
cmyth_database_set_name(cmyth_database_t db, char *name)
{
	cmyth_database_close(db);
	ref_release(db->db_name);
	db->db_name = ref_strdup(name);
	if (!db->db_name)
		return 0;
	else
		return 1;
}

int
cmyth_database_set_port(cmyth_database_t db, uint16_t port)
{
	cmyth_database_close(db);
	db->db_port = port;
	return 1;
}

static int
cmyth_database_check_version(cmyth_database_t db)
{
	int err = 0;
	/*
	 * Since version 0.26, mythbackend stores in the database an exhaustive list of datetime fields
	 * using UTC zone, not all. Others fields keep local system time zone. Also the field types don't
	 * have extension 'with time zone'. To manage this situation we check the DB schema version to
	 * know if we must manually convert these date, time or datetime fields during exchanges with the
	 * database (read or store). If true then the attribute db_tz_utc is set to 1 else 0.
	 * This check is done when the first connection is created and the attribute db_setup is NULL
	 */
	if (!db->db_setup && (err = cmyth_database_setup(db)) < 0)
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: database setup failed (%d)\n", __FUNCTION__, err);
	return err;
}

uint32_t
cmyth_database_get_version(cmyth_database_t db)
{
	if (cmyth_database_check_version(db) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			__FUNCTION__);
		return 0;
	}

	return db->db_version;
}

static int
cmyth_db_check_connection(cmyth_database_t db)
{
	if (db->mysql != NULL) {
		/* Fetch the mysql stats (uptime and stuff) to check the connection is
		 * still good
		 */
		if (mysql_stat(db->mysql) == NULL) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_stat() failed: %s\n", __FUNCTION__, mysql_error(db->mysql));
			cmyth_database_close(db);
		}
	}
	if (db->mysql == NULL) {
		db->mysql = mysql_init(NULL);
		if (db->mysql == NULL) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_init() failed, insufficient memory?\n", __FUNCTION__);
			return -1;
		}
		if (NULL == mysql_real_connect(db->mysql, db->db_host, db->db_user, db->db_pass, db->db_name, db->db_port, NULL, CLIENT_FOUND_ROWS)) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_connect() failed: %s\n", __FUNCTION__, mysql_error(db->mysql));
			cmyth_database_close(db);
			return -1;
		}
	}
	return 0;
}

MYSQL *
cmyth_db_get_connection(cmyth_database_t db)
{
	if (cmyth_db_check_connection(db) != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n", __FUNCTION__);
		return NULL;
	}

	/*
	 * mythbackend stores any multi-byte characters using utf8 encoding within latin1 database
	 * columns. The MySQL connection needs to be told to use a utf8 character set when reading the
	 * database columns or any multi-byte characters will be treated as 2 or 3 subsequent latin1
	 * characters with nonsense values.
	 *
	 * http://www.mythtv.org/wiki/Fixing_Corrupt_Database_Encoding#Note_on_MythTV_0.21-fixes_and_below_character_encoding
	 * http://dev.mysql.com/doc/refman/5.0/en/charset-connection.html
	 */
	if (mysql_query(db->mysql, "SET NAMES utf8;")) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query() failed: %s\n", __FUNCTION__, mysql_error(db->mysql));
		return NULL;
	}

	/*
	 * Setting the TIME_ZONE is not really required since all datetime fields don't have extension
	 * 'with time zone'. But we do nevertheless.
	 */
	if (mysql_query(db->mysql, "SET TIME_ZONE='SYSTEM';")) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: SET TIME_ZONE failed: %s\n", __FUNCTION__, mysql_error(db->mysql));
		return NULL;
	}

	return db->mysql;
}

char *
cmyth_mysql_escape_chars(cmyth_database_t db, char *string)
{
	char *N_string;
	size_t len;

	if (cmyth_db_check_connection(db) != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n", __FUNCTION__);
		return NULL;
	}

	len = strlen(string);
	N_string = ref_alloc(len * 2 + 1);
	mysql_real_escape_string(db->mysql, N_string, string, len);

	return (N_string);
}

int
cmyth_mysql_set_watched_status(cmyth_database_t db, cmyth_proginfo_t prog, int watchedStat)
{
	int ret;
	MYSQL* sql = cmyth_db_get_connection(db);
	const char *query_str = "UPDATE recorded SET watched = ? WHERE chanid = ? AND starttime = ?";
	cmyth_mysql_query_t *query;
	time_t starttime;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	if (watchedStat > 1) watchedStat = 1;
	if (watchedStat < 0) watchedStat = 0;
	starttime = cmyth_timestamp_to_unixtime(prog->proginfo_rec_start_ts);
	query = cmyth_mysql_query_create(db, query_str);

	if (cmyth_mysql_query_param_int(query, watchedStat) < 0
			|| cmyth_mysql_query_param_uint32(query, prog->proginfo_chanId) < 0
			|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}

	ret = cmyth_mysql_query(query);

	ref_release(query);

	if (ret < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return ret;
	}

	return (int)mysql_affected_rows(sql);
}

int
cmyth_mysql_get_prev_recorded(cmyth_database_t db, cmyth_epginfolist_t *epglist)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	int n = 0;
	int rows = 0;
	const char *query_str = "SELECT oldrecorded.chanid, UNIX_TIMESTAMP(CONVERT_TZ(oldrecorded.starttime, ?, 'SYSTEM')), "
			"UNIX_TIMESTAMP(CONVERT_TZ(oldrecorded.endtime, ?, 'SYSTEM')), oldrecorded.title, oldrecorded.description, "
			"oldrecorded.subtitle, oldrecorded.programid, oldrecorded.seriesid, oldrecorded.category, "
			"channel.channum, channel.callsign, channel.name "
			"FROM oldrecorded LEFT JOIN channel ON oldrecorded.chanid=channel.chanid ORDER BY oldrecorded.starttime ASC";
	cmyth_mysql_query_t *query;
	cmyth_epginfo_t epg;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	query = cmyth_mysql_query_create(db, query_str);

	if (cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
			|| cmyth_mysql_query_param_str(query, db->db_tz_name) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	*epglist = cmyth_epginfolist_create();
	(*epglist)->epginfolist_count = (int)mysql_num_rows(res);
	(*epglist)->epginfolist_list = malloc((*epglist)->epginfolist_count * sizeof(cmyth_epginfo_t));
	if (!(*epglist)->epginfolist_list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: malloc() failed for list\n",
			  __FUNCTION__);
		ref_release(*epglist);
		mysql_free_result(res);
		return -ENOMEM;
	}
	memset((*epglist)->epginfolist_list, 0, (*epglist)->epginfolist_count * sizeof(cmyth_epginfo_t));

	while ((row = mysql_fetch_row(res))) {
		epg = cmyth_epginfo_create();
		epg->chanid = safe_atol(row[0]);
		epg->starttime = safe_atol(row[1]);
		epg->endtime = safe_atol(row[2]);
		epg->title = ref_strdup(row[3]);
		epg->description = ref_strdup(row[4]);
		epg->subtitle = ref_strdup(row[5]);
		epg->programid = ref_strdup(row[6]);
		epg->seriesid = ref_strdup(row[7]);
		epg->category = ref_strdup(row[8]);
		epg->category_type = ref_strdup("");
		epg->channum = safe_atol(row[9]);
		epg->callsign = ref_strdup(row[10]);
		epg->channame = ref_strdup(row[11]);
		epg->sourceid = 0;
		(*epglist)->epginfolist_list[rows] = epg;
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: [%d] chanid = %"PRIu32" title = %s\n", __FUNCTION__, rows, epg->chanid, epg->title);
		rows++;
	}
	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}

int
cmyth_mysql_get_guide(cmyth_database_t db, cmyth_epginfolist_t *epglist, uint32_t chanid, time_t starttime, time_t endtime)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT program.chanid, UNIX_TIMESTAMP(CONVERT_TZ(program.starttime,?,'SYSTEM')), "
			"UNIX_TIMESTAMP(CONVERT_TZ(program.endtime,?,'SYSTEM')), program.title, program.description, "
			"program.subtitle, program.programid, program.seriesid, program.category, program.category_type, "
			"channel.channum, channel.callsign, channel.name, channel.sourceid "
			"FROM program LEFT JOIN channel on program.chanid=channel.chanid "
			"WHERE channel.chanid = ? AND ((program.endtime > ? AND program.endtime < ?) OR "
			"(program.starttime >= ? AND program.starttime <= ?) OR "
			"(program.starttime <= ? AND program.endtime >= ?)) "
			"ORDER BY (channel.channum + 0), program.starttime ASC";
	int rows = 0;
	int n = 0;
	cmyth_mysql_query_t *query;
	cmyth_epginfo_t epg;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	query = cmyth_mysql_query_create(db, query_str);

	if (cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
			|| cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
			|| cmyth_mysql_query_param_uint32(query, chanid) < 0
			|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
			|| cmyth_mysql_query_param_unixtime(query, endtime, db->db_tz_utc) < 0
			|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
			|| cmyth_mysql_query_param_unixtime(query, endtime, db->db_tz_utc) < 0
			|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
			|| cmyth_mysql_query_param_unixtime(query, endtime, db->db_tz_utc) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	*epglist = cmyth_epginfolist_create();
	(*epglist)->epginfolist_count = (int)mysql_num_rows(res);
	(*epglist)->epginfolist_list = malloc((*epglist)->epginfolist_count * sizeof(cmyth_epginfo_t));
	if (!(*epglist)->epginfolist_list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: malloc() failed for list\n",
			  __FUNCTION__);
		ref_release(*epglist);
		mysql_free_result(res);
		return -ENOMEM;
	}
	memset((*epglist)->epginfolist_list, 0, (*epglist)->epginfolist_count * sizeof(cmyth_epginfo_t));

	while ((row = mysql_fetch_row(res))) {
		epg = cmyth_epginfo_create();
		epg->chanid = safe_atol(row[0]);
		epg->starttime = safe_atol(row[1]);
		epg->endtime = safe_atol(row[2]);
		epg->title = ref_strdup(row[3]);
		epg->description = ref_strdup(row[4]);
		epg->subtitle = ref_strdup(row[5]);
		epg->programid = ref_strdup(row[6]);
		epg->seriesid = ref_strdup(row[7]);
		epg->category = ref_strdup(row[8]);
		epg->category_type = ref_strdup(row[9]);
		epg->channum = safe_atol(row[10]);
		epg->callsign = ref_strdup(row[11]);
		epg->channame = ref_strdup(row[12]);
		epg->sourceid = safe_atol(row[13]);
		(*epglist)->epginfolist_list[rows] = epg;
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: [%d] chanid = %"PRIu32" title = %s\n", __FUNCTION__, rows, epg->chanid, epg->title);
		rows++;
	}
	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}

int
cmyth_mysql_get_recgroups(cmyth_database_t db, cmyth_recgroups_t **sqlrecgroups)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT DISTINCT recgroup FROM record";
	int rows = 0;
	int n = 0;
	cmyth_mysql_query_t *query;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	query = cmyth_mysql_query_create(db, query_str);

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	while ((row = mysql_fetch_row(res))) {
		if (rows == n) {
			n++;
			*sqlrecgroups = realloc(*sqlrecgroups, sizeof(**sqlrecgroups) * (n));
		}
		sizeof_strncpy((*sqlrecgroups)[rows].recgroups, row[0]);
		cmyth_dbg(CMYTH_DBG_DEBUG, "(*sqlrecgroups)[%d].recgroups = %s\n", rows, (*sqlrecgroups)[rows].recgroups);
		rows++;
	}
	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}

int
cmyth_mysql_get_prog_finder_char_title(cmyth_database_t db, cmyth_epginfolist_t *epglist, time_t starttime, char *program_name)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char *N_title;
	char *query_str;
	int rows = 0;
	int n = 0;
	cmyth_mysql_query_t *query;
	cmyth_epginfo_t epg;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	if (strncmp(program_name, "@", 1) == 0) {
		query_str = "SELECT program.chanid, UNIX_TIMESTAMP(CONVERT_TZ(program.starttime,?,'SYSTEM')), "
			"UNIX_TIMESTAMP(CONVERT_TZ(program.endtime,?,'SYSTEM')), program.title, program.description, "
			"program.subtitle, program.programid, program.seriesid, program.category, program.category_type, "
			"channel.channum, channel.callsign, channel.name, channel.sourceid "
			"FROM program LEFT JOIN channel on program.chanid=channel.chanid "
			"WHERE ( program.title NOT REGEXP '^[A-Z0-9]' AND program.title NOT REGEXP '^The [A-Z0-9]' "
			"AND program.title NOT REGEXP '^A [A-Z0-9]' AND program.starttime >= ?) ORDER BY program.title ASC";
		query = cmyth_mysql_query_create(db, query_str);
		if (cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
			ref_release(query);
			return -1;
		}
	} else {
		query_str = "SELECT program.chanid, UNIX_TIMESTAMP(CONVERT_TZ(program.starttime,?,'SYSTEM')), "
			"UNIX_TIMESTAMP(CONVERT_TZ(program.endtime,?,'SYSTEM')), program.title, program.description, "
			"program.subtitle, program.programid, program.seriesid, program.category, program.category_type, "
			"channel.channum, channel.callsign, channel.name, channel.sourceid "
			"FROM program LEFT JOIN channel on program.chanid=channel.chanid "
			"WHERE program.starttime >= ? and program.title like ? ORDER BY program.title ASC";
		query = cmyth_mysql_query_create(db, query_str);
		N_title = ref_alloc(strlen(program_name) * 2 + 3);
		sprintf(N_title, "%%%s%%", program_name);
		if (cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
				|| cmyth_mysql_query_param_str(query, N_title) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
			ref_release(query);
			ref_release(N_title);
			return -1;
		}
		ref_release(N_title);
	}

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	*epglist = cmyth_epginfolist_create();
	(*epglist)->epginfolist_count = (int)mysql_num_rows(res);
	(*epglist)->epginfolist_list = malloc((*epglist)->epginfolist_count * sizeof(cmyth_epginfo_t));
	if (!(*epglist)->epginfolist_list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: malloc() failed for list\n",
			  __FUNCTION__);
		ref_release(*epglist);
		mysql_free_result(res);
		return -ENOMEM;
	}
	memset((*epglist)->epginfolist_list, 0, (*epglist)->epginfolist_count * sizeof(cmyth_epginfo_t));

	while ((row = mysql_fetch_row(res))) {
		epg = cmyth_epginfo_create();
		epg->chanid = safe_atol(row[0]);
		epg->starttime = safe_atol(row[1]);
		epg->endtime = safe_atol(row[2]);
		epg->title = ref_strdup(row[3]);
		epg->description = ref_strdup(row[4]);
		epg->subtitle = ref_strdup(row[5]);
		epg->programid = ref_strdup(row[6]);
		epg->seriesid = ref_strdup(row[7]);
		epg->category = ref_strdup(row[8]);
		epg->category_type = ref_strdup(row[9]);
		epg->channum = safe_atol(row[10]);
		epg->callsign = ref_strdup(row[11]);
		epg->channame = ref_strdup(row[12]);
		epg->sourceid = safe_atol(row[13]);
		(*epglist)->epginfolist_list[rows] = epg;
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: [%d] chanid = %"PRIu32" title = %s\n", __FUNCTION__, rows, epg->chanid, epg->title);
		rows++;
	}
	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}

int
cmyth_mysql_get_prog_finder_time(cmyth_database_t db, cmyth_epginfolist_t *epglist, time_t starttime, char *program_name)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT program.chanid, UNIX_TIMESTAMP(CONVERT_TZ(program.starttime,?,'SYSTEM')), "
			"UNIX_TIMESTAMP(CONVERT_TZ(program.endtime,?,'SYSTEM')), program.title, program.description, "
			"program.subtitle, program.programid, program.seriesid, program.category, program.category_type, "
			"channel.channum, channel.callsign, channel.name, channel.sourceid "
			"FROM program LEFT JOIN channel on program.chanid=channel.chanid "
			"WHERE program.starttime >= ? and program.title = ? ORDER BY program.starttime ASC";
	int rows = 0;
	cmyth_mysql_query_t *query;
	cmyth_epginfo_t epg;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	query = cmyth_mysql_query_create(db, query_str);

	if (cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
			|| cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
			|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
			|| cmyth_mysql_query_param_str(query, program_name) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	*epglist = cmyth_epginfolist_create();
	(*epglist)->epginfolist_count = (int)mysql_num_rows(res);
	(*epglist)->epginfolist_list = malloc((*epglist)->epginfolist_count * sizeof(cmyth_epginfo_t));
	if (!(*epglist)->epginfolist_list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: malloc() failed for list\n",
			  __FUNCTION__);
		ref_release(*epglist);
		mysql_free_result(res);
		return -ENOMEM;
	}
	memset((*epglist)->epginfolist_list, 0, (*epglist)->epginfolist_count * sizeof(cmyth_epginfo_t));

	while ((row = mysql_fetch_row(res))) {
		epg = cmyth_epginfo_create();
		epg->chanid = safe_atol(row[0]);
		epg->starttime = safe_atol(row[1]);
		epg->endtime = safe_atol(row[2]);
		epg->title = ref_strdup(row[3]);
		epg->description = ref_strdup(row[4]);
		epg->subtitle = ref_strdup(row[5]);
		epg->programid = ref_strdup(row[6]);
		epg->seriesid = ref_strdup(row[7]);
		epg->category = ref_strdup(row[8]);
		epg->category_type = ref_strdup(row[9]);
		epg->channum = safe_atol(row[10]);
		epg->callsign = ref_strdup(row[11]);
		epg->channame = ref_strdup(row[12]);
		epg->sourceid = safe_atol(row[13]);
		(*epglist)->epginfolist_list[rows] = epg;
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: [%d] chanid = %"PRIu32" title = %s\n", __FUNCTION__, rows, epg->chanid, epg->title);
		rows++;
	}
	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}

int
cmyth_mysql_update_bookmark_setting(cmyth_database_t db, cmyth_proginfo_t prog)
{
	int ret;
	MYSQL* sql = cmyth_db_get_connection(db);
	const char *query_str = "UPDATE recorded SET bookmark = 1 WHERE chanid = ? AND starttime = ?";
	cmyth_mysql_query_t * query;
	time_t starttime;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	starttime = cmyth_timestamp_to_unixtime(prog->proginfo_rec_start_ts);
	query = cmyth_mysql_query_create(db, query_str);
	if (cmyth_mysql_query_param_uint32(query, prog->proginfo_chanId) < 0
			|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}

	ret = cmyth_mysql_query(query);

	ref_release(query);

	if (ret < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return ret;
	}

	return (int)mysql_affected_rows(sql);
}

int64_t
cmyth_mysql_get_bookmark_mark(cmyth_database_t db, cmyth_proginfo_t prog, int64_t bk, int mode)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT mark, type FROM recordedseek WHERE chanid = ? AND offset < ? AND (type = 6 or type = 9 ) AND starttime = ? ORDER by MARK DESC LIMIT 0, 1;";
	int rows = 0;
	int64_t mark = 0;
	int rectype = 0;
	time_t start_ts_dt;
	cmyth_mysql_query_t * query;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	start_ts_dt = cmyth_timestamp_to_unixtime(prog->proginfo_rec_start_ts);
	query = cmyth_mysql_query_create(db, query_str);

	if (cmyth_mysql_query_param_uint32(query, prog->proginfo_chanId) < 0
			|| cmyth_mysql_query_param_int64(query, bk) < 0
			|| cmyth_mysql_query_param_unixtime(query, start_ts_dt, db->db_tz_utc) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}
	while ((row = mysql_fetch_row(res))) {
		mark = safe_atoll(row[0]);
		rectype = safe_atoi(row[1]);
		rows++;
	}
	mysql_free_result(res);

	if (rectype == 6) {
		if (mode == 0) {
			mark = (mark - 1) * 15;
		} else if (mode == 1) {
			mark = (mark - 1) * 12;
		}
	}

	return mark;
}

int64_t
cmyth_mysql_get_bookmark_offset(cmyth_database_t db, uint32_t chanid, int64_t mark, time_t starttime, int mode)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	int64_t offset = 0;
	int rows = 0;
	int rectype = 0;
	const char *query_str = "SELECT chanid, UNIX_TIMESTAMP(CONVERT_TZ(starttime,?,'SYSTEM')), mark, offset, type FROM recordedseek WHERE chanid = ? AND mark<= ? AND starttime = ? ORDER BY MARK DESC LIMIT 1;";
	cmyth_mysql_query_t * query;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	query = cmyth_mysql_query_create(db, query_str);
	if (cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
			|| cmyth_mysql_query_param_uint32(query, chanid) < 0
			|| cmyth_mysql_query_param_int64(query, mark) < 0
			|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}
	while ((row = mysql_fetch_row(res))) {
		offset = safe_atoll(row[3]);
		rectype = safe_atoi(row[4]);
		rows++;
	}
	if (rectype != 9) {
		if (mode == 0) {
			mark = (mark / 15) + 1;
		} else if (mode == 1) {
			mark = (mark / 12) + 1;
		}
		query = cmyth_mysql_query_create(db, query_str);
		if (cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
				|| cmyth_mysql_query_param_uint32(query, chanid) < 0
				|| cmyth_mysql_query_param_int64(query, mark) < 0
				|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
			ref_release(query);
			return -1;
		}
		res = cmyth_mysql_query_result(query);
		ref_release(query);
		if (res == NULL) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
			return -1;
		}
		while ((row = mysql_fetch_row(res))) {
			offset = safe_atoll(row[3]);
			rows++;
		}
	}
	mysql_free_result(res);
	return offset;
}

int
cmyth_mysql_tuner_type_check(cmyth_database_t db, cmyth_recorder_t rec, int check_tuner_type)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char * query_str = "SELECT cardtype from capturecard WHERE cardid=?";
	cmyth_mysql_query_t * query;

	if (check_tuner_type == 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "MythTV Tuner check not enabled in Mythtv Options\n");
		return 1;
	}

	if (cmyth_database_check_version(db) < 0)
		return -1;

	query = cmyth_mysql_query_create(db, query_str);
	if (cmyth_mysql_query_param_uint32(query, rec->rec_id) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query failed\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);

	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution\n", __FUNCTION__);
		return -1;
	}
	row = mysql_fetch_row(res);
	mysql_free_result(res);
	if (strcmp(row[0], "MPEG") == 0) {
		return (1); //return the first available MPEG tuner
	} else if (strcmp(row[0], "HDHOMERUN") == 0) {
		return (1); //return the first available MPEG2TS tuner
	} else if (strcmp(row[0], "DVB") == 0) {
		return (1); //return the first available DVB tuner
	} else {
		return (0);
	}
}

int
cmyth_mysql_testdb_connection(cmyth_database_t db, char **message)
{
	char *buf;

	if (db->mysql != NULL) {
		if (mysql_stat(db->mysql) == NULL) {
			cmyth_database_close(db);
			return -1;
		}
	}

	buf = ref_alloc(sizeof(char) * 1001);

	if (db->mysql == NULL) {
		db->mysql = mysql_init(NULL);
		if (db->mysql == NULL) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_init() failed, insufficient memory?", __FUNCTION__);
			snprintf(buf, 1000, "mysql_init() failed, insufficient memory?");
			*message = buf;
			return -1;
		}
		if (NULL == mysql_real_connect(db->mysql, db->db_host, db->db_user, db->db_pass, db->db_name, db->db_port, NULL, CLIENT_FOUND_ROWS)) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_connect() failed: %s\n", __FUNCTION__, mysql_error(db->mysql));
			snprintf(buf, 1000, "%s", mysql_error(db->mysql));
			*message = buf;
			cmyth_database_close(db);
			return -1;
		}
	}
	snprintf(buf, 1000, "All Test Successful\n");
	*message = buf;
	return 1;
}

int
cmyth_mysql_get_chanlist(cmyth_database_t db, cmyth_chanlist_t *chanlist)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	/*
	 * The is_audio_service (radio) flag is only available from the channel scan.
	 * The subquery therefore get the flag from the most recent channel scan.
	 */
	const char *query_str = "SELECT c.chanid, c.channum, c.name, c.icon, c.visible, c.sourceid, c.mplexid, c.callsign, "
		"IFNULL(cs.is_audio_service, 0) AS is_audio_service "
		"FROM channel c "
		"LEFT JOIN (SELECT service_id, MAX(scanid) AS scanid FROM channelscan_channel GROUP BY service_id) s "
		"ON s.service_id = c.serviceid "
		"LEFT JOIN channelscan_channel cs "
		"ON cs.service_id = s.service_id AND cs.scanid = s.scanid;";
	int rows = 0;
	cmyth_mysql_query_t * query;
	cmyth_channel_t channel;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	query = cmyth_mysql_query_create(db, query_str);

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	*chanlist = cmyth_chanlist_create();

	(*chanlist)->chanlist_count = (int)mysql_num_rows(res);
	(*chanlist)->chanlist_list = malloc((*chanlist)->chanlist_count * sizeof(cmyth_chanlist_t));
	if (!(*chanlist)->chanlist_list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: malloc() failed for list\n", __FUNCTION__);
		ref_release(*chanlist);
		return -1;
	}
	memset((*chanlist)->chanlist_list, 0, (*chanlist)->chanlist_count * sizeof(cmyth_chanlist_t));

	while ((row = mysql_fetch_row(res))) {
		channel = cmyth_channel_create();
		channel->chanid = safe_atol(row[0]);
		channel->channum = safe_atol(row[1]);
		channel->chanstr = ref_strdup(row[1]);
		channel->name = ref_strdup(row[2]);
		channel->icon = ref_strdup(row[3]);
		channel->visible = safe_atoi(row[4]);
		channel->sourceid = safe_atol(row[5]);
		channel->multiplex = safe_atol(row[6]);
		channel->callsign = ref_strdup(row[7]);
		channel->radio = safe_atol(row[8]);
		(*chanlist)->chanlist_list[rows] = channel;
		rows++;
	}

	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}

int
cmyth_mysql_get_recordingrules(cmyth_database_t db, cmyth_recordingrulelist_t *rrl)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char *query_str;
	int rows = 0;
	cmyth_recordingrule_t rr;
	cmyth_mysql_query_t * query;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	if (db->db_version >= 1278) {
		query_str = "SELECT recordid, chanid, UNIX_TIMESTAMP(CONVERT_TZ(ADDTIME(startdate,starttime),?,'SYSTEM')), "
			"UNIX_TIMESTAMP(CONVERT_TZ(ADDTIME(enddate,endtime),?,'SYSTEM')),title,description, type, category, "
			"subtitle, recpriority, startoffset, endoffset, search, inactive, station, dupmethod, dupin, recgroup, "
			"storagegroup, playgroup, autotranscode, (autouserjob1 | (autouserjob2 << 1) | (autouserjob3 << 2) | "
			"(autouserjob4 << 3)), autocommflag, autoexpire, maxepisodes, maxnewest, transcoder, profile, prefinput, "
			"autometadata, inetref, season, episode , filter "
			"FROM record ORDER BY recordid";
	}
	else {
		query_str = "SELECT recordid, chanid, UNIX_TIMESTAMP(CONVERT_TZ(ADDTIME(startdate,starttime),?,'SYSTEM')), "
			"UNIX_TIMESTAMP(CONVERT_TZ(ADDTIME(enddate,endtime),?,'SYSTEM')),title,description, type, category, "
			"subtitle, recpriority, startoffset, endoffset, search, inactive, station, dupmethod, dupin, recgroup, "
			"storagegroup, playgroup, autotranscode, (autouserjob1 | (autouserjob2 << 1) | (autouserjob3 << 2) | "
			"(autouserjob4 << 3)), autocommflag, autoexpire, maxepisodes, maxnewest, transcoder, profile, prefinput "
			"FROM record ORDER BY recordid";
	}

	query = cmyth_mysql_query_create(db, query_str);
	if (cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
			|| cmyth_mysql_query_param_str(query, db->db_tz_name) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	*rrl = cmyth_recordingrulelist_create();

	(*rrl)->recordingrulelist_count = (int)mysql_num_rows(res);
	(*rrl)->recordingrulelist_list = malloc((*rrl)->recordingrulelist_count * sizeof(cmyth_recordingrulelist_t));
	if (!(*rrl)->recordingrulelist_list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: malloc() failed for recschedule list\n", __FUNCTION__);
		ref_release(*rrl);
		return -1;
	}
	memset((*rrl)->recordingrulelist_list, 0, (*rrl)->recordingrulelist_count * sizeof(cmyth_recordingrulelist_t));

	while ((row = mysql_fetch_row(res))) {
		rr = cmyth_recordingrule_create();
		rr->recordid = safe_atol(row[0]);
		rr->chanid = safe_atol(row[1]);
		rr->starttime = cmyth_timestamp_from_unixtime((time_t)safe_atol(row[2]));
		rr->endtime = cmyth_timestamp_from_unixtime((time_t)safe_atol(row[3]));
		rr->title = ref_strdup(row[4]);
		rr->description = ref_strdup(row[5]);
		rr->type = safe_atoi(row[6]);
		rr->category = ref_strdup(row[7]);
		rr->subtitle = ref_strdup(row[8]);
		rr->recpriority = safe_atoi(row[9]);
		rr->startoffset = safe_atoi(row[10]);
		rr->endoffset = safe_atoi(row[11]);
		rr->searchtype = safe_atoi(row[12]);
		rr->inactive = safe_atoi(row[13]);
		rr->callsign = ref_strdup(row[14]);
		rr->dupmethod = safe_atoi(row[15]);
		rr->dupin = safe_atoi(row[16]);
		rr->recgroup = ref_strdup(row[17]);
		rr->storagegroup = ref_strdup(row[18]);
		rr->playgroup = ref_strdup(row[19]);
		rr->autotranscode = safe_atoi(row[20]);
		rr->userjobs = safe_atoi(row[21]);
		rr->autocommflag = safe_atoi(row[22]);
		rr->autoexpire = safe_atoi(row[23]);
		rr->maxepisodes = safe_atol(row[24]);
		rr->maxnewest = safe_atoi(row[25]);
		rr->transcoder = safe_atol(row[26]);
		rr->profile = ref_strdup(row[27]);
		rr->prefinput = safe_atol(row[28]);
		if (db->db_version >= 1278) {
			rr->autometadata = safe_atoi(row[29]);
			rr->inetref = ref_strdup(row[30]);
			rr->season = safe_atoi(row[31]);
			rr->episode = safe_atoi(row[32]);
			rr->filter = safe_atol(row[33]);
		}
		else {
			rr->autometadata = 0;
			rr->inetref = ref_strdup("");
			rr->season = 0;
			rr->episode = 0;
			rr->filter = 0;
		}
		(*rrl)->recordingrulelist_list[rows] = rr;
		rows++;
	}

	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}

int
cmyth_mysql_add_recordingrule(cmyth_database_t db, cmyth_recordingrule_t rr)
{
	int ret;
	MYSQL* sql = cmyth_db_get_connection(db);
	char *query_str;
	cmyth_mysql_query_t * query;
	time_t starttime;
	time_t endtime;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	if (db->db_version >= 1278) {
		query_str = "INSERT INTO record (record.type, chanid, starttime, startdate, endtime, enddate, title, "
			"description, category, findid, findtime, station, subtitle, recpriority, startoffset, endoffset, "
			"search, inactive, dupmethod, dupin, recgroup, storagegroup, playgroup, autotranscode, autouserjob1, "
			"autouserjob2, autouserjob3, autouserjob4, autocommflag, autoexpire, maxepisodes, maxnewest, transcoder, "
			"profile, prefinput, autometadata, inetref, season, episode, filter) "
			"VALUES (?, ?, TIME(?), DATE(?), TIME(?), DATE(?), ?, ?, ?, TO_DAYS(DATE(?)), TIME(?), "
			"?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? ,? ,?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
	}
	else {
		query_str = "INSERT INTO record (record.type, chanid, starttime, startdate, endtime, enddate, title, "
			"description, category, findid, findtime, station, subtitle, recpriority, startoffset, endoffset, "
			"search, inactive, dupmethod, dupin, recgroup, storagegroup, playgroup, autotranscode, autouserjob1, "
			"autouserjob2, autouserjob3, autouserjob4, autocommflag, autoexpire, maxepisodes, maxnewest, transcoder, "
			"profile, prefinput) "
			"VALUES (?, ?, TIME(?), DATE(?), TIME(?), DATE(?), ?, ?, ?, TO_DAYS(DATE(?)), TIME(?), "
			"?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? ,? ,?, ?, ?, ?, ?);";
	}

	starttime = cmyth_timestamp_to_unixtime(rr->starttime);
	endtime = cmyth_timestamp_to_unixtime(rr->endtime);

	query = cmyth_mysql_query_create(db, query_str);
	if ((cmyth_mysql_query_param_uint(query, rr->type) < 0
			|| cmyth_mysql_query_param_uint32(query, rr->chanid) < 0
			|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
			|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
			|| cmyth_mysql_query_param_unixtime(query, endtime, db->db_tz_utc) < 0
			|| cmyth_mysql_query_param_unixtime(query, endtime, db->db_tz_utc) < 0
			|| cmyth_mysql_query_param_str(query, rr->title) < 0
			|| cmyth_mysql_query_param_str(query, rr->description) < 0
			|| cmyth_mysql_query_param_str(query, rr->category) < 0
			|| cmyth_mysql_query_param_unixtime(query, starttime, 0) < 0
			|| cmyth_mysql_query_param_unixtime(query, starttime, 0) < 0
			|| cmyth_mysql_query_param_str(query, rr->callsign) < 0
			|| cmyth_mysql_query_param_str(query, rr->subtitle) < 0
			|| cmyth_mysql_query_param_int(query, rr->recpriority) < 0
			|| cmyth_mysql_query_param_uint(query, rr->startoffset) < 0
			|| cmyth_mysql_query_param_uint(query, rr->endoffset) < 0
			|| cmyth_mysql_query_param_uint(query, rr->searchtype) < 0
			|| cmyth_mysql_query_param_uint(query, rr->inactive) < 0
			|| cmyth_mysql_query_param_uint(query, rr->dupmethod) < 0
			|| cmyth_mysql_query_param_uint(query, rr->dupin) < 0
			|| cmyth_mysql_query_param_str(query, rr->recgroup) < 0
			|| cmyth_mysql_query_param_str(query, rr->storagegroup) < 0
			|| cmyth_mysql_query_param_str(query, rr->playgroup) < 0
			|| cmyth_mysql_query_param_uint(query, rr->autotranscode) < 0
			|| cmyth_mysql_query_param_uint(query, rr->userjobs & 1) < 0
			|| cmyth_mysql_query_param_uint(query, rr->userjobs & 2) < 0
			|| cmyth_mysql_query_param_uint(query, rr->userjobs & 4) < 0
			|| cmyth_mysql_query_param_uint(query, rr->userjobs & 8) < 0
			|| cmyth_mysql_query_param_uint(query, rr->autocommflag) < 0
			|| cmyth_mysql_query_param_uint(query, rr->autoexpire) < 0
			|| cmyth_mysql_query_param_uint32(query, rr->maxepisodes) < 0
			|| cmyth_mysql_query_param_uint(query, rr->maxnewest) < 0
			|| cmyth_mysql_query_param_uint32(query, rr->transcoder) < 0
			|| cmyth_mysql_query_param_str(query, rr->profile) < 0
			|| cmyth_mysql_query_param_uint32(query, rr->prefinput) < 0)
			|| (db->db_version >= 1278
				&& (cmyth_mysql_query_param_uint(query, rr->autometadata) < 0
				|| cmyth_mysql_query_param_str(query, rr->inetref) < 0
				|| cmyth_mysql_query_param_uint(query, rr->season) < 0
				|| cmyth_mysql_query_param_uint(query, rr->episode) < 0
				|| cmyth_mysql_query_param_uint32(query, rr->filter) < 0))) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}

	ret = cmyth_mysql_query(query);

	ref_release(query);

	if (ret != 0)
		return -1;

	rr->recordid = (uint32_t)mysql_insert_id(sql);

	return (int)mysql_affected_rows(sql);
}

int
cmyth_mysql_delete_recordingrule(cmyth_database_t db, uint32_t recordid)
{
	int ret;
	MYSQL* sql = cmyth_db_get_connection(db);
	const char *query_str = "DELETE FROM record WHERE recordid = ?;";
	cmyth_mysql_query_t * query;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	query = cmyth_mysql_query_create(db, query_str);
	if (cmyth_mysql_query_param_uint32(query, recordid) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}

	ret = cmyth_mysql_query(query);
	ref_release(query);

	if (ret != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	return (int)mysql_affected_rows(sql);
}

int
cmyth_mysql_update_recordingrule(cmyth_database_t db, cmyth_recordingrule_t rr)
{
	int ret;
	MYSQL* sql = cmyth_db_get_connection(db);
	char *query_str;
	cmyth_mysql_query_t * query;
	time_t starttime;
	time_t endtime;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	if (db->db_version >= 1278) {
		query_str = "UPDATE record SET record.type = ?, chanid = ?, starttime = TIME(?), startdate = DATE(?), "
			"endtime = TIME(?), enddate = DATE(?) ,title = ?, description = ?, category = ?, subtitle = ?, "
			"recpriority = ?, startoffset = ?, endoffset = ?, search = ?, inactive = ?, station = ?, "
			"dupmethod = ?, dupin = ?, recgroup = ?, storagegroup = ?, playgroup = ?, autotranscode = ?, "
			"autouserjob1 = ?, autouserjob2 = ?, autouserjob3 = ?, autouserjob4 = ?, autocommflag = ?, "
			"autoexpire = ?, maxepisodes = ?, maxnewest = ?, transcoder = ?, profile = ?, prefinput = ?, "
			"autometadata = ?, inetref = ?, season = ?, episode = ?, filter = ? "
			"WHERE recordid = ? ;";
	}
	else {
		query_str = "UPDATE record SET record.type = ?, chanid = ?, starttime = TIME(?), startdate = DATE(?), "
			"endtime = TIME(?), enddate = DATE(?) ,title = ?, description = ?, category = ?, subtitle = ?, "
			"recpriority = ?, startoffset = ?, endoffset = ?, search = ?, inactive = ?, station = ?, "
			"dupmethod = ?, dupin = ?, recgroup = ?, storagegroup = ?, playgroup = ?, autotranscode = ?, "
			"autouserjob1 = ?, autouserjob2 = ?, autouserjob3 = ?, autouserjob4 = ?, autocommflag = ?, "
			"autoexpire = ?, maxepisodes = ?, maxnewest = ?, transcoder = ?, profile = ?, prefinput = ? "
			"WHERE recordid = ? ;";
	}
	starttime = cmyth_timestamp_to_unixtime(rr->starttime);
	endtime = cmyth_timestamp_to_unixtime(rr->endtime);

	query = cmyth_mysql_query_create(db, query_str);
	if ((cmyth_mysql_query_param_uint(query, rr->type) < 0
			|| cmyth_mysql_query_param_uint32(query, rr->chanid) < 0
			|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
			|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
			|| cmyth_mysql_query_param_unixtime(query, endtime, db->db_tz_utc) < 0
			|| cmyth_mysql_query_param_unixtime(query, endtime, db->db_tz_utc) < 0
			|| cmyth_mysql_query_param_str(query, rr->title) < 0
			|| cmyth_mysql_query_param_str(query, rr->description) < 0
			|| cmyth_mysql_query_param_str(query, rr->category) < 0
			|| cmyth_mysql_query_param_str(query, rr->subtitle) < 0
			|| cmyth_mysql_query_param_int(query, rr->recpriority) < 0
			|| cmyth_mysql_query_param_uint(query, rr->startoffset) < 0
			|| cmyth_mysql_query_param_uint(query, rr->endoffset) < 0
			|| cmyth_mysql_query_param_uint(query, rr->searchtype) < 0
			|| cmyth_mysql_query_param_uint(query, rr->inactive) < 0
			|| cmyth_mysql_query_param_str(query, rr->callsign) < 0
			|| cmyth_mysql_query_param_uint(query, rr->dupmethod) < 0
			|| cmyth_mysql_query_param_uint(query, rr->dupin) < 0
			|| cmyth_mysql_query_param_str(query, rr->recgroup) < 0
			|| cmyth_mysql_query_param_str(query, rr->storagegroup) < 0
			|| cmyth_mysql_query_param_str(query, rr->playgroup) < 0
			|| cmyth_mysql_query_param_uint(query, rr->autotranscode) < 0
			|| cmyth_mysql_query_param_uint(query, rr->userjobs & 1) < 0
			|| cmyth_mysql_query_param_uint(query, rr->userjobs & 2) < 0
			|| cmyth_mysql_query_param_uint(query, rr->userjobs & 4) < 0
			|| cmyth_mysql_query_param_uint(query, rr->userjobs & 8) < 0
			|| cmyth_mysql_query_param_uint(query, rr->autocommflag) < 0
			|| cmyth_mysql_query_param_uint(query, rr->autoexpire) < 0
			|| cmyth_mysql_query_param_uint32(query, rr->maxepisodes) < 0
			|| cmyth_mysql_query_param_uint(query, rr->maxnewest) < 0
			|| cmyth_mysql_query_param_uint32(query, rr->transcoder) < 0
			|| cmyth_mysql_query_param_str(query, rr->playgroup) < 0
			|| cmyth_mysql_query_param_uint32(query, rr->prefinput) < 0)
			|| (db->db_version >= 1278
				&& ( cmyth_mysql_query_param_uint(query, rr->autometadata) < 0
				|| cmyth_mysql_query_param_str(query, rr->inetref) < 0
				|| cmyth_mysql_query_param_uint(query, rr->season) < 0
				|| cmyth_mysql_query_param_uint(query, rr->episode) < 0
				|| cmyth_mysql_query_param_uint32(query, rr->filter) < 0))
			|| cmyth_mysql_query_param_uint32(query, rr->recordid) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}

	ret = cmyth_mysql_query(query);

	ref_release(query);

	if (ret < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return ret;
	}

	return (int)mysql_affected_rows(sql);
}

/*
 * cmyth_mysql_recordingrule_from_template()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Returns a recording rule initialized from template.
 *
 * Success: returns 0 for template unavailable else 1
 *
 * Failure: -1
 */
int
cmyth_mysql_recordingrule_from_template(cmyth_database_t db, const char *category, const char *category_type, cmyth_recordingrule_t *rr)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char *query_str;
	int ret = 0;
	cmyth_mysql_query_t * query;
	char *buf;
	uint32_t recordid;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	*rr = cmyth_recordingrule_init();
	if (!*rr)
		return -1;

	if (db->db_version >= 1302) {
		query_str = "SELECT recordid, recpriority, startoffset, endoffset, search, dupmethod, dupin, recgroup, storagegroup, "
			"playgroup, autotranscode, (autouserjob1 | (autouserjob2 << 1) | (autouserjob3 << 2) | (autouserjob4 << 3)), "
			"autocommflag, autoexpire, profile, prefinput, autometadata, maxepisodes, maxnewest, filter, "
			"(category = ?) AS catmatch, (category = ?) AS typematch "
			"FROM record WHERE type = ? AND (category = ? OR category = ? OR category = 'Default') "
			"ORDER BY catmatch DESC, typematch DESC";

		query = cmyth_mysql_query_create(db, query_str);
		if (cmyth_mysql_query_param_str(query, category) < 0
			|| cmyth_mysql_query_param_str(query, category_type) < 0
			|| cmyth_mysql_query_param_uint(query, RRULE_TEMPLATE_RECORD) < 0
			|| cmyth_mysql_query_param_str(query, category) < 0
			|| cmyth_mysql_query_param_str(query, category_type) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
			ref_release(query);
			return -1;
		}

		res = cmyth_mysql_query_result(query);
		ref_release(query);

		if (res == NULL) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
			return -1;
		}

		if ((row = mysql_fetch_row(res))) {
			recordid = safe_atol(row[0]);
			cmyth_dbg(CMYTH_DBG_DEBUG, "%s: use record template id: %"PRIu32"\n", __FUNCTION__, recordid);
			(*rr)->type = RRULE_TEMPLATE_RECORD;
			(*rr)->recpriority = safe_atoi(row[1]);
			(*rr)->startoffset = safe_atoi(row[2]);
			(*rr)->endoffset = safe_atoi(row[3]);
			(*rr)->searchtype = safe_atoi(row[4]);
			(*rr)->dupmethod = safe_atoi(row[5]);
			(*rr)->dupin = safe_atoi(row[6]);
			(*rr)->recgroup = ref_strdup(row[7]);
			(*rr)->storagegroup = ref_strdup(row[8]);
			(*rr)->playgroup = ref_strdup(row[9]);
			(*rr)->autotranscode = safe_atoi(row[10]);
			(*rr)->userjobs = safe_atoi(row[11]);
			(*rr)->autocommflag = safe_atoi(row[12]);
			(*rr)->autoexpire = safe_atoi(row[13]);
			(*rr)->profile = ref_strdup(row[14]);
			(*rr)->prefinput = safe_atol(row[15]);
			(*rr)->autometadata = safe_atoi(row[16]);
			(*rr)->maxepisodes = safe_atol(row[17]);
			(*rr)->maxnewest = safe_atoi(row[18]);
			(*rr)->filter = safe_atol(row[19]);
			ret = 1;
		}
		mysql_free_result(res);
	}
	else {
		if (cmyth_mysql_get_setting(db, "AutoRunUserJob1", &buf) > 0) {
			cmyth_dbg(CMYTH_DBG_DEBUG, "%s: use setting AutoRunUserJob1: %s\n", __FUNCTION__, buf);
			(*rr)->userjobs |= (atoi(buf) == 1 ? 1 : 0 );
		}
		if (cmyth_mysql_get_setting(db, "AutoRunUserJob2", &buf) > 0) {
			cmyth_dbg(CMYTH_DBG_DEBUG, "%s: use setting AutoRunUserJob2: %s\n", __FUNCTION__, buf);
			(*rr)->userjobs |= (atoi(buf) == 1 ? 1 : 0 ) << 1;
		}
		if (cmyth_mysql_get_setting(db, "AutoRunUserJob3", &buf) > 0) {
			cmyth_dbg(CMYTH_DBG_DEBUG, "%s: use setting AutoRunUserJob3: %s\n", __FUNCTION__, buf);
			(*rr)->userjobs |= (atoi(buf) == 1 ? 1 : 0 ) << 2;
		}
		if (cmyth_mysql_get_setting(db, "AutoRunUserJob4", &buf) > 0) {
			cmyth_dbg(CMYTH_DBG_DEBUG, "%s: use setting AutoRunUserJob4: %s\n", __FUNCTION__, buf);
			(*rr)->userjobs |= (atoi(buf) == 1 ? 1 : 0 ) << 3;
		}
		if (cmyth_mysql_get_setting(db, "AutoTranscode", &buf) > 0) {
			cmyth_dbg(CMYTH_DBG_DEBUG, "%s: use setting AutoTranscode: %s\n", __FUNCTION__, buf);
			(*rr)->autotranscode = atoi(buf);
		}
		if (cmyth_mysql_get_setting(db, "DefaultTranscoder", &buf) > 0) {
			cmyth_dbg(CMYTH_DBG_DEBUG, "%s: use setting DefaultTranscoder: %s\n", __FUNCTION__, buf);
			(*rr)->transcoder = atol(buf);
		}
		if (cmyth_mysql_get_setting(db, "AutoCommercialFlag", &buf) > 0) {
			cmyth_dbg(CMYTH_DBG_DEBUG, "%s: use setting AutoCommercialFlag: %s\n", __FUNCTION__, buf);
			(*rr)->autocommflag = atoi(buf);
		}
		if (cmyth_mysql_get_setting(db, "AutoExpireDefault", &buf) > 0) {
			cmyth_dbg(CMYTH_DBG_DEBUG, "%s: use setting AutoExpireDefault: %s\n", __FUNCTION__, buf);
			(*rr)->autoexpire = atoi(buf);
		}
		if (db->db_version >= 1278 && cmyth_mysql_get_setting(db, "AutoMetadataLookup", &buf) > 0) {
			cmyth_dbg(CMYTH_DBG_DEBUG, "%s: use setting AutoMetadataLookup: %s\n", __FUNCTION__, buf);
			(*rr)->autometadata = atoi(buf);
		}
		ret = 1;
	}
	return ret;
}

int
cmyth_mysql_get_channelgroups(cmyth_database_t db, cmyth_channelgroup_t **changroups)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT grpid, name FROM channelgroupnames";
	int rows = 0;
	cmyth_channelgroup_t *ret;
	cmyth_mysql_query_t * query;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	query = cmyth_mysql_query_create(db, query_str);

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	ret = ref_alloc(sizeof(cmyth_channelgroup_t) * (int)mysql_num_rows(res));

	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: alloc() failed for list\n", __FUNCTION__);
		mysql_free_result(res);
		return -1;
	}

	while ((row = mysql_fetch_row(res))) {
		ret[rows].grpid = safe_atol(row[0]);
		safe_strncpy(ret[rows].name, row[1], 65);
		rows++;
	}

	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);

	*changroups = ret;
	return rows;
}

int
cmyth_mysql_get_channelids_in_group(cmyth_database_t db, uint32_t grpid, uint32_t **chanids)
{

	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT chanid FROM channelgroup WHERE grpid = ?";
	int rows = 0;
	uint32_t *ret;
	cmyth_mysql_query_t * query;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	query = cmyth_mysql_query_create(db, query_str);
	if (cmyth_mysql_query_param_uint32(query, grpid) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	ret = ref_alloc(sizeof(uint32_t) * (int)mysql_num_rows(res));

	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: alloc() failed for list\n", __FUNCTION__);
		mysql_free_result(res);
		return -1;
	}

	while ((row = mysql_fetch_row(res))) {
		ret[rows] = safe_atol(row[0]);
		rows++;
	}

	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	*chanids = ret;
	return rows;
}

int
cmyth_mysql_get_recorder_source_list(cmyth_database_t db, cmyth_recorder_source_t **rsrc)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT cardid, sourceid FROM cardinput";
	int rows = 0;
	cmyth_recorder_source_t *ret;
	cmyth_mysql_query_t * query;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	query = cmyth_mysql_query_create(db, query_str);

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	ret = ref_alloc(sizeof(cmyth_recorder_source_t) * (int)mysql_num_rows(res));

	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: alloc() failed for list\n", __FUNCTION__);
		mysql_free_result(res);
		return -1;
	}

	while ((row = mysql_fetch_row(res))) {
		ret[rows].recid = safe_atol(row[0]);
		ret[rows].sourceid = safe_atol(row[1]);
		rows++;
	}

	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);

	*rsrc = ret;
	return rows;
}

int
cmyth_mysql_get_recorder_source_channum(cmyth_database_t db, char *channum, cmyth_recorder_source_t **rsrc)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str1 = "SELECT cardinput.cardid, cardinput.sourceid FROM channel INNER JOIN cardinput ON channel.sourceid = cardinput.sourceid WHERE channel.channum = ? ORDER BY cardinput.cardinputid";
	const char *query_str2 = "SELECT cardinput.cardid, cardinput.sourceid FROM channel INNER JOIN cardinput ON channel.sourceid = cardinput.sourceid WHERE channel.channum = ? AND cardinput.livetvorder > 0 ORDER BY cardinput.livetvorder, cardinput.cardinputid";
	int rows = 0;
	cmyth_recorder_source_t *ret;
	cmyth_mysql_query_t * query;

	if (cmyth_database_check_version(db) < 0)
		return -1;


	//DB version change at mythtv/libs/libmythtv/dbcheck.cpp:1789
	if (db->db_version < 1293)
		query = cmyth_mysql_query_create(db, query_str1);
	else
		query = cmyth_mysql_query_create(db, query_str2);

	if (cmyth_mysql_query_param_str(query, channum) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	ret = ref_alloc(sizeof(cmyth_recorder_source_t) * (int)mysql_num_rows(res));

	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: alloc() failed for list\n", __FUNCTION__);
		mysql_free_result(res);
		return -1;
	}

	while ((row = mysql_fetch_row(res))) {
		ret[rows].recid = safe_atol(row[0]);
		ret[rows].sourceid = safe_atol(row[1]);
		rows++;
	}

	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);

	*rsrc = ret;
	return rows;
}

int
cmyth_mysql_get_prog_finder_chan(cmyth_database_t db, cmyth_epginfo_t *epg, uint32_t chanid)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT program.chanid, UNIX_TIMESTAMP(CONVERT_TZ(program.starttime,?,'SYSTEM')), "
			"UNIX_TIMESTAMP(CONVERT_TZ(program.endtime,?,'SYSTEM')), program.title, program.description, "
			"program.subtitle, program.programid, program.seriesid, program.category, program.category_type, "
			"channel.channum, channel.callsign, channel.name, channel.sourceid "
			"FROM program LEFT JOIN channel on program.chanid=channel.chanid "
			"WHERE program.chanid = ? AND UNIX_TIMESTAMP(NOW())>=UNIX_TIMESTAMP(CONVERT_TZ(program.starttime,?,'SYSTEM')) "
			"AND UNIX_TIMESTAMP(NOW())<=UNIX_TIMESTAMP(CONVERT_TZ(program.endtime,?,'SYSTEM')) AND program.manualid = 0 "
			"ORDER BY (channel.channum + 0), program.starttime ASC";
	int rows = 0;
	cmyth_mysql_query_t * query;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	query = cmyth_mysql_query_create(db, query_str);

	if (cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
			|| cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
			|| cmyth_mysql_query_param_uint32(query, chanid) < 0
			|| cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
			|| cmyth_mysql_query_param_str(query, db->db_tz_name) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}

	res = cmyth_mysql_query_result(query);
	ref_release(query);

	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	if ((row = mysql_fetch_row(res))) {
		rows++;
		*epg = cmyth_epginfo_create();
		(*epg)->chanid = safe_atol(row[0]);
		(*epg)->starttime = safe_atol(row[1]);
		(*epg)->endtime = safe_atol(row[2]);
		(*epg)->title = ref_strdup(row[3]);
		(*epg)->description = ref_strdup(row[4]);
		(*epg)->subtitle = ref_strdup(row[5]);
		(*epg)->programid = ref_strdup(row[6]);
		(*epg)->seriesid = ref_strdup(row[7]);
		(*epg)->category = ref_strdup(row[8]);
		(*epg)->category_type = ref_strdup(row[9]);
		(*epg)->channum = safe_atol(row[10]);
		(*epg)->callsign = ref_strdup(row[11]);
		(*epg)->channame = ref_strdup(row[12]);
		(*epg)->sourceid = safe_atol(row[13]);
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: chanid = %"PRIu32" title = %s\n", __FUNCTION__, (*epg)->chanid, (*epg)->title);
	}
	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}

int
cmyth_mysql_get_prog_finder_time_title_chan(cmyth_database_t db, cmyth_epginfo_t *epg, time_t starttime, char *program_name, uint32_t chanid)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT program.chanid, UNIX_TIMESTAMP(CONVERT_TZ(program.starttime,?,'SYSTEM')), "
			"UNIX_TIMESTAMP(CONVERT_TZ(program.endtime,?,'SYSTEM')), program.title, program.description, "
			"program.subtitle, program.programid, program.seriesid, program.category, program.category_type, "
			"channel.channum, channel.callsign, channel.name, channel.sourceid "
			"FROM program LEFT JOIN channel on program.chanid=channel.chanid "
			"WHERE program.chanid = ? AND program.title LIKE ? AND program.starttime = ? AND program.manualid = 0 "
			"ORDER BY (channel.channum + 0), program.starttime ASC";
	int rows = 0;
	cmyth_mysql_query_t * query;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	query = cmyth_mysql_query_create(db, query_str);

	if (cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
			|| cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
			|| cmyth_mysql_query_param_uint32(query, chanid) < 0
			|| cmyth_mysql_query_param_str(query, program_name) < 0
			|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}

	res = cmyth_mysql_query_result(query);
	ref_release(query);

	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	if ((row = mysql_fetch_row(res))) {
		rows++;
		*epg = cmyth_epginfo_create();
		(*epg)->chanid = safe_atol(row[0]);
		(*epg)->starttime = safe_atol(row[1]);
		(*epg)->endtime = safe_atol(row[2]);
		(*epg)->title = ref_strdup(row[3]);
		(*epg)->description = ref_strdup(row[4]);
		(*epg)->subtitle = ref_strdup(row[5]);
		(*epg)->programid = ref_strdup(row[6]);
		(*epg)->seriesid = ref_strdup(row[7]);
		(*epg)->category = ref_strdup(row[8]);
		(*epg)->category_type = ref_strdup(row[9]);
		(*epg)->channum = safe_atol(row[10]);
		(*epg)->callsign = ref_strdup(row[11]);
		(*epg)->channame = ref_strdup(row[12]);
		(*epg)->sourceid = safe_atol(row[13]);
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: chanid = %"PRIu32" title = %s\n", __FUNCTION__, (*epg)->chanid, (*epg)->title);
	}
	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}

static void
destroy_char_array(void *p)
{
	char **ptr = (char**)p;
	if (!ptr)
		return;
	while (*ptr) {
		ref_release(*ptr);
		ptr++;
	}
}

int
cmyth_mysql_get_storagegroups(cmyth_database_t db, char** *sg)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT groupname FROM storagegroup";
	int rows = 0;
	char **ret;
	cmyth_mysql_query_t * query;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	query = cmyth_mysql_query_create(db, query_str);
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	ret = ref_alloc(sizeof(char*) * ((int) mysql_num_rows(res) + 1));
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: alloc() failed for list\n", __FUNCTION__);
		mysql_free_result(res);
		return -1;
	}

	ref_set_destroy(ret, destroy_char_array);

	while ((row = mysql_fetch_row(res))) {
		ret[rows] = ref_strdup(row[0]);
		rows++;
	}
	ret[rows] = NULL;

	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	*sg = ret;

	return rows;
}

int
cmyth_mysql_get_playgroups(cmyth_database_t db, char** *pg)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT name FROM playgroup";
	int rows = 0;
	char **ret;
	cmyth_mysql_query_t * query;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	query = cmyth_mysql_query_create(db, query_str);

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	ret = ref_alloc(sizeof(char*) * ((int) mysql_num_rows(res) + 1));
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: alloc() failed for list\n", __FUNCTION__);
		mysql_free_result(res);
		return -1;
	}

	ref_set_destroy(ret, destroy_char_array);

	while ((row = mysql_fetch_row(res))) {
		ret[rows] = ref_strdup(row[0]);
		rows++;
	}
	ret[rows] = NULL;

	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	*pg = ret;

	return rows;
}

int
cmyth_mysql_get_recprofiles(cmyth_database_t db, cmyth_recprofile_t **profiles)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT recordingprofiles.id, recordingprofiles.name, profilegroups.cardtype FROM recordingprofiles INNER JOIN profilegroups ON recordingprofiles.profilegroup = profilegroups.id";
	int rows = 0;
	cmyth_recprofile_t *ret = NULL;
	cmyth_mysql_query_t * query;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	query = cmyth_mysql_query_create(db, query_str);

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	ret = ref_alloc(sizeof(cmyth_recprofile_t) * (int)mysql_num_rows(res));
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: alloc() failed for list\n", __FUNCTION__);
		mysql_free_result(res);
		return -1;
	}

	while ((row = mysql_fetch_row(res))) {
		ret[rows].id = safe_atol(row[0]);
		safe_strncpy(ret[rows].name, row[1], 128);
		safe_strncpy(ret[rows].cardtype, row[2], 32);
		rows++;
	}

	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	*profiles = ret;

	return rows;
}

int
cmyth_mysql_get_cardtype(cmyth_database_t db, uint32_t chanid, char* *cardtype)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT cardtype FROM channel LEFT JOIN cardinput ON channel.sourceid=cardinput.sourceid LEFT JOIN capturecard ON cardinput.cardid=capturecard.cardid WHERE channel.chanid = ?";
	int rows = 0;
	cmyth_mysql_query_t * query;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	query = cmyth_mysql_query_create(db, query_str);

	if (cmyth_mysql_query_param_uint32(query, chanid) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}

	res = cmyth_mysql_query_result(query);
	ref_release(query);

	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	if ((row = mysql_fetch_row(res))) {
		*cardtype = ref_strdup(row[0]);
		rows++;
	}

	mysql_free_result(res);
	return rows;
}

/*
 * cmyth_mysql_get_recording_markup()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Get recordings markup data. Type specifies the markup type to query, such as
 * framerate, keyframe, scene change, a flagged commercial ...
 *
 * Success: returns markup data (bigint >= 0)
 *
 * Failure: -(errno)
 */
int64_t
cmyth_mysql_get_recording_markup(cmyth_database_t db, cmyth_proginfo_t prog, cmyth_recording_markup_t type)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT data FROM recordedmarkup WHERE chanid = ? AND starttime = ? AND type = ? LIMIT 0, 1;";
	int rows = 0;
	int64_t data = 0;
	time_t start_ts_dt;
	cmyth_mysql_query_t *query;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	start_ts_dt = cmyth_timestamp_to_unixtime(prog->proginfo_rec_start_ts);
	query = cmyth_mysql_query_create(db, query_str);

	if (cmyth_mysql_query_param_uint32(query, prog->proginfo_chanId) < 0
			|| cmyth_mysql_query_param_unixtime(query, start_ts_dt, db->db_tz_utc) < 0
			|| cmyth_mysql_query_param_int(query, type) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}
	while ((row = mysql_fetch_row(res))) {
		data = safe_atoll(row[0]);
		rows++;
	}
	mysql_free_result(res);

	return data;
}

/*
 * cmyth_mysql_estimate_rec_framerate()
 *
 * Scope: PRIVATE
 *
 * Description
 *
 * Estimate framerate using seek mark for MPEG recordings.
 *
 * Success: returns 0 for invalid value else framerate (fps x 1000)
 *
 * Failure: -1
 */
int64_t
cmyth_mysql_estimate_rec_framerate(cmyth_database_t db, cmyth_proginfo_t prog)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	/*
	 * Force usage of primary key index to retrieve last mark value:
	 * const DESC, const DESC, const DESC
	 */
	const char *query_str = "SELECT mark FROM recordedseek WHERE chanid = ? AND starttime = ? AND type = 9 ORDER BY chanid DESC, starttime DESC, type DESC, mark DESC LIMIT 1;";
	int rows = 0;
	int64_t mark = 0;
	int64_t dsecs = 0;
	int64_t fpms;
	time_t start_ts_dt;
	time_t end_ts_dt;
	cmyth_mysql_query_t * query;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	start_ts_dt = cmyth_timestamp_to_unixtime(prog->proginfo_rec_start_ts);
	end_ts_dt = cmyth_timestamp_to_unixtime(prog->proginfo_rec_end_ts);
	dsecs = (int64_t)(end_ts_dt - start_ts_dt);
	if (dsecs <= 0) {
		return 0;
	}

	query = cmyth_mysql_query_create(db, query_str);

	if (cmyth_mysql_query_param_uint32(query, prog->proginfo_chanId) < 0
			|| cmyth_mysql_query_param_unixtime(query, start_ts_dt, db->db_tz_utc) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}
	while ((row = mysql_fetch_row(res))) {
		mark = safe_atoll(row[0]);
		rows++;
	}
	mysql_free_result(res);

	if (mark > 0) {
		fpms = (mark * 1000) / dsecs;
	} else {
		return 0;
	}
	return fpms;
}

/*
 * cmyth_mysql_get_recording_framerate()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Returns framerate for a recording.
 *
 * Success: returns 0 for invalid value else framerate (fps x 1000)
 */
int64_t
cmyth_mysql_get_recording_framerate(cmyth_database_t db, cmyth_proginfo_t prog)
{
	int64_t ret;
	ret = cmyth_mysql_get_recording_markup(db, prog, MARK_VIDEO_RATE);
	if (ret > 10000 && ret < 80000) {
		return ret;
	} else {
		cmyth_dbg(CMYTH_DBG_WARN, "%s, implausible frame rate: %"PRId64"\n", __FUNCTION__, ret);
	}

	ret = cmyth_mysql_estimate_rec_framerate(db, prog);
	if (ret > 10000 && ret < 80000) {
		return ret;
	} else {
		cmyth_dbg(CMYTH_DBG_WARN, "%s, failed to estimate frame rate: %"PRId64"\n", __FUNCTION__, ret);
	}

	return 0;
}

/*
 * cmyth_mysql_get_recording_artwork()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Returns artworks for a recording.
 *
 * Success: returns 0 for unavailable else 1
 *
 * Failure: -1
 */
int
cmyth_mysql_get_recording_artwork(cmyth_database_t db, cmyth_proginfo_t prog, char **coverart, char **fanart, char **banner)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT coverart, fanart, banner FROM recordedartwork WHERE inetref = ? AND host = ? AND season = ? "
				"UNION ALL "
				"SELECT coverart, fanart, banner FROM recordedartwork WHERE inetref = ? AND host = ?";
	int rows = 0;
	cmyth_mysql_query_t * query;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	/* DB version change at mythtv/libs/libmythtv/dbcheck.cpp:1473 */
	if (db->db_version < 1279)
		return 0;

	query = cmyth_mysql_query_create(db, query_str);

	if (cmyth_mysql_query_param_str(query, prog->proginfo_inetref) < 0
			|| cmyth_mysql_query_param_str(query, prog->proginfo_hostname) < 0
			|| cmyth_mysql_query_param_uint(query, prog->proginfo_season) < 0
			|| cmyth_mysql_query_param_str(query, prog->proginfo_inetref) < 0
			|| cmyth_mysql_query_param_str(query, prog->proginfo_hostname) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	if ((row = mysql_fetch_row(res))) {
		*coverart = ref_strdup(row[0]);
		*fanart = ref_strdup(row[1]);
		*banner = ref_strdup(row[2]);
		rows++;
	}

	mysql_free_result(res);
	return rows;
}

/*
 * cmyth_mysql_get_setting()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Returns data for global (non host specific) setting.
 *
 * Success: returns 0 for unavailable else 1
 *
 * Failure: -1
 */
int
cmyth_mysql_get_setting(cmyth_database_t db, char *setting, char **data)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT data FROM settings WHERE value = ? AND hostname IS NULL;";
	int rows = 0;
	cmyth_mysql_query_t * query;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	query = cmyth_mysql_query_create(db, query_str);

	if (cmyth_mysql_query_param_str(query, setting) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	if ((row = mysql_fetch_row(res))) {
		*data = ref_strdup(row[0]);
		rows++;
	}

	mysql_free_result(res);
	return rows;
}

/*
 * cmyth_mysql_keep_livetv_recording()
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * keep/ignore recording.
 *
 * Success: returns 0 for unavailable else 1
 *
 * Failure: -1
 */
int cmyth_mysql_keep_livetv_recording(cmyth_database_t db, cmyth_proginfo_t prog, int8_t keep)
{
	int ret;
	MYSQL* sql = cmyth_db_get_connection(db);
	const char *query_str = "UPDATE recorded SET autoexpire = ?, recgroup = ? WHERE chanid = ? AND starttime = ? AND storagegroup = 'LiveTV'";
	cmyth_mysql_query_t * query;
	time_t starttime;
	int autoexpire;
	const char* recgroup;

	if (cmyth_database_check_version(db) < 0)
		return -1;

	if (keep) {
		char* data;
		if (cmyth_mysql_get_setting(db, "AutoExpireDefault", &data) > 0) {
			autoexpire = safe_atol(data);
			ref_release(data);
		} else {
			autoexpire = 0;
		}
		recgroup = "Default";
	}
	else
	{
		autoexpire = 10000;
		recgroup = "LiveTV";
	}

	starttime = cmyth_timestamp_to_unixtime(prog->proginfo_rec_start_ts);
	query = cmyth_mysql_query_create(db, query_str);
	if (cmyth_mysql_query_param_int32(query,autoexpire) < 0
		|| cmyth_mysql_query_param_str(query,recgroup) < 0
		|| cmyth_mysql_query_param_uint32(query, prog->proginfo_chanId) < 0
		|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}

	ret = cmyth_mysql_query(query);

	ref_release(query);

	if (ret < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return ret;
	}

	return (int)mysql_affected_rows(sql);
}
