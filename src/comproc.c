/*
   Copyright (c) 1993 Richard V. Nash.
   Copyright (c) 2000 Dan Papasian
   Copyright (C) Andrew Tridgell 2002

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include "includes.h"


const int none = 0;
const int blitz_rat = 1;
const int std_rat = 2;
const int wild_rat = 3;
const int light_rat = 4;
const int bug_rat = 5;

int com_more(int p, param_list param)
{
	pmore_text(p);
	return COM_OK;
}

int com_quit(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];

	/* Examined games are killed on logout */
	if ((pp->game >= 0) &&
	    ((game_globals.garray[pp->game].status != GAME_EXAMINE) &&
	     (game_globals.garray[pp->game].status != GAME_SETUP))) {
		pprintf(p, "You can't quit while you are playing a game.\nType 'resign' to resign the game, or you can request an abort with 'abort'.\n");
		return COM_OK;
	}

	psend_logoutfile(p, MESS_DIR, MESS_LOGOUT);
	return COM_LOGOUT;
}

int com_set(int p, param_list param)
{
  int result;
  int which;
  char *val;

  if (param[1].type == TYPE_NULL)
    val = NULL;
  else
    val = param[1].val.string;
  result = var_set(p, param[0].val.word, val, &which);
  switch (result) {
  case VAR_OK:
    break;
  case VAR_BADVAL:
    pprintf(p, "Bad value given for variable %s.\n", param[0].val.word);
    break;
  case VAR_NOSUCH:
    pprintf(p, "No such variable name %s.\n", param[0].val.word);
    break;
  case VAR_AMBIGUOUS:
    pprintf(p, "Ambiguous variable name %s.\n", param[0].val.word);
    break;
  }
  /* player_save(p); */
  return COM_OK;
}

int FindPlayer(int p, char* name, int *p1, int *connected)
{
  *p1 = player_search(p, name);
  if (*p1 == 0)
    return 0;
  if (*p1 < 0) {		/* player had to be connected and will be
			   removed later */
    *connected = 0;
    *p1 = (-*p1) - 1;
  } else {
    *connected = 1;
    *p1 = *p1 - 1;
  }
  return 1;
}

static void com_stats_andify(int *numbers, int howmany, char *dest)
{
  char tmp[10];

  *dest = '\0';
  while (howmany--) {
    sprintf(tmp, "%d", numbers[howmany]);
    strcat(dest, tmp);
    if (howmany > 1)
      sprintf(tmp, ", ");
    else if (howmany == 1)
      sprintf(tmp, " and ");
    else
      sprintf(tmp, ".\n");
    strcat(dest, tmp);
  }
  return;
}

static void com_stats_rating(char *hdr, struct statistics * stats, char *dest, long now)
{
  char tmp[1000];

  *dest = 0;

  if (stats->num == 0)
    return;

  sprintf(dest, "%-10s%4s    %5.1f   %4d   %4d   %4d   %4d", hdr,
	  ratstr(stats->rating), current_sterr(stats->sterr, now-stats->ltime),
          stats->win, stats->los, stats->dra, stats->num);
  if (stats->whenbest) {
    sprintf(tmp, "   %d", stats->best);
    strcat(dest, tmp);
//    strftime(tmp, sizeof(tmp), " (%d-%b-%Y)", localtime((time_t *) & stats->whenbest));
//    strcat(dest, tmp);
  }
  strcat(dest, "\n");
  return;
}

int com_finger(int p, param_list param)
{
    if (param[1].type == TYPE_NULL)
        return com_stats(p, param);
    else
        return com_teamfinger(p, param);
}

int com_teamfinger(int p, param_list param)
{
    if (param[0].type != TYPE_WORD || param[1].type != TYPE_WORD) {
        pprintf(p, "We need two usernames to lookup.\n");
        return COM_OK;
    }

    struct bugteam team = {param[0].val.word, param[1].val.word, NULL};
    team.stats = bugteamstats_getStats(&team);

    if (!team.stats)
        pprintf(p, "No statistics for this team.\n");
    else {
        char tmp[255];
        int now = time(NULL);

        pprintf(p, "Finger of %s and %s:\n", team.partnerone, team.partnertwo);
        pprintf(p, "\n         rating     RD     win   loss   draw  total  \n");

        com_stats_rating("Bug Team", team.stats, tmp, now);
        if (*tmp != '\0') pprintf(p, tmp);
    }

    return COM_OK;
}

