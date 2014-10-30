extern "C" {
#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
/* #include <err.h>*/
};
#include "../../../tools/rng.h"
#include "setdest.h"

//#define		DEBUG
#define		SANITY_CHECKS
//#define		SHOW_SYMMETRIC_PAIRS


#define GOD_FORMAT	"$ns_ at %.12f \"$god_ set-dist %d %d %d\"\n"
#define GOD_FORMAT2	"$god_ set-dist %d %d %d\n"
#define NODE_FORMAT	"$ns_ at %.12f \"$node_(%d) setdest %.12f %.12f %.12f\"\n"
#define NODE_FORMAT2	"$node_(%d) setdest %.12f %.12f %.12f\n"
#define NODE_FORMAT3	"$node_(%d) set %c_ %.12f\n"

#undef		INFINITY
#define		INFINITY	0x00ffffff
#define		min(x,y)	((x) < (y) ? (x) : (y))
#define		max(x,y)	((x) > (y) ? (x) : (y))
#define		ROUND_ERROR	1e-9

static int count = 0;

/* ======================================================================
   Function Prototypes
   ====================================================================== */
void		usage(char**);
void		init(void);
double		uniform(void);

void		dumpall(void);
void		ComputeW(void);
void		floyd_warshall(void);

void		show_diffs(void);
void		show_routes(void);
void		show_counters(void);


/* ======================================================================
   Global Variables
   ====================================================================== */
double	        RANGE = 250.0;		// transmitter range in meters
double		TIME = 0.0;		// my clock;
double		MAXTIME = 0.0;		// duration of simulation

double		MAXX = 0.0;
double		MAXY = 0.0;
double		MAXSPEED = 0.0;
double		PAUSE = 0.0;
u_int32_t	NODES = 0;
u_int32_t	RouteChangeCount = 0;
u_int32_t       LinkChangeCount = 0;
u_int32_t	DestUnreachableCount = 0;

Node		*NodeList = 0;
u_int32_t	*D1 = 0;
u_int32_t	*D2 = 0;

FILE            *in_file;
FILE            *out_file;


/* ======================================================================
   Random Number Generation
   ====================================================================== */
#define M		2147483647L
#define INVERSE_M	((double)4.656612875e-10)

char random_state[32];
RNG *rng;

double
uniform()
{
	count++;
	return rng->uniform_double();
}


/* ======================================================================
   Misc Functions...
   ====================================================================== */
void
usage(char **argv)
{
	fprintf(stderr,
		"\nusage: %s\t[-n <nodes>] \n",
		argv[0]);
	fprintf(stderr,
		"\t\t[-t <simulation time>]\n");
	fprintf(stderr,
		"\t\t-i <input_file> -o <output file>\n");
	fprintf(stderr,
"\t\t #nodes, max time, and range read from scenario file if possible\n\n");
	
}

void
init()
{
	/*
	 * Initialized the Random Number Generation
	 */

	/*
	 * Allocate memory for globals
	 */
	NodeList = new Node[NODES];
	if(NodeList == 0) {
		perror("new");
		exit(1);
	}

	D1 = new u_int32_t[NODES * NODES];
	if(D1 == 0) {
		perror("new");
		exit(1);
	}
	memset(D1, '\xff', sizeof(u_int32_t) * NODES * NODES);

	D2 = new u_int32_t[NODES * NODES];
	if(D2 == 0) {
		perror("new");
		exit(1);
	}
	memset(D2, '\xff', sizeof(u_int32_t) * NODES * NODES);
}

void
OpenAndReadHeader(char *in_filename, char *out_filename)
  /* assumes all initialization comments are at top of file with no
     no lines that start with other than '#' */
{
  char buf[256];
  
  in_file = fopen(in_filename,"r");
  out_file = fopen(out_filename,"w");
  
  if (NULL == in_file) 
	fprintf(stderr, "*** can't open inputfile %s",in_filename);
  if (NULL == out_file) 
	fprintf(stderr,"can't open outputfile %s",out_filename);
  
  while (!feof(in_file)) {

    *buf = fgetc(in_file);
    ungetc(*buf,in_file);
    if (*buf != '#') break;

    fgets(buf, sizeof(buf), in_file);

    /* check to see if we need data from the line */
    sscanf(buf, "# nodes: %d, max time: %lf", &NODES, &MAXTIME);
    sscanf(buf, "# nominal range: %lf", &RANGE);
    
    fprintf(out_file, "%s", buf);
  }
  NODES += 1;			// correct for 1-based indexs
  fflush(out_file);
}

