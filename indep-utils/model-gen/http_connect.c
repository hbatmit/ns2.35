/*
http_connect

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

/* 
   This program performs an analysis of tcpdump output (the ASCII print
   lines that tcpdump generates to stdout) and produces a summary of the
   TCP connections used for HTTP.  It assumes that the tcpdump has been
   filtered for packets that are from TCP source port 80 and that
   the result has been sorted so that packets are in ascending time
   order within each TCP connection.  The script given below will properly
   prepare the input for this program given a tcpdump binary file that 
   may contain more than just HTTP packets (the file extensions are
   just examples, the program does not make any assumptions about input
   file names).  Note that this filtering produces a UNI-DIRECTIONAL 
   trace containing only those TCP segments sent from the server to the
   client.

   The output is a file with summary records for each connection (basically
   the data data summarized into response and request lengths).  There is
   also a <file name>.log file with summary counts.

   To get usage information, invoke the program with the -h switch.

   WARNING! THIS PROGRAM DEPENDS ON THE FORMAT OF THE OUTPUT OF TCPDUMP
   AS PRINTED ON STDOUT (ASCII).  IT HAS BEEN TESTED ONLY ON THE 
   FREEBSD VERSION OF TCPDUMP CORRESPONDING TO RELEASES UP THROUGH 4.1

   KNOWN BUG: Does not process tcpdump output for ECN flags correctly.

#! /bin/sh
tcpdump -n -tt -r $1 tcp src port 80 > $1.http-srv
sort -s -o $1.http-srv-sort +1 -2 +3 -4 +0 -1 -T /tmp $1.http-srv

*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>

#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))

int report_ACK_err = 1;

void Usage(char *s)
{
  fprintf (stderr,"\nUsage: %s\n", s);
  fprintf (stderr,"    [-w file_name] (name for output file)\n");
  fprintf (stderr,"    [-r file_name] (name for input file)\n");
  fprintf (stderr,"If either -w or -r is omitted, stdout(stdin) is used\n");
  fprintf (stderr,"\n");
  exit(-1);
}

  FILE *dumpFP, *outFP;
  FILE *logFP;

  struct timeval recvTime;
  struct timeval lastTime = {0,0};

  char ts[20]; /* ASCII timestamp in tcpdump output */
               /* max. ssssssssss.mmmmmm characters + EOL */
  char sh[25]; /* ASCII source host.port in tcpdump output */
               /* max. hhh.hhh.hhh.hhh.ppppp + EOL */
  char gt[3];  /* ">" symbol in tcpdump output */
  char lt[3];  /* "<" symbol in tcpdump output */
  char dh[25]; /* ASCII destination host.port in tcpdump output */
               /* max. hhh.hhh.hhh.hhh.ppppp + EOL */
  char fl[5];  /* ASCII TCP flags field in tcpdump output */
  char p1[50]; /* ASCII first field after flags in tcpdump output */
  char p2[50]; /* ASCII second field after flags */
  char p3[50]; /* ASCII third field after flags */

/* These are read from the tcpdump records */
  unsigned long begin_seq, end_seq, seq_bytes, new_ack;
  unsigned long current_synseq;

  int has_seq, has_ack;

/* In TCP ACKs should be monotonically increasing so the length of the current
   request in a sequence can be computed as the difference between the
   ACK value marking the end of the previous request and the ACK value
   marking the end of the current request.  Unfortunately, out-of-order
   segments can cause ACKs to appear as if they "go backward".  For segments
   without data (ACK_ONLY), simply ignoring the "backward" ACK is fine. 
   In many cases where there is data along with a "backward" ACK,
   this is the result of (un-necessary) retransmission of segments
   containing data and those cases are handled properly.  In other cases,
   the results can be incorrect for HTTP 1.1 connections.  Each case of
   suspected ACK mis-ordering that might lead to erroneous request or
   response lengths is noted for off-line investigation.
   */

  unsigned long current_request_end, last_request_end;

/* In TCP segments the sequence numbers we see may NOT be monotonically
   increasing (retransmission, out-of-order).  Instead we record the 
   largest value seen at any point.  Then the length of the current
   response in a sequence can be computed as the difference between the
   (largest) sequence at the end of the previous response and the
   (largest) sequence at the end of the current response.  */

  unsigned long current_response_end, last_response_end;

/* Various counters for logging connection summary information */

  int syn_count = 0; /* connections beginning with SYN in trace */
  int req_count = 0; /* number of identified requests */
  int rsp_count = 0; /* number of identified responses */
  int fin_count = 0; /* connections ending with FIN in trace */
  int rst_count = 0; /* connections ending with Reset (no FIN) */
  int trm_count = 0; /* connections ending with no Reset or FIN */
  int err_count = 0; /* connections with at least one suspected error */
  int act_req_count = 0; /* connections beginning in a request */
  int act_rsp_count = 0; /* connections beginning in a response */
  int pending_fin_count = 0; /* partial with only FIN(s) */
  int pending_rst_count = 0; /* partial with only Reset(s) */
  int pending_ack_count = 0; /* partial with only ACK(s) */
  int pending_oth_count = 0; /* partial -- others */
  int pending_cmb_count = 0; /* partials with FIN, Reset, ACK combinations */

/* Various counters for events within a single connection */
  /* the following apply only when state == PENDING; see pending_xxx_counts
     above for explanations */
  int have_pending_acks = 0;
  int have_pending_fins = 0;
  int have_pending_rsts = 0;
  int have_pending_othr = 0;
  /* the following apply only when state != PENDING */
  int have_ACK_error = 0;   /* seen "backwards" ACK */
  int have_value_error = 0; /* seen suspect ACK or sequence # */
  int have_FINdata_error = 0; /* seen data after FIN */

  enum states {PENDING, SYN_SENT, FIN_SENT, RESET, IN_REQUEST, IN_RESPONSE};

  enum states connection_state = PENDING;
  enum states last_state = PENDING;

  enum inputs {SYN, FIN, RST, ACK_ONLY, DATA_ACK};
  enum inputs input_type;

  char current_src[25] = "";
  char src_host[25];
  char src_port[10];

  char current_dst[25] = "";
  char dst_host[25];
  char dst_port[10];


/* A request is considered to start at the timestamp on the tcpdump record
   containing the first advance in the ACK field (a) following the connection
   establishment or (b) while the connection is sending response data. */

  char start_request_time[20];