int com_stats(int p, param_list param)
{
  // p = requesting user; p1 = user being queried

  int g, i;
  time_t t;
  int p1, connected;
  char line[255], tmp[255];//, tmp2[255];
  int numbers[MAX_OBSERVE];
  int onTime;
  int showRatingsFlag, showNotesFlag;
  int showCommentsFlag = 0;
  long now;

  showRatingsFlag = showNotesFlag = 1; /* everything on by default */

  if (param[0].type == TYPE_WORD) {
    if (!FindPlayer(p, param[0].val.word, &p1, &connected))
      return COM_OK;
    if (param[1].type == TYPE_WORD) { /* optional second parameter */
      char *c = param[1].val.word;
       showRatingsFlag = showNotesFlag = 0;
      while (*c != '\0') {
        if (*c == 'r') { showRatingsFlag = 1; c++; }
        else if (*c == 'n') { showNotesFlag = 1; c++; }
	else if ((*c == 'c') && check_admin(p, ADMIN_ADMIN)) {
	  showCommentsFlag = 1; c++; }
        else {
	  if (!connected)
	    player_remove(p1);
	  return COM_BADPARAMETERS;
	}
      }
    }
  } else {
      p1 = p;
      connected = 1;
  }
  char titles[100];
  char titlesName[255];
  titles [0]='\0';
  AddPlayerLists(p1,titles);
  sprintf (titlesName,"%s%s", player_globals.parray[p1].name, titles);

  sprintf(line, "\nFinger of %s\n", titlesName);
  if ((connected) && (player_globals.parray[p1].status == PLAYER_PROMPT)) {
    sprintf(tmp, "On for: %s", hms_desc(player_ontime(p1)));
    strcat(line, tmp);
    sprintf(tmp, "   Idle: %s\n", hms_desc(player_idle(p1)));
  } else {
    if ((t = player_lastdisconnect(p1)))
      sprintf(tmp, "Last disconnected: %s\n", strltime(&t));
    else
      sprintf(tmp, "Never connected.\n");
  }
  strcat(line, tmp);
  pprintf(p, "%s", line);
  if (player_globals.parray[p1].game >= 0) {
    g = player_globals.parray[p1].game;
    if (game_globals.garray[g].status == GAME_EXAMINE) {
      pprintf(p, "(Examining game %d: %s vs. %s)\n", g + 1,
            game_globals.garray[g].white_name, game_globals.garray[g].black_name);
    } else if (game_globals.garray[g].status == GAME_SETUP) {
      pprintf(p, "(Setting up game %d: %s vs. %s)\n", g + 1,
            game_globals.garray[g].white_name, game_globals.garray[g].black_name);
    } else {
      pprintf(p, "(playing game %d: %s vs. %s)\n", g + 1,
	    player_globals.parray[game_globals.garray[g].white].name, player_globals.parray[game_globals.garray[g].black].name);
      if (game_globals.garray[g].link >= 0) {
	pprintf(p, "(partner is playing game %d: %s vs. %s)\n", game_globals.garray[g].link + 1,
	    player_globals.parray[game_globals.garray[game_globals.garray[g].link].white].name,
	    player_globals.parray[game_globals.garray[game_globals.garray[g].link].black].name);
      }
    }
  }
  if (player_globals.parray[p1].num_observe) {
    for (i = 0, t = 0; i < player_globals.parray[p1].num_observe; i++) {
      g = player_globals.parray[p1].observe_list[i];
      if ((g != -1) && (check_admin(p, ADMIN_ADMIN)))
		  numbers[t++] = g + 1;
    }
    if (t) {
      pprintf(p, "%s is observing game%s ", player_globals.parray[p1].name, ((t > 1) ? "s" : ""));
      com_stats_andify(numbers, t, tmp);
      pprintf(p, tmp);
    }
  }
  if (player_globals.parray[p1].busy != NULL) {
    pprintf(p, "(%s %s)\n", player_globals.parray[p1].name, player_globals.parray[p1].busy);
  }
  if (!CheckPFlag(p1, PFLAG_REG)) {
    pprintf(p, "%s is NOT a registered player.\n\n", player_globals.parray[p1].name);
  } else {
    if (showRatingsFlag > 0) {
      now = time(NULL);
      pprintf(p, "\n         rating     RD     win   loss   draw  total   \n"); //remove best string, because best rating not shown now

      com_stats_rating("Crazyhouse", &player_globals.parray[p1].z_stats, tmp, now);
      if (*tmp) pprintf(p, tmp);

      com_stats_rating("Standard", &player_globals.parray[p1].s_stats, tmp, now);
      if (*tmp) pprintf(p, tmp);

      com_stats_rating("SimulBug", &player_globals.parray[p1].simul_stats, tmp, now);
      if (*tmp) pprintf(p, tmp);

      com_stats_rating("FRBug", &player_globals.parray[p1].fr_bug_stats, tmp, now);
      if (*tmp) pprintf(p, tmp);

      com_stats_rating("Bughouse", &player_globals.parray[p1].bug_stats, tmp, now);
      if (*tmp) pprintf(p, tmp);

      com_stats_rating("Tourney", &player_globals.parray[p1].tourney_stats, tmp, now);
      if (*tmp) pprintf(p, tmp);

      struct statistics stats;
      struct bugteam team = {NULL, NULL, &stats};
      bugteamstats_bestOfPlayer(player_globals.parray[p1].name, &team);
      if (team.partnerone && team.partnertwo) {
        pprintf(p, "\nBest team: %s and %s. ", team.partnerone, team.partnertwo);
        pprintf(p, "\"finger %s %s\":\n", team.partnerone, team.partnertwo);
        com_stats_rating("Bug Team", team.stats, tmp, now);
        if (*tmp) pprintf(p, tmp);
      }
    }
  }
  pprintf(p, "\n");
  if (player_globals.parray[p1].adminLevel > 0) {
    pprintf(p, "Admin Level: ");
    if (player_ishead(p1)) { pprintf(p,"Head Administrator\n"); } else
    {
    switch (5*(player_globals.parray[p1].adminLevel/5)) {
    case 5:
      pprintf(p, "Authorized Helper Person\n");
      break;
    case 10:
      pprintf(p, "Junior Administrator\n");
      break;
    case 15:
      pprintf(p, "Service Representative/Administrator\n");
      break;
    case 20:
      pprintf(p, "Administrator\n");
      break;
    case 50:
      pprintf(p, "Senior Administrator\n");
      break;
    case 60:
      pprintf(p, "Super Administrator\n");
      break;
    case 100:
      pprintf(p, "Assistant Head Administrator\n");
      break;
    default:
      pprintf(p, "%d\n", player_globals.parray[p1].adminLevel);
      break;
    }
    }
  }

  if (check_admin(p, 1) && !CheckPFlag(p, PFLAG_HIDEINFO)) {
    pprintf(p, "Full Name  : %s\n", (player_globals.parray[p1].fullName ? player_globals.parray[p1].fullName : "(none)"));
    pprintf(p, "Address    : %s\n", (player_globals.parray[p1].emailAddress ? player_globals.parray[p1].emailAddress : "(none)"));
    pprintf(p, "Host       : %s\n",
	    dotQuad(connected ? player_globals.parray[p1].thisHost : player_globals.parray[p1].lastHost));
    if (CheckPFlag(p1, PFLAG_REG))
      if (player_globals.parray[p1].num_comments)
	pprintf(p, "Comments   : %d\n", player_globals.parray[p1].num_comments);
  } else if ((p1 == p) && CheckPFlag(p1, PFLAG_REG))
    pprintf(p, "Address    : %s\n", (player_globals.parray[p1].emailAddress ? player_globals.parray[p1].emailAddress : "(none)"));

  if (player_globals.parray[p1].socket != -1) {
    pprintf(p, "\nTimeseal: %s\n", net_globals.con[player_globals.parray[p1].socket]->timeseal ? "Yes" : "No");
  }

  if (connected && CheckPFlag(p1, PFLAG_REG)
      && (p==p1 || (check_admin(p, 1) && !CheckPFlag(p, PFLAG_HIDEINFO)))) {
    char *timeToStr = ctime((time_t *) &player_globals.parray[p1].timeOfReg);

    timeToStr[strlen(timeToStr)-1]='\0';
    pprintf(p, "\n");
    onTime = ((int)time(0) - player_globals.parray[p1].logon_time) + player_globals.parray[p1].totalTime;

    pprintf(p, "Total time on-line: %s\n", hms_desc(onTime) );
    pprintf(p, "%% of life on-line:  %3.1f  (since %s)\n",
           (double)((onTime*100)/(double)(time(0)-player_globals.parray[p1].timeOfReg)),
           timeToStr);
  }

  int isAdmin = player_globals.parray[p].adminLevel;
  if (in_list(p1, L_NOTEBAN, player_globals.parray[p1].name))
    {
    	showNotesFlag = 0; // Do NOT show notes.
    	if (p == p1) { pprintf(p,"\nYou are notebanned, and others cannot see your finger notes.\n"); showNotesFlag = 1; } else
    	if (isAdmin) { showNotesFlag = 1; pprintf(p,"\n%s is notebanned.\n",player_globals.parray[p1].name); } // unless its to an administrator
    }

  if (player_globals.parray[p1].num_plan && (showNotesFlag > 0)) {
    pprintf(p, "\n");
    for (i = 0; i < player_globals.parray[p1].num_plan; i++)
      pprintf(p, "%2d: %s\n", i + 1, (player_globals.parray[p1].planLines[i] != NULL) ? player_globals.parray[p1].planLines[i] : "");
  }
  if (showCommentsFlag) {
    pprintf(p, "\n");
    pprintf(p, "Comments for %s:\n",player_globals.parray[p1].name);
    player_show_comments(p, p1);
  }
  if (!connected)
    player_remove(p1);
  return COM_OK;
}

int com_password(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];
	char *oldpassword = param[0].val.word;
	char *newpassword = param[1].val.word;
	char salt[3];

	if (!CheckPFlag(p, PFLAG_REG)) {
		pprintf(p, "Setting a password is only for registered players.\n");
		return COM_OK;
	}
	if (pp->passwd) {
		salt[0] = pp->passwd[3];
		salt[1] = pp->passwd[4];
		salt[2] = '\0';
		if (strcmp(chessd_crypt(oldpassword,salt), pp->passwd)) {
			pprintf(p, "Incorrect password, password not changed!\n");
			return COM_OK;
		}
		free(pp->passwd);
		pp->passwd = NULL;
	}
	salt[0] = 'a' + random() % 26;
	salt[1] = 'a' + random() % 26;
	salt[2] = '\0';
	pp->passwd = strdup(chessd_crypt(newpassword,salt));
	pprintf(p, "Password changed to \"%s\".\n", newpassword);
	return COM_OK;
}