void
ReadInMovementPattern()
{
  char buf[256];
  u_int n;
  double x,y,z,t,s;
  struct setdest *setdest;

  while (!feof(in_file)) {

    fgets(buf, sizeof(buf), in_file);
    fprintf(out_file, "%s", buf);
    if (*buf == '#') continue;
    if (*buf == '\n') continue;

    /* check to see if we need data from the line */
    if (2 == sscanf(buf,"$node_(%d) set Z_ %lf", &n, &z)) 
      {
	assert(n < NODES);
	NodeList[n].position.Z = z;
      }
    else if (2 == sscanf(buf,"$node_(%d) set X_ %lf", &n, &x)) 
      {
	assert(n < NODES);
	NodeList[n].position.X = x;
      }
    else if (2 == sscanf(buf,"$node_(%d) set Y_ %lf", &n, &y)) 
      {
	assert(n < NODES);
	NodeList[n].position.Y = y;
      }
    else if (5 == sscanf(buf,"$ns_ at %lf \"$node_(%d) setdest %lf %lf %lf\"", 
			&t, &n, &x, &y, &s)) 
      {
	assert(n < NODES);
	assert(t <= MAXTIME);
	setdest = (struct setdest *)malloc(sizeof(*setdest));
	assert(setdest);
	setdest->X = x; setdest->Y = y; setdest->Z = 0;
	setdest->time = t;
	setdest->speed = s;
	if (NodeList[n].traj.lh_first 
	    && t > NodeList[n].traj.lh_first->time)
	  {
	    printf("setdest's must be in anti-chronological order in input file!\n");
	    printf("failed on node %d\n",n);
	    exit(-1);
	  }
	LIST_INSERT_HEAD(&NodeList[n].traj,setdest,traj);
      }
    else 
      {
	printf("unparsable line: '%s'", buf);
	continue;
      }
  }  
  fflush(out_file);  
}

extern "C" char *optarg;
int
main(int argc, char **argv)
{
	char ch;
	char *in_filename = NULL;
	char *out_filename = NULL;

	while ((ch = getopt(argc, argv, "n:t:i:o:")) != EOF) {       

		switch (ch) { 

		case 'n':
			NODES = atoi(optarg) + 1;
			break;

		case 't':
			MAXTIME = atof(optarg);
			break;
	
		case 'i':
			in_filename = optarg;
			break;

		case 'o':
			out_filename = optarg;
			break;

		default:
			usage(argv);
			exit(1);
		}
	}

	if (NULL == in_filename || NULL == out_filename) {
	        usage(argv);
		exit(1);
	}

	OpenAndReadHeader(in_filename, out_filename);

	if(NODES == 0 || MAXTIME == 0.0) {
		usage(argv);
		exit(1);
	}
	// A more portable solution for random number generation
        rng = new RNG;
        rng->set_seed(RNG::HEURISTIC_SEED_SOURCE); 
	init();

	ReadInMovementPattern();

	while(TIME <= MAXTIME) {
		double nexttime = 0.0;
		u_int32_t i;

		for(i = 1; i < NODES; i++) {
			NodeList[i].Update();
		}

		for(i = 1; i < NODES; i++) {
			NodeList[i].UpdateNeighbors();
		}

		for(i = 1; i < NODES; i++) {
			Node *n = &NodeList[i];
	
			if(n->time_transition > 0.0) {
				if(nexttime == 0.0)
					nexttime = n->time_transition;
				else
					nexttime = min(nexttime, n->time_transition);
			}

			if(n->time_arrival > 0.0) {
				if(nexttime == 0.0)
					nexttime = n->time_arrival;
				else
					nexttime = min(nexttime, n->time_arrival);
			}
		}

		floyd_warshall();

#ifdef DEBUG
		show_routes();
#endif

	 	show_diffs();

#ifdef DEBUG
		dumpall();
#endif

		if (nexttime <= TIME + ROUND_ERROR) 
		  TIME = MAXTIME + 1;   /* we're done */
		else 
		  TIME = nexttime;
#ifdef OLD
		assert(nexttime > TIME + ROUND_ERROR);
		TIME = nexttime;
#endif

	}

	show_counters();

	int of;
	if ((of = open(".rand_state",O_WRONLY | O_TRUNC | O_CREAT, 0777)) < 0)
	  fprintf(stderr,"open rand state");
	for (unsigned int i = 0; i < sizeof(random_state); i++)
	  random_state[i] = 0xff & (int) (uniform() * 256);
	if (write(of,random_state, sizeof(random_state)) < 0)
	  fprintf(stderr,"writing rand state");
	close(of);
}