/* A response is considered to start at the timestamp on the tcpdump record
   containing the first advance in the sequence number field following a
   period when the ACK has been advanced (receiving request data).  */

  char start_response_time[20];

/* A response is considered to end at the timestamp on the tcpdump record 
   that LAST advanced the data sequence number before the response ends 
   (a new request starts, a FIN is sent, etc.).  Since we would have to
   look ahead in the trace to be sure the current record has the LAST 
   advance, we go ahead and assume it is the last and record its 
   timestamp as response_end_time.  This way, when we know the sequence
   number will not advance more for this response from processsing some 
   subsequent record in the trace (FIN or ACK advance), the last timestamp
   has already been saved.  Similarly, a request is considered to end at
   the timestamp on the last record of the request. */
  
  char response_end_time[20];
  char request_end_time[20];

  char FIN_sent_time[20];   
  char RST_sent_time[20];
  char last_connection_time[20];

  char input_name[256] = "";
  char output_name[256] = "";
  char log_name[256] = "";

  char new_line[500];  /* assumes no tcpdump output line is longer */

  long elapsed;

  int new_address = 0;
  int rc = 0;

void error_line(char *s);  
void error_state(char *s);
int parse_dump_record(void);
void init_connection(void);
void init_active(void);
void check_tuple_reuse(void);
int check_ACK_advance(unsigned long old_ack);
void begin_REQ(void);
void more_REQ(void);
void begin_RSP(void);
void more_RSP(void);
void log_REQ(void);
void log_RSP(void);
void log_SYN(void);
void log_END(char *how);
void log_ACT(char *how);
void log_nosyn(void);
void log_connection(void);
void log_log(void);
long elapsed_ms(char *end, char *start);
void get_host_port(char *adr, char *host, char *port);
int  get_sequence(char *p, unsigned long *begin, unsigned long *end,
                           unsigned long *bytes);