int com_uptime(int p, param_list param)
{
  time_t uptime = time(0) - command_globals.startuptime;
  int days  = (uptime / (60*60*24));
  int hours = ((uptime % (60*60*24)) / (60*60));
  int mins  = (((uptime % (60*60*24)) % (60*60)) / 60);
  int secs  = (((uptime % (60*60*24)) % (60*60)) % 60);

  pprintf(p, "Server location: %s   Server version : %s\n",
	  config_get_tmp("SERVER_HOSTNAME"), VERS_NUM);
  pprintf(p, "The server has been up since %s.\n", strltime(&command_globals.startuptime));

  pprintf(p,"Up for");

  if(days)
    pprintf(p," %d day%s", days, (days == 1) ? "" : "s");

  if(hours)
    pprintf(p," %d hour%s", hours, (hours == 1) ? "" : "s");

  if(mins)
    pprintf(p," %d minute%s", mins, (mins == 1) ? "" : "s");

  if(secs)
    pprintf(p," %d second%s", secs, (secs == 1) ? "" : "s");

  pprintf(p,".\n");

  pprintf(p, "\nPlayer limit: %d\n", config_get_int("MAX_PLAYER", DEFAULT_MAX_PLAYER));
  pprintf(p, "\nThere are currently %d players, with a high of %d since last restart.\n", player_count(1), command_globals.player_high);
  pprintf(p, "There are currently %d games, with a high of %d since last restart.\n", game_count(), command_globals.game_high);
  pprintf(p, "\nCompiled on %s\n", COMP_DATE);
  return COM_OK;
}

int com_date(int p, param_list param)
{
	time_t t = time(0);
	pprintf(p, "Local time     - %s\n", strltime(&t));
	pprintf(p, "Greenwich time - %s\n", strgtime(&t));
	return COM_OK;
}
static const char *inout_string[] = {
  "login", "logout"
};

static int plogins(int p, char *fname)
{
	FILE *fp;
	int inout, registered;
	time_t thetime;
	char loginName[MAX_LOGIN_NAME + 1];
	char ipstr[20];

	fp = fopen_p("%s", "r", fname);
	if (!fp) {
		pprintf(p, "Sorry, no login information available.\n");
		return COM_OK;
	}
	while (!feof(fp)) {
		unsigned t;
		if (fscanf(fp, "%d %s %u %d %s\n", &inout, loginName, &t,
			   &registered, ipstr) != 5) {
			d_printf( "CHESSD: Error in login info format. %s\n",
				fname);
			fclose(fp);
			return COM_OK;
		}
		thetime = (time_t)t;
		pprintf(p, "%s: %-17s %-6s", strltime(&thetime), loginName,
			inout_string[inout]);
		if (check_admin(p, 1) && !CheckPFlag(p, PFLAG_HIDEINFO)) {
			pprintf(p, " from %s\n", ipstr);
		} else
			pprintf(p, "\n");
	}
	fclose(fp);
	return COM_OK;
}

int com_llogons(int p, param_list param)
{
	int result;
	if (!CheckPFlag(p, PFLAG_REG)) {
		pprintf(p,"Sorry, guest users may not use this command\n");
		return COM_OK;
	}
	result = plogins(p, STATS_DIR "/" STATS_LOGONS);
	return result;
}

int com_logons(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];
	char fname[MAX_FILENAME_SIZE];

	if (param[0].type == TYPE_WORD) {
		int p1, connected;
		if (!FindPlayer(p, param[0].val.word, &p1, &connected))
			return COM_OK;
		sprintf(fname, "%s/player_data/%c/%s.%s",
			STATS_DIR,
			player_globals.parray[p1].login[0],
			player_globals.parray[p1].login, STATS_LOGONS);
		if (!connected)
			player_remove(p1);
	} else {
		sprintf(fname, "%s/player_data/%c/%s.%s",
			STATS_DIR,
			pp->login[0],
			pp->login,
			STATS_LOGONS);
	}
	return plogins(p, fname);
}

#define WHO_OPEN 0x01
#define WHO_CLOSED 0x02
#define WHO_RATED 0x04
#define WHO_UNRATED 0x08
#define WHO_FREE 0x10
#define WHO_PLAYING 0x20
#define WHO_REGISTERED 0x40
#define WHO_UNREGISTERED 0x80
#define WHO_BUGTEAM 0x100

#define WHO_ALL 0xff

void AddPlayerLists (int p1, char *ptmp)
{
	if (check_admin(p1, ADMIN_ADMIN) && CheckPFlag(p1, PFLAG_ADMINLIGHT))
		strcat(ptmp, "(*)");

	if (!CheckPFlag(p1, PFLAG_REG)) // if the user is unregistered, add (U) to handle
			strcat(ptmp, "(U)");

    if (in_list(p1, L_SR, player_globals.parray[p1].name))
		strcat(ptmp, "(SR)"); else // this else statement disallows (SR)(SR) by +sr AND lvl 15 admin.
    if (check_admin(p1, ADMIN_ADMIN) && ((5*(player_globals.parray[p1].adminLevel/5))==15)   )
		strcat(ptmp, "(SR)");

    if (in_list(p1, L_COMPUTER, player_globals.parray[p1].name))
		strcat(ptmp, "(C)");
	if (in_list(p1, L_FM, player_globals.parray[p1].name))
		strcat(ptmp, "(FM)");
	if (in_list(p1, L_IM, player_globals.parray[p1].name))
		strcat(ptmp, "(IM)");
	if (in_list(p1, L_GM, player_globals.parray[p1].name))
		strcat(ptmp, "(GM)");
        if (in_list(p1, L_TM, player_globals.parray[p1].name))
		strcat(ptmp, "(TM)");
        if (in_list(p1, L_WFM, player_globals.parray[p1].name))
                strcat(ptmp, "(WFM)");
        if (in_list(p1, L_WIM, player_globals.parray[p1].name))
                strcat(ptmp, "(WIM)");
        if (in_list(p1, L_WGM, player_globals.parray[p1].name))
		strcat(ptmp, "(WGM)");
	if (in_list(p1, L_TD, player_globals.parray[p1].name))
		strcat(ptmp, "(TD)");
	if (in_list(p1, L_TEAMS, player_globals.parray[p1].name))
		strcat(ptmp, "(T)");
	if (in_list(p1, L_BLIND, player_globals.parray[p1].name))
		strcat(ptmp, "(B)");
}