/* ======================================================================
   Node Class Functions
   ====================================================================== */
u_int32_t Node::NodeIndex = 0;

Node::Node()
{
	u_int32_t i;

	index = NodeIndex++;

	if(index == 0)
		return;

	route_changes = 0;
        link_changes = 0;

        /*
         * For the first PAUSE seconds of the simulation, all nodes
         * are stationary.
         */
	time_arrival = TIME;
	time_update = TIME;
	time_transition = 0.0;

	position.X = position.Y = position.Z = 0.0;
	destination.X = destination.Y = destination.Z = 0.0;
	direction.X = direction.Y = direction.Z = 0.0;
	speed = 0.0;

	neighbor = new Neighbor[NODES];
	if(neighbor == 0) {
		perror("new");
		exit(1);
	}

	LIST_INIT(&traj);

	for(i = 1; i < NODES; i++) {
		neighbor[i].index = i;
		neighbor[i].reachable = (index == i) ? 1 : 0;
		neighbor[i].time_transition = 0.0;
	}
}



void
Node::RandomPosition()
{
	position.X = uniform() * MAXX;
	position.Y = uniform() * MAXY;
	position.Z = 0.0;
}


void
Node::RandomDestination()
{
	destination.X = uniform() * MAXX;
	destination.Y = uniform() * MAXY;
	destination.Z = 0.0;
	assert(destination != position);
}

void
Node::RandomSpeed()
{
	speed = uniform() * MAXSPEED;

	assert(speed != 0.0);
}


void
Node::Update()
{
        struct setdest *setdest = traj.lh_first;

	position += (speed * (TIME - time_update)) * direction;

	if(TIME == time_arrival) {

	  if (NULL == setdest) 
	    {
	      destination = position;
	      direction.X = direction.Y = direction.Z = 0.0;
	      speed = 0.0;
	      time_arrival = MAXTIME + 1;
	    } 
	  else 
	    {
	      vector v;
	      destination.X = setdest->X;
	      destination.Y = setdest->Y;
	      speed = setdest->speed;
	      if (0.0 == speed)
		{ // it's a pause at the current location
		  if (LIST_NEXT(setdest,traj))
		    time_arrival = LIST_NEXT(setdest,traj)->time;
		  else
		    time_arrival = MAXTIME + 1;
		}
	      else 
		{ // we're moving somewhere, when do we get there?
		  v = destination - position;
		  direction = v / v.length();
		  time_arrival = TIME + v.length() / speed;
		}
	      LIST_REMOVE(setdest,traj);
	      free(setdest);
	    }
	}

	time_update = TIME;
	time_transition = 0.0;
}