void main (int argc, char* argv[])
{
  int i;

  /* Parse the command line */
  i = 1;
  while (i < argc) {
    if (strcmp (argv[i], "-r") == 0) {
      /* -r flag is followed by name of file to read */
      if (++i >= argc) Usage (argv[0]);
      strcpy (input_name, argv[i]);
    }
    else if (strcmp (argv[i], "-w") == 0) {
      /* -w flag is followed by the name of file to write */
      if (++i >= argc) Usage (argv[0]);
      strcpy (output_name, argv[i]);
    }
    else 
      Usage (argv[0]);
    i++;
  }

  /* Open files */
  /* Note: program is written to also be used as a filter */

  if (strcmp(output_name, "") == 0) 
    /* if no explicit output file named with -w, use stdout */
     outFP = stdout;
  else 
     {
      if ((outFP = fopen (output_name, "w")) == NULL) {
          fprintf (stderr, "error opening %s\n", output_name);
          exit (-1);
          }
     }

  if (strcmp(input_name, "") == 0)
    /* if no explicit input file named with -r, use stdin */
     dumpFP = stdin;
  else 
     {
      if ((dumpFP = fopen (input_name, "r")) == NULL) {
          fprintf (stderr, "error opening %s\n", input_name);
          exit (-1);
         }
     }
  
  strcpy(log_name, output_name);
  strcat(log_name, ".log");
  if ((logFP = fopen (log_name, "w")) == NULL) {
      fprintf (stderr, "error opening %s\n", log_name);
      exit (-1);
     }

  /* begin main loop; once through loop for each line in the tcpdump */
  /* a <continue> anywhere in the loop (usually after an error case)
     implies beginning of processing a new tcpdump record */

  while (!feof (dumpFP)) {

/* printf("State is %d, last_state is %d\n", connection_state, last_state); */

    /* Get and parse line of tcpdump file */

    fgets (new_line, sizeof(new_line), dumpFP);

    /* get line pieces; this works because there are always 8 or more 
       fields separated by white space in tcpdump ASCII-format lines */
/*    sscanf (new_line, "%s %s %s %s %s %s %s %s %s",
                      &ts, &lt, &sh, &gt, &dh, &fl, &p1, &p2, &p3);
*/
    sscanf (new_line, "%s %s %s %s %s %s %s %s",
                      &ts, &sh, &gt, &dh, &fl, &p1, &p2, &p3);

   /* If any part of the connection tuple (source host.port,destination
      host.port) differs from the current values, there are no more 
      tcpdump records for that connection so treat as end of connection.
      The action taken at the end of a connection depends on the current
      state of processing in that connection */  

   if ((strcmp(current_src, sh) != 0) ||  /* new source host/port */
       (strcmp(current_dst, dh) != 0))    /* new dest. host/port */
       {
        log_connection(); 

        /* begin processing this record as being from a potential new
           TCP connection so initialize connection's state */

        strcpy(current_src, sh);  /* new connection tuple */
        strcpy(current_dst, dh);  /* host and port for src & dest */

        have_pending_acks = 0;
        have_pending_fins = 0;
        have_pending_rsts = 0;
        have_pending_othr = 0;

        current_synseq = 0;
        connection_state = PENDING;  /* unconnected pending a SYN */
        last_state = PENDING;
        new_address = 1;        /* true only for very first record
                                   from a different TCP connection -- 
                                   avoids multiple error messages */
       }
   
   /* break dump record into essential data fields; initializes the
      following variables: begin_seq, end_seq, seq_bytes, new_ack,
                           has_ack, has_seq, input_type.
      Also checks for suspect sequence and ACK values.
   */

   if ((rc = parse_dump_record()) < 0) 
       continue;

   /* processing records from a TCP connections is based on a notion of
      the current "state" of the connection.  The defined states are
        PENDING := record is from different connection than before but
                   a beginning SYN has not yet been identified.
        SYN_SENT:= have identified SYN sent from source port 80 (server)
        FIN_SENT:= have identified a FIN sent from source port 80 (server)
                   This terminates any data from server.
        RESET   := have identified a Reset sent from source port 80 (server)
                   This means that the server should not accept any more
                   client data.
        IN_REQUEST := processing ACKs sent from source port 80 (server)
                      in response to data (request) from the client.  In
                      this state, need to identify end of client data (request)
                      and start of server data (response).
        IN_RESPONSE := processing data sequence #s from source port 80
                       In this state, need to identify the end of server data
                       (response) and, possibly the beginning of a new request.
      Whenever the state changes, the prior state is also noted.

      Fundamental to all of this is the notion that a server cannot  
      possibly be sending data in response to a request unless that
      data is accompanied or preceeded by an advance in the ACK sequence
      indicating receipt of the request data.  Similarly, we assume that
      any new data sent by a server that follows or is accompanied by an
      advance in the ACK sequence number is a response to the request 
      that caused the ACK sequence to advance.  Put another way, response
      data (sequence # advance) marks the end of a request and ACK 
      advance marks the end of a response.  Of course other events such
      as FIN or Reset can mark ends also.  The use of ACK advance to mark
      the end of a response assumes that HTTP/1.1 browsers don't overlap 
      requests on a single TCP connection even if they may "batch" requests, 
      i.e., a new request will not be generated until the response has 
      been received. If requests and responses are batched but not 
      overlapped this will understate the number of objects requested 
      and overstate request and response sizes.

      This use of advancing sequence numbers (ACK or data) to mark
      requests and responses is disturbed by segment reorderings in 
      the network.  In some cases, such as reordering of only data segments
      in a response, there is no problem since only the highest value seen
      is used.  Reordering of ACKs (especially with data) presents real
      problems since boundaries between requests and responses are missed
      which can result in overstating request and response sizes.  For this
      reason, all such cases of ACK misordering are logged and reported.
   */

   switch (connection_state)

     /* a <break> anywhere ends processing of the current tcpdump record 
        by ending the switch (which continues the main read loop) */

      {
       case PENDING:
	 {
          switch (input_type)
             {
	     case FIN:
	        {
		 /* Ignore random FIN before something useful */ 

                 have_pending_fins++;
                 break;
                }

	     case SYN:
	        {
		  /* normal connection start, initialize connection state */

                 init_connection();
                 break;
		}

	     case RST:
                {
		  /* Ignore random Reset before something useful */

                 have_pending_rsts++;
                 break;
                }

             /* The trace may start after a connection is established and we
                do not see a SYN.  Determine if the connection is most likely
                in a request or in a response and log its status.  As before,
                a request is indicated if the ACK sequence (after the first
                one in the trace) is advancing and a response is indicated if
                the data sequence number is advancing. */

             case ACK_ONLY:
	        {
                  /* ignore initial ACK (has absolute value, not relative) */

                 if (new_address == 1)
		   {
                    new_address = 0;
                    have_pending_acks++;
                   }
                 else
		    {
		     /* If ACK advances, the connection is in a request */

                     if ((new_ack > 2) &&
                         (new_ack < 16384))
		        {
                         log_ACT("REQ");

                         last_request_end = 1;
                         current_request_end = new_ack;

                         last_response_end = 1;
                         current_response_end = 1;

                       /* record the request start time as beginning at the 
                          tcpdump time stamp on this record */

                          strcpy(start_request_time, ts);
                          strcpy(request_end_time, ts);

                          last_state = connection_state;
                          connection_state = IN_REQUEST;
 
                          init_active();  /* initialize connection state */
			 }
                     else /* ignore ACKs that are not advancing */
                        have_pending_acks++;
		    }
                 break;
                }

	     case DATA_ACK:          
	        {
		 /* If data sequence advances, connection is in response */

                 if ((seq_bytes > 1) &&
                     (seq_bytes < 65535))
		    {
                     log_ACT("RSP");

                     /* assume tcpdump relative addressing and initialize */

                     last_request_end = 1;    
                     current_request_end = 1;

                     last_response_end = 0;     
                     current_response_end = seq_bytes;

                     /* record timestamp of tcpdump record as current
                        value of both start and end times of response
                        (in case no later end time is found) */

                     strcpy(start_response_time, ts); 
                     strcpy(response_end_time, ts);

                     last_state = connection_state;
                     connection_state = IN_RESPONSE;
 
                     init_active();
                    }
                 else /* ignore data lengths of 0 or 1 */
                    have_pending_othr++;
                 break;
                }
	     default:
                break;
             }  /* end switch on input_type */
          break;
	 } /* end case PENDING */

       case SYN_SENT:
         {

         /* Treat this case as the establishment of a connection.  Usually
            the first activity on the connection will be request data from
            the client, but some servers appear to "pre-send" data (maybe
            the headers) speculatively before ACKing any client data. */

          switch (input_type)
             {
	     case FIN:

              /* A FIN marks the end of either or both request and response */

	        {
                 if ((has_ack == 1) &&
                     (new_ack > (current_request_end + 1)))
                               /* ignore possible ACK of FIN */

	           { /* this record had advanced the ACK sequence so
                        save current request data and log it (since
                        the FIN ends the connection it also ends the 
                        request). */
               
                     begin_REQ();
                     log_REQ();
                    }

                 /* If the data sequence number advances, then there was
                    response data.  Save the current response info. and
                    log it (since the FIN ends the connection, it also
                    ends the response). */

                 if ((has_seq == 1) &&
                     (end_seq > current_response_end))  
		    {
                     begin_RSP();
                     log_RSP();
                    }
                 last_state = SYN_SENT;
                 connection_state = FIN_SENT;

                 /* record timestamp of first FIN seen on connection */
                 if (strcmp(FIN_sent_time, "") == 0)
                    strcpy(FIN_sent_time, ts);
                 break;
                }

	     case SYN:
	        {
		  /* In some cases the same host/port pairs are reused in a
                     single trace; check for plausible reuse */

                 check_tuple_reuse();
                 break;
		}

	     case RST:
                {
                 connection_state = RESET;
                 last_state = SYN_SENT;

                 /* record timestamp of first Reset on the connection */

                 if (strcmp(RST_sent_time, "") == 0)
                    strcpy(RST_sent_time, ts);
                 break;
                }

             case ACK_ONLY:
	        {
                 /* since there is no data sequence # present in the record
                    and, therefore, the server is not sending data so
                    all the client's data may not have arrived.  Note the
                    current extent of ACKed client data and change state */

                 if (new_ack > (current_request_end + 1))
                    begin_REQ();
                 break;
                }

	     case DATA_ACK:          
	        {
                /* check for presence of data sequence # in the common cases
                   in SYN_SENT state, data presence indicates that 
                   request data has been received and server is sending data
                   that should be a response */

                 if (new_ack > (current_request_end + 1)) 
        	    {/* this record had advanced the ACK sequence so
                     save current request info. and change state */

                     begin_REQ();

                     /* the server's data sequence may not have advanced;
                        but if it has, treat it as the start of a response
                        and the end of the client (request) data */

                     if (end_seq > current_response_end)
		        {
                          /* start of response ends the request, log
                          it and change state to look for end of response */  

                         log_REQ();
                         begin_RSP();
		        }
		    }
                 else 
		   /* some servers appear to send response data immediately
                      on completing the TCP connection without receiving 
                      any request data */

                    if ((end_seq > last_response_end) && 
                        (seq_bytes > 0))
                       begin_RSP();
                break;
		}
	     default:
                break;
	     } /* end switch on input type */
          break;
	 } /* end case SYN_SENT */

       case FIN_SENT:
	  {
           switch (input_type)
              {
	       case SYN:
	          {
		  /* In some cases the same host/port pairs are reused in a
                     single trace; check for plausible reuse */

                   check_tuple_reuse();
                   break;
		  }

	        case FIN:  /* ignore multiple FINs */
                   break;

                case RST:  /* ignore reset after FIN */
                   break;

	        case ACK_ONLY:
                    /* If there is an ACK (only) coming after a FIN is sent, it
                      is ether the normal one in the 4-way termination or it
                      is to ACK more request data which will be ignored (there
                      can be no response).  
                      In either case, it is ignored here. */

                   break;

	        case DATA_ACK:
		  {

              /* All others may have sequence number fields.  Treat them as
                 errors if they advance the sequence number after a FIN.  
                 The usual case here is just retransmissions (including 
                 retransmission of the FIN) which should not advance it
                 because the highest sequence number possible was on the FIN */

                   if ((end_seq > (current_response_end + 2)) &&
                       (have_FINdata_error == 0))
		      {
                       error_state("new data in FIN_SENT state");
                       have_FINdata_error = 1;
                      }
                   break;
		  }
	        default:
                   break;
	      } /* end switch on input type */
            break; 
	  } /* end case FIN_SENT */

       case RESET:
	   {
	    /* A Reset nominally means an abnormal close of the connection 
               with any queued data discarded before transmitting it.  We
               observe, however, that under some (unknown) circumstances
               segments that advance the data sequence number continue in 
               the trace after the Reset.  If these are part of a response, 
               it makes sense to count them in the response size since 
               the server obviously sent them and the network has to handle 
               them.  This also means that a FIN following the Reset may be
               a valid indication of the end of a response. Just ignore 
               anything else unexpected (e.g., Reset) */

            switch (input_type)
               {
	        case RST:
                   break;

	        case SYN:
	          {
		  /* In some cases the same host/port pairs are reused in a
                     single trace; check for plausible reuse */

                   check_tuple_reuse();
                   break;
		  }

	        case ACK_ONLY:

               /* an ACK might advance possibly indicating a request.  However,
                  since the connection is Reset and there should be no new
                  response, just ignore it */

                    break;

	        case FIN:
		  {

            /* Only if the state is IN_RESPONSE do we try to continue looking
               for the end of the response.  FIN is treated as the true end
               of a response. */

                   if (last_state == IN_RESPONSE)
        	      {/* ends the current response, ignore any ACK */
                       if ((has_seq == 1) &&
                           (end_seq > current_response_end)) 
                          more_RSP();
                       log_RSP();
		  
                       last_state = RESET;
                       connection_state = FIN_SENT;

                       /* record timestamp of first FIN on connection */

                       if (strcmp(FIN_sent_time, "") == 0)
                          strcpy(FIN_sent_time, ts);
		      }
                    break;
		   }

	       case DATA_ACK:
                 {
                /* segments with data may advance the data sequence number as 
                   more parts of the response.  They may also advance the ACK
                   which normally would indicate a new request.  However,
                   after a Reset, it probably will be ignored by the 
                   server so it is also ignored here except to mark the end
                   of the response. */

                   if (last_state == IN_RESPONSE)
		      {
                       if (new_ack > (last_request_end + 1)) /* new request */
                          { /* implies end of current response */
                           log_RSP(); 
                           last_state = RESET; /* will ignore all else */
                           break; 
                          }
 
                     /* As long as the data sequence # advances, continue to
                        save info about the current response. */

                       if ((end_seq > current_response_end) &&
                           (seq_bytes > 0))
                          more_RSP();
                      }
                  break;
		 }
               default:
                  break;
	       } /* end switch on input type */
            break;
           } /* end case RESET */

       case IN_RESPONSE:
	   {
            /* In this state, look for events that will indicate the end of
               the response data (ACK advances for request, FIN, Reset).  If
               the event is ACK advance, this also initiates the start of a
               new request.  */

            switch (input_type)
               {
	        case FIN:
		  {         
                   /* FIN is complicated since the segment that carries it may
                    also advance the ACK (new request), advance the data
                    sequence # and end the connection.  If the ACK advances,
                    any sequence # advance is for the new request; otherwise
                    a sequence # advance extends and completes the current 
                    response. */

                   if (has_ack == 1) 
		      {
		       if ((rc = check_ACK_advance(current_request_end)) < 0)
                          break;
                      }

		   /* The ACK advance amount has to be greater than 1 to be
                     considered a real new request.  This is primarily to
                    filter out just an ACK for a FIN from the client.  Note
                    the ACK must advance so duplicate ACKs are ignored. */

                   if ((has_ack == 1) &&
                       (new_ack > (current_request_end + 1)))
      	              { /* implies end of current response, begin request 
                           Log the info. for the current response. */

                       log_RSP();  /*end of previous response */

                       /* on FIN, assume end of request also since server is
                          closing the connection. */

                       begin_REQ();
                       log_REQ(); 

                       /* if the data sequence # also advances, this segment
                          (only -- because of the FIN) carries the last part
                          of the response */
 
                       if ((has_seq == 1) &&
                           (end_seq > last_response_end)) 
                          {/* response begins and ends in the FIN segment */
                           begin_RSP();
                           log_RSP();  /* also end of any response */
                          }

                       /* Note that the null "else" here is the case that the
                          data sequence # does not advance (there is no 
                          response to the request) -- probably an HTTP/1.1
                          broswer attempting multiple requests on a TCP 
                          connection where the server doesn't play nice. */

                      }
                   else 
                      /* the ACK did not advance so no request; if the data
                       sequence # advances, it extends the current response */

		      {
                       if ((has_seq == 1) &&
                           (end_seq > current_response_end)) 
                          more_RSP();
                       log_RSP();    /* FIN always implies end of response */
                      }

                   last_state = IN_RESPONSE;
                   connection_state = FIN_SENT;

                   if (strcmp(FIN_sent_time, "") == 0)
                      strcpy(FIN_sent_time, ts);
                   break;
		  }

               case RST:

                 /* Look for a Reset and only change state to RESET to
                   continue processing.  This is explained in comments in 
                   processing for the RESET state when the last_state is
                   IN_RESPONSE. */

 	         {/* ignore any advance in seq# on Reset */
                  /* it may not be wise to trust ACK on RST */

                  last_state = IN_RESPONSE;
                  connection_state = RESET;   

                  if (strcmp(RST_sent_time, "") == 0)
                     strcpy(RST_sent_time, ts);
                  break;
                 }

               case SYN:
 	         {
		  /* In some cases the same host/port pairs are reused in a
                     single trace; check for plausible reuse */

                  check_tuple_reuse();
                  break;
		 }

               case ACK_ONLY:

  	         /* Begin checking for cases where the ACK may advance
                    and really indicate that a new request starts. */

                 {
 		  /* The ACK advance amount has to be greater than 1 to be
                     considered a real new request.  This is primarily to
                     filter out just an ACK for a FIN from the client.  Note
                     the ACK must advance so duplicate or OOO
                     ACKs are ignored. */

                  if (new_ack > (last_request_end + 1)) 
		     { /* implies end of current response, begin request.
                         Log the info. for the current response and change
                         states. */
                      log_RSP();

                    /* since there is no data sequence # present in the record
                    and, therefore, the server is not sending data so
                    all the client's data may not have arrived.  Note the
                    current extent of ACKed client data and start time */

                      begin_REQ();
                     }
                  break;
                 }

               case DATA_ACK:

            /* cases with data and ACKs have two important sub-cases: (a) 
               ACK sequence number advances -- implies the start of a
               new request and the end of the current response; if the
               data sequence number also advances, it is assumed that
               all the request data has been received and the first of
               the response data is contained in the segment. (b) the
               ACK sequence number does not advance so any advance in
               the data sequence number is more of the current response. */

	       {
		if ((rc = check_ACK_advance(last_request_end)) < 0)
                   break;

		 /* The ACK advance amount has to be greater than 1 to be
                    considered a real new request.  This is primarily to
                    filter out just an ACK for a FIN from the client.  Note
                    the ACK must advance so duplicate ACKs are ignored. */

                if (new_ack > (last_request_end + 1)) 
	           { /* implies end of current response, begin request 
                         Log the info. for the current response. */

                    log_RSP();  
                    begin_REQ();

                    /* If there is also new data (sequence # advances), then
                       the request is considered completed and a new response
                       has started but not necessarily completed. */

                    if ((end_seq > current_response_end) &&
                        (seq_bytes > 0))
		       {/* Log info. about this request */
                        log_REQ();
                        begin_RSP();
		       }
		   }
                else
                    /* the ACK did not advance so no request; if the data
                       sequence # advances, it extends the current response */

                    if (end_seq > current_response_end) 
                       more_RSP();
                break;
               }
	     default:
                break;
	    } /* end switch on input type */
           break;
	  }  /* end case IN_RESPONSE */

       case IN_REQUEST:
	   {
	    /* In this state check for events that indicate the end of the
               request data (advance of data sequence #, FIN, Reset).  An
               advance in the data sequence # also marks the beginning of
               the response to the request */

            switch (input_type)
               {
	        case FIN:

            /* FIN is somewhat less complicated here since the segment that
               implies that the server will send no more data.  Treat this
               as effectively ending the request.  If the ACK advances, it
               is considered as part of the request and if the data 
               sequence # advances, it is the response data (all in this
               segment). */

	        { /* FIN can ACK more of current request */

		  /* The ACK advance amount has to be greater than 1 to be
                    considered as extending the request.  This is primarily 
                    to filter out just an ACK for a FIN from the client.  Note
                    the ACK must advance so duplicate ACKs are ignored. */

                 if ((has_ack == 1) &&
                     (new_ack > (current_request_end + 1)))
                    more_REQ();
                 else
                    if (has_ack == 1)
		       {
		        if ((rc = check_ACK_advance(current_request_end)) < 0)
                           break;
                       }

                 log_REQ();  /* on FIN, assume end of request */

                /* the server's data sequence may not have advanced;
                   but if it has, treat it as the complete response */

                 if ((has_seq == 1) &&
                     (end_seq > last_response_end))  
                    {
                     begin_RSP();
                     log_RSP();  /* also end of any response */
                    }

                 last_state = IN_REQUEST;
                 connection_state = FIN_SENT;

                 if (strcmp(FIN_sent_time, "") == 0)
                    strcpy(FIN_sent_time, ts);
                 break;
		}

	     case RST:
               {
                /* Treat a Reset as ending the request with no response */
	        /* ignore any advance in ack on Reset */
		/* treat as ending any request */

                log_REQ();                
                last_state = IN_REQUEST;
                connection_state = RESET;

                if (strcmp(RST_sent_time, "") == 0)
                   strcpy(RST_sent_time, ts);
                break;
	       }

	     case SYN:
	        {
		 /* In some cases the same host/port pairs are reused in a
                    single trace; check for plausible reuse */

                 check_tuple_reuse();
                 break;
		}

	     case ACK_ONLY:
	        {
		 /* The ACK advance amount has to be greater than 1 to be
                    considered as extending the requesst.  This is primarily 
                    to filter out just an ACK for a FIN from the client.  Note
                    the ACK must advance so duplicate or OOO 
                    ACKs are ignored. */

                 if (new_ack > (current_request_end + 1))
                    more_REQ();
                 break;
                }

	     case DATA_ACK:
               {
                /* cases with data and ACKs have two important sub-cases: 
                  (a) ACK sequence number advances which extends the amount
                  of request data, and (b) the data sequence # advances
                  which ends the request and is the beginning of the 
                  response to that request. */
		/* The ACK advance amount has to be greater than 1 to be
                   considered as extending the request.  This is primarily 
                   to filter out just an ACK for a FIN from the client.  Note
                   the ACK must advance so duplicate ACKs are ignored. */

                if (new_ack > (current_request_end + 1))
                   more_REQ();
                else
		    if ((rc = check_ACK_advance(current_request_end)) < 0)
                       break;

                /* the server's data sequence may not have advanced;
                   but if it has, treat it as the start of a response
                   and the end of the request data */

                if ((end_seq > last_response_end) && 
                    (seq_bytes > 0))
                   {
		    /* record info. for current request and change state 
                       to look for the end of the response*/

                    log_REQ();
                    begin_RSP();
                   }
                break;
               }
	     default:
                break;
	    } /* end switch on input type */ 
          break;
	 }  /* end case IN_REQUEST */

       default:
       break;
      } /* end switch on connection state */

   /* save the last known time for the connection from current record */
   strcpy(last_connection_time, ts); 

 }  /* end main loop */  
  log_log();
  close (dumpFP);
  close (outFP);
  close (logFP);
} /* end main() */


