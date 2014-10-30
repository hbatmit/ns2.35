/*
http_active

Copyright July 5, 2001, The University of North Carolina at Chapel Hill

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, 
     this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.
  3. The name of the author may not be used to endorse or promote products 
     derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO 
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

Contact person:

    Frank D. Smith, University of North Carolina at Chapel Hill
        email: smithfd@cs.unc.edu
	phone: 919-962-1884
	fax:   919-962-1799
*/

/* Program to create an activity trace (summary form) of web browsing clients
   with respect to three types of activity: client sending request data, 
   server sending response data, client is idle (no request or response).
   Identification of idle periods is used to infer user "think" times 
   between requests for new top-level pages.    A client is defined by
    a single IP address. 
  
   "Idle" is defined as a period of time greater than a threshold value
   ("idle_limit" with a default of 2 seconds) during which a client has no
   requests outstanding.  A request is outstanding from the
   start time of a request until the end time (normal or terminated) of
   the corresponding response.

   The input to this program is the SORTed output from http_connect.  
   The sort to be applied is produced with the following shell script:

      sort -s -o $1.sort +1 -2 +0 -1 -T /tmp $1

   This sorts all the records for a given client IP address in timestamp 
   order.  

   The output is also time ordered with respect to a single client (IP
   address) and consists only of client request entries (in the same format
   as the input) and ordered by start time, server responses (in the same 
   format as the input) and ordered by end time, and client idle periods 
   giving the elapsed idle time and ordered by the end of the idle period.  
   The output file has extension ".activity" added by the program.

   To get usage information, invoke the program with the -h switch.

*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>

#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))

void Usage(char *s)
{
  fprintf (stderr,"\nUsage: %s\n", s);
  fprintf (stderr,"    [-w file_name] (name for output file)\n");
  fprintf (stderr,"    [-r file_name] (name for input file)\n");
  fprintf (stderr,"    [-I idle_limit] (min inactivity interval)\n");
  fprintf (stderr,"\n");
  exit(-1);
}

  FILE *dumpFP, *outFP;

  struct timeval time_stamp = {0,0};

  int req;
  int rsp;

  char ts[20]; 
  char sh[25]; 
  char sp[10];
  char gt[3]; 
  char dh[25]; 
  char dp[10];
  char fl[5]; 

  char current_src[25] = "";

  enum client_states 
             {PENDING_ACTIVE, ACTIVE, PENDING_IDLE, IDLE};
  enum client_states client_state = PENDING_ACTIVE;

  enum event_types {SYN, ACT_REQ, ACT_RSP, END, REQ, RSP};
  enum event_types event_type;


  char idle_begin[20];
  char earliest_end[20];
  char last_client_ts[20];

/* For each client (IP address) maintain a table of HTTP connections
   that are "active" with the following information about each connection:
     id (host/port 4-tuple identifying the connection 
     activity (1 if a request has been sent and the response is not
               yet complete; 0 otherwise)
     state (like activity)
   A connection is "active" (in the table) from the time a connection
   start (SYN, ACT-REQ or ACT-RSP) is seen in the input until a 
   connection end (FIN, RST, TRM) is also seen in the input
*/
     
int active_connections = 0;
#define MAX_CONNECTIONS 1000
  struct connect
    {
     char id[50];
     int  activity;
     enum event_types state;
    }connections[MAX_CONNECTIONS];

  char new_line[500];

void error_line(char *s);
void error_state(char *s);

void log_REQ(void);
void log_RSP(void);
void log_IDLE(char *s);

void set_connection(char *sp, char *dh, char *dp, enum event_types type);
void ClearConnections(void);
int ConnectionsActive(void);
int FindConnection(char *sp, char *dh, char *dp);
int AddConnection(char *sp, char *dh, char *dp);
int RemoveConnection(char *sp, char *dh, char *dp);

long elapsed_ms(char *end, char *start);

