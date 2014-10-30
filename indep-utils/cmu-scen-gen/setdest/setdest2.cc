/*
 * a simplified version of setdest which bypass the computation of route length.
 */

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
#if !defined(sun) && !defined(__CYGWIN__)
#include <err.h>
#endif
};
#include "../../../rng.h"

#include "setdest.h"


// #define		DEBUG
#define		SANITY_CHECKS
//#define		SHOW_SYMMETRIC_PAIRS


#define GOD_FORMAT	"$ns_ at %.12f \"$god_ set-dist %d %d %d\"\n"
#define GOD_FORMAT2	"$god_ set-dist %d %d %d\n"
#define NODE_FORMAT	"$ns_ at %.12f \"$node_(%d) setdest %.12f %.12f %.12f\"\n"
#define NODE_FORMAT2	"$node_(%d) setdest %.12f %.12f %.12f\n"
#define NODE_FORMAT3	"$node_(%d) set %c_ %.12f\n"

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
const double	RANGE = 250.0;		// transmitter range in meters
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
        return rng->uniform_double() ;
} 


/* ======================================================================
   Misc Functions...
   ====================================================================== */
void
usage(char **argv)
{
	fprintf(stderr,
		"\nusage: %s\t-n <nodes> -p <pause time> -s <max speed>\n",
		argv[0]);
	fprintf(stderr,
		"\t\t-t <simulation time> -x <max X> -y <max Y>\n\n");
}