/* Initialize connection state when a new connection is recognized by
   the presence of a SYN. */ 

void init_connection(void)
{
 log_SYN();                /* log start of connection */
 connection_state = SYN_SENT;  /* new connection state */

 /* save SYN sequence number for duplicate detection */
 if (has_seq == 1)
     current_synseq = begin_seq;
 else
     error_line ("SYN without valid sequence #");

 /* assume tcpdump relative addressing and initialize */
 /* note that initializing to 1 instead of 0 adjusts for ACK
    being the next expected sequence number */

 last_request_end = 1;       /* need "last" and "current" */
 last_response_end = 1;      /* values since there may be */
 current_response_end = 1;   /* > 1 request per connection */
 current_request_end = 1;

 strcpy(FIN_sent_time, "");
 strcpy(RST_sent_time, "");
 strcpy(last_connection_time, "");

 have_ACK_error = 0;
 have_value_error = 0;
 have_FINdata_error = 0;
}

/* Initialize connection state when a new connection is recognized by a
   change in the host/port 4-tuple, there is no SYN but request or 
   response activity can be determined.  Request or response state is
   initialized in the main program when the type of activity is determined */ 

void init_active(void)
{
 strcpy(FIN_sent_time, "");
 strcpy(RST_sent_time, "");
 strcpy(last_connection_time, "");

 have_ACK_error = 0;
 have_value_error = 0;
 have_FINdata_error = 0;

}