void main (int argc, char* argv[])
{
  int i;

  char input_name[256] = "";
  char output_name[256] = "";
  long idle_limit = 2000;  /* default threshold for idleness in millisec. */ 
  long elapsed;

  char parse_line[500];
  char discard[50];

  char *cursor;
  char *vp;

  /* Parse the command line */
  i = 1;
  while (i < argc) {
    if (strcmp (argv[i], "-r") == 0) {
      if (++i >= argc) Usage (argv[0]);
      strcpy (input_name, argv[i]);
    }
    else if (strcmp (argv[i], "-w") == 0) {
      if (++i >= argc) Usage (argv[0]);
      strcpy (output_name, argv[i]);
    }
    else if (strcmp (argv[i], "-I") == 0) {
      if (++i >= argc) Usage (argv[0]);
      idle_limit = (long)atoi(argv[i]);
    }
    else 
      Usage (argv[0]);
    i++;
  }

  /* Open files */
  if (strcmp(output_name, "") == 0)
     outFP = stdout;
  else 
     {
      strcat(output_name, ".activity");
      if ((outFP = fopen (output_name, "w")) == NULL) {
          fprintf (stderr, "error opening %s\n", output_name);
          exit (-1);
          }
     }

  if (strcmp(input_name, "") == 0)
     dumpFP = stdin;
  else 
     {
      if ((dumpFP = fopen (input_name, "r")) == NULL) {
          fprintf (stderr, "error opening %s\n", input_name);
          exit (-1);
         }
     }

  /* Read each record in the input file.  Look for a change in the 
     source IP address (which indicates a new client).  If a new
     client, log the end of an idle period (if any) for the old
     client and initialize the connection table for the new client.
     If a record for the current client has been read, classify the
     type of event it represent and process it to update the client
     and connection state.
  */

  while (!feof (dumpFP)) {
    /* Get and parse line of data */
    if (fgets (new_line, sizeof(new_line), dumpFP) == NULL)
       break;
    /* get first line pieces */

    sscanf (new_line, "%s %s %s %s %s %s %s",
                      &ts, &sh, &sp, &gt, &dh, &dp, &fl);

    /* if an ERR line, just show it */
    if (strcmp(fl, "ERR:") == 0)
       {
        error_line(new_line);
        continue;
       }

    /* now get variable part starting with the ":" considering that */
    /* interpretation of the remaining fields depends on the flag value */ 
    /* This is necessary to find the ending timestamp for FIN, RST, and 
       TRM events.
    */

    strcpy(parse_line, new_line);
    cursor = parse_line;
    vp = (char *)strsep(&cursor, ":" );
    if ((cursor == (char *)NULL) ||
        (vp == (char *)NULL))
        {
         error_line(new_line);
         continue;
        }

    /* Classify the event type by looking at the flag field from input
       records */

    if ((strcmp(fl, "REQ") == 0) || 
        (strcmp(fl, "REQ-") == 0))
       event_type = REQ;
    else
      {
       if ((strcmp(fl, "RSP") == 0) ||
           (strcmp(fl, "RSP-") == 0))
          event_type = RSP;
       else
	 {
          if ((strcmp(fl, "FIN") == 0) ||
              (strcmp(fl, "TRM") == 0) ||
              (strcmp(fl, "RST") == 0))
	     {
	       /* need the ending timestamp from these record types */
              sscanf(cursor, "%s %s", &discard, &earliest_end);
              event_type = END;
             }
           else 
             {
              if (strcmp(fl, "SYN") == 0)
                 event_type = SYN;
              else
		{
                 if (strcmp(fl, "ACT-REQ") == 0)
                    event_type = ACT_REQ;
                 else
                     if (strcmp(fl, "ACT-RSP") == 0)
                        event_type = ACT_RSP;
                }
             }
	 }
      }

   /* now use data from new trace record to update status */  
   /* first check to see if this is the same client host */
   if (strcmp(current_src, sh) != 0)
       {
        if (client_state == IDLE)
            log_IDLE(last_client_ts);  
        ClearConnections();
        client_state = PENDING_ACTIVE;
        strcpy(current_src, sh);
       }


   /* update the connection status for this client's connection */

   set_connection(sp, dh, dp, event_type);

   
   /* The main processing for idle periods is done by maintaining a state
      variable (client_status) for the client and looking for specific input 
      record types at different values of the state variable.  The 
      values of client_state and their implications are:
      PENDING_ACTIVE   - A new client is started and remains PENDING_ACTIVE
                         until an activity indication such as ACT-REQ,
                         ACT-RSP, or REQ is seen in which case it enters
                         the ACTIVE state.  If there is an initial response,
                         PENDING_IDLE is entered.
      ACTIVE           - At least one request is outstanding and the state
                         can only change if there is a response completion
                         or connection termination.
      PENDING_IDLE     - There are no requests outstanding but the idle
                         period threshold has not elapsed since it entered
                         the PENDING_IDLE state.
      IDLE             - No outstanding requests for a period greater than
                         the idle threshold.  The IDLE (and PENDING_IDLE)
                         states are exited on activity indication such as 
                         ACT-REQ, ACT-RSP, or REQ
   */

   switch (client_state)
      {
       case PENDING_ACTIVE:
            switch (event_type)
	       {
	        case SYN:
                    break;
	        case ACT_REQ:
	        case ACT_RSP:
                    client_state = ACTIVE;
                    break;
	        case REQ:
                    client_state = ACTIVE;
                    log_REQ();
                    break;
	        case RSP:
                    client_state = PENDING_IDLE;
                    strcpy(idle_begin, ts);
                    log_RSP();
                    break;
	        case END:
                    break;
               }
            break;

       case ACTIVE:
            switch (event_type)
	       {
	        case SYN:
	        case ACT_REQ:
	        case ACT_RSP:
                    break;
	        case REQ:
                    log_REQ();
                    break;
	        case RSP:
                    log_RSP(); 
                    if (ConnectionsActive() == 0) /* Any active connections?*/
		       {
                        client_state = PENDING_IDLE;
                        strcpy(idle_begin, ts);
                       }
                    break;
	        case END:
                    if (ConnectionsActive() == 0) /* Any active connections?*/
		       {
                        client_state = PENDING_IDLE;
                        strcpy(idle_begin, earliest_end);
                       }
                    break;
               }
            break;

       case PENDING_IDLE:
	    /* must start checking time, if > n seconds elapse since
               entering PENDING_IDLE state, enter IDLE state */
            elapsed = elapsed_ms(ts, idle_begin);
            if (elapsed < idle_limit)
	       {
                switch (event_type)
		   {
		    case SYN:
		    case END:
                        break;
	            case ACT_REQ:
	            case ACT_RSP:
                        client_state = ACTIVE;
                        break;
		    case REQ:
                        client_state = ACTIVE;
                        log_REQ();
                        break;
		    case RSP:
                        log_RSP();
                        break;
                   }
                break;  /* ends case PENDING_IDLE */
               }
            else          /* it has crossed the idle threshold */ 
                client_state = IDLE;
       /* NOTE: drop through to IDLE to handle the current event */

       case IDLE:
            switch (event_type)
	       {
		case SYN:
		case END:
                    break;
	        case ACT_REQ:
	        case ACT_RSP:
                    client_state = ACTIVE;
                    log_IDLE(ts);
                    break;
		case REQ:
                    client_state = ACTIVE;
                    log_IDLE(ts);
                    log_REQ();
                    break;
		case RSP:
                    log_RSP();
                    break;
                break;  /* ends case PENDING_IDLE */
               }
             break;

       default:
             break;
      } /* end switch */

    strcpy(last_client_ts, ts);

   } /* end while (!feof ....) */ 
  close (dumpFP);
  close (outFP);
}

