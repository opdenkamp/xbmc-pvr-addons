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
    if(db->mysql != NULL)
    {
		mysql_close(db->mysql);
		db->mysql = NULL;
		db->db_version = -1;
		db->db_tz_utc = 0;
    }
}

static void
cmyth_database_destroy(cmyth_database_t db)
{
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	cmyth_database_close(db);
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
	    rtrn->db_version = -1;
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
	int dbversion;
	const char *query_str = "SELECT data FROM settings WHERE value = 'DBSchemaVer' AND hostname IS NULL LIMIT 1;";
	cmyth_mysql_query_t * query;

	// Set db version to unknown
	db->db_version = 0;
	// Get db version from settings

	query = cmyth_mysql_query_create(db,query_str);
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}
	row = mysql_fetch_row(res);
	dbversion = safe_atoi(row[0]);
	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s, database version is: %d", __FUNCTION__, dbversion);
	if (dbversion > 0) {
		db->db_version = dbversion;
		if (dbversion >= 1307) {
			db->db_tz_utc = 1;
			strcpy(db->db_tz_name,"UTC");
		}
		else {
			db->db_tz_utc = 0;
			strcpy(db->db_tz_name,"SYSTEM");
		}
	}

	return 1;
}

int
cmyth_database_set_host(cmyth_database_t db, char *host)
{
	cmyth_database_close(db);
	ref_release(db->db_host);
	db->db_host = ref_strdup(host);
	if(! db->db_host)
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
	if(! db->db_user)
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
	if(! db->db_pass)
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
	if(! db->db_name)
	    return 0;
	else
	    return 1;
}

int
cmyth_database_set_port(cmyth_database_t db, unsigned short port)
{
	cmyth_database_close(db);
	db->db_port = port;
	return 1;
}

int
cmyth_database_get_version(cmyth_database_t db)
{
	return db->db_version;
}

static int
cmyth_db_check_connection(cmyth_database_t db)
{
	if(db->mysql != NULL) {
		/* Fetch the mysql stats (uptime and stuff) to check the connection is
		 * still good
		 */
		if (mysql_stat(db->mysql) == NULL) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_stat() failed: %s\n", __FUNCTION__, mysql_error(db->mysql));
			cmyth_database_close(db);
			mysql_close(db->mysql);
			db->mysql = NULL;
		}
	}
	if (db->mysql == NULL) {
		db->mysql = mysql_init(NULL);
		if(db->mysql == NULL) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_init() failed, insufficient memory?\n", __FUNCTION__);
			return -1;
		}
		if(NULL == mysql_real_connect(db->mysql,db->db_host,db->db_user,db->db_pass,db->db_name,db->db_port,NULL,0))
		{
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_connect() failed: %s\n", __FUNCTION__, mysql_error(db->mysql));
			cmyth_database_close(db);
			mysql_close(db->mysql);
			db->mysql = NULL;
			return -1;
		}
	}
	return 0;
}

MYSQL *
cmyth_db_get_connection(cmyth_database_t db)
{
    int err;

    if(cmyth_db_check_connection(db) != 0)
    {
       cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n",
       					__FUNCTION__);
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
    if(mysql_query(db->mysql,"SET NAMES utf8;")) {
      cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query() failed: %s\n", __FUNCTION__, mysql_error(db->mysql));
      return NULL;
    }
    /*
     * Since version 0.26, mythbackend store in the database an exhaustive list of datetime fields
     * using UTC zone, not all. Others fields keep local system time zone. Also the field types don't
     * have extension 'with time zone'. To manage this situation we check the DB schema version to
     * know if we must manually convert these date, time or datetime fields during exchanges with the
     * database (read or store). If true then the attribute db_tz_utc is set to 1 else 0.
     * This check is done one time at the first getting connection when the attribute db_version is
     * not yet known (-1).
     */
    if (db->db_version < 0 && (err=cmyth_database_setup(db)) < 0) {
            cmyth_dbg(CMYTH_DBG_ERROR, "%s: database setup failed (%d)\n",
                    __FUNCTION__, err);
            return NULL;
    }
    /*
     * Setting the TIME_ZONE is not really required since all datetime fields don't have extension
     * 'with time zone'. But we do even.
     */
    if(mysql_query(db->mysql,"SET TIME_ZONE='SYSTEM';")) {
            cmyth_dbg(CMYTH_DBG_ERROR, "%s: SET TIME_ZONE failed: %s\n",
                    __FUNCTION__, mysql_error(db->mysql));
            return NULL;
    }

    return db->mysql;
}

char *
cmyth_mysql_escape_chars(cmyth_database_t db, char * string)
{
	char *N_string;
	size_t len;

	if(cmyth_db_check_connection(db) != 0)
	{
               cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n", __FUNCTION__);
	       return NULL;
	}

	len = strlen(string);
	N_string=ref_alloc(len*2+1);
	mysql_real_escape_string(db->mysql,N_string,string,len);

	return (N_string);
}

int
cmyth_mysql_get_offset(cmyth_database_t db, int type, unsigned long recordid, unsigned long chanid, char *title, char *subtitle, char *description, char *seriesid, char *programid)
{
	MYSQL_RES *res=NULL;
	MYSQL_ROW row;
	char query[1000];
	int count;

	if(cmyth_db_check_connection(db) != 0)
	{
               cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n", __FUNCTION__);
	       return -1;
	}
	if (type == 1) { // startoffset
		sprintf (query,"SELECT startoffset FROM record WHERE (recordid=%ld AND chanid=%ld AND title='%s' AND subtitle='%s' AND description='%s' AND seriesid='%s' AND programid='%s')",recordid,chanid,title,subtitle,description,seriesid,programid);
	}
	else if (type == 0) { //endoffset
		sprintf (query,"SELECT endoffset FROM record WHERE (recordid=%ld AND chanid=%ld AND title='%s' AND subtitle='%s' AND description='%s' AND seriesid='%s' AND programid='%s')",recordid,chanid,title,subtitle,description,seriesid,programid);
	}

	cmyth_dbg(CMYTH_DBG_ERROR, "%s : query=%s\n",__FUNCTION__, query);

        if(mysql_query(db->mysql,query)) {
                 cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query() Failed: %s\n", __FUNCTION__, mysql_error(db->mysql));
		return -1;
        }
        res = mysql_store_result(db->mysql);
	if ( (count = (int)mysql_num_rows(res)) >0) {
		row = mysql_fetch_row(res);
		cmyth_dbg(CMYTH_DBG_DEBUG, "row grabbed done count=%d\n", count);
        	mysql_free_result(res);
		return atoi(row[0]);
	}
	else {
        	mysql_free_result(res);
		return 0;
	}
}