void
Node::UpdateNeighbors()
{
	static Node *n2;
	static Neighbor *m1, *m2;
	static vector D, B, v1, v2;
	static double a, b, c, t1, t2, Q;
	static u_int32_t i, reachable;

	v1 = speed * direction;

	/*
	 *  Only need to go from INDEX --> N for each one since links
	 *  are symmetric.
	 */
	for(i = index+1; i < NODES; i++) {

		m1 = &neighbor[i];
		n2 = &NodeList[i];
		m2 = &n2->neighbor[index];

		assert(i == m1->index);
		assert(m1->index == n2->index);
		assert(index == m2->index);
                assert(m1->reachable == m2->reachable);

                reachable = m1->reachable;

		/* ==================================================
		   Determine Reachability
		   ================================================== */
		{	vector d = position - n2->position;

			if(d.length() < RANGE) {
#ifdef SANITY_CHECKS
				if(TIME > 0.0 && m1->reachable == 0)
					assert(RANGE - d.length() < ROUND_ERROR);
#endif
				m1->reachable = m2->reachable = 1;
			}
			// Boundary condition handled below.
			else {
#ifdef SANITY_CHECKS
				if(TIME > 0.0 && m1->reachable == 1)
					assert(d.length() - RANGE < ROUND_ERROR);
#endif
				m1->reachable = m2->reachable = 0;
			}
#ifdef DEBUG
                        fprintf(stdout, "# %.6f (%d, %d) %.2fm\n",
                                TIME, index, m1->index, d.length());
#endif
		}

		/* ==================================================
		   Determine Next Event Time
		   ================================================== */
		v2 = n2->speed * n2->direction;

		D = v2 - v1;
		B = n2->position - position;

		a = (D.X * D.X) + (D.Y * D.Y) + (D.Z * D.Z);
		b = 2 * ((D.X * B.X) + (D.Y * B.Y) + (D.Z * B.Z));
		c = (B.X * B.X) + (B.Y * B.Y) + (B.Z * B.Z) - (RANGE * RANGE);

		if(a == 0.0) {
			/*
			 *  No Finite Solution
			 */
			m1->time_transition= 0.0;
			m2->time_transition= 0.0;
			goto  next;
		}

		Q = b * b - 4 * a * c;
		if(Q < 0.0) {
			/*
			 *  No real roots.
			 */
			m1->time_transition = 0.0;
			m2->time_transition = 0.0;
			goto next;
		}
		Q = sqrt(Q);

		t1 = (-b + Q) / (2 * a);
		t2 = (-b - Q) / (2 * a);

		// Stupid Rounding/Boundary Cases
		if(t1 > 0.0 && t1 < ROUND_ERROR) t1 = 0.0;
		if(t1 < 0.0 && -t1 < ROUND_ERROR) t1 = 0.0;
		if(t2 > 0.0 && t2 < ROUND_ERROR) t2 = 0.0;
		if(t2 < 0.0 && -t2 < ROUND_ERROR) t2 = 0.0;

		if(t1 < 0.0 && t2 < 0.0) {
			/*
			 *  No "future" time solution.
			 */
			m1->time_transition = 0.0;
			m2->time_transition = 0.0;
			goto next;
		}

		/*
		 * Boundary conditions.
		 */
		if((t1 == 0.0 && t2 > 0.0) || (t2 == 0.0 && t1 > 0.0)) {
			m1->reachable = m2->reachable = 1;
			m1->time_transition = m2->time_transition = TIME + max(t1, t2);
		}
		else if((t1 == 0.0 && t2 < 0.0) || (t2 == 0.0 && t1 < 0.0)) {
			m1->reachable = m2->reachable = 0;
			m1->time_transition = m2->time_transition = 0.0;
		}

		/*
		 * Non-boundary conditions.
		 */
		else if(t1 > 0.0 && t2 > 0.0) {
			m1->time_transition = TIME + min(t1, t2);
			m2->time_transition = TIME + min(t1, t2);
		}
		else if(t1 > 0.0) {
			m1->time_transition = TIME + t1;
			m2->time_transition = TIME + t1;
		}
		else {
			m1->time_transition = TIME + t2;
			m2->time_transition = TIME + t2;
		}

		/* ==================================================
		   Update the transition times for both NODEs.
		   ================================================== */
		if(time_transition == 0.0 || (m1->time_transition &&
		   time_transition > m1->time_transition)) {
			time_transition = m1->time_transition;
		}
		if(n2->time_transition == 0.0 || (m2->time_transition &&
		   n2->time_transition > m2->time_transition)) {
			n2->time_transition = m2->time_transition;
		}
        next:
                if(reachable != m1->reachable && TIME > 0.0) {
                        LinkChangeCount++;
                        link_changes++;
                        n2->link_changes++;
                }
	}
}

void
Node::Dump()
{
	Neighbor *m;
	u_int32_t i;

	fprintf(stdout,
		"Node: %d\tpos: (%.2f, %.2f, %.2f) dst: (%.2f, %.2f, %.2f)\n",
		index, position.X, position.Y, position.Z,
		destination.X, destination.Y, destination.Z);
	fprintf(stdout, "\tdir: (%.2f, %.2f, %.2f) speed: %.2f\n",
		direction.X, direction.Y, direction.Z, speed);
	fprintf(stdout, "\tArrival: %.2f, Update: %.2f, Transition: %.2f\n",
		time_arrival, time_update, time_transition);

	for(i = 1; i < NODES; i++) {
		m = &neighbor[i];
		fprintf(stdout, "\tNeighbor: %d (%lx), Reachable: %d, Transition Time: %.2f\n",
			m->index, (long) m, m->reachable, m->time_transition);
	}
}