/* Check for some indications of possible errors 
   such as the ACK appearing to move back -- may indicate
   out-of-order or something worse. */

int check_ACK_advance(unsigned long old_ack)
{
 
 if ((new_ack < old_ack) &&
      report_ACK_err)
    {
     if (have_ACK_error == 0)
        {
         error_state("ACK error -- backward");
         have_ACK_error = 1;
        }
     return(-1);
    }
 else
    return (0);
}

/* Check to see if reuse of a connection in a trace is reasonable */

void check_tuple_reuse(void)
{
 /* ignore duplicate SYNs (have same SYN sequence number) */

 if ((has_seq == 1) &&
     (current_synseq != begin_seq))
    {
      /* if a non-duplicate SYN comes before any other activity,
         treat it as terminating the incomplete connection and starting
         another */
     
     if ((connection_state == SYN_SENT) ||
         ((connection_state == RESET) && (last_state == SYN_SENT)))
        {
         log_END("TRM");
         init_connection();
         return;
        }


    /* It is quite possible to have a valid new
    connection that reuses the same source and destination
    IP.port address 4-tuple.  Treat this as valid if 
    there has been a FIN from the server or at least
    2*MSL has passed since seeing any previous packet
    from this connection (in case we missed the FIN).  If there
    was a FIN from the server, allow for the case it was an
    active close by the server (normal for HTTP 1.0), the 
    stack is a BSD derivative, and SO_REUSEADDR is is use
    (see Stevens vol. 1, pp 245) */ 

     if (connection_state == FIN_SENT)
        elapsed = 60001;  /* force beyond 2MSL test */
     else        
        elapsed = elapsed_ms(ts, last_connection_time);

     if (elapsed < 60000)  /* MSL of 30 seconds, minimum */
        error_state("Non-duplicate SYN in connection");
     else
	{
	 /* perform all actions associated with the end of
            one connection and the beginning of the next 
            (see state PENDING) */
         switch (connection_state)
	    {
	    case FIN_SENT:
                 log_END("FIN");
                 break;
            case RESET:
                 if (last_state == IN_RESPONSE)
                    log_RSP();

                 log_END("RST");
                 break;
            case IN_RESPONSE:
                 log_RSP();
                 log_END("TRM");
                 break;
            case IN_REQUEST:
                 log_REQ();
                 log_END("TRM");
                 break;
            default:
                 break;
	    }
         init_connection();
        }
    }
}