int
cmyth_mysql_set_watched_status(cmyth_database_t db, cmyth_proginfo_t prog, int watchedStat)
{
	MYSQL_RES *res = NULL;
	const char *query_str = "UPDATE recorded SET watched = ? WHERE chanid = ? AND starttime = ?";
	cmyth_mysql_query_t * query;
	time_t starttime;

	if (watchedStat > 1) watchedStat = 1;
	if (watchedStat < 0) watchedStat = 0;
	starttime = cmyth_timestamp_to_unixtime(prog->proginfo_rec_start_ts);
	query = cmyth_mysql_query_create(db, query_str);

	if (cmyth_mysql_query_param_long(query, watchedStat) < 0
	 || cmyth_mysql_query_param_ulong(query, prog->proginfo_chanId) < 0
	 || cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0 ) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	if (cmyth_mysql_query(query) != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, finalisation/execution of query failed!\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	mysql_free_result(res);
	ref_release(query);
	return (1);
}

char *
cmyth_mysql_get_recordid(cmyth_database_t db, unsigned long chanid, char *title, char *subtitle, char *description, char *seriesid, char *programid)
{
	MYSQL_RES *res=NULL;
	MYSQL_ROW row;
	char query[1000];
	int count;

	if(cmyth_db_check_connection(db) != 0)
	{
               cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n", __FUNCTION__);
	       return NULL;
	}
	sprintf (query,"SELECT recordid FROM record WHERE (chanid=%ld AND title='%s' AND subtitle='%s' AND description='%s' AND seriesid='%s' AND programid='%s')",chanid,title,subtitle,description,seriesid,programid);

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s : query=%s\n",__FUNCTION__, query);

        if(mysql_query(db->mysql,query)) {
                 cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query() Failed: %s\n", __FUNCTION__, mysql_error(db->mysql));
		return NULL;
        }
        res = mysql_store_result(db->mysql);
	if ( (count = (int)mysql_num_rows(res)) >0) {
		row = mysql_fetch_row(res);
		cmyth_dbg(CMYTH_DBG_DEBUG, "row grabbed done count=%d\n", count);
        	mysql_free_result(res);
		return row[0];
	}
	else {
        	mysql_free_result(res);
		return "NULL";
	}
}

int
cmyth_mysql_get_prev_recorded(cmyth_database_t db, cmyth_program_t **prog)
{
	MYSQL_RES *res= NULL;
	MYSQL_ROW row;
	int n=0;
	int rows=0;
	const char *query_str = "SELECT oldrecorded.chanid, UNIX_TIMESTAMP(CONVERT_TZ(starttime, ?, 'SYSTEM')), UNIX_TIMESTAMP(CONVERT_TZ(endtime, ?, 'SYSTEM')), title, subtitle, description, category, seriesid, programid, channel.channum, channel.callsign, channel.name, findid, rectype, recstatus, recordid, duplicate FROM oldrecorded LEFT JOIN channel ON oldrecorded.chanid = channel.chanid ORDER BY starttime ASC";
	cmyth_mysql_query_t *query;
	query = cmyth_mysql_query_create(db, query_str);

	if(cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
	 || cmyth_mysql_query_param_str(query, db->db_tz_name) < 0)
	{
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		ref_release(prog);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL)
	{
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	while((row = mysql_fetch_row(res))) {
        	if (rows >= n) {
                	n+=10;
                       	*prog=realloc(*prog,sizeof(**prog)*(n));
               	}
		(*prog)[rows].chanid = safe_atol(row[0]);
               	(*prog)[rows].recording = 0;
		(*prog)[rows].starttime =(time_t)safe_atol(row[1]);
		(*prog)[rows].endtime = (time_t)safe_atol(row[2]);
		sizeof_strncpy((*prog)[rows].title, row[3]);
		sizeof_strncpy((*prog)[rows].subtitle, row[4]);
		sizeof_strncpy((*prog)[rows].description, row[5]);
		sizeof_strncpy((*prog)[rows].category, row[6]);
		sizeof_strncpy((*prog)[rows].seriesid, row[7]);
		sizeof_strncpy((*prog)[rows].programid, row[8]);
		(*prog)[rows].channum = safe_atol(row[9]);
		sizeof_strncpy((*prog)[rows].callsign, row[10]);
		sizeof_strncpy((*prog)[rows].name, row[11]);
		//sizeof_strncpy((*prog)[rows].rec_status, row[14]);
		(*prog)[rows].rec_status = safe_atol(row[14]);
          	rows++;
        }
        mysql_free_result(res);
        cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}

int
cmyth_mysql_get_guide(cmyth_database_t db, cmyth_program_t **prog, time_t starttime, time_t endtime)
{
	MYSQL_RES *res= NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT program.chanid, UNIX_TIMESTAMP(CONVERT_TZ(program.starttime, ?, 'SYSTEM')), UNIX_TIMESTAMP(CONVERT_TZ(program.endtime, ?, 'SYSTEM')), "
		"program.title, program.description, program.subtitle, program.programid, program.seriesid, program.category, "
		"channel.channum, channel.callsign, channel.name, channel.sourceid "
		"FROM program INNER JOIN channel ON program.chanid=channel.chanid "
		"WHERE channel.visible = 1 AND ((program.endtime > ? AND program.endtime < ?) OR (program.starttime >= ? AND program.starttime <= ?) OR (program.starttime <= ? AND program.endtime >= ?)) "
		"ORDER BY (channel.channum + 0), program.starttime ASC";
	int rows=0;
	int n=0;
	cmyth_mysql_query_t * query;
	query = cmyth_mysql_query_create(db,query_str);

	if(cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
	    || cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
	    || cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
	    || cmyth_mysql_query_param_unixtime(query, endtime, db->db_tz_utc) < 0
	    || cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
	    || cmyth_mysql_query_param_unixtime(query, endtime, db->db_tz_utc) < 0
	    || cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
	    || cmyth_mysql_query_param_unixtime(query, endtime, db->db_tz_utc) < 0)
	{
	    cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
	    ref_release(query);
	    return -1;
 	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if(res == NULL)
	{
	    cmyth_dbg(CMYTH_DBG_ERROR,"%s, finalisation/execution of query failed!\n", __FUNCTION__);
	    return -1;
	}


	while((row = mysql_fetch_row(res))) {
        	if (rows >= n) {
                	n+=10;
                       	*prog=ref_realloc(*prog,sizeof(**prog)*(n));
               	}
		(*prog)[rows].chanid = safe_atol(row[0]);
               	(*prog)[rows].recording = 0;
		(*prog)[rows].starttime = (time_t)safe_atol(row[1]);
		(*prog)[rows].endtime = (time_t)safe_atol(row[2]);
		sizeof_strncpy((*prog)[rows].title, row[3]);
		sizeof_strncpy((*prog)[rows].description, row[4]);
		sizeof_strncpy((*prog)[rows].subtitle, row[5]);
		sizeof_strncpy((*prog)[rows].programid, row[6]);
		sizeof_strncpy((*prog)[rows].seriesid, row[7]);
		sizeof_strncpy((*prog)[rows].category, row[8]);
		(*prog)[rows].channum = safe_atol(row[9]);
		sizeof_strncpy((*prog)[rows].callsign, row[10]);
		sizeof_strncpy((*prog)[rows].name, row[11]);
		(*prog)[rows].sourceid = safe_atol(row[12]);
		(*prog)[rows].startoffset = 0;
		(*prog)[rows].endoffset = 0;
          	rows++;
        }
        mysql_free_result(res);
        cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}

int
cmyth_mysql_get_recgroups(cmyth_database_t db, cmyth_recgroups_t **sqlrecgroups)
{
	MYSQL_RES *res=NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT DISTINCT recgroup FROM record";
	int rows=0;
	int n=0;
	cmyth_mysql_query_t *query;
	query = cmyth_mysql_query_create(db, query_str);

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL)
	{
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

        while((row = mysql_fetch_row(res))) {
        	if (rows == n ) {
                	n++;
                       	*sqlrecgroups=realloc(*sqlrecgroups,sizeof(**sqlrecgroups)*(n));
               	}
		sizeof_strncpy ( (*sqlrecgroups)[rows].recgroups, row[0]);
		cmyth_dbg(CMYTH_DBG_DEBUG, "(*sqlrecgroups)[%d].recgroups =  %s\n",rows, (*sqlrecgroups)[rows].recgroups);
		rows++;
        }
        mysql_free_result(res);
        cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}


int
cmyth_mysql_get_prog_finder_char_title(cmyth_database_t db, cmyth_program_t **prog, time_t starttime, char *program_name)
{
	MYSQL_RES *res=NULL;
	MYSQL_ROW row;
	char *N_title;
	char *esctitle;
	char *query_str;
	int rows=0;
	int n = 0;
	cmyth_mysql_query_t *query;

	if (strncmp(program_name, "@", 1) == 0)
	{
		query_str = "SELECT DISTINCT title FROM program WHERE ( title NOT REGEXP '^[A-Z0-9]' AND title NOT REGEXP '^The [A-Z0-9]' AND title NOT REGEXP '^A [A-Z0-9]' AND starttime >= ?) ORDER BY title";
		query = cmyth_mysql_query_create(db, query_str);
		if(cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0)
		{
			cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
			ref_release(query);
			return -1;
		}
	}
	else
	{
		query_str = "SELECT DISTINCT title FROM program where starttime >= ? and title like ? ORDER BY title ASC";
		query = cmyth_mysql_query_create(db, query_str);
		N_title = ref_alloc(strlen(program_name) * 2 + 3);
		sprintf(N_title,"%%%s%%", program_name);
		esctitle = cmyth_mysql_escape_chars(db, N_title);
		ref_release(N_title);
		if(cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
		    || cmyth_mysql_query_param_str(query, esctitle) < 0)
		{
			cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
			ref_release(esctitle);
			ref_release(query);
			return -1;
		}
		ref_release(esctitle);
	}

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL)
	{
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

        while((row = mysql_fetch_row(res))) {
        	if (rows == n) {
                	n++;
                       	*prog=realloc(*prog,sizeof(**prog)*(n));
               	}
		sizeof_strncpy ( (*prog)[rows].title, row[0]);
		cmyth_dbg(CMYTH_DBG_DEBUG, "prog[%d].title =  %s\n",rows, (*prog)[rows].title);
		rows++;
        }
        mysql_free_result(res);
        cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}

int
cmyth_mysql_get_prog_finder_time(cmyth_database_t db, cmyth_program_t **prog,  time_t starttime, char *program_name)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char *esctitle;
	const char *query_str = "SELECT program.chanid, UNIX_TIMESTAMP(CONVERT_TZ(program.starttime,?,'SYSTEM')), UNIX_TIMESTAMP(CONVERT_TZ(program.endtime,?,'SYSTEM')), program.title, program.description, program.subtitle, program.programid, program.seriesid, program.category, channel.channum, channel.callsign, channel.name, channel.sourceid FROM program LEFT JOIN channel on program.chanid=channel.chanid WHERE starttime >= ? and title = ? ORDER BY starttime ASC";
	int rows = 0;
	int n = 0;
	cmyth_mysql_query_t *query;
	query = cmyth_mysql_query_create(db, query_str);

	esctitle = cmyth_mysql_escape_chars(db, program_name);
	if(cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
	    || cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
	    || cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
	    || cmyth_mysql_query_param_str(query, esctitle) < 0)
	{
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(esctitle);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(esctitle);
	ref_release(query);
	if (res == NULL)
	{
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	while((row = mysql_fetch_row(res))) {
        	if (rows == n) {
                	n++;
                       	*prog=realloc(*prog,sizeof(**prog)*(n));
               	}
			(*prog)[rows].chanid = atol(row[0]);
			(*prog)[rows].recording = 0;
			(*prog)[rows].starttime = atol(row[1]);
			(*prog)[rows].endtime = atol(row[2]);
			sizeof_strncpy ((*prog)[rows].title, row[3]);
			sizeof_strncpy ((*prog)[rows].description, row[4]);
			sizeof_strncpy ((*prog)[rows].subtitle, row[5]);
			sizeof_strncpy ((*prog)[rows].programid, row[6]);
			sizeof_strncpy ((*prog)[rows].seriesid, row[7]);
			sizeof_strncpy ((*prog)[rows].category, row[8]);
			(*prog)[rows].channum = atol(row[9]);
			sizeof_strncpy ((*prog)[rows].callsign,row[10]);
			sizeof_strncpy ((*prog)[rows].name,row[11]);
			(*prog)[rows].sourceid = atol(row[12]);
			cmyth_dbg(CMYTH_DBG_DEBUG, "prog[%d].chanid =  %ld\n",rows, (*prog)[rows].chanid);
			cmyth_dbg(CMYTH_DBG_DEBUG, "prog[%d].title =  %s\n",rows, (*prog)[rows].title);
			rows++;
        }
        mysql_free_result(res);
        cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}

int
cmyth_mysql_update_bookmark_setting(cmyth_database_t db, cmyth_proginfo_t prog)
{
	MYSQL_RES *res = NULL;
	const char *query_str = "UPDATE recorded SET bookmark = 1 WHERE chanid = ? AND starttime = ?";
	cmyth_mysql_query_t * query;
	time_t starttime;

	starttime = cmyth_timestamp_to_unixtime(prog->proginfo_rec_start_ts);
	query = cmyth_mysql_query_create(db,query_str);
	if (cmyth_mysql_query_param_ulong(query, prog->proginfo_chanId) < 0
		|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0 ) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}
	mysql_free_result(res);
	return (1);
}

long long
cmyth_mysql_get_bookmark_mark(cmyth_database_t db, cmyth_proginfo_t prog, long long bk, int mode)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT mark, type FROM recordedseek WHERE chanid = ? AND offset < ? AND (type = 6 or type = 9 ) AND starttime = ? ORDER by MARK DESC LIMIT 0, 1;";
	int rows = 0;
	long long mark = 0;
	int rectype = 0;
	time_t start_ts_dt;
	cmyth_mysql_query_t * query;
	start_ts_dt = cmyth_timestamp_to_unixtime(prog->proginfo_rec_start_ts);
	query = cmyth_mysql_query_create(db,query_str);

	if (cmyth_mysql_query_param_ulong(query, prog->proginfo_chanId) < 0
		|| cmyth_mysql_query_param_int64(query, (int64_t)bk) < 0
		|| cmyth_mysql_query_param_unixtime(query, start_ts_dt, db->db_tz_utc) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
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
			mark=(mark-1)*15;
		}
		else if (mode == 1) {
			mark=(mark-1)*12;
		}
	}

	return mark;
}

long long
cmyth_mysql_get_bookmark_offset(cmyth_database_t db, unsigned long chanid, long long mark, time_t starttime, int mode)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	long long offset = 0;
	int rows = 0;
	int rectype = 0;
	cmyth_mysql_query_t * query;

	const char *query_str = "SELECT chanid, UNIX_TIMESTAMP(CONVERT_TZ(starttime,?,'SYSTEM')), mark, offset, type FROM recordedseek WHERE chanid = ? AND mark<= ? AND starttime = ? ORDER BY MARK DESC LIMIT 1;";

	query = cmyth_mysql_query_create(db,query_str);
	if (cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
		|| cmyth_mysql_query_param_ulong(query, chanid) < 0
		|| cmyth_mysql_query_param_int64(query, (int64_t)mark) < 0
		|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
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
			mark=(mark/15)+1;
		}
		else if (mode == 1) {
			mark=(mark/12)+1;
		}
		query = cmyth_mysql_query_create(db, query_str);
		if (cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
			|| cmyth_mysql_query_param_ulong(query, chanid) < 0
			|| cmyth_mysql_query_param_int64(query, (int64_t)mark) < 0
			|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
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
cmyth_mysql_query_commbreak_count(cmyth_database_t db, unsigned long chanid, time_t start_ts_dt) {
	MYSQL_RES *res = NULL;
	int count = 0;
	char * query_str;
  cmyth_mysql_query_t * query;
	query_str = "SELECT * FROM recordedmarkup WHERE chanid = ? AND starttime = ? AND TYPE IN ( 4 )";

	query = cmyth_mysql_query_create(db,query_str);
	if (cmyth_mysql_query_param_ulong(query, chanid) < 0
		|| cmyth_mysql_query_param_unixtime(query, start_ts_dt, db->db_tz_utc) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}
	count = (int)mysql_num_rows(res);
	mysql_free_result(res);
	return (count);
}

int
cmyth_mysql_get_commbreak_list(cmyth_database_t db, unsigned long chanid, time_t start_ts_dt, cmyth_commbreaklist_t breaklist, int conn_version)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	int resolution = 30;
	char * query_str;
	int rows = 0;
	int i;
	cmyth_mysql_query_t * query;
	cmyth_commbreak_t commbreak = NULL;
	long long start_previous = 0;
	long long end_previous = 0;

	if (conn_version >= 43) {
		query_str = "SELECT m.type,m.mark,s.mark,s.offset  FROM recordedmarkup m INNER JOIN recordedseek AS s ON m.chanid = s.chanid AND m.starttime = s.starttime  WHERE m.chanid = ? AND m.starttime = ? AND m.type in (?,?) and FLOOR(m.mark/?)=FLOOR(s.mark/?) ORDER BY `m`.`mark` LIMIT 300 ";
	}
	else {
		query_str = "SELECT m.type AS type, m.mark AS mark, s.offset AS offset FROM recordedmarkup m INNER JOIN recordedseek AS s ON (m.chanid = s.chanid AND m.starttime = s.starttime AND (FLOOR(m.mark / 15) + 1) = s.mark) WHERE m.chanid = ? AND m.starttime = ? AND m.type IN (?, ?) ORDER BY mark;";
	}

	cmyth_dbg(CMYTH_DBG_DEBUG,"%s, query=%s\n", __FUNCTION__,query_str);

	query = cmyth_mysql_query_create(db,query_str);

	if ((conn_version >= 43) && (
		cmyth_mysql_query_param_ulong(query, chanid) < 0
		|| cmyth_mysql_query_param_unixtime(query, start_ts_dt, db->db_tz_utc) < 0
		|| cmyth_mysql_query_param_int(query, CMYTH_COMMBREAK_START) < 0
		|| cmyth_mysql_query_param_int(query, CMYTH_COMMBREAK_END) < 0
		|| cmyth_mysql_query_param_int(query, resolution) < 0
		|| cmyth_mysql_query_param_int(query, resolution) < 0
		)) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}

	if ((conn_version < 43) && (
		cmyth_mysql_query_param_ulong(query, chanid) < 0
		|| cmyth_mysql_query_param_unixtime(query, start_ts_dt, db->db_tz_utc) < 0
		|| cmyth_mysql_query_param_int(query, CMYTH_COMMBREAK_START) < 0
		|| cmyth_mysql_query_param_int(query, CMYTH_COMMBREAK_END) < 0
		)) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	if (conn_version >= 43) {
		breaklist->commbreak_count = cmyth_mysql_query_commbreak_count(db, chanid, start_ts_dt);
	}
	else {
		breaklist->commbreak_count = (long)mysql_num_rows(res) / 2;
	}
	breaklist->commbreak_list = malloc(breaklist->commbreak_count * sizeof(cmyth_commbreak_t));

	if (!breaklist->commbreak_list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: malloc() failed for list\n",
			__FUNCTION__);
		return -1;
	}
	memset(breaklist->commbreak_list, 0, breaklist->commbreak_count * sizeof(cmyth_commbreak_t));

	i = 0;
	if (conn_version >= 43) {
		while ((row = mysql_fetch_row(res))) {
			if (safe_atoi(row[0]) == CMYTH_COMMBREAK_START) {
				if (safe_atoll(row[1]) != start_previous) {
					commbreak = cmyth_commbreak_create();
					commbreak->start_mark = safe_atoll(row[1]);
					commbreak->start_offset = safe_atoll(row[3]);
					start_previous = commbreak->start_mark;
				}
				else if (safe_atoll(row[1]) == safe_atoll(row[2])) {
					commbreak = cmyth_commbreak_create();
					commbreak->start_mark = safe_atoll(row[1]);
					commbreak->start_offset = safe_atoll(row[3]);
				}
			} else if (safe_atoi(row[0]) == CMYTH_COMMBREAK_END) {
				if (safe_atoll(row[1]) != end_previous) {
					commbreak->end_mark = safe_atoll(row[1]);
					commbreak->end_offset = safe_atoll(row[3]);
					breaklist->commbreak_list[rows] = commbreak;
					end_previous = commbreak->end_mark;
					rows++;
				}
				else if (safe_atoll(row[1]) == safe_atoll(row[2])) {
					commbreak->end_mark = safe_atoll(row[1]);
					commbreak->end_offset = safe_atoll(row[3]);
					breaklist->commbreak_list[rows] = commbreak;
					if (end_previous != safe_atoll(row[1])) {
						rows++;
					}
				}
			}
			else {
				cmyth_dbg(CMYTH_DBG_ERROR, "%s: Unknown COMMBREAK returned\n",
					__FUNCTION__);
				return -1;
			}
			i++;
		}
	}

	// mythtv protolcol version < 43
	else {
		while ((row = mysql_fetch_row(res))) {
			if ((i % 2) == 0) {
				if (safe_atoi(row[0]) != CMYTH_COMMBREAK_START) {
					return -1;
				}
				commbreak = cmyth_commbreak_create();
				commbreak->start_mark = safe_atoll(row[1]);
				commbreak->start_offset = safe_atoll(row[2]);
				i++;
			} else {
				if (safe_atoi(row[0]) != CMYTH_COMMBREAK_END) {
					return -1;
				}
				commbreak->end_mark = safe_atoll(row[1]);
				commbreak->end_offset = safe_atoll(row[2]);
				breaklist->commbreak_list[rows] = commbreak;
				i = 0;
				rows++;
			}
		}
	}
	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: COMMBREAK rows= %d\n", __FUNCTION__, rows);
	return rows;
}