/* updates the status of connections for each interesting event */

void set_connection(char *sp, char *dh, char *dp, enum event_types type)
{

  int cx;

  /* A connection is identified by the host/port 3-tuple (the source IP
     address is not needed because only one client is handled at a time).
     The connection's status depends on the type of the event that caused
     the update.  The following event type are defined with their effect
     on the connection's status:
           SYN, ACT-REQ, - The connection has begun (is an "active"
           ACT-RSP         connection.  Add it to the table as idle
                           (activity == 0) if a SYN or with request/
                           response activity (activity == 1) for ACT-REQ
                           or ACT-RSP.
           REQ           - Find the connection in the table and mark it with
                           an outstanding request (activity == 1).
           RSP           - Find the connection in the table and mark it with
                           a completed request (activity == 0).
           END           - The connection has ended (is no longer an "active"
                           connection).  Remove it from the table.
  */

  switch (type)
     {   
      case SYN:
      case ACT_REQ:
      case ACT_RSP:
	 {
          cx = AddConnection(sp, dh, dp);
          if (cx < 0)  /* already there */
	     {
              error_state("Add for existing connection");
              return;
             }            
          if (cx > MAX_CONNECTIONS)  /* table overflow */
	     {
              error_state("Active connections exceeds maximum");
              exit (-1);
             } 
          connections[cx].state = type;
          if (type == SYN)
             connections[cx].activity = 0;
          else
             connections[cx].activity = 1;
          break;
	 }    

      case REQ:
	 {
          cx = FindConnection(sp, dh, dp);
          if (cx < 0)  /* not there */
	     {
              error_state("REQ for non-existent connection");
              return;
             }
          if ((connections[cx].state == RSP) ||
              (connections[cx].state == ACT_REQ) ||
              (connections[cx].state == SYN))
	     {
              connections[cx].activity = 1; 
              connections[cx].state = REQ;
             }       
          else
              error_state("REQ in invalid connection state");

          break;
         }

      case RSP:
	 {
          cx = FindConnection(sp, dh, dp);
          if (cx < 0)  /* not there */
	     {
              error_state("RSP for non-existent connection");
              return;
             }
          if ((connections[cx].state == REQ) ||
              (connections[cx].state == ACT_RSP) ||
              (connections[cx].state == SYN))
	     {
              connections[cx].activity = 0; 
              connections[cx].state = RSP;
             }      
          else
              error_state("RSP in invalid connection state");

          break;
         }

      case END:
	 {
          cx = FindConnection(sp, dh, dp);
          if (cx < 0)  /* not there */
	     {
              error_state("End for non-existent connection");
              return;
             }
          connections[cx].activity = 0; 
          connections[cx].state = END;
          cx = RemoveConnection(sp, dh, dp);
          break;
	 }
      default:
      break;

     }
}