/* display partnership : aramen */
int com_who_bug(int p,param_list param)
{

  char ptmp[80 + 20];		/* for highlight */
  int rat = 0;
  int i, j;
  int wp, bp;

  int totalcount=0;

  char test[4];
  if (param[0].val.word != NULL && strlen(param[0].val.word) <= 3)
      { strcpy(test,param[0].val.word); }
  else
      { param[0].val.word[0] = '\0'; }

  if (param[0].val.word[0] == '\0') { strcpy(test,"ugp"); }
  // -------------

  if (strchr(test,'g'))
  {
  ptmp[0] = '\0';
  pprintf(p,"Bughouse games in progress:\n\n");
  totalcount=0;

    for (i = 0; i < game_globals.g_num; i++) {
 	  if ((game_globals.garray[i].status == GAME_ACTIVE)&&(game_globals.garray[i].type == TYPE_BUGHOUSE) && (game_globals.garray[i].link > i))
	  {
				 wp = game_globals.garray[i].white;
                 bp = game_globals.garray[i].black;
					pprintf_noformat(p, "%3d %4s %-11.11s %4s %-10.10s [ %c%c%3d %3d] ",
                                                   i + 1,
                                                 ratstrii(GetRating(&player_globals.parray[wp],game_globals.garray[i].type), wp),
                                                 player_globals.parray[wp].name,
                                                  ratstrii(GetRating(&player_globals.parray[bp],game_globals.garray[i].type), bp),
                                                   player_globals.parray[bp].name,
                                                 *bstr[game_globals.garray[i].type],
                                                 *rstr[game_globals.garray[i].rated],
                                               game_globals.garray[i].wInitTime / 60000,
                                                   game_globals.garray[i].wIncrement / 1000);
                             pprintf_noformat(p, "%6s -", tenth_str(game_globals.garray[i].wRealTime , 0));
                             pprintf_noformat(p, "%6s\n",
                                                  tenth_str(game_globals.garray[i].bRealTime , 0)
                                                                                           );

                             j = game_globals.garray[i].link;
                             wp = game_globals.garray[j].white;
                             bp = game_globals.garray[j].black;

                             pprintf_noformat(p, "%3d %4s %-11.11s %4s %-10.10s [ %c%c%3d %3d] ",
                                                   j + 1,
                                                   ratstrii(GetRating(&player_globals.parray[wp],game_globals.garray[j].type), wp),
                                                   player_globals.parray[wp].name,
                                                   ratstrii(GetRating(&player_globals.parray[bp],game_globals.garray[j].type), bp),
                                                   player_globals.parray[bp].name,
                                                   *bstr[game_globals.garray[j].type],
                                                   *rstr[game_globals.garray[j].rated],
                                                   game_globals.garray[j].wInitTime / 60000,
                                                  game_globals.garray[j].wIncrement / 1000);
                              pprintf_noformat(p, "%6s -", tenth_str(game_globals.garray[j].wRealTime , 0));
                              pprintf_noformat(p, "%6s\n",
                                                  tenth_str(game_globals.garray[j].bRealTime , 0)
                                                  );
                                                totalcount++;
          }
         }

 pprintf(p, " %3d game%s displayed.\n\n", totalcount, (totalcount == 1) ? "" : "s");
 }
 totalcount=0;

 if (strchr(test,'p'))
 {
 ptmp[0] = '\0';
 pprintf(p,"Partnerships not playing bughouse:\n\n");

  for (i = 0; i < player_globals.p_num; i++) {
	struct player *pp = &player_globals.parray[i];
	if ((player_globals.parray[i].game >= 0)&&
		(game_globals.garray[player_globals.parray[i].game].type == TYPE_BUGHOUSE)) continue;

	if (pp->partner > i)
	{
	int p2=pp->partner;
	rat = player_globals.parray[i].bug_stats.rating;
     sprintf(ptmp, "%-4s", ratstrii(rat, i));

     // show status
     if ((player_globals.parray[i].game >= 0) &&
		 ((game_globals.garray[player_globals.parray[i].game].status == GAME_EXAMINE) ||
		 (game_globals.garray[player_globals.parray[i].game].status == GAME_SETUP)))
			strcat(ptmp, "#");
      else if (player_globals.parray[i].game >= 0)
	strcat(ptmp, "^");
      else if (!CheckPFlag(i, PFLAG_OPEN))
	strcat(ptmp, ":");
      else if (CheckPFlag(i,PFLAG_TOURNEY))
    strcat(ptmp, "&");
      else if (player_idle(i) > 300)
	strcat(ptmp, ".");
      else
	strcat(ptmp, " ");
	strcat(ptmp, player_globals.parray[i].name);

	strcat(ptmp, " / "); // " + "
	rat = player_globals.parray[p2].bug_stats.rating;
     strcat(ptmp,  ratstrii(rat, p2));
     if ((player_globals.parray[p2].game >= 0) &&
		 ((game_globals.garray[player_globals.parray[p2].game].status == GAME_EXAMINE) ||
		 (game_globals.garray[player_globals.parray[p2].game].status == GAME_SETUP)))
			strcat(ptmp, "#");
      else if (player_globals.parray[p2].game >= 0)
	strcat(ptmp, "^");
      else if (!CheckPFlag(p2, PFLAG_OPEN))
	strcat(ptmp, ":");
      else if (CheckPFlag(p2,PFLAG_TOURNEY))
    strcat(ptmp, "&");
      else if (player_idle(p2) > 300)
	strcat(ptmp, ".");
      else
	strcat(ptmp, " ");
    strcat(ptmp, player_globals.parray[p2].name);
	pprintf(p,"%s\n",ptmp);
	totalcount++;
	}

  }

pprintf(p, " %3d partnership%s displayed.\n\n", totalcount,(totalcount == 1) ? "" : "s");
 }

    // Alex Guo: alias "bugwho u" with "who". This means that the player
    // will see -all- the people on the server when he searches for
    // unpartnered people, which is OK because we are a bughouse server
    // anyways.
    if (strchr(test, 'u')) {
        pprintf(p,"All players:\n");
        pcommand(p, "who");
    }

 /*if (strchr(test,'u'))
   {
   ptmp[0] = '\0';
   pprintf(p,"Unpartnered players with bugopen on\n\n");
   for (i = 0; i < player_globals.p_num; i++) {
            struct player *pp = &player_globals.parray[i];
            if (pp->partner > 0) continue;

          // show status

            rat = pp->bug_stats.rating;
            strcat(ptmp,  ratstrii(rat, i));
            if ((player_globals.parray[i].game >= 0)
              && ((game_globals.garray[player_globals.parray[i].game].status == GAME_EXAMINE) ||
                  (game_globals.garray[player_globals.parray[i].game].status == GAME_SETUP)))
            strcat(ptmp, "#");
          else if (player_globals.parray[i].game >= 0)
            strcat(ptmp, "^");
          else if (!CheckPFlag(i, PFLAG_OPEN))
            strcat(ptmp, ":");
          else if (CheckPFlag(i,PFLAG_TOURNEY))
            strcat(ptmp, "&");
          else if (player_idle(i) > 300)
            strcat(ptmp, ".");
          else
            strcat(ptmp, " ");
          strcat(ptmp, pp->name);
          strcat(ptmp,"\0");
          pprintf(p,"%s\n",ptmp);
   }
   }*/

return COM_OK;
}