int
cmyth_mysql_tuner_type_check(cmyth_database_t db, cmyth_recorder_t rec, int check_tuner_type) {
	MYSQL_RES *res=NULL;
	MYSQL_ROW row;
	const char * query_str = "SELECT cardtype from capturecard WHERE cardid=?";
	cmyth_mysql_query_t * query;

	if ( check_tuner_type == 0 ) {
		cmyth_dbg(CMYTH_DBG_ERROR,"MythTV Tuner check not enabled in Mythtv Options\n");
		return (1);
	}


	query = cmyth_mysql_query_create(db,query_str);
	if (cmyth_mysql_query_param_uint(query,rec->rec_id) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query failed\n",__FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);

	if(res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, finalisation/execution\n",__FUNCTION__);
		return -1;
	}
	row = mysql_fetch_row(res);
	ref_release(query);
	mysql_free_result(res);
	if (strcmp(row[0],"MPEG") == 0) {
		return (1); //return the first available MPEG tuner
	}
	else if (strcmp(row[0],"HDHOMERUN") == 0) {
		return (1); //return the first available MPEG2TS tuner
	}
	else if (strcmp(row[0],"DVB") == 0) {
		return (1); //return the first available DVB tuner
	}
	else {
		return (0);
	}
}

int
cmyth_mysql_testdb_connection(cmyth_database_t db,char **message) {
	char *buf=ref_alloc(sizeof(char)*1001);
	if (db->mysql != NULL) {
		if (mysql_stat(db->mysql) == NULL) {
			cmyth_database_close(db);
			return -1;
			}
	}
	if (db->mysql == NULL) {
		db->mysql = mysql_init(NULL);
		if(db->mysql == NULL) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_init() failed, insufficient memory?", __FUNCTION__);
			snprintf(buf, 1000, "mysql_init() failed, insufficient memory?");
			*message=buf;
			return -1;
		}
		if (NULL == mysql_real_connect(db->mysql, db->db_host,db->db_user,db->db_pass,db->db_name,db->db_port,NULL,0)) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_connect() failed: %s\n", __FUNCTION__, mysql_error(db->mysql));
			snprintf(buf, 1000, "%s",mysql_error(db->mysql));
			*message=buf;
			cmyth_database_close(db);
			return -1;
		}
	}
	snprintf(buf, 1000, "All Test Successful\n");
	*message=buf;
	return 1;
}