/* ======================================================================
   Dijkstra's Shortest Path Algoritm
   ====================================================================== */
void dumpall()
{
	u_int32_t i;

	fprintf(stdout, "\nTime: %.2f\n", TIME);

	for(i = 1; i < NODES; i++) {
		NodeList[i].Dump();
	}
}

void
ComputeW()
{
	u_int32_t i, j;
	u_int32_t *W = D2;

	memset(W, '\xff', sizeof(int) * NODES * NODES);

	for(i = 1; i < NODES; i++) {
		for(j = i; j < NODES; j++) {
			Neighbor *m = &NodeList[i].neighbor[j];
			if(i == j)
				W[i*NODES + j] = W[j*NODES + i] = 0;
			else
				W[i*NODES + j] = W[j*NODES + i] = m->reachable ? 1 : INFINITY;	
		}
	}
}

void
floyd_warshall()
{
	u_int32_t i, j, k;

	ComputeW();	// the connectivity matrix

	for(i = 1; i < NODES; i++) {
		for(j = 1; j < NODES; j++) {
			for(k = 1; k < NODES; k++) {
				D2[j*NODES + k] = min(D2[j*NODES + k], D2[j*NODES + i] + D2[i*NODES + k]);
			}
		}
	}

#ifdef SANITY_CHECKS
	for(i = 1; i < NODES; i++)
		for(j = 1; j < NODES; j++) {
			assert(D2[i*NODES + j] == D2[j*NODES + i]);
			assert(D2[i*NODES + j] <= INFINITY);
		}
#endif
}


/*
 *  Write the actual GOD entries to a TCL script.
 */
void
show_diffs()
{
	u_int32_t i, j;

	for(i = 1; i < NODES; i++) {
		for(j = i + 1; j < NODES; j++) {
			if(D1[i*NODES + j] != D2[i*NODES + j]) {

				if(D2[i*NODES + j] == INFINITY)
					DestUnreachableCount++;

                                if(TIME > 0.0) {
                                        RouteChangeCount++;
                                        NodeList[i].route_changes++;
                                        NodeList[j].route_changes++;
                                }

				if(TIME == 0.0) {
					fprintf(out_file, GOD_FORMAT2,
						i, j, D2[i*NODES + j]);
#ifdef SHOW_SYMMETRIC_PAIRS
					fprintf(out_file, GOD_FORMAT2,
						j, i, D2[j*NODES + i]);
#endif
				}
				else {
					fprintf(out_file, GOD_FORMAT, 
						TIME, i, j, D2[i*NODES + j]);
#ifdef SHOW_SYMMETRIC_PAIRS
					fprintf(out_file, GOD_FORMAT, 
						TIME, j, i, D2[j*NODES + i]);
#endif
				}
			}
		}
	}

	memcpy(D1, D2, sizeof(int) * NODES * NODES);
}


void
show_routes()
{
	u_int32_t i, j;

	fprintf(stdout, "#\n# TIME: %.12f\n#\n", TIME);
	for(i = 1; i < NODES; i++) {
		fprintf(stdout, "# %2d) ", i);
		for(j = 1; j < NODES; j++)
			fprintf(stdout, "%3d ", D2[i*NODES + j] & 0xff);
		fprintf(stdout, "\n");
	}
	fprintf(stdout, "#\n");
}

void
show_counters()
{
	u_int32_t i;

	fprintf(out_file, "#\n# Destination Unreachables: %d\n#\n",
		DestUnreachableCount);
	fprintf(out_file, "# Route Changes: %d\n#\n", RouteChangeCount);
        fprintf(out_file, "# Link Changes: %d\n#\n", LinkChangeCount);
        fprintf(out_file, "# Node | Route Changes | Link Changes\n");
	for(i = 1; i < NODES; i++)
		fprintf(out_file, "# %4d |          %4d |         %4d\n",
                        i, NodeList[i].route_changes,
                        NodeList[i].link_changes);
	fprintf(out_file, "#\n");
}

