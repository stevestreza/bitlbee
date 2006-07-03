  /********************************************************************\
  * BitlBee -- An IRC to other IM-networks gateway                     *
  *                                                                    *
  * Copyright 2002-2004 Wilmer van der Gaast and others                *
  \********************************************************************/

/* Storage backend that uses the same file format as <=1.0 */

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License with
  the Debian GNU/Linux distribution in /usr/share/common-licenses/GPL;
  if not, write to the Free Software Foundation, Inc., 59 Temple Place,
  Suite 330, Boston, MA  02111-1307  USA
*/

#define BITLBEE_CORE
#include "bitlbee.h"
#include "crypting.h"

/* DO NOT USE THIS FUNCTION IN NEW CODE. This 
 * function is here merely because the save/load code still uses 
 * ids rather than names */
static struct prpl *find_protocol_by_id(int id)
{
	switch (id) {
	case 0: case 1: case 3: return find_protocol("oscar");
	case 4: return find_protocol("msn");
	case 2: return find_protocol("yahoo");
	case 8: return find_protocol("jabber");
	default: break;
	}
	return NULL;
}

static int find_protocol_id(const char *name)
{
	if (!strcmp(name, "oscar")) return 1;
	if (!strcmp(name, "msn")) return 4;
	if (!strcmp(name, "yahoo")) return 2;
	if (!strcmp(name, "jabber")) return 8;

	return -1;
}


static void text_init (void)
{
	if( access( global.conf->configdir, F_OK ) != 0 )
		log_message( LOGLVL_WARNING, "The configuration directory %s does not exist. Configuration won't be saved.", CONFIG );
	else if( access( global.conf->configdir, R_OK ) != 0 || access( global.conf->configdir, W_OK ) != 0 )
		log_message( LOGLVL_WARNING, "Permission problem: Can't read/write from/to %s.", global.conf->configdir );
}

static storage_status_t text_load ( const char *my_nick, const char* password, irc_t *irc )
{
	char s[512];
	char *line;
	int proto;
	char nick[MAX_NICK_LENGTH+1];
	FILE *fp;
	user_t *ru = user_find( irc, ROOT_NICK );
	
	if( irc->status & USTATUS_IDENTIFIED )
		return( 1 );
	
	g_snprintf( s, 511, "%s%s%s", global.conf->configdir, my_nick, ".accounts" );
   	fp = fopen( s, "r" );
   	if( !fp ) return STORAGE_NO_SUCH_USER;
	
	fscanf( fp, "%32[^\n]s", s );

	if (checkpass (password, s) != 0) 
	{
		fclose( fp );
		return STORAGE_INVALID_PASSWORD;
	}
	
	/* Do this now. If the user runs with AuthMode = Registered, the
	   account command will not work otherwise. */
	irc->status |= USTATUS_IDENTIFIED;
	
	while( fscanf( fp, "%511[^\n]s", s ) > 0 )
	{
		fgetc( fp );
		line = deobfucrypt( s, password );
		if (line == NULL) return STORAGE_OTHER_ERROR;
		root_command_string( irc, ru, line, 0 );
		g_free( line );
	}
	fclose( fp );
	
	g_snprintf( s, 511, "%s%s%s", global.conf->configdir, my_nick, ".nicks" );
	fp = fopen( s, "r" );
	if( !fp ) return STORAGE_NO_SUCH_USER;
	while( fscanf( fp, "%s %d %s", s, &proto, nick ) > 0 )
	{
		struct prpl *prpl;

		prpl = find_protocol_by_id(proto);

		if (!prpl)
			continue;

		http_decode( s );
		// FIXME!!!! nick_set( irc, s, prpl, nick );
	}
	fclose( fp );
	
	return STORAGE_OK;
}

static storage_status_t text_check_pass( const char *nick, const char *password )
{
	char s[512];
	FILE *fp;
	
	g_snprintf( s, 511, "%s%s%s", global.conf->configdir, nick, ".accounts" );
	fp = fopen( s, "r" );
	if (!fp)
		return STORAGE_NO_SUCH_USER;

	fscanf( fp, "%32[^\n]s", s );
	fclose( fp );

	if (checkpass( password, s) == -1)
		return STORAGE_INVALID_PASSWORD;

	return STORAGE_OK;
}

static storage_status_t text_remove( const char *nick, const char *password )
{
	char s[512];
	storage_status_t status;

	status = text_check_pass( nick, password );
	if (status != STORAGE_OK)
		return status;

	g_snprintf( s, 511, "%s%s%s", global.conf->configdir, nick, ".accounts" );
	if (unlink( s ) == -1)
		return STORAGE_OTHER_ERROR;
	
	g_snprintf( s, 511, "%s%s%s", global.conf->configdir, nick, ".nicks" );
	if (unlink( s ) == -1)
		return STORAGE_OTHER_ERROR;

	return STORAGE_OK;
}

storage_t storage_text = {
	.name = "text",
	.init = text_init,
	.check_pass = text_check_pass,
	.remove = text_remove,
	.load = text_load
};