cmyth_chanlist_t
cmyth_mysql_get_chanlist(cmyth_database_t db)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT chanid, channum, name, icon, visible, sourceid, mplexid, callsign FROM channel;";
	int rows = 0;
	cmyth_mysql_query_t * query;
	cmyth_channel_t channel;
	cmyth_chanlist_t chanlist;
	query = cmyth_mysql_query_create(db,query_str);

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return NULL;
	}

	chanlist = cmyth_chanlist_create();

	chanlist->chanlist_count = (int)mysql_num_rows(res);
	chanlist->chanlist_list = malloc(chanlist->chanlist_count * sizeof(cmyth_chanlist_t));
	if (!chanlist->chanlist_list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: malloc() failed for list\n",
			__FUNCTION__);
                ref_release(chanlist);
		return NULL;
	}
	memset(chanlist->chanlist_list, 0, chanlist->chanlist_count * sizeof(cmyth_chanlist_t));

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
		chanlist->chanlist_list[rows] = channel;
		rows++;
	}

	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	return chanlist;
}

int
cmyth_mysql_is_radio(cmyth_database_t db,  unsigned long chanid)
{
	cmyth_mysql_query_t * query;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	int retval=-1;

	if(cmyth_db_check_connection(db) != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n", __FUNCTION__);
		return -1;
	}

	query = cmyth_mysql_query_create(db,"SELECT is_audio_service FROM channelscan_channel INNER JOIN channel ON channelscan_channel.service_id=channel.serviceid WHERE channel.chanid = ? ORDER BY channelscan_channel.scanid DESC;");

	if(cmyth_mysql_query_param_ulong(query,chanid) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
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
		retval = safe_atoi(row[0]);
	} else {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, Channum %ld not found\n", __FUNCTION__, chanid);
		return -1;
	}
	mysql_free_result(res);
	return retval;
}

