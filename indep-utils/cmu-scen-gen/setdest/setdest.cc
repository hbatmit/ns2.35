/*
 *
 * The original code of setdest was included in ns-2.1b8a.
 * This file is the modified version by J. Yoon <jkyoon@eecs.umich.edu>,
 *	Department of EECS, University of Michigan, Ann Arbor. 
 *
 * (1) Input parameters
 *	<Original version>
 *		=> -M maximum speed (minimum speed is zero as a default)
 *		=> -p pause time (constant) 
 *		=> -n number of nodes
 *		=> -x x dimension of space
 *		=> -y y dimension of space
 *
 *	<Modified version>
 *		=> -s speed type (uniform, normal)
 *		=> -m minimum speed > 0 
 *		=> -M maximum speed
 *		=> -P pause type (constant, uniform)
 *		=> -p pause time (a median if uniform is chosen)
 *		=> -n number of nodes
 *		=> -x x dimension of space
 *		=> -y y dimension of space
 *
 * (2) In case of modified version, the steady-state speed distribution is applied to 
 *	the first trip to eliminate any speed decay. If pause is not zero, the first 
 *	trip could be either a move or a pause depending on the probabilty that the 
 *	first trip is a pause. After the first trip regardless of whether it is 
 *	a move or a pause, all subsequent speeds are determined from the given speed 
 *	distribution (e.g., uniform or normal).
 *
 * (3) Refer to and use scenario-generating scripts (make-scen.csh for original version, 
 *	make-scen-steadystate.csh for modified version).
 *
 *
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
#if !defined(sun)
#include <err.h>
#endif
};
#include "../../../tools/rng.h"

#include "setdest.h"


// #define		DEBUG
#define		SANITY_CHECKS
//#define		SHOW_SYMMETRIC_PAIRS


#define GOD_FORMAT	"$ns_ at %.12f \"$god_ set-dist %d %d %d\"\n"
#define GOD_FORMAT2	"$god_ set-dist %d %d %d\n"
#define NODE_FORMAT	"$ns_ at %.12f \"$node_(%d) setdest %.12f %.12f %.12f\"\n"
#define NODE_FORMAT2	"$node_(%d) setdest %.12f %.12f %.12f\n"
#define NODE_FORMAT3	"$node_(%d) set %c_ %.12f\n"

#define		RTG_INFINITY	0x00ffffff
#ifndef min
#define		min(x,y)	((x) < (y) ? (x) : (y))
#define		max(x,y)	((x) > (y) ? (x) : (y))
#endif
#define		ROUND_ERROR	1e-9
#ifndef PI
#define 	PI	 	3.1415926	
#endif


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
const double	RANGE = 250.0;	// transmitter range in meters
double		TIME = 0.0;			// my clock;
double		MAXTIME = 0.0;		// duration of simulation

double		MAXX = 0.0;			// width of space
double		MAXY = 0.0;			// height of space
double		PAUSE = 0.0;		// pause time
double		MAXSPEED = 0.0;		// max speed
double		MINSPEED = 0.0;		// min speed 
double		SS_AVGSPEED = 0.0;	// steady-state avg speed 
double 		KAPPA = 0.0;		// normalizing constant 
double 		MEAN = 0.0;			// mean for normal speed
double 		SIGMA = 0.0;		// std for normal speed
double 		EXP_1_V = 0.0;		// expactation of 1/V
double 		EXP_R = 0.0;		// expectation of travel distance R
double 		PDFMAX = 0.0;		// max of pdf for rejection technique
u_int32_t	SPEEDTYPE = 1;		// speed type (default = uniform)
u_int32_t	PAUSETYPE = 1;		// pause type (default = constant)
u_int32_t	VERSION = 1;		// setdest version (default = original by CMU) 
u_int32_t	NODES = 0;			// number of nodes
u_int32_t	RouteChangeCount = 0;
u_int32_t	LinkChangeCount = 0;
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


/* compute the expectation of travel distance E[R] in a rectangle */
void
compute_EXP_R()
{
	#define csc(x) 		(1.0/sin(x))		// csc function
	#define sec(x)		(1.0/cos(x))		// sec function
	#define sin2(x)		(sin(x)*sin(x))		// sin^2
	#define sin3(x)		(sin2(x)*sin(x))	// sin^3
	#define cos2(x)		(cos(x)*cos(x))		// cos^2
	#define cos3(x)		(cos2(x)*cos(x))	// cos^3

	double x = MAXX, y = MAXY;			// max x and max y
	double x2 = x*x, x3 = x*x*x;			// x^2 and x^3
	double y2 = y*y, y3 = y*y*y;			// y^2 and y^3

	double term1 = sin(atan2(y,x)) / 2.0 / cos2(atan2(y,x));
	double term2 = 0.5 * log( sec(atan2(y,x)) + y/x );
	double term3 = -1.0 * x3 / y2 / 60.0 / cos3(atan2(y,x)) + 1.0/60.0 * x3 / y2;
	double term4 = (term1 + term2) * x2 / 12.0 / y + term3;

	double term5 = -1.0 * cos(atan2(y,x)) / 2.0 / sin2(atan2(y,x));
	double term6 = 0.5 * log( csc(atan2(y,x)) - x/y );
	double term7 = -1.0 * y3 / x2 / 60.0 / sin3(atan2(y,x)) + 1.0/60.0 * y3 / x2;
	double term8 = -1.0 * (term5 + term6) * y2 / 12.0 / x + term7;

	EXP_R = (4 * (term4 + term8)); 			// E[R]
}