/* Record information about the beginning of a new request */

void begin_REQ(void)
{
 current_request_end = new_ack;

 /* record the request start time as beginning at the 
    tcpdump time stamp on this record */

 strcpy(start_request_time, ts);
 strcpy(request_end_time, ts);

 last_state = connection_state;
 connection_state = IN_REQUEST;
}

/* Record information about an in-progress request */

void more_REQ(void)
{
 current_request_end = new_ack;
 strcpy(request_end_time, ts);
}

/* Record information about the beginning of a new response */

void begin_RSP(void)
{
 current_response_end = end_seq;

 /* record timestamp of tcpdump record as current
 value of both start and end times of response
 (in case no later end time is found) */

 strcpy(start_response_time, ts); 
 strcpy(response_end_time, ts);

 last_state = connection_state;
 connection_state = IN_RESPONSE;
}

/* Record information about an in-progress response */

void more_RSP(void)
{
 current_response_end = end_seq;
 /* save the new (potential) response end time */
 strcpy(response_end_time, ts);
}


/* Breaks dump record into essential data fields; initializes the
   following variables: begin_seq, end_seq, seq_bytes, new_ack,
                        has_ack, has_seq, input_type.
   Also checks for suspect sequence and ACK values.
*/

int parse_dump_record()
{
    begin_seq = end_seq = seq_bytes = new_ack = 0;
    has_ack = has_seq = 0;

   /* The following flag combinations are flagged because they (a) make no
      real sense for SYNs and (b) should be very rare. */

    if ((strcmp(fl, "SFRP") == 0) ||
	(strcmp(fl, "SFR") == 0) ||
	(strcmp(fl, "SFP") == 0) ||
	(strcmp(fl, "SF") == 0) ||
	(strcmp(fl, "SRP") == 0) ||
	(strcmp(fl, "SR") == 0))
       {
	 /* If out of PENDING state (initial SYN recognized), just note
            the error, ignore it and continue.  */

        if (connection_state != PENDING)
            error_line ("SYN in combination with F or R");
        return(-1);
       }

    /* In tcpdump format, the fields coming after flags are, in order,
       data-seqno (format "bbb:eee(ccc)") and ack (format "ack xxx").
       Both the data-seqno and ack fields may not be present if they
       do not have valid information.  If the data-seqno field is not
       present, the first field after the flag is the string "ack";
       if both are not present the field after the flag is the 
       string "win".  Also check to see if tcpdump used absolute instead
       of relative values */

    if (strcmp(p1, "ack") == 0)  /* ack and no data sequence no. */
       {
        has_seq = 0;
        has_ack = 1; 
        new_ack = strtoul(p2, (char **)NULL, 10);
       }
    else
       {
        if (strcmp(p1, "win") == 0) /* no ack, no data */
	   {
            has_ack = 0;
            has_seq = 0;
           }
        else 
	   {
            /* assume it is a valid sequence number field */
	    /* parse sequence field in header */
	    /* sequence field format is
               <begin_seq #>:<end_seq #>(<seq_bytes count>) */

            if ((rc = get_sequence(p1, &begin_seq, &end_seq, &seq_bytes)) < 0)
	       {
                error_line ("invalid sequence # field");
                return (-1);
               }
            has_seq = 1;

            /* check the field following the data sequence # for an ACK */

            if (strcmp(p2, "ack") == 0)  
               {
                has_ack = 1; 
                new_ack = strtoul(p3, (char **)NULL, 10);
               }
            else
               has_ack = 0;
           }
       }

   /* classify flag combinations into equivalence classes for later steps */

      if ((strcmp(fl, "F") == 0) || 
          (strcmp(fl, "FP") == 0) ||
          (strcmp(fl, "FR") == 0) ||
          (strcmp(fl, "FRP") == 0))
         input_type = FIN;
      else
	 {
          if ((strcmp(fl, "R") == 0) ||
              (strcmp(fl, "RP") == 0))
             input_type = RST;
          else
	     {
              if ((strcmp(fl, "S") == 0) || 
                  (strcmp(fl, "SP") == 0))
                 input_type = SYN;
              else
                {
                 if ((has_ack == 1) &&
                     (has_seq == 0))
                    input_type = ACK_ONLY;
                 else
                    if ((has_seq == 1) &&
                        (has_ack == 1))
                       input_type = DATA_ACK;
                    else
		       {   
                        error_line("Unexpected Data/ACK combination");
                        return (-1);
                       }
		}
	     }
	 }


      if ((connection_state == IN_RESPONSE) ||
          (connection_state == IN_REQUEST) ||
          (connection_state == SYN_SENT) ||
          ((connection_state == RESET) && (last_state == IN_RESPONSE)))
	 {   
	   /* allow for gaps of up to one normal TCP window */

          if (((input_type == FIN) ||
               (input_type == DATA_ACK)) &&
               (end_seq > (current_response_end + 65535)))
	     {
              if (have_value_error == 0)
		 {
                  error_line ("suspect sequence # value");
                  have_value_error = 1;
                 }
              return (-1);
             }

          /* ACK is required for each 2 segments with space for gaps */

          if (((input_type == FIN) ||
               (input_type == ACK_ONLY) ||
               (input_type == DATA_ACK)) &&
               (new_ack > (current_request_end + 16384)))
	     {
              if (have_value_error == 0)
		 {
                  error_line ("suspect ACK value");
                  have_value_error = 1;
                 }
              return (-1);
             }
         }
 return(0);
}