cmyth_recordingrulelist_t
cmyth_mysql_get_recordingrules(cmyth_database_t db)
{
	MYSQL_RES *res= NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT recordid, chanid, UNIX_TIMESTAMP(CONVERT_TZ(ADDTIME(startdate,starttime),?,'SYSTEM')), UNIX_TIMESTAMP(CONVERT_TZ(ADDTIME(enddate,endtime),?,'SYSTEM')),title,description, type, category, subtitle, recpriority, startoffset, endoffset, search, inactive, station, dupmethod, dupin, recgroup, storagegroup, playgroup, autotranscode, (autouserjob1 | (autouserjob2 << 1) | (autouserjob3 << 2) | (autouserjob4 << 3)), autocommflag, autoexpire, maxepisodes, maxnewest, transcoder FROM record ORDER BY recordid";
	int rows=0;
	cmyth_recordingrule_t rr;
	cmyth_recordingrulelist_t rrl;

	cmyth_mysql_query_t * query;
	query = cmyth_mysql_query_create(db,query_str);
	if (cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
		|| cmyth_mysql_query_param_str(query, db->db_tz_name) < 0)
	{
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return NULL;
	}

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if(res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return NULL;
	}

	rrl = cmyth_recordingrulelist_create();

	rrl->recordingrulelist_count = (int)mysql_num_rows(res);
	rrl->recordingrulelist_list = malloc(rrl->recordingrulelist_count * sizeof(cmyth_recordingrulelist_t));
	if (!rrl->recordingrulelist_list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: malloc() failed for recschedule list\n",
			  __FUNCTION__);
		ref_release(rrl);
		return NULL;
	}
	memset(rrl->recordingrulelist_list, 0, rrl->recordingrulelist_count * sizeof(cmyth_recordingrulelist_t));

	while ((row = mysql_fetch_row(res))) {
		rr = cmyth_recordingrule_create();
		rr->recordid = safe_atol(row[0]);
		rr->chanid = safe_atol(row[1]);
		rr->starttime = cmyth_timestamp_from_unixtime((time_t)safe_atol(row[2]));
		rr->endtime = cmyth_timestamp_from_unixtime((time_t)safe_atol(row[3]));
		rr->title = ref_strdup(row[4]);
		rr->description = ref_strdup(row[5]);
		rr->type = safe_atol(row[6]);
		rr->category = ref_strdup(row[7]);
		rr->subtitle = ref_strdup(row[8]);
		rr->recpriority = safe_atol(row[9]);
		rr->startoffset = safe_atol(row[10]);
		rr->endoffset = safe_atol(row[11]);
		rr->searchtype = safe_atol(row[12]);
		rr->inactive = safe_atoi(row[13]);
		rr->callsign = ref_strdup(row[14]);
		rr->dupmethod = safe_atol(row[15]);
		rr->dupin = safe_atol(row[16]);
		rr->recgroup = ref_strdup(row[17]);
		rr->storagegroup = ref_strdup(row[18]);
		rr->playgroup = ref_strdup(row[19]);
		rr->autotranscode = safe_atoi(row[20]);
		rr->userjobs = safe_atoi(row[21]);
		rr->autocommflag = safe_atoi(row[22]);
		rr->autoexpire = safe_atol(row[23]);
		rr->maxepisodes = safe_atol(row[24]);
		rr->maxnewest = safe_atol(row[25]);
		rr->transcoder = safe_atol(row[26]);
		rrl->recordingrulelist_list[rows] = rr;
		rows++;
	}

	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	return rrl;
}