void
usage(char **argv)
{
	fprintf(stderr, "\nusage:\n");
	fprintf(stderr,
		"\n<original 1999 CMU version (version 1)>\n %s\t-v <1> -n <nodes> -p <pause time> -M <max speed>\n",
		argv[0]);
	fprintf(stderr,
		"\t\t-t <simulation time> -x <max X> -y <max Y>\n");
	fprintf(stderr,
		"\nOR\n<modified 2003 U.Michigan version (version 2)>\n %s\t-v <2> -n <nodes> -s <speed type> -m <min speed> -M <max speed>\n",
		argv[0]);
	fprintf(stderr,
		"\t\t-t <simulation time> -P <pause type> -p <pause time> -x <max X> -y <max Y>\n");
	fprintf(stderr,
		"\t\t(Refer to the script files make-scen.csh and make-scen-steadystate.csh for detail.) \n\n");
}

void
init()
{
	/*
	 * Initialized the Random Number Generation
	 */
	/* 
	This part of init() is commented out and is replaced by more
	portable RNG (random number generator class of ns) functions.	

	struct timeval tp;
	int fd, seed, bytes;

	if((fd = open("/dev/random", O_RDONLY)) < 0) {
		perror("open");
		exit(1);
	}
	if((bytes = read(fd, random_state, sizeof(random_state))) < 0) {
		perror("read");
		exit(1);
	}
	close(fd);

	fprintf(stderr, "*** read %d bytes from /dev/random\n", bytes);

	if(bytes != sizeof(random_state)) {
	  fprintf(stderr,"Not enough randomness. Reading `.rand_state'\n");
	  if((fd = open(".rand_state", O_RDONLY)) < 0) {
	    perror("open .rand_state");
	    exit(1);
	  }
	  if((bytes = read(fd, random_state, sizeof(random_state))) < 0) {
	    perror("reading .rand_state");
	    exit(1);
	  }
	  close(fd);
	}

         if(gettimeofday(&tp, 0) < 0) {
		perror("gettimeofday");
		exit(1);
	}
	seed = (tp.tv_sec  >> 12 ) ^ tp.tv_usec;
        (void) initstate(seed, random_state, bytes & 0xf8);*/

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

extern "C" char *optarg;

int
main(int argc, char **argv)
{
	char ch;

	while ((ch = getopt(argc, argv, "v:n:s:m:M:t:P:p:x:y:i:o:")) != EOF) {       

		switch (ch) { 
		
		case 'v':
		  VERSION = atoi(optarg);
		  break;

		case 'n':
		  NODES = atoi(optarg);
		  break;

		case 's':
			SPEEDTYPE = atoi(optarg);	
			break;

		case 'm':
			MINSPEED = atof(optarg);	
			break;

		case 'M':
			MAXSPEED = atof(optarg);
			break;

		case 't':
			MAXTIME = atof(optarg);
			break;

		case 'P':
			PAUSETYPE = atoi(optarg);	
			break;

		case 'p':
			PAUSE = atof(optarg);
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
	
	/* specify the version */
	if (VERSION != 1 && VERSION != 2) {
	  printf("Please specify the setdest version you want to use. For original 1999 CMU version use 1; For modified 2003 U.Michigan version use 2\n");
	  exit(1);
	}

	if (VERSION == 2 && MINSPEED <= 0) {
	  usage(argv);
	  exit(1);
	} else if (VERSION == 1 && MINSPEED > 0) {
	  usage(argv);
	  exit(1);
	}


	// The more portable solution for random number generation
	rng = new RNG;
	rng->set_seed(RNG::HEURISTIC_SEED_SOURCE); 



	/****************************************************************************************
 	 * Steady-state avg speed and distribution depending on the initial distirbutions given
	 ****************************************************************************************/
	
	/* original setdest */	
	if (VERSION == 1) {	
		fprintf(stdout, "#\n# nodes: %d, pause: %.2f, max speed: %.2f, max x: %.2f, max y: %.2f\n#\n",
			NODES, PAUSE, MAXSPEED, MAXX, MAXY);
	}	

	/* modified version */
	else if (VERSION == 2) {
		/* compute the expectation of travel distance in a rectangle */
		compute_EXP_R();

		/* uniform speed from min to max */
		if (SPEEDTYPE == 1) {
			EXP_1_V = log(MAXSPEED/MINSPEED) / (MAXSPEED - MINSPEED);	// E[1/V]
			SS_AVGSPEED = EXP_R / (EXP_1_V*EXP_R + PAUSE);				// steady-state average speed
			PDFMAX = 1/MINSPEED*EXP_R / (EXP_1_V*EXP_R + PAUSE) / (MAXSPEED-MINSPEED);	// max of pdf for rejection technique
		}
		
		/* normal speed clipped from min to max */
		else if (SPEEDTYPE == 2) {
			int bin_no = 10000;									// the number of bins for summation
			double delta = (MAXSPEED - MINSPEED)/bin_no; 		// width of each bin 
			int i;
			double acc_k, acc_e, square, temp_v;

			MEAN = (MAXSPEED + MINSPEED)/2.0;					// means for normal dist.
			SIGMA = (MAXSPEED - MINSPEED)/4.0;					// std for normal dist.
			/* computing a normalizing constant KAPPA, E[1/V], and pdf max */
			KAPPA = 0.0;
			EXP_1_V = 0.0;
			PDFMAX = 0.0;

			/* numerical integrals */
			for (i=0; i<bin_no; ++i) {
				temp_v = MINSPEED + i*delta;		// ith v from min speed
				square = (temp_v - MEAN)*(temp_v - MEAN)/SIGMA/SIGMA;

				acc_k = 1.0/sqrt(2.0*PI*SIGMA*SIGMA)*exp(-0.5*square);
				KAPPA += (acc_k*delta);				// summing up the area of rectangle

				acc_e = 1.0/temp_v/sqrt(2.0*PI*SIGMA*SIGMA)*exp(-0.5*square);
				EXP_1_V += (acc_e*delta);			// summing up for the denominator of pdf
	
				/* find a max of pdf */
				if (PDFMAX < acc_e) PDFMAX = acc_e;
			}
			EXP_1_V /= KAPPA;						// normalizing
			SS_AVGSPEED = EXP_R / (EXP_1_V*EXP_R + PAUSE);			// steady-state average speed
			PDFMAX = EXP_R*PDFMAX/KAPPA / (EXP_1_V*EXP_R + PAUSE);	// max of pdf for rejection technique
		}
		/* other types of speed for future use */
		else
			;
	
		fprintf(stdout, "#\n# nodes: %d, speed type: %d, min speed: %.2f, max speed: %.2f\n# avg speed: %.2f, pause type: %d, pause: %.2f, max x: %.2f, max y: %.2f\n#\n",
			NODES , SPEEDTYPE, MINSPEED, MAXSPEED, SS_AVGSPEED, PAUSETYPE, PAUSE, MAXX, MAXY);
	} 	


	init();

	while(TIME <= MAXTIME) {
		double nexttime = 0.0;
		u_int32_t i;

		for(i = 0; i < NODES; i++) {
			NodeList[i].Update();
		}

		for(i = 0; i < NODES; i++) {
			NodeList[i].UpdateNeighbors();
		}

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

		floyd_warshall();

#ifdef DEBUG
		show_routes();
#endif

	 	show_diffs();

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



    /*******************************************************************************
	 * Determine if the first trip is a pause or a move with the steady-state pdf
     *******************************************************************************/

	/* original version */
	if (VERSION == 1) {
			time_arrival = TIME + PAUSE;			// constant pause
	}

	/* modified version */ 
	else if (VERSION == 2) {
		/* probability that the first trip would be a pause */
		double prob_pause = PAUSE / (EXP_1_V*EXP_R + PAUSE);
	
		/* the first trip is a pause */
		if (prob_pause > uniform()) {
			/* constant pause */
			if (PAUSETYPE == 1) {
				time_arrival = TIME + PAUSE;				// constant pause
			}
			/* uniform pause */
			else if (PAUSETYPE == 2) {
				time_arrival = TIME + 2*PAUSE*uniform();	// uniform pause [0, 2*PAUSE]
			}
				
			first_trip = 0;						// indicating the first trip is a pause 
		}
		/* the first trip is a move based on the steady-state pdf */
		else {
			time_arrival = TIME;
			first_trip = 1;						// indicating the first trip is a move 
		}
	}


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


/****************************************************************************************** 
 * Speeds are chosen based on the given type and distribution
 ******************************************************************************************/
void
Node::RandomSpeed()
{
	/* original version */
	if (VERSION == 1) {
       	speed = uniform() * MAXSPEED;
		assert(speed != 0.0);
	}

	/* modified version */
	else if (VERSION == 2) {
		/* uniform speed */
		if (SPEEDTYPE == 1) {
			/* using steady-state distribution for the first trip */
			if (first_trip == 1) {
				/* pick a speed by rejection technique */
				double temp_v, temp_fv;
	
				do {
	        	 	temp_v = uniform() * (MAXSPEED - MINSPEED) + MINSPEED;
					temp_fv = uniform() * PDFMAX;
				} while (temp_fv > 1/temp_v*EXP_R / (EXP_1_V*EXP_R + PAUSE) / (MAXSPEED-MINSPEED));
	 
				speed = temp_v;
				first_trip = 0;		// reset first_trip flag 	
			}
			/* using the original distribution from the second trip on */
			else {
	        	speed = uniform() * (MAXSPEED - MINSPEED) + MINSPEED;
				assert(speed != 0.0);
			}
		}
		/* normal speed */
		else if (SPEEDTYPE == 2) {
			/* using steady-state distribution for the first trip */
			if (first_trip == 1) {
				double temp_v, temp_fv, square, fv;
	
				/* rejection technique */
				do {
	        	 	temp_v = uniform() * (MAXSPEED - MINSPEED) + MINSPEED;
					temp_fv = uniform() * PDFMAX;
					square = (temp_v - MEAN)*(temp_v - MEAN)/SIGMA/SIGMA;
					fv = 1/KAPPA/sqrt(2.0*PI*SIGMA*SIGMA) * exp(-0.5*square);
				} while (temp_fv > 1.0/temp_v*fv*EXP_R / (EXP_1_V*EXP_R + PAUSE));
	 
				speed = temp_v;
				first_trip = 0;
			}
			/* using the original distribution from the second trip on */
			else {
				double temp_v, temp_fv, square;
				double max_normal = 1.0/KAPPA/sqrt(2.0*PI*SIGMA*SIGMA);		// max of normal distribution
	
				/* rejection technique */
				do {
	         		temp_v = uniform() * (MAXSPEED - MINSPEED) + MINSPEED;
					temp_fv = uniform() * max_normal;
					square = (temp_v - MEAN)*(temp_v - MEAN)/SIGMA/SIGMA;
				} while (temp_fv > max_normal * exp(-0.5*square));
	 
				speed = temp_v;
				assert(speed != 0.0);
			}
		}
		/* other types of speed for future use */
		else
			;
	}
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

			/* original version */
			if (VERSION == 1) {
				time_arrival = TIME + PAUSE;
			}
			/* modified version */
			else if (VERSION == 2) {
				/* constant pause */
				if (PAUSETYPE == 1) {
					time_arrival = TIME + PAUSE;
				}
				/* uniform pause */
				else if (PAUSETYPE == 2) {
					time_arrival = TIME + 2*PAUSE*uniform();
				}
			}
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
				W[i*NODES + j] = W[j*NODES + i] = m->reachable ? 1 : RTG_INFINITY;	
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
			assert(D2[i*NODES + j] <= RTG_INFINITY);
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

				if(D2[i*NODES + j] == RTG_INFINITY)
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