/* A set of functions to maintain the table of "active" connections for the
   current client (IP address). All of these use simple linear scans of the
   table because we expect the number of concurrently active connections 
   from a client to be small (< 100) */

/* Clears the active connections from the connection table and 
   resets the count of active connections to zero */

void ClearConnections(void)
{
  int i;

  for (i = 0; i < active_connections; i++)
      {
       strcpy(connections[i].id, "");
       connections[i].activity = 0;
       connections[i].state = END;
      }
  active_connections = 0;
}

/* Count the number of active connections that have an outstanding
   request (activity == 1) */

int ConnectionsActive(void)
{
  int count = 0; 
  int i;

  for (i = 0; i < active_connections; i++)
      count = count + connections[i].activity;

  return (count);
}

/* Find a connection in the table by its identifying host/port 3-tuple
   and return its index (or -1 if not found).  Note that the source IP
   address is not necessary since the table is used for one client
   at a time.
*/

int FindConnection(char *sp, char *dh, char *dp)
{
  char connection[50];
  int i;

  strcpy(connection, sp);
  strcat(connection, dh);
  strcat(connection, dp);

  /* find the connection in the table */
  for (i = 0; i < active_connections; i++)
      {
       if (strcmp(connections[i].id, connection) == 0)
           break;
      }
  if (i == active_connections) /* not there */
     return(-1);
  else
     return (i);
}

/* Add a new connection to the table identified by its host/port
   3-tuple (source IP is not needed because the table is for one
   client (IP address) only).  Return the index of the added 
   connection or -1 if it is already in the table.  Increase the
   count of active connections by 1 if added.  Initialize the
   state of the added connection to the reset state.
*/  