int
cmyth_mysql_add_recordingrule(cmyth_database_t db, cmyth_recordingrule_t rr)
{
	int ret = -1;
	int id=0;
	MYSQL* sql=cmyth_db_get_connection(db);
	const char *query_str = "INSERT INTO record (record.type, chanid, starttime, startdate, endtime, enddate, title, description, category, findid, findtime, station, subtitle, recpriority, startoffset, endoffset, search, inactive, dupmethod, dupin, recgroup, storagegroup, playgroup, autotranscode, autouserjob1, autouserjob2, autouserjob3, autouserjob4, autocommflag, autoexpire, maxepisodes, maxnewest, transcoder) VALUES (?, ?, TIME(?), DATE(?), TIME(?), DATE(?), ?, ?, ?, TO_DAYS(DATE(?)), TIME(?), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? ,?, ?, ?, ?);";

	char* esctitle = cmyth_mysql_escape_chars(db, rr->title);
	char* escdescription = cmyth_mysql_escape_chars(db, rr->description);
	char* esccategory = cmyth_mysql_escape_chars(db, rr->category);
	char* esccallsign = cmyth_mysql_escape_chars(db, rr->callsign);
	char* escsubtitle = cmyth_mysql_escape_chars(db, rr->subtitle);

	char* escrec_group = cmyth_mysql_escape_chars(db, rr->recgroup);
	char* escstore_group = cmyth_mysql_escape_chars(db, rr->storagegroup);
	char* escplay_group = cmyth_mysql_escape_chars(db, rr->playgroup);

	time_t starttime = cmyth_timestamp_to_unixtime(rr->starttime);
	time_t endtime = cmyth_timestamp_to_unixtime(rr->endtime);

	cmyth_mysql_query_t * query;
	query = cmyth_mysql_query_create(db,query_str);
	if ( cmyth_mysql_query_param_long(query, rr->type) < 0
		|| cmyth_mysql_query_param_ulong(query, rr->chanid) < 0
		|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
		|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
		|| cmyth_mysql_query_param_unixtime(query, endtime, db->db_tz_utc) < 0
		|| cmyth_mysql_query_param_unixtime(query, endtime, db->db_tz_utc) < 0
		|| cmyth_mysql_query_param_str(query, rr->title ) < 0
		|| cmyth_mysql_query_param_str(query, rr->description ) < 0
		|| cmyth_mysql_query_param_str(query, rr->category ) < 0
		|| cmyth_mysql_query_param_unixtime(query, starttime, 0) < 0
		|| cmyth_mysql_query_param_unixtime(query, starttime, 0) < 0
		|| cmyth_mysql_query_param_str(query, rr->callsign ) < 0
		|| cmyth_mysql_query_param_str(query, rr->subtitle ) < 0
		|| cmyth_mysql_query_param_long(query, rr->recpriority ) < 0
		|| cmyth_mysql_query_param_long(query, rr->startoffset ) < 0
		|| cmyth_mysql_query_param_long(query, rr->endoffset ) < 0
		|| cmyth_mysql_query_param_long(query, rr->searchtype ) < 0
		|| cmyth_mysql_query_param_uint(query, rr->inactive ) < 0
		|| cmyth_mysql_query_param_long(query, rr->dupmethod ) < 0
		|| cmyth_mysql_query_param_long(query, rr->dupin ) < 0
		|| cmyth_mysql_query_param_str(query, rr->recgroup ) < 0
		|| cmyth_mysql_query_param_str(query, rr->storagegroup ) < 0
		|| cmyth_mysql_query_param_str(query, rr->playgroup ) < 0
		|| cmyth_mysql_query_param_uint(query, rr->autotranscode ) < 0
		|| cmyth_mysql_query_param_uint(query, rr->userjobs & 1) < 0
		|| cmyth_mysql_query_param_uint(query, rr->userjobs & 2) < 0
		|| cmyth_mysql_query_param_uint(query, rr->userjobs & 4) < 0
		|| cmyth_mysql_query_param_uint(query, rr->userjobs & 8) < 0
		|| cmyth_mysql_query_param_uint(query, rr->autocommflag ) < 0
		|| cmyth_mysql_query_param_long(query, rr->autoexpire ) < 0
		|| cmyth_mysql_query_param_long(query, rr->maxepisodes ) < 0
		|| cmyth_mysql_query_param_long(query, rr->maxnewest ) < 0
		|| cmyth_mysql_query_param_ulong(query, rr->transcoder ) < 0
		) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		ref_release(esctitle);
		ref_release(escdescription);
		ref_release(esccategory);
		ref_release(esccallsign);
		ref_release(escsubtitle);
		ref_release(escrec_group);
		ref_release(escstore_group);
		ref_release(escplay_group);
		return -1;
	}

	ret = cmyth_mysql_query(query);

	ref_release(query);
	ref_release(esctitle);
	ref_release(escdescription);
	ref_release(esccategory);
	ref_release(esccallsign);
	ref_release(escsubtitle);

	ref_release(escrec_group);
	ref_release(escstore_group);
	ref_release(escplay_group);

	if (ret!=0)
		return -1;

	id = (int)mysql_insert_id(sql);

	return id;
}