static void who_terse(int p, int num, int *plist, int type)
{
  struct player *pp = &player_globals.parray[p];
  char ptmp[80 + 20];		/* for highlight */
  multicol *m = multicol_start(player_globals.parray_size);
  int i;
  int p1;
  int rat = 0;

  /* altered DAV 3/15/95 */

  for (i = 0; i < num; i++) {
    p1 = plist[i];

    if (type == blitz_rat)
      rat = player_globals.parray[p1].z_stats.rating;
    else if (type == wild_rat)
      rat = player_globals.parray[p1].fr_bug_stats.rating;
    else if (type == std_rat)
      rat = player_globals.parray[p1].s_stats.rating;
    else if (type == light_rat)
      rat = player_globals.parray[p1].simul_stats.rating;
    else if (type == bug_rat)
      rat = player_globals.parray[p1].bug_stats.rating;

    if (type == none) {
      sprintf(ptmp, "     ");
    } else {
      sprintf(ptmp, "%-4s", ratstrii(rat, p1));
      if ((player_globals.parray[p1].game >= 0) && ((game_globals.garray[player_globals.parray[p1].game].status == GAME_EXAMINE) || (game_globals.garray[player_globals.parray[p1].game].status == GAME_SETUP)))
        strcat(ptmp, "#");
      else if (player_globals.parray[p1].game >= 0)
	strcat(ptmp, "^");
      else if (CheckPFlag(p1,PFLAG_TOURNEY))
        strcat(ptmp, "&");
      else if (!CheckPFlag(p1, PFLAG_OPEN))
	strcat(ptmp, ":");
  /*  else if (CheckPFlag(i,PFLAG_TOURNEY))
    strcat(ptmp, "&");*/ // playerdb.h says PFLAG_TOURNEY is not yet coded.
      else if (player_idle(p1) > 300)
	strcat(ptmp, ".");
      else
	strcat(ptmp, " ");
    }

    if (p == p1) {
      psprintf_highlight(p, ptmp + strlen(ptmp), "%s", player_globals.parray[p1].name);
    } else {
      strcat(ptmp, player_globals.parray[p1].name);
    }

    AddPlayerLists(p1, ptmp);
    multicol_store(m, ptmp);
  }
  multicol_pprint(m, p, pp->d_width, 2);
  multicol_end(m);
  pprintf(p, "\n %d players displayed (of %d). (*) indicates system administrator.\n", num, player_count(1));
}
/*
static void who_verbose(p, num, plist)
int p;
int num;
int plist[];
{
  int i, p1;
  char playerLine[255], tmp[255];
  char p1WithAttrs[255];

  pprintf(p,
      " +---------------------------------------------------------------+\n"
    );
  pprintf(p,
      " |      User              Standard    Blitz        On for   Idle |\n"
    );
  pprintf(p,
      " +---------------------------------------------------------------+\n"
    );

  for (i = 0; i < num; i++) {
    p1 = plist[i];

    strcpy(playerLine, " |");

    if (player_globals.parray[p1].game >= 0)
      sprintf(tmp, "%3d", player_globals.parray[p1].game + 1);
    else
      sprintf(tmp, "   ");
    strcat(playerLine, tmp);

    if (!CheckPFlag(p1, PFLAG_OPEN))
      sprintf(tmp, "X");
    else
      sprintf(tmp, " ");
    strcat(playerLine, tmp);

    if (CheckPFlag(p1, PFLAG_REG))
      if (CheckPFlag(p1, PFLAG_RATED)) {
	sprintf(tmp, " ");
      } else {
	sprintf(tmp, "u");
      }
    else
      sprintf(tmp, "U");
    strcat(playerLine, tmp);


    strcpy (p1WithAttrs, player_globals.parray[p1].name);
    AddPlayerLists(p1, p1WithAttrs);
    p1WithAttrs[17] = '\0';


    if (p == p1) {
      strcpy(tmp, " ");
      psprintf_highlight(p, tmp + strlen(tmp), "%-17s", p1WithAttrs);
    } else {
      sprintf(tmp, " %-17s", p1WithAttrs);
    }
    strcat(playerLine, tmp);

    sprintf(tmp, " %4s        %-4s        %5s  ",
	    ratstrii(player_globals.parray[p1].s_stats.rating, p1),
	    ratstrii(player_globals.parray[p1].z_stats.rating, p1),
	    hms(player_ontime(p1), 0, 0, 0, 0));
    strcat(playerLine, tmp);

    if (player_idle(p1) >= 60) {
      sprintf(tmp, "%5s   |\n", hms(player_idle(p1), 0, 0, 0, 0));
    } else {
      sprintf(tmp, "        |\n");
    }
    strcat(playerLine, tmp);
    pprintf(p, "%s", playerLine);
  }

  pprintf(p,
      " |                                                               |\n"
    );
  pprintf(p,
     " |    %3d Players Displayed                                      |\n",
	  num
    );
  pprintf(p,
      " +---------------------------------------------------------------+\n"
    );
}
*/
/*static void who_winloss(p, num, plist)
int p;
int num;
int plist[];
{
  int i, p1;
  char playerLine[255], tmp[255];
  char p1WithAttrs[255];

  pprintf(p,
	  "Name               Stand     win loss draw   Blitz    win loss draw    idle\n"
    );
  pprintf(p,
	  "----------------   -----     -------------   -----    -------------    ----\n"
    );

  for (i = 0; i < num; i++) {
    playerLine[0] = '\0';
    p1 = plist[i];


    strcpy (p1WithAttrs, player_globals.parray[p1].name);
    AddPlayerLists(p1, p1WithAttrs);
    p1WithAttrs[17] = '\0';

    if (p1 == p) {
      psprintf_highlight(p, playerLine, "%-17s", p1WithAttrs);
    } else {
      sprintf(playerLine, "%-17s", p1WithAttrs);
    }
    sprintf(tmp, "  %4s     %4d %4d %4d   ",
	    ratstrii(player_globals.parray[p1].s_stats.rating, p1),
	    (int) player_globals.parray[p1].s_stats.win,
	    (int) player_globals.parray[p1].s_stats.los,
	    (int) player_globals.parray[p1].s_stats.dra);
    strcat(playerLine, tmp);

    sprintf(tmp, "%4s    %4d %4d %4d   ",
	    ratstrii(player_globals.parray[p1].z_stats.rating, p1),
	    (int) player_globals.parray[p1].z_stats.win,
	    (int) player_globals.parray[p1].z_stats.los,
	    (int) player_globals.parray[p1].z_stats.dra);
    strcat(playerLine, tmp);

    if (player_idle(p1) >= 60) {
      sprintf(tmp, "%5s\n", hms(player_idle(p1), 0, 0, 0, 0));
    } else {
      sprintf(tmp, "     \n");
    }
    strcat(playerLine, tmp);

    pprintf(p, "%s", playerLine);
  }
  pprintf(p, "    %3d Players Displayed.\n", num);
}*/

static int who_ok(int p, unsigned int sel_bits)
{
  struct player *pp = &player_globals.parray[p];
  int p2;
  if (pp->status != PLAYER_PROMPT)
    return 0;
  if (sel_bits == WHO_ALL)
    return 1;
  if (sel_bits & WHO_OPEN)
    if (!CheckPFlag(p, PFLAG_OPEN) || CheckPFlag(p, PFLAG_TOURNEY))
      return 0;
  if (sel_bits & WHO_CLOSED)
    if (CheckPFlag(p, PFLAG_OPEN))
      return 0;
  if (sel_bits & WHO_RATED)
    if (!CheckPFlag(p, PFLAG_RATED))
      return 0;
  if (sel_bits & WHO_UNRATED)
    if (CheckPFlag(p, PFLAG_RATED))
      return 0;
  if (sel_bits & WHO_FREE)
    if (pp->game >= 0)
      return 0;
  if (sel_bits & WHO_PLAYING)
    if (pp->game < 0)
      return 0;
  if (sel_bits & WHO_REGISTERED)
    if (!CheckPFlag(p, PFLAG_REG))
      return 0;
  if (sel_bits & WHO_UNREGISTERED)
    if (CheckPFlag(p, PFLAG_REG))
      return 0;
  if (sel_bits & WHO_BUGTEAM) {
    p2 = pp->partner;
    if (p2 < 0 || player_globals.parray[p2].partner != p)
      return 0;
    }
  return 1;
}


/*static int blitz_cmp(const void *pp1, const void *pp2)
{
  register int p1 = *(int *) pp1;
  register int p2 = *(int *) pp2;
  if (player_globals.parray[p1].status != PLAYER_PROMPT) {
    if (player_globals.parray[p2].status != PLAYER_PROMPT)
      return 0;
    else
      return -1;
  }
  if (player_globals.parray[p2].status != PLAYER_PROMPT)
    return 1;
  if (player_globals.parray[p1].z_stats.rating > player_globals.parray[p2].z_stats.rating)
    return -1;
  if (player_globals.parray[p1].z_stats.rating < player_globals.parray[p2].z_stats.rating)
    return 1;
  if (CheckPFlag(p1, PFLAG_REG) && !CheckPFlag(p2, PFLAG_REG))
    return -1;
  if (!CheckPFlag(p1, PFLAG_REG) && CheckPFlag(p2, PFLAG_REG))
    return 1;
  return strcmp(player_globals.parray[p1].login, player_globals.parray[p2].login);
}*/