/* Output data associated with ending a connection */

void log_connection(void)
{
/* if no more tcpdump records found while processing an http
   request, log (perhaps incomplete) client request */  

   if (connection_state == IN_REQUEST)
      log_REQ();
   else
      {
       /* if no more records found while processing an http 
          response, log (perhaps incomplete) response information */

       if ((connection_state == IN_RESPONSE) ||
           ((connection_state == RESET) && last_state == IN_RESPONSE))
	  { /* don't log if just ACKed 1 (assume  FIN) */
           if (current_response_end > (last_response_end + 1))
               log_RSP();
          }
      }

   /* make log entry indicating type of connection termination;
      entry for connection is made only if a valid start (SYN) was
      previously recognized */ 

   if (connection_state != PENDING)  /* saw SYN */
      {
       if (connection_state == FIN_SENT) 
           log_END("FIN");
       else
	  {
           if (connection_state == RESET) 
               log_END("RST");
           else
               log_END("TRM");
	  }
      }
   else
      {
       if (((have_pending_fins > 0) +
            (have_pending_rsts > 0) +
            (have_pending_othr > 0) +
            (have_pending_acks > 0)) > 1)
          pending_cmb_count++;
       else
	  {
           pending_fin_count += (have_pending_fins > 0);
           pending_rst_count += (have_pending_rsts > 0);
           pending_ack_count += (have_pending_acks > 0);
           pending_oth_count += (have_pending_othr > 0);
          }
      }
}


void log_log(void)
{
 fprintf(logFP, "Input tcpdump file: %s \n", input_name);
 fprintf(logFP, "Output connection file: %s \n", output_name);
 fprintf(logFP, "   SYNs     %8d \n", syn_count);
 fprintf(logFP, "   REQs     %8d \n", req_count);
 fprintf(logFP, "   ACT-REQs %8d \n", act_req_count);
 fprintf(logFP, "   RSPs     %8d \n", rsp_count);
 fprintf(logFP, "   ACT-RSPs %8d \n", act_rsp_count);
 fprintf(logFP, "   FINs     %8d \n", fin_count);
 fprintf(logFP, "   RSTs     %8d \n", rst_count);
 fprintf(logFP, "   TRMs     %8d \n", trm_count);
 fprintf(logFP, "   ERRs     %8d \n", err_count);
 fprintf(logFP, "Partial Connections:\n");
 fprintf(logFP, " FIN only   %8d \n", pending_fin_count);
 fprintf(logFP, " RST only   %8d \n", pending_rst_count);
 fprintf(logFP, " ACK only   %8d \n", pending_ack_count);
 fprintf(logFP, " Combos     %8d \n", pending_cmb_count);
 fprintf(logFP, " Other      %8d \n", pending_oth_count);
}

/* A set of event-specific data logging functions.  A critical part of
   the logging functions for Requests and Responses is to save the 
   "current" value of the sequence number (ACK or data) that marks the
   end of it as the "last" value.  This is done to tell when the 
   sequence number advances again for multiple request/response pairs
   in a connection and to allow computing its size as (current - last). */

