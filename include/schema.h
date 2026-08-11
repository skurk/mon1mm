#ifndef DB_SCHEMA_H
#define DB_SCHEMA_H

#include <mysql/mysql.h>

char		db_row_id[50];
char		db_row_app[50];
char		db_row_contestname[50];
int			db_row_contestnr;
MYSQL_TIME	db_row_timestamp;
char		db_row_mycall[50];
char		db_row_band[20];
char		db_row_rxfreq[32];
char		db_row_txfreq[32];
char		db_row_operator[50];
char		db_row_mode[20];
char		db_row_callsign[30];
char		db_row_countryprefix[20];
char		db_row_wpxprefix[20];
char		db_row_stationprefix[50];
char		db_row_continent[10];
char		db_row_snt[20];
int			db_row_sntnr;
char		db_row_rcv[20];
int			db_row_rcvnr;
char		db_row_gridsquare[20];
char		db_row_exchange1[50];
char		db_row_section[20];
char		db_row_comment[255];
char		db_row_qth[100];
char		db_row_name[100];
char		db_row_power[20];
char		db_row_misctext[255];
int			db_row_zone;
char		db_row_prec[10];
int			db_row_ck;
int			db_row_ismultiplier1;
int			db_row_ismultiplier2;
int			db_row_ismultiplier3;
int			db_row_points;
int			db_row_radionr;
int			db_row_run1run2;
char		db_row_roverlocation[50];
int			db_row_radiointerfaced;
int			db_row_networkedcompnr;
char		db_row_isoriginal[20];
char		db_row_netbiosname[100];
int			db_row_isrunqso;
int			db_row_isclaimedqso;
char		db_row_stationname[100];
MYSQL_TIME	db_row_oldtimestamp;
char		db_row_oldcall[30];
int			db_row_syncedtolotw;
MYSQL_TIME	db_row_updated_at;

struct DbEntry {
    enum enum_field_types buffer_type; 
	void *buffer;
	size_t buffer_length;
};

struct DbEntry DbColumns[] = {
	{
		MYSQL_TYPE_STRING,
		db_row_id,
		50
	},
	{
		MYSQL_TYPE_STRING,
		db_row_app,
		100,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_contestname,
		100,
	},
	{
		MYSQL_TYPE_INT24,
		&db_row_contestnr,
		1,
	},
	{
		MYSQL_TYPE_DATETIME,
		&db_row_timestamp,
		1,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_mycall,
		50,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_band,
		20,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_rxfreq,
		32,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_txfreq,
		32,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_operator,
		50,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_mode,
		20,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_callsign,
		30,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_countryprefix,
		20,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_wpxprefix,
		20,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_stationprefix,
		50,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_continent,
		10,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_snt,
		20,
	},
	{
		MYSQL_TYPE_INT24,
		&db_row_sntnr,
		1,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_rcv,
		20,
	},
	{
		MYSQL_TYPE_INT24,
		&db_row_rcvnr,
		1,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_gridsquare,
		20,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_exchange1,
		50,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_section,
		20,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_comment,
		255,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_qth,
		100,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_name,
		100,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_power,
		20,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_misctext,
		255,
	},
	{
		MYSQL_TYPE_INT24,
		&db_row_zone,
		255,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_prec,
		10,
	},
	{
		MYSQL_TYPE_INT24,
		&db_row_ck,
		1,
	},
	{
		MYSQL_TYPE_INT24,
		&db_row_ismultiplier1,
		1,
	},
	{
		MYSQL_TYPE_INT24,
		&db_row_ismultiplier2,
		1,
	},
	{
		MYSQL_TYPE_INT24,
		&db_row_ismultiplier3,
		1,
	},
	{
		MYSQL_TYPE_INT24,
		&db_row_points,
		1,
	},
	{
		MYSQL_TYPE_INT24,
		&db_row_radionr,
		1,
	},
	{
		MYSQL_TYPE_INT24,
		&db_row_run1run2,
		1,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_roverlocation,
		50,
	},
	{
		MYSQL_TYPE_INT24,
		&db_row_radiointerfaced,
		1,
	},
	{
		MYSQL_TYPE_INT24,
		&db_row_networkedcompnr,
		1,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_isoriginal,
		20,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_netbiosname,
		100,
	},
	{
		MYSQL_TYPE_INT24,
		&db_row_isrunqso,
		1,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_stationname,
		100,
	},
	{
		MYSQL_TYPE_INT24,
		&db_row_isclaimedqso,
		1,
	},
	{
		MYSQL_TYPE_DATETIME,
		&db_row_oldtimestamp,
		1,
	},
	{
		MYSQL_TYPE_STRING,
		db_row_oldcall,
		30,
	},
	{
		MYSQL_TYPE_TINY,
		&db_row_syncedtolotw,
		1,
	},
	{
		MYSQL_TYPE_DATETIME,
		&db_row_updated_at,
		1,
	},
};

#endif

