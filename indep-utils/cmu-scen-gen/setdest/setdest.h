#ifndef __setdest_h__
#define __setdest_h__

/*#include <sys/queue.h>*/
#include "../../../config.h"
#include "../../../lib/bsd-list.h"

#ifndef LIST_FIRST
#define LIST_FIRST(head)	((head)->lh_first)
#endif
#ifndef LIST_NEXT
#define LIST_NEXT(elm, field)	((elm)->field.le_next)
#endif

void ReadInMovementPattern(void);

class vector {
public:
	vector(double x = 0.0, double y = 0.0, double z = 0.0) {
		X = x; Y = y; Z = z;
	}
	double length() {
		return sqrt(X*X + Y*Y + Z*Z);
	}

	inline void operator=(const vector a) {
		X = a.X;
		Y = a.Y;
		Z = a.Z;
	}
	inline void operator+=(const vector a) {
		X += a.X;
		Y += a.Y;
		Z += a.Z;
	}
	inline int operator==(const vector a) {
		return (X == a.X && Y == a.Y && Z == a.Z);
	}
	inline int operator!=(const vector a) {
		return (X != a.X || Y != a.Y || Z != a.Z);
	}
	inline vector operator-(const vector a) {
		return vector(X-a.X, Y-a.Y, Z-a.Z);
	}
	friend inline vector operator*(const double a, const vector b) {
		return vector(a*b.X, a*b.Y, a*b.Z);
	}
	friend inline vector operator/(const vector a, const double b) {
		return vector(a.X/b, a.Y/b, a.Z/b);
	}

	double X;
	double Y;
	double Z;
};


class Neighbor {
public:
	u_int32_t	index;			// index into NodeList
	u_int32_t	reachable;		// != 0 --> reachable.
	double		time_transition;	// next change

};

struct setdest {
  double time;
  double X, Y, Z;
  double speed;
  LIST_ENTRY(setdest)   traj;
};

class Node {
  friend void ReadInMovementPattern(void);
public:
	Node(void);
	void	Update(void);
	void	UpdateNeighbors(void);
	void	Dump(void);

	double		time_arrival;		// time of arrival at dest
	double		time_transition;	// min of all neighbor times

	// # of optimal route changes for this node
	int		route_changes;
        int             link_changes;

private:
	void	RandomPosition(void);
	void	RandomDestination(void);
	void	RandomSpeed(void);

	u_int32_t	index;                  // unique node identifier
	u_int32_t 	first_trip;		// 1 if first trip, 0 otherwise. (by J. Yoon)

	vector		position;		// current position
	vector		destination;		// destination
	vector		direction;		// computed from pos and dest

	double		speed;
	double		time_update;		// when pos last updated

	static u_int32_t	NodeIndex;

	LIST_HEAD(traj, setdest) traj;

public:
	// An array of NODES neighbors.
	Neighbor	*neighbor;
};

#endif /* __setdest_h__ */