int
cmyth_mysql_delete_recordingrule(cmyth_database_t db, unsigned long recordid)
{
	int ret = -1;
	const char *query_str = "DELETE FROM record WHERE recordid = ?;";

	cmyth_mysql_query_t * query;
	query = cmyth_mysql_query_create(db,query_str);
	if (cmyth_mysql_query_param_ulong(query, recordid) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}

	ret = cmyth_mysql_query(query);

	if (ret != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	return 0;
}

int
cmyth_mysql_update_recordingrule(cmyth_database_t db, cmyth_recordingrule_t rr)
{
	int ret = -1;

	const char *query_str = "UPDATE record SET record.type = ?, chanid = ?, starttime = TIME(?), startdate = DATE(?), endtime = TIME(?), enddate = DATE(?) ,title = ?, description = ?, category = ?, subtitle = ?, recpriority = ?, startoffset = ?, endoffset = ?, search = ?, inactive = ?, station = ?, dupmethod = ?, dupin = ?, recgroup = ?, storagegroup = ?, playgroup = ?, autotranscode = ?, autouserjob1 = ?, autouserjob2 = ?, autouserjob3 = ?, autouserjob4 = ?, autocommflag = ?, autoexpire = ?, maxepisodes = ?, maxnewest = ?, transcoder = ? WHERE recordid = ? ;";

	char* esctitle = cmyth_mysql_escape_chars(db, rr->title);
	char* escdescription = cmyth_mysql_escape_chars(db, rr->description);
	char* esccategory = cmyth_mysql_escape_chars(db, rr->category);
	char* esccallsign = cmyth_mysql_escape_chars(db, rr->callsign);
	char* escsubtitle = cmyth_mysql_escape_chars(db, rr->subtitle);

	char* escrec_group = cmyth_mysql_escape_chars(db, rr->recgroup);
	char* escstore_group = cmyth_mysql_escape_chars(db, rr->storagegroup);
	char* escplay_group = cmyth_mysql_escape_chars(db, rr->playgroup);

	time_t starttime = cmyth_timestamp_to_unixtime(rr->starttime);
	time_t endtime = cmyth_timestamp_to_unixtime(rr->endtime);

	cmyth_mysql_query_t * query;
	query = cmyth_mysql_query_create(db,query_str);
	if ( cmyth_mysql_query_param_long(query, rr->type) < 0
		|| cmyth_mysql_query_param_ulong(query, rr->chanid) < 0
		|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
		|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
		|| cmyth_mysql_query_param_unixtime(query, endtime, db->db_tz_utc) < 0
		|| cmyth_mysql_query_param_unixtime(query, endtime, db->db_tz_utc) < 0
		|| cmyth_mysql_query_param_str(query, rr->title ) < 0
		|| cmyth_mysql_query_param_str(query, rr->description ) < 0
		|| cmyth_mysql_query_param_str(query, rr->category ) < 0
		|| cmyth_mysql_query_param_str(query, rr->subtitle ) < 0
		|| cmyth_mysql_query_param_long(query, rr->recpriority ) < 0
		|| cmyth_mysql_query_param_long(query, rr->startoffset ) < 0
		|| cmyth_mysql_query_param_long(query, rr->endoffset ) < 0
		|| cmyth_mysql_query_param_long(query, rr->searchtype ) < 0
		|| cmyth_mysql_query_param_uint(query, rr->inactive ) < 0
		|| cmyth_mysql_query_param_str(query, rr->callsign ) < 0
		|| cmyth_mysql_query_param_long(query, rr->dupmethod ) < 0
		|| cmyth_mysql_query_param_long(query, rr->dupin ) < 0
		|| cmyth_mysql_query_param_str(query, rr->recgroup ) < 0
		|| cmyth_mysql_query_param_str(query, rr->storagegroup ) < 0
		|| cmyth_mysql_query_param_str(query, rr->playgroup ) < 0
		|| cmyth_mysql_query_param_uint(query, rr->autotranscode ) < 0
		|| cmyth_mysql_query_param_uint(query, rr->userjobs & 1) < 0
		|| cmyth_mysql_query_param_uint(query, rr->userjobs & 2) < 0
		|| cmyth_mysql_query_param_uint(query, rr->userjobs & 4) < 0
		|| cmyth_mysql_query_param_uint(query, rr->userjobs & 8) < 0
		|| cmyth_mysql_query_param_uint(query, rr->autocommflag ) < 0
		|| cmyth_mysql_query_param_long(query, rr->autoexpire ) < 0
		|| cmyth_mysql_query_param_long(query, rr->maxepisodes ) < 0
		|| cmyth_mysql_query_param_long(query, rr->maxnewest ) < 0
		|| cmyth_mysql_query_param_ulong(query, rr->transcoder ) < 0
		|| cmyth_mysql_query_param_ulong(query, rr->recordid) < 0
		) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		ref_release(esctitle);
		ref_release(escdescription);
		ref_release(esccategory);
		ref_release(esccallsign);
		ref_release(escsubtitle);
		ref_release(escrec_group);
		ref_release(escstore_group);
		ref_release(escplay_group);
		return -1;
	}

	ret = cmyth_mysql_query(query);

	ref_release(query);
	ref_release(esctitle);
	ref_release(escdescription);
	ref_release(esccategory);
	ref_release(esccallsign);
	ref_release(escsubtitle);

	ref_release(escrec_group);
	ref_release(escstore_group);
	ref_release(escplay_group);

	if (ret == -1) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	return 0;
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
	query = cmyth_mysql_query_create(db, query_str);

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if(res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return 0;
	}

	ret = ref_alloc( sizeof( cmyth_channelgroup_t ) * (int)mysql_num_rows(res));

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
cmyth_mysql_get_channelids_in_group(cmyth_database_t db, unsigned long grpid, unsigned long **chanids)
{

	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT chanid FROM channelgroup WHERE grpid = ?";
	int rows = 0;
	unsigned long *ret;

	cmyth_mysql_query_t * query;
	query = cmyth_mysql_query_create(db, query_str);
	if (cmyth_mysql_query_param_ulong(query, grpid) < 0
		) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if(res == NULL)
	{
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return 0;
	}

	ret = ref_alloc(sizeof(unsigned long)* (int)mysql_num_rows(res));

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
	query = cmyth_mysql_query_create(db, query_str);

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if(res == NULL)
	{
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return 0;
	}

	ret = ref_alloc( sizeof( cmyth_recorder_source_t ) * (int)mysql_num_rows(res));

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
cmyth_mysql_get_prog_finder_time_title_chan(cmyth_database_t db, cmyth_program_t *prog, time_t starttime, char *program_name, unsigned long chanid)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT program.chanid,UNIX_TIMESTAMP(CONVERT_TZ(program.starttime,?,'SYSTEM')),UNIX_TIMESTAMP(CONVERT_TZ(program.endtime,?,'SYSTEM')),program.title,program.description,program.subtitle,program.programid,program.seriesid,program.category,channel.channum,channel.callsign,channel.name,channel.sourceid FROM program INNER JOIN channel ON program.chanid=channel.chanid WHERE program.chanid = ? AND program.title LIKE ? AND program.starttime = ? AND program.manualid = 0 ORDER BY (channel.channum + 0), program.starttime ASC ";
	int rows = 0;

	char* esctitle=cmyth_mysql_escape_chars(db, program_name);

	cmyth_mysql_query_t * query;
	query = cmyth_mysql_query_create(db, query_str);

	if (cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
		|| cmyth_mysql_query_param_str(query, db->db_tz_name) < 0
		|| cmyth_mysql_query_param_ulong(query, chanid) < 0
		|| cmyth_mysql_query_param_str(query, esctitle ) < 0
		|| cmyth_mysql_query_param_unixtime(query, starttime, db->db_tz_utc) < 0
		) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		ref_release(esctitle);
		return -1;
	}

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	ref_release(esctitle);

	if(res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return 0;
	}

	if ((row = mysql_fetch_row(res))) {
		rows++;
		if(prog) {
			prog->chanid = safe_atol(row[0]);
			prog->recording = 0;
			prog->starttime = (time_t)safe_atol(row[1]);
			prog->endtime = (time_t)safe_atol(row[2]);
			sizeof_strncpy(prog->title, row[3]);
			sizeof_strncpy(prog->description, row[4]);
			sizeof_strncpy(prog->subtitle, row[5]);
			sizeof_strncpy(prog->programid, row[6]);
			sizeof_strncpy(prog->seriesid, row[7]);
			sizeof_strncpy(prog->category, row[8]);
			prog->channum = safe_atol(row[9]);
			sizeof_strncpy(prog->callsign, row[10]);
			sizeof_strncpy(prog->name, row[11]);
			prog->sourceid = safe_atol(row[12]);
			prog->startoffset = 0;
			prog->endoffset = 0;
		}
	}

	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}