void log_REQ(void)
{
/* parse sourse host/port */
  get_host_port(current_src, src_host, src_port);

/* parse destination host/port */
  get_host_port(current_dst, dst_host, dst_port);

  /* for requests we log the request start time  -- the tcpdump 
     timestamp on the first record associated with a request -- 
     along with the TCP connection information and the size of the 
     request data */

  fprintf(outFP, "%s %-15s %5s > %-15s %4s: REQ %12d  %s\n", 
                                    start_request_time,  
                                    dst_host, dst_port, src_host, src_port,  
                                    current_request_end - last_request_end,
                                    request_end_time);
  /* IMPORTANT */
  last_request_end = current_request_end;
  req_count++;
}

void log_RSP(void)
{
/* parse sourse host/port */
  get_host_port(current_src, src_host, src_port);

/* parse destination host/port */
  get_host_port(current_dst, dst_host, dst_port);

  /* for responses we log the response end time  -- the tcpdump 
     timestamp on the last record associated with a response -- 
     along with the TCP connection information, the size of the 
     response data, and the response start time -- the tcpdump
     timestamp on the first record associated with the response. */

  fprintf(outFP, "%s %-15s %5s > %-15s %4s: RSP %12d  %s\n", 
                                   response_end_time, 
                                   dst_host, dst_port, src_host, src_port,  
                                   current_response_end - last_response_end,
                                   start_response_time);
#ifdef FOO
  fprintf(outFP, "%s %-15s %5s > %-15s %4s RSP %d %s\n", start_response_time, 
                                   src_host, src_port, dst_host, dst_port, 
                                   current_response_end - last_response_end,
                                   response_end_time);
  fprintf(outFP, "%s %s > %s RSP %d\n", start_response_time, current_src, 
                                   current_dst, 
                                   current_response_end - last_response_end);
#endif
  /* IMPORTANT */
  last_response_end = current_response_end;
  rsp_count++;
}

void log_SYN(void)
{
/* parse sourse host/port */
  get_host_port(current_src, src_host, src_port);

/* parse destination host/port */
  get_host_port(current_dst, dst_host, dst_port);

  fprintf(outFP, "%s %-15s %5s > %-15s %4s: SYN\n", ts, 
                                     dst_host, dst_port, src_host, src_port);  
  syn_count++;
}

void log_END(char *how)
{
  char logical_end_time[20];
  
/* parse sourse host/port */
  get_host_port(current_src, src_host, src_port);

/* parse destination host/port */
  get_host_port(current_dst, dst_host, dst_port);

  if (strcmp(how, "FIN") == 0)
     {
      fin_count++;
      strcpy(logical_end_time, FIN_sent_time);
     }
  else
    {
     if (strcmp(how, "RST") == 0)
        {
         rst_count++;
         strcpy(logical_end_time, RST_sent_time);
        }
     else
        if (strcmp(how, "TRM") == 0)
	   {
            trm_count++;
            strcpy(logical_end_time, last_connection_time);
           }
    }

  /* for termination of a connection we record the tcpdump timestamp of
     the last record of any kind associated with that conneciton along
     with the TCP connection 4-tuple and the way the connection ended
     (FIN, Reset, or just no more records in the trace). */

  fprintf(outFP, "%s %-15s %5s > %-15s %4s: %s               %s\n", 
                                    last_connection_time, 
                                    dst_host, dst_port, src_host, src_port,  
                                    how, logical_end_time);
}

void log_ACT(char *how)
{
/* parse sourse host/port */
  get_host_port(current_src, src_host, src_port);

/* parse destination host/port */
  get_host_port(current_dst, dst_host, dst_port);

  /* for activity on a SYN-less connection we record the tcpdump timestamp
     of the first record of activiy associated with that conneciton along
     with the TCP connection 4-tuple and the way the connection started
     (Request or Response). */

  fprintf(outFP, "%s %-15s %5s > %-15s %4s: ACT-%s\n", ts, 
                                    dst_host, dst_port, src_host, src_port,  
                                    how);
  if (strcmp(how, "REQ") == 0)
     act_req_count++;
  else
     if (strcmp(how, "RSP") == 0)
        act_rsp_count++;
}

void error_line(char * s)
{
/* parse sourse host/port */
  get_host_port(sh, src_host, src_port);

/* parse destination host/port */
  get_host_port(dh, dst_host, dst_port);

  fprintf(outFP, "%s %-15s %5s > %-15s %4s: ERR: %s\n", ts, 
                                   dst_host, dst_port, src_host, src_port, s);
  err_count++;
}

void error_state(char * s)
{
/* parse sourse host/port */
  get_host_port(sh, src_host, src_port);

/* parse destination host/port */
  get_host_port(dh, dst_host, dst_port);

  fprintf(outFP, "%s %-15s %5s > %-15s %4s: ERR: %s\n", ts, 
                                   dst_host, dst_port, src_host, src_port, s);
  err_count++;
}

void get_host_port(char *adr, char *host, char *port)
{
 char *fp;
 char *fpx;
 char adr_field[50];

 strcpy(adr_field, adr);
 /* break string at '.' separating host and port fields (last in string) */
 fp = (char *)rindex(adr_field, '.');
 *fp = '\0';   /* replace '.' with string terminator */
 strcpy(host, adr_field); /* copies host name up to terminator */ 

 fp++;  /* move pointer past terminator to 1st char in port field */
 fpx = (char *)index(fp, ':');   /* see if we have the ':' after a dst port */
 if (fpx != NULL)
     *fpx = '\0';  /* if so, replace with string terminator */
 strcpy(port, fp); 
}

int get_sequence(char *p, unsigned long *begin, unsigned long *end,
                           unsigned long *bytes)
{
 char seq_field[50];
 char *cursor = seq_field;
 char *fp;

 strcpy (seq_field, p);

 fp = (char *)strsep(&cursor, ":" );
 if ((cursor == (char *)NULL) ||
     (fp == (char *)NULL))
    return (-1);
 else
    *begin = strtoul(fp, (char **)NULL, 10);

 fp = (char *)strsep(&cursor, "(" );
 if ((cursor == (char *)NULL) ||
     (fp == (char *)NULL))
    return (-1);
 else
    *end = strtoul(fp, (char **)NULL, 10);

 fp = (char *)strsep(&cursor, ")" );
 if ((cursor == (char *)NULL) ||
     (fp == (char *)NULL))
    return (-1);
 else
    *bytes = strtoul(fp, (char **)NULL, 10);
 return(0);
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