void
init()
{
	/*
	 * Initialized the Random Number Generation
	 */
"
	/*"
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

extern "C" char *optarg;

int
main(int argc, char **argv)
{
	char ch;

	while ((ch = getopt(argc, argv, "n:p:s:t:x:y:i:o:")) != EOF) {       

		switch (ch) { 

		case 'n':
			NODES = atoi(optarg);
			break;

		case 'p':
			PAUSE = atof(optarg);
			break;

		case 's':
			MAXSPEED = atof(optarg);
			break;

		case 't':
			MAXTIME = atof(optarg);
			break;

		case 'x':
			MAXX = atof(optarg);
			break;

		case 'y':
			MAXY = atof(optarg);
			break;

		default:
			usage(argv);
			exit(1);
		}
	}

	if(MAXX == 0.0 || MAXY == 0.0 || NODES == 0 || MAXTIME == 0.0) {
		usage(argv);
		exit(1);
	}

	fprintf(stdout, "#\n# nodes: %d, pause: %.2f, max speed: %.2f  max x = %.2f, max y: %.2f\n#\n",
		NODES , PAUSE, MAXSPEED, MAXX, MAXY);

	// The more portable solution for random number generation
	rng = new RNG;
	rng->set_seed(RNG::HEURISTIC_SEED_SOURCE); 

	init();

	while(TIME <= MAXTIME) {
		double nexttime = 0.0;
		u_int32_t i;

		for(i = 0; i < NODES; i++) {
			NodeList[i].Update();
		}
/*
		for(i = 0; i < NODES; i++) {
			NodeList[i].UpdateNeighbors();
		}
*/
		for(i = 0; i < NODES; i++) {
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

		// floyd_warshall();

#ifdef DEBUG
		show_routes();
#endif

	 	// show_diffs();

#ifdef DEBUG
		dumpall();
#endif

		assert(nexttime > TIME + ROUND_ERROR);
		TIME = nexttime;
	}

	show_counters();

	int of;
	if ((of = open(".rand_state",O_WRONLY | O_TRUNC | O_CREAT, 0777)) < 0) {
	  fprintf(stderr, "open rand state\n");
	  exit(-1);
	  }
	for (unsigned int i = 0; i < sizeof(random_state); i++)
          random_state[i] = 0xff & (int) (uniform() * 256);
	if (write(of,random_state, sizeof(random_state)) < 0) {
	  fprintf(stderr, "writing rand state\n");
	  exit(-1);
	  }
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

	//if(index == 0)
	//	return;

	route_changes = 0;
        link_changes = 0;

        /*
         * For the first PAUSE seconds of the simulation, all nodes
         * are stationary.
         */
	time_arrival = TIME + PAUSE;
	time_update = TIME;
	time_transition = 0.0;

	position.X = position.Y = position.Z = 0.0;
	destination.X = destination.Y = destination.Z = 0.0;
	direction.X = direction.Y = direction.Z = 0.0;
	speed = 0.0;

	RandomPosition();

	fprintf(stdout, NODE_FORMAT3, index, 'X', position.X);
	fprintf(stdout, NODE_FORMAT3, index, 'Y', position.Y);
	fprintf(stdout, NODE_FORMAT3, index, 'Z', position.Z);

	neighbor = new Neighbor[NODES];
	if(neighbor == 0) {
		perror("new");
		exit(1);
	}

	for(i = 0; i < NODES; i++) {
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
	position += (speed * (TIME - time_update)) * direction;

	if(TIME == time_arrival) {
		vector v;

		if(speed == 0.0 || PAUSE == 0.0) {

                       	RandomDestination();
			RandomSpeed();

			v = destination - position;
			direction = v / v.length();
			time_arrival = TIME + v.length() / speed;
		}
		else {

			destination = position;
			speed = 0.0;

			time_arrival = TIME + PAUSE;
		}

		fprintf(stdout, NODE_FORMAT,
			TIME, index, destination.X, destination.Y, speed);
	
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

	for(i = 0; i < NODES; i++) {
		m = &neighbor[i];
		fprintf(stdout, "\tNeighbor: %d (%x), Reachable: %d, Transition Time: %.2f\n",
			m->index, (int) m, m->reachable, m->time_transition);
	}
}


/* ======================================================================
   Dijkstra's Shortest Path Algoritm
   ====================================================================== */
void dumpall()
{
	u_int32_t i;

	fprintf(stdout, "\nTime: %.2f\n", TIME);

	for(i = 0; i < NODES; i++) {
		NodeList[i].Dump();
	}
}

void
ComputeW()
{
	u_int32_t i, j;
	u_int32_t *W = D2;

	memset(W, '\xff', sizeof(int) * NODES * NODES);

	for(i = 0; i < NODES; i++) {
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

	for(i = 0; i < NODES; i++) {
		for(j = 0; j < NODES; j++) {
			for(k = 0; k < NODES; k++) {
				D2[j*NODES + k] = min(D2[j*NODES + k], D2[j*NODES + i] + D2[i*NODES + k]);
			}
		}
	}

#ifdef SANITY_CHECKS
	for(i = 0; i < NODES; i++)
		for(j = 0; j < NODES; j++) {
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

	for(i = 0; i < NODES; i++) {
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
					fprintf(stdout, GOD_FORMAT2,
						i, j, D2[i*NODES + j]);
#ifdef SHOW_SYMMETRIC_PAIRS
					fprintf(stdout, GOD_FORMAT2,
						j, i, D2[j*NODES + i]);
#endif
				}
				else {
					fprintf(stdout, GOD_FORMAT, 
						TIME, i, j, D2[i*NODES + j]);
#ifdef SHOW_SYMMETRIC_PAIRS
					fprintf(stdout, GOD_FORMAT, 
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
	for(i = 0; i < NODES; i++) {
		fprintf(stdout, "# %2d) ", i);
		for(j = 0; j < NODES; j++)
			fprintf(stdout, "%3d ", D2[i*NODES + j] & 0xff);
		fprintf(stdout, "\n");
	}
	fprintf(stdout, "#\n");
}

void
show_counters()
{
	u_int32_t i;

	fprintf(stdout, "#\n# Destination Unreachables: %d\n#\n",
		DestUnreachableCount);
	fprintf(stdout, "# Route Changes: %d\n#\n", RouteChangeCount);
        fprintf(stdout, "# Link Changes: %d\n#\n", LinkChangeCount);
        fprintf(stdout, "# Node | Route Changes | Link Changes\n");
	for(i = 0; i < NODES; i++)
		fprintf(stdout, "# %4d |          %4d |         %4d\n",
                        i, NodeList[i].route_changes,
                        NodeList[i].link_changes);
	fprintf(stdout, "#\n");
}