static void
destroy_char_array(void *p)
{
	char **ptr = (char**)p;
	if(!ptr)
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
	query = cmyth_mysql_query_create(db,query_str);
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL)
	{
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return 0;
	}

	ret = ref_alloc(sizeof( char*) * ((int) mysql_num_rows(res) +1 ));
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: alloc() failed for list\n",
			  __FUNCTION__);
		mysql_free_result(res);
		return 0;
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
	query = cmyth_mysql_query_create(db,query_str);

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return 0;
	}

	ret = ref_alloc( sizeof( char*) * ((int) mysql_num_rows(res) +1 ) );
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: alloc() failed for list\n",
			  __FUNCTION__);
		mysql_free_result(res);
		return 0;
	}

	ref_set_destroy(ret,destroy_char_array);

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
	int rows=0;
	cmyth_recprofile_t *ret = NULL;

	cmyth_mysql_query_t* query;
	query = cmyth_mysql_query_create(db,query_str);

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if(res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return 0;
	}

	ret = ref_alloc(sizeof( cmyth_recprofile_t ) * (int)mysql_num_rows(res));
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: alloc() failed for list\n",
			  __FUNCTION__);
		mysql_free_result(res);
		return 0;
	}

	while ((row = mysql_fetch_row(res))) {
		ret[rows].id=safe_atoi(row[0]);
		safe_strncpy(ret[rows].name, row[1], 128);
		safe_strncpy(ret[rows].cardtype, row[2], 32);
		rows++;
	}

	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: rows= %d\n", __FUNCTION__, rows);
	*profiles = ret;

	return rows;
}

char *
cmyth_mysql_get_cardtype(cmyth_database_t db, unsigned long chanid)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT cardtype FROM channel LEFT JOIN cardinput ON channel.sourceid=cardinput.sourceid LEFT JOIN capturecard ON cardinput.cardid=capturecard.cardid WHERE channel.chanid = ?";
	char* retval = "";

	cmyth_mysql_query_t * query;
	query = cmyth_mysql_query_create(db,query_str);

	if ( cmyth_mysql_query_param_ulong(query, chanid) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return NULL;
	}

	res = cmyth_mysql_query_result(query);
	ref_release(query);

	if(res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return NULL;
	}

	if ((row = mysql_fetch_row(res))) {
		retval = ref_strdup(row[0]);
	}

	mysql_free_result(res);
	return retval;
}

/*
 * cmyth_mysql_get_recording_markup(...)
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
long long
cmyth_mysql_get_recording_markup(cmyth_database_t db, cmyth_proginfo_t prog, cmyth_recording_markup_t type)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT data FROM recordedmarkup WHERE chanid = ? AND starttime = ? AND type = ? LIMIT 0, 1;";
	int rows = 0;
	long long data = 0;
	time_t start_ts_dt;
	cmyth_mysql_query_t *query;
	start_ts_dt = cmyth_timestamp_to_unixtime(prog->proginfo_rec_start_ts);
	query = cmyth_mysql_query_create(db, query_str);

	if (cmyth_mysql_query_param_ulong(query, prog->proginfo_chanId) < 0
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
 * cmyth_mysql_estimate_rec_framerate(...)
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
long long
cmyth_mysql_estimate_rec_framerate(cmyth_database_t db, cmyth_proginfo_t prog)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	/*
	 *  Force usage of primary key index to retrieve last mark value:
	 *  const DESC, const DESC, const DESC
	 */
	const char *query_str = "SELECT mark FROM recordedseek WHERE chanid = ? AND starttime = ? AND type = 9 ORDER BY chanid DESC, starttime DESC, type DESC, mark DESC LIMIT 1;";
	int rows = 0;
	long long mark = 0;
	long long dsecs = 0;
	long long fpms;
	time_t start_ts_dt;
	time_t end_ts_dt;
	cmyth_mysql_query_t * query;

	start_ts_dt = cmyth_timestamp_to_unixtime(prog->proginfo_rec_start_ts);
	end_ts_dt = cmyth_timestamp_to_unixtime(prog->proginfo_rec_end_ts);
	dsecs = (long long)(end_ts_dt - start_ts_dt);
	if (dsecs <= 0) {
		return 0;
	}

	query = cmyth_mysql_query_create(db,query_str);

	if (cmyth_mysql_query_param_ulong(query, prog->proginfo_chanId) < 0
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
	}
	else {
		return 0;
	}
	return fpms;
}

/*
 * cmyth_mysql_get_recording_framerate(...)
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Returns framerate for a recording.
 *
 * Success: returns 0 for invalid value else framerate (fps x 1000)
 */
long long
cmyth_mysql_get_recording_framerate(cmyth_database_t db, cmyth_proginfo_t prog)
{
	long long ret;
	ret = cmyth_mysql_get_recording_markup(db, prog, MARK_VIDEO_RATE);
	if (ret > 10000 && ret < 80000) {
		return ret;
	} else {
		cmyth_dbg(CMYTH_DBG_WARN, "%s, implausible frame rate: %lld\n", __FUNCTION__, ret);
	}

	ret = cmyth_mysql_estimate_rec_framerate(db, prog);
	if (ret > 10000 && ret < 80000) {
		return ret;
	} else {
		cmyth_dbg(CMYTH_DBG_WARN, "%s, failed to estimate frame rate: %lld\n", __FUNCTION__, ret);
	}

	return 0;
}

/*
 * cmyth_mysql_get_recording_artwork(...)
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
	const char *query_str = "SELECT coverart, fanart, banner FROM recordedartwork WHERE inetref = ? AND season = ? AND host = ?;";
	int rows = 0;
	cmyth_mysql_query_t * query;

	query = cmyth_mysql_query_create(db, query_str);

	if (cmyth_mysql_query_param_str(query, prog->proginfo_inetref) < 0
		|| cmyth_mysql_query_param_uint(query, prog->proginfo_season) < 0
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
