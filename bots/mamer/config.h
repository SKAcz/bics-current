//--------------------------------------------------------------------------
//  mamer - Tournament Director for the Free Internet Chess Server
//  
//  Copyright (C) 1995 Fred Baumgarten
//  Copyright (C) 1996-2001 Michael A. Long
//  Copyright (C) 1996-2001 Matthew E. Moses
//  Copyright (C) 2002 Richard Archer
//  
//  $Id: config.h,v 1.10 2002/08/08 02:53:35 rha Exp $
//  
//  mamer is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published 
//  by the Free Software Foundation; either version 2 of the License, 
//  or (at your option) any later version.
//  
//  mamer is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty
//  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//  See the GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with mamer; if not, write to the Free Software Foundation, 
//  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
//--------------------------------------------------------------------------
// config.h - default configuration parameters if the config file doesn't
//            exist, or the value isn't in the config file.  Can be
//            overridden during the build by using the DEFINES flags.
//
// Matthew E. Moses & Michael A. Long
//
// $Revision: 1.10 $
// $Date: 2002/08/08 02:53:35 $
//
// $Author: rha $
// $Locker:  $
//
// $Log: config.h,v $
// Revision 1.10  2002/08/08 02:53:35  rha
// Relicense code under the GPL.
//
// Revision 1.9  2002/08/08 01:45:15  rha
// Merge in changes made to mamer outside the RCS
// environment from March 1999 to January 2000.
//
// Revision 1.8  1999/01/02 21:47:49  mlong
// added bughouse support
//
// Revision 1.7  1998/10/26 17:27:31  mlong
// *** empty log message ***
//
// Revision 1.6  1998/09/16 18:26:06  mlong
// *** empty log message ***
//
// Revision 1.5  1998/09/10 19:58:20  mlong
// *** empty log message ***
//
// Revision 1.4  1998/06/18 18:42:09  mlong
// prepairing for yet another move.
//
// Revision 1.3  1998/04/18 19:00:07  mlong
// y
//
// Revision 1.2  1997/10/08 21:03:08  chess
// no log message
//
// Revision 1.1  1996/09/30 20:52:48  moses
// Initial revision
//
//
//--------------------------------------------------------------------------

#ifndef _CONFIG_
#define _CONFIG_

#ifndef VERSION
#define VERSION "Mamer 2.0.6"
#endif

#ifndef CONFIG_FILENAME
#define CONFIG_FILENAME "./mamer.config"
#endif

#ifndef DEFAULT_PATH
#define DEFAULT_PATH "/home/mlong/mamer"
#endif

#ifndef DEFAULT_HELP_FILE_PATH
#define DEFAULT_HELP_FILE_PATH "help"
#endif

#ifndef DEFAULT_DATA_PATH
#define DEFAULT_DATA_PATH "data"
#endif

#ifndef DEFAULT_LOG_FILE 
#define DEFAULT_LOG_FILE "logs/mamer"
#endif

#ifndef DEFAULT_USER_PATH
#define DEFAULT_USER_PATH "players"
#endif

#ifndef DEFAULT_COMMENTS_PATH
#define DEFAULT_COMMENTS_PATH "comments"
#endif

#ifndef DEFAULT_HOSTNAME
#define DEFAULT_HOSTNAME "freechess.org"
#endif
 
#ifndef DEFAULT_PORT
#define DEFAULT_PORT 5000
#endif

#ifndef DEFAULT_CHANNEL
#define DEFAULT_CHANNEL 49
#endif

#ifndef DEFAULT_USERNAME
#define DEFAULT_USERNAME "mamertesti"
#endif

#ifndef DEFAULT_PASSWORD
#define DEFAULT_PASSWORD "secret"
#endif

#ifndef ABUSE_INCREMENT_VALUE
#define ABUSE_INCREMENT_VALUE 1
#endif

#ifndef ABUSE_RESET_VALUE
#define ABUSE_RESET_VALUE 5
#endif

#ifndef FIRSTPLACEVALUE 
#define FIRSTPLACEVALUE 5
#endif

#ifndef MAX_ROUNDS
#define MAX_ROUNDS 10
#endif

#ifndef MAX_INCREMENT
#define MAX_INCREMENT 60
#endif

#ifndef MAX_TIME
#define MAX_TIME 9000
#endif

#ifndef DEFAULT_MAX_PLAYERS
#define DEFAULT_MAX_PLAYERS 16
#endif

#ifndef DEFAULT_TIME
#define DEFAULT_TIME 3
#endif

#ifndef DEFAULT_INCREMENT
#define DEFAULT_INCREMENT 0
#endif

#ifndef DEFAULT_ROUNDS
#define DEFAULT_ROUNDS 0
#endif

#ifndef PENALTY_PER_ROUND
#define PENALTY_PER_ROUND 10
#endif

#ifndef KEEP_TOURNEY_TIME
#define KEEP_TOURNEY_TIME 7200
#endif

#ifndef SEC_BETWEEN_CSHOUTS
#define SEC_BETWEEN_CSHOUTS 240
#endif

#ifndef MAX_ADD_CHAOS_POINTS
#define MAX_ADD_CHAOS_POINTS 30
#endif

#ifndef MAX_CHAOS_POINTS
#define MAX_CHAOS_POINTS 100
#endif

// MINIMUM_ROUNDS should always be less than MINIMUM_PLAYERS

#ifndef MINIMUM_ROUNDS
#define MINIMUM_ROUNDS 3
#endif

#ifndef MINIMUM_PLAYERS
#define MINIMUM_PLAYERS 4
#endif

#endif