/*static int light_cmp(const void *pp1, const void *pp2)
{
  register int p1 = *(int *) pp1;
  register int p2 = *(int *) pp2;
  if (player_globals.parray[p1].status != PLAYER_PROMPT) {
    if (player_globals.parray[p2].status != PLAYER_PROMPT)
      return 0;
    else
      return -1;
  }
  if (player_globals.parray[p2].status != PLAYER_PROMPT)
    return 1;
  if (player_globals.parray[p1].simul_stats.rating > player_globals.parray[p2].simul_stats.rating)
    return -1;
  if (player_globals.parray[p1].simul_stats.rating < player_globals.parray[p2].simul_stats.rating)
    return 1;
  if (CheckPFlag(p1, PFLAG_REG) && !CheckPFlag(p2, PFLAG_REG))
    return -1;
  if (!CheckPFlag(p1, PFLAG_REG) && CheckPFlag(p2, PFLAG_REG))
    return 1;
  return strcmp(player_globals.parray[p1].login, player_globals.parray[p2].login);
}*/

static int bug_cmp(const void *pp1, const void *pp2)
{
  register int p1 = *(int *) pp1;
  register int p2 = *(int *) pp2;
  if (player_globals.parray[p1].status != PLAYER_PROMPT) {
    if (player_globals.parray[p2].status != PLAYER_PROMPT)
      return 0;
    else
      return -1;
  }
  if (player_globals.parray[p2].status != PLAYER_PROMPT)
    return 1;
  if (player_globals.parray[p1].bug_stats.rating > player_globals.parray[p2].bug_stats.rating)
    return -1;
  if (player_globals.parray[p1].bug_stats.rating < player_globals.parray[p2].bug_stats.rating)
    return 1;
  if (CheckPFlag(p1, PFLAG_REG) && !CheckPFlag(p2, PFLAG_REG))
    return -1;
  if (!CheckPFlag(p1, PFLAG_REG) && CheckPFlag(p2, PFLAG_REG))
    return 1;
  return strcmp(player_globals.parray[p1].login, player_globals.parray[p2].login);
}

/*static int stand_cmp(const void *pp1, const void *pp2)
{
  register int p1 = *(int *) pp1;
  register int p2 = *(int *) pp2;
  if (player_globals.parray[p1].status != PLAYER_PROMPT) {
    if (player_globals.parray[p2].status != PLAYER_PROMPT)
      return 0;
    else
      return -1;
  }
  if (player_globals.parray[p2].status != PLAYER_PROMPT)
    return 1;
  if (player_globals.parray[p1].s_stats.rating > player_globals.parray[p2].s_stats.rating)
    return -1;
  if (player_globals.parray[p1].s_stats.rating < player_globals.parray[p2].s_stats.rating)
    return 1;
  if (CheckPFlag(p1, PFLAG_REG) && !CheckPFlag(p2, PFLAG_REG))
    return -1;
  if (!CheckPFlag(p1, PFLAG_REG) && CheckPFlag(p2, PFLAG_REG))
    return 1;
  return strcmp(player_globals.parray[p1].login, player_globals.parray[p2].login);
}*/

/*static int wild_cmp(const void *pp1, const void *pp2)
{
  register int p1 = *(int *) pp1;
  register int p2 = *(int *) pp2;
  if (player_globals.parray[p1].status != PLAYER_PROMPT) {
    if (player_globals.parray[p2].status != PLAYER_PROMPT)
      return 0;
    else
      return -1;
  }
  if (player_globals.parray[p2].status != PLAYER_PROMPT)
    return 1;
  if (player_globals.parray[p1].fr_bug_stats.rating > player_globals.parray[p2].fr_bug_stats.rating)
    return -1;
  if (player_globals.parray[p1].fr_bug_stats.rating < player_globals.parray[p2].fr_bug_stats.rating)
    return 1;
  if (CheckPFlag(p1, PFLAG_REG) && !CheckPFlag(p2, PFLAG_REG))
    return -1;
  if (!CheckPFlag(p1, PFLAG_REG) && CheckPFlag(p2, PFLAG_REG))
    return 1;
  return strcmp(player_globals.parray[p1].login, player_globals.parray[p2].login);
}*/

/*static int alpha_cmp(const void *pp1, const void *pp2)
{
  register int p1 = *(int *) pp1;
  register int p2 = *(int *) pp2;
  if (player_globals.parray[p1].status != PLAYER_PROMPT) {
    if (player_globals.parray[p2].status != PLAYER_PROMPT)
      return 0;
    else
      return -1;
  }
  if (player_globals.parray[p2].status != PLAYER_PROMPT)
    return 1;
  return strcmp(player_globals.parray[p1].login, player_globals.parray[p2].login);
}*/

static void sort_players(int *players,
			 int ((*cmp_func) (const void *, const void *)))
{
	int i;

	for (i = 0; i < player_globals.p_num; i++) {
		players[i] = i;
	}
	qsort(players, player_globals.p_num, sizeof(int), cmp_func);
}


int com_showadmins(int p)
{
 char playerLine[255];
 char tmp[255];
 int p1;
 pprintf(p, " +---------------------------------------------------------------+\n");
 pprintf(p, " +                          ADMINS                               +\n");
 pprintf(p, " +---------------------------------------------------------------+\n");
 pprintf(p, " |      Name                                                Idle |\n");
 pprintf(p, " +---------------------------------------------------------------+\n");

for (p1 = 0; p1 < player_globals.p_num; p1++) {
     if (player_globals.parray[p1].adminLevel <= 9) continue; // speed things up a bit
     if (player_globals.parray[p1].adminLevel>=10/* && CheckPFlag(p1, PFLAG_ADMINLIGHT)*/)
     {
     strcpy(playerLine, " |");
     sprintf(tmp,"%-6s",player_globals.parray[p1].name);
     strcat(playerLine, tmp);

     if (player_idle(p1) >= 60)
         sprintf(tmp, "%5s   |\n", hms(player_idle(p1), 1, 0, 0, 0));
     //else
	 sprintf(tmp, "        |\n");

         strcat(playerLine, tmp);

     //int len = strlen(playerLine);
     //for(int i=0;i<58-len;i++) { strcat(playerLine," "); }

     pprintf(p, "%s", playerLine);
     }
}

         pprintf(p, " |                                                               |\n");
  pprintf_prompt(p, " +---------------------------------------------------------------+\n");
	return COM_OK;
}

int com_listBugTeams(int p, param_list param)
{
    dbi_result result;

    result = bugteam_getAll();

    if (!db_check_error())
        return COM_OK;

    if (dbi_result_get_numrows(result) == 0) {
        pprintf(p, "There are no bug teams that have played yet.\n");
        return COM_OK;
    }

    // TODO: check return of dbi_result_first_row
    dbi_result_first_row(result);

    do {
        char *partnerone = dbi_result_get_string(result, "partnerone");
        char *partnertwo = dbi_result_get_string(result, "partnertwo");
        int rating = dbi_result_get_int(result, "rating");
        double RD = dbi_result_get_double(result, "sterr");

        pprintf(p, "%-20s %-20s %-4d\n", partnerone, partnertwo, rating);

    } while (dbi_result_next_row(result));

    dbi_result_free(result);

    return COM_OK;
}