int AddConnection(char *sp, char *dh, char *dp)
{
  char connection[50];
  int i;

  strcpy(connection, sp);
  strcat(connection, dh);
  strcat(connection, dp);

  /* check to see if connection already in the table; if not there, add it */
  for (i = 0; i < active_connections; i++)
      {
       if (strcmp(connections[i].id, connection) == 0)
           break;
      }
  if (i < active_connections)
     return(-1);   /* already there */
  else
     {
      active_connections += 1;
      if (active_connections > MAX_CONNECTIONS)
         return (active_connections);    /* table overflow */
      strcpy(connections[i].id, connection);
      connections[i].activity = 0;
      connections[i].state = END;
      return (i);
     }
}


/* Remove a connection from the table and compact the table by shifting
   all connections above the "hole" down by one index value.  Return 0
   if all is OK or -1 if the connection was not there.  Decrease the 
   count of active connections by one if one was removed.  Reset the 
   vacated table entry to the reset state
*/

int RemoveConnection(char *sp, char *dh, char *dp)
{
  char connection[50];
  int i, j;

  strcpy(connection, sp);
  strcat(connection, dh);
  strcat(connection, dp);

  /* find the connection in the table; if not there, error */
  for (i = 0; i < active_connections; i++)
      {
       if (strcmp(connections[i].id, connection) == 0)
           break;
      }
  if (i == active_connections)
      return (-1);

  /* move all active connections above this down by one with overwriting */ 
  for (j = i + 1; j < active_connections; j++)
      {
       strcpy(connections[j-1].id, connections[j].id);
       connections[j-1].activity = connections[j].activity;
       connections[j-1].state = connections[j].state;
      }

  /* clearing the top vacated slot is not strictly necessary but it may
     help with debugging */
  strcpy(connections[active_connections - 1].id, "");
  connections[active_connections - 1].activity = 0;
  connections[active_connections - 1].state = END;

  active_connections -= 1;
  return (0);
}

/* Copy the input line to the output file */
void log_REQ(void)
{
  fprintf(outFP, "%s", new_line);
}

/* Copy the input line to the output file */
void log_RSP(void)
{
  fprintf(outFP, "%s", new_line);
}


/* create a line in the output for the idle period */
void log_IDLE(char *ts)
{
  int elapsed;

  elapsed = (int) elapsed_ms(ts, idle_begin);
  fprintf(outFP, "%s %-15s %5s > %-15s %5s IDLE%12d  %s\n", 
                  ts, current_src, "*", "*", "*", elapsed, idle_begin);
                            
}

void error_line(char * s)
{
  fprintf(outFP, "%s", s);
}

void error_state(char * s)
{
  fprintf(outFP, "%s %-15s %5s > %-15s %5s ERROR %s\n", 
                  ts, current_src, "*", "*", "*", s);

}

/*--------------------------------------------------------------*/ 
/* subtract two timevals (t1 - t0) with result in tdiff         */
/* tdiff, t1 and t0 are all pointers to struct timeval          */
/*--------------------------------------------------------------*/ 
static void
tvsub(tdiff, t1, t0)
struct timeval *tdiff, *t1, *t0;
{

        tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
        tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
        if (tdiff->tv_usec < 0)
           {
            tdiff->tv_sec--;
            tdiff->tv_usec += 1000000;
           }
}

/*--------------------------------------------------------------*/ 
/* compute the elapsed time in milliseconds to end_time         */
/* from some past time given by start_time (both formatted timevals) */
/*--------------------------------------------------------------*/ 
long elapsed_ms(char *end, char *start)
{
 struct timeval delta, end_time, start_time;
 long elapsed_time;

 char end_tmp[20];
 char start_tmp[20];

 char *cursor;
 char *cp;

 strcpy(end_tmp, end);
 cursor = end_tmp;
 cp = (char *)strsep(&cursor, "." );
 end_time.tv_sec = atoi(end_tmp);
 end_time.tv_usec = atoi(cursor);

 strcpy(start_tmp, start);
 cursor = start_tmp;
 cp = (char *)strsep(&cursor, "." );
 start_time.tv_sec = atoi(start_tmp);
 start_time.tv_usec = atoi(cursor);

 tvsub(&delta, &end_time, &start_time);
 /* express as milliseconds */
 elapsed_time = (delta.tv_sec * 1000) + (delta.tv_usec/1000);
 return (elapsed_time);
}



