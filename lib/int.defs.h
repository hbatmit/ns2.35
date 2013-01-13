// -*- C++ -*-

#define DEFAULT_INITIAL_CAPACITY 8
//     The initial capacity for containers (e.g., hash tables) that
//     require an initial capacity argument for constructors.  Default:
//     100

#define intEQ(a, b) ((a)==(b))
//     return true if a is considered equal to b for the purposes of
//     locating, etc., an element in a container.  Default: (a == b)

#define intLE(a, b) ((a)<=(b))
//     return true if a is less than or equal to b Default: (a <= b)

#define intCMP(a, b) (((a) <= (b))? ((a)==(b))? 0 : -1 : 1)
//     return an integer < 0 if a<b, 0 if a==b, or > 0 if a>b.  Default:
//     (a <= b)? (a==b)? 0 : -1 : 1

#define intHASH(a) (a)
//     return an unsigned integer representing the hash of a.  Default:
//     hash(a) ; where extern unsigned int hash(<T&>).  (note: several
//     useful hash functions are declared in builtin.h and defined in
//     hash.cc)