/* This is the of the most complicated commands in terms of parameters */
int com_who(int p, param_list param)
{
	//struct player *pp = &player_globals.parray[p];
	//int style = 0;
	float stop_perc = 1.0;
	float start_perc = 0;
	unsigned int sel_bits = WHO_ALL;
	int *sortlist, *plist;
	int ((*cmp_func) (const void *, const void *));
	int startpoint;
	int stoppoint;
	//int i, len;
	//int tmpI, tmpJ;
	//char c;
	int p1, count, num_who;
	int sort_type;
	//int total;


	/* good idea but not for bug-server, i remove this : aramen
	total = pp->d_time * 60 + pp->d_inc * 40;
	if (total < 180) {
		sort_type = light_rat;
		cmp_func = light_cmp;
	} else if (total >= 900) {
		sort_type = std_rat;
		cmp_func = stand_cmp;
	} else {
		sort_type = blitz_rat;
		cmp_func = blitz_cmp;
	} */
		cmp_func = bug_cmp;  //default bug rating  : aramen
		sort_type = bug_rat;

    // Special case for Bug team ratings. "who tb" => list all bug teams
    // TODO: redesign this whole damn function
    if (param[0].type != TYPE_NULL) {
        char *gametype = param[0].val.string;

        if (gametype[0] == 'b' && gametype[1] == 't') {
            com_listBugTeams(p, param);
            // TODO: check return value of listBugTeams
            return COM_OK;
        }
    }

	if (param[0].type != TYPE_NULL)
		return COM_BADPARAMETERS;

	sortlist = malloc(sizeof(int) * player_globals.p_num);
	plist = malloc(sizeof(int) * player_globals.p_num);

	/*
	{
		len = strlen(param[0].val.string);
		for (i = 0; i < len; i++) {
			c = param[0].val.string[i];
			if (isdigit(c)) {
				if (i == 0 || !isdigit(param[0].val.string[i - 1])) {
					tmpI = c - '0';
					if (tmpI == 1) {
						start_perc = 0.0;
						stop_perc = 0.333333;
					} else if (tmpI == 2) {
						start_perc = 0.333333;
						stop_perc = 0.6666667;
					} else if (tmpI == 3) {
						start_perc = 0.6666667;
						stop_perc = 1.0;
					} else if ((i == len - 1) || (!isdigit(param[0].val.string[i + 1])))
						goto bad_parameters;
				} else {
					tmpI = c - '0';
					tmpJ = param[0].val.string[i - 1] - '0';
					if (tmpI == 0)
						goto bad_parameters;
					if (tmpJ > tmpI)
						goto bad_parameters;
					start_perc = ((float) tmpJ - 1.0) / (float) tmpI;
					stop_perc = ((float) tmpJ) / (float) tmpI;
				}
			} else {
				switch (c) {
				case ' ':
				case '\n':
				case '\t':
					break;
				case 'o':
					if (sel_bits == WHO_ALL)
						sel_bits = WHO_OPEN;
					else
						sel_bits |= WHO_OPEN;
					break;
				case 'r':
					if (sel_bits == WHO_ALL)
						sel_bits = WHO_RATED;
					else
						sel_bits |= WHO_RATED;
					break;
				case 'f':
					if (sel_bits == WHO_ALL)
						sel_bits = WHO_FREE;
					else
						sel_bits |= WHO_FREE;
					break;
				case 'a':
					if (sel_bits == WHO_ALL)
						sel_bits = WHO_FREE | WHO_OPEN;
					else
						sel_bits |= (WHO_FREE | WHO_OPEN);
					break;
				case 'R':
					if (sel_bits == WHO_ALL)
						sel_bits = WHO_REGISTERED;
					else
						sel_bits |= WHO_REGISTERED;
					break;
				case 'l':
					cmp_func = alpha_cmp;
					sort_type = none;
					break;
				case 'A':
					cmp_func = alpha_cmp;
					break;
				case 'w':
					cmp_func = wild_cmp;
					sort_type = wild_rat;
					break;
				case 's':
					cmp_func = stand_cmp;
					sort_type = std_rat;
					break;
				case 'b':
					cmp_func = blitz_cmp;
					sort_type = blitz_rat;
					break;
				case 'L':
					cmp_func = light_cmp;
					sort_type = light_rat;
					break;
				case 't':
					style = 0;
					break;
				case 'v':
					style = 1;
					break;
				case 'n':
					style = 2;
					break;
				case 'U':
					if (sel_bits == WHO_ALL)
						sel_bits = WHO_UNREGISTERED;
					else
						sel_bits |= WHO_UNREGISTERED;
					break;
				case 'B':
					if (sel_bits == WHO_ALL)
						sel_bits = WHO_BUGTEAM;
					else
						sel_bits |= WHO_BUGTEAM;
					cmp_func = bug_cmp;
					sort_type = bug_rat;
					style=3; // to partner display : aramen
					break;
				default:
					goto bad_parameters;
				}
			}
		}
	}*/
	sort_players(sortlist, cmp_func);
	count = 0;
	for (p1 = 0; p1 < player_globals.p_num; p1++) {
		if (!who_ok(sortlist[p1], sel_bits))
			continue;
		count++;
	}
	startpoint = floor((float) count * start_perc);
	stoppoint = ceil((float) count * stop_perc) - 1;
	num_who = 0;
	count = 0;
	for (p1 = 0; p1 < player_globals.p_num; p1++) {
		if (!who_ok(sortlist[p1], sel_bits))
			continue;
		if ((count >= startpoint) && (count <= stoppoint)) {
			plist[num_who++] = sortlist[p1];
		}
		count++;

	}

	/*if (num_who == 0) {
		pprintf(p, "No logged in players match the flags in your who request.\n");
		return COM_OK;
	}*/

	//switch (style) {
	//case 0:			/* terse */
		who_terse(p, num_who, plist, sort_type);
	FREE(plist);
	FREE(sortlist);
	//	break;
	//case 1:			/* verbose */
	//	who_verbose(p, num_who, plist);
	//	break;
	//case 2:			/* win-loss */
	//	who_winloss(p, num_who, plist);
	//	break;
	//case 3:			// bug : aramen
	//	who_bug(p);
	//	break;
	//default:
	//	goto bad_parameters;
	//}

	return COM_OK;

/*bad_parameters:
	return COM_BADPARAMETERS;*/
}

int com_open(int p, param_list param)
{
	int retval;
	if ((retval = pcommand(p, "set open")) != COM_OK)
		return retval;
	else
		return COM_OK_NOPROMPT;
}




int com_bell(int p, param_list param)
{
  int retval;
  if ((retval = pcommand(p, "set bell")) != COM_OK)
    return retval;
  else
    return COM_OK_NOPROMPT;
}

int com_flip(int p, param_list param)
{
  int retval;
  if ((retval = pcommand(p, "set flip")) != COM_OK)
    return retval;
  else
    return COM_OK_NOPROMPT;
}

int com_style(int p, param_list param)
{
  int retval;
  if ((retval = pcommand(p, "set style %d", param[0].val.integer)) != COM_OK)
    return retval;
  else
    return COM_OK_NOPROMPT;
}

int com_promote(int p, param_list param)
{
  int retval;
  if ((retval = pcommand(p, "set promote %s", param[0].val.word)) != COM_OK)
    return retval;
  else
    return COM_OK_NOPROMPT;
}

void alias_add(int p, const char *name, const char *value)
{
	struct player *pp = &player_globals.parray[p];

	pp->alias_list = (struct alias_type *)realloc(pp->alias_list,
						      sizeof(struct alias_type) * (pp->numAlias+1));
	pp->alias_list[pp->numAlias].comm_name = strdup(name);
	pp->alias_list[pp->numAlias].alias = strdup(value);
	pp->numAlias++;
}


int com_alias(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];
	int al, i;
	const char *noalias[] = {"quit", "unalias", NULL};

	if (param[0].type == TYPE_NULL) {
		for (al = 0; al < pp->numAlias; al++) {
			pprintf(p, "%s -> %s\n",
				pp->alias_list[al].comm_name,
				pp->alias_list[al].alias);
		}
		return COM_OK;
	}

	al = alias_lookup(param[0].val.word, pp->alias_list, pp->numAlias);
	if (param[1].type == TYPE_NULL) {
		if (al < 0) {
			pprintf(p, "You have no alias named '%s'.\n", param[0].val.word);
		} else {
			pprintf(p, "%s -> %s\n",
				pp->alias_list[al].comm_name,
				pp->alias_list[al].alias);
		}
		return COM_OK;
	}

	if (al >= 0) {
		FREE(pp->alias_list[al].alias);
		pp->alias_list[al].alias = strdup(param[1].val.string);
		pprintf(p, "Alias %s replaced.\n", param[0].val.string);
		return COM_OK;
	}

	if (pp->numAlias >= config_get_int("MAX_ALIASES", DEFAULT_MAX_ALIASES)) {
		pprintf(p, "You have your maximum number of aliases.\n");
		return COM_OK;
	}

	for (i=0;noalias[i];i++) {
		if (strcasecmp(param[0].val.string, noalias[i]) == 0) {
			pprintf(p, "Sorry, you can't alias this command.\n");
			return COM_OK;
		}
	}

	alias_add(p, param[0].val.word, param[1].val.string);

	pprintf(p, "Alias set.\n");

	return COM_OK;
}

int com_unalias(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];
	int al;
	int i;

	al = alias_lookup(param[0].val.word, pp->alias_list, pp->numAlias);
	if (al < 0) {
		pprintf(p, "You have no alias named '%s'.\n", param[0].val.word);
		return COM_OK;
	}

	FREE(pp->alias_list[al].comm_name);
	FREE(pp->alias_list[al].alias);
	for (i = al; i < pp->numAlias-1; i++) {
		pp->alias_list[i].comm_name = pp->alias_list[i+1].comm_name;
		pp->alias_list[i].alias = pp->alias_list[i+1].alias;
	}
	pp->numAlias--;
	pp->alias_list = (struct alias_type *)realloc(pp->alias_list,
						      sizeof(struct alias_type) * pp->numAlias);
	pprintf(p,"Alias removed.\n");

	return COM_OK;
}


int com_handles(int p, param_list param)
{
	char *buffer[1000];
	char pdir[MAX_FILENAME_SIZE];
	int count;

	sprintf(pdir, "%s/%c", PLAYER_DIR, param[0].val.word[0]);
	count = search_directory(pdir, param[0].val.word, buffer, 1000);
	pprintf(p, "Found %d names.\n", count);
	if (count > 0)
		display_directory(p, buffer, count);
	return COM_OK;
}

int com_getgi(int p, param_list param)
{
  int p1, g;
  struct player *pp = &player_globals.parray[p];

  if (!in_list(p, L_TD, pp->name)) {
    pprintf(p, "Only TD programs are allowed to use this command.\n");
    return COM_OK;
  }
  if (((p1 = player_find_bylogin(param[0].val.word)) < 0)
      || (!CheckPFlag(p1, PFLAG_REG))) {
    /* Darkside suggested not to return anything */
    return COM_OK;
  }
  if (!CheckPFlag(p1, PFLAG_REG)) {
    pprintf(p, "*getgi %s none none -1 -1 -1 -1 -1*\n", player_globals.parray[p1].name);
  } else if (player_globals.parray[p1].game >= 0) {
    g = player_globals.parray[p1].game;
    if (game_globals.garray[g].status == GAME_ACTIVE) {
      pprintf(p, "*getgi %s %s %s %d %d %d %d 0 %d*\n",
	      player_globals.parray[p1].name,
	      game_globals.garray[g].white_name,
	      game_globals.garray[g].black_name,
	      g + 1,
	      game_globals.garray[g].wInitTime/60000,
	      game_globals.garray[g].wIncrement/1000,
	      game_globals.garray[g].rated,
	      game_globals.garray[g].type);
    } else {
      pprintf(p, "%s is not playing a game.\n", player_globals.parray[p1].name);
    }
  } else {
    pprintf(p, "%s is not playing a game.\n", player_globals.parray[p1].name);
  }
  return COM_OK;
}

int com_getpi(int p, param_list param)
{
  struct player *pp = &player_globals.parray[p];
  int p1;

  if (!in_list(p, L_TD, pp->name)) {
    pprintf(p, "Only TD programs are allowed to use this command.\n");
    return COM_OK;
  }
  if (((p1 = player_find_bylogin(param[0].val.word)) < 0)
//      || (!CheckPFlag(p1, PFLAG_REG))
      )
      {
    /* Darkside suggested not to return anything */
    return COM_OK;
  }



  int nor = 0;
  if (!CheckPFlag(p1, PFLAG_REG)) {
    pprintf(p, "*getpi %s -1 -1 -1 %s %d*\n", player_globals.parray[p1].name,
    		// Added the below checks if the unrated player had a partner.
    		// Edited by johnthegreat: May 21, 2009
    		player_globals.parray[p1].partner >= 0 ? player_globals.parray[player_globals.parray[p1].partner].name : "none",
    		player_globals.parray[p1].partner >=0 ? player_globals.parray[player_globals.parray[p1].partner].bug_stats.rating : nor
		);
  } else {
    pprintf(p, "*getpi %s %d %d %d %s %d*\n", player_globals.parray[p1].name,
	    player_globals.parray[p1].z_stats.rating,
	    player_globals.parray[p1].s_stats.rating,
	    player_globals.parray[p1].bug_stats.rating,
		player_globals.parray[p1].partner >=0 ? player_globals.parray[player_globals.parray[p1].partner].name : "none",
		player_globals.parray[p1].partner >=0 ? player_globals.parray[player_globals.parray[p1].partner].bug_stats.rating : nor);


  }
  return COM_OK;
}

int com_limits(int p, param_list param)
{
	struct player *pp = &player_globals.parray[p];

	pprintf(p, "\nCurrent hardcoded limits:\n");
	pprintf(p, "  Max number of channels and max capacity: %d\n", MAX_CHANNELS);
	pprintf(p, "  Max number of channels one can be in: %d\n", MAX_INCHANNELS);
	pprintf(p, "  Max number of people on the notify list: %d\n", MAX_NOTIFY);
	pprintf(p, "  Max number of people on the censor list: %d\n", MAX_CENSOR);

	pprintf(p, "  Max number of messages one can receive: %d\n", MAX_MESSAGES);
	pprintf(p, "  Min number of games to be active: %d\n", PROVISIONAL);
	if (!check_admin(p, ADMIN_ADMIN) &&
	    !titled_player(p,pp->login)) {
		pprintf(p, "  Size of journal (entries): %d\n", MAX_JOURNAL);
	} else {
		pprintf(p, "  Size of journal (entries): 26\n");
	}

	// Changed "settable" to "changeable"
	// Edited by johnthegreat: May 21, 2009
	pprintf(p, "\nAdmin changeable limits:\n");
	pprintf(p, "  Shout quota gives two shouts per %d seconds.\n", seek_globals.quota_time);
	return COM_OK;
}


int com_remote(int p, param_list param)
{
	int p1;

	if ((p1 = player_find_part_login(param[0].val.word)) < 0) {
		pprintf(p, "%s is not logged in.\n", param[0].val.word);
		return COM_OK;
	}
	if (!in_list(p1, L_REMOTE, player_globals.parray[p].login)) {
		pprintf(p, "You are not in the remote list for %s\n",
			player_globals.parray[p1].login);
		return COM_OK;
	}
	pprintf(p, "Command issued as %s\n", player_globals.parray[p1].name);
	pprintf(p1, "Remote command [%s] issued by %s\n",
		param[1].val.string, player_globals.parray[p].name);
	pcommand(p1, "%s\n", param[1].val.string);
	return COM_OK;
}
