/* -*- c++ -*-
   marshall.h
   $Id: marshall.h,v 1.4 1999/04/30 00:59:43 haldar Exp $

   safely marshall and unmarshall 2 and 4 byte quantities into 
   possibly unaligned feilds

   NOTE: unlike the normal version of marshall, we marshall the bytes
   in *HOST*BYTE*ORDER* and *not* network byte order.  we do this since
   all the other quanities in the packets will be in host byte order, and
   we want consistency.

   */

#ifndef _cmu_marshall_h_
#define _cmu_marshall_h_

/*#ifdef sparc  */

/*#define GET4BYTE(x) (  (*(((unsigned char *)(x))  ) << 24) \
  | (*(((unsigned char *)(x))+1) << 16) \
  | (*(((unsigned char *)(x))+2) << 8) \
  | (*(((unsigned char *)(x))+3)     ))
  */
/* #define GET2BYTE(x) (  (*(((unsigned char *)(x))  ) << 8) \
   | (*(((unsigned char *)(x))+1)     ))
   */
/* let x,y be ptrs to ints, copy *x into *y */
/* #define STORE4BYTE(x,y) (((unsigned char *)y)[0] = ((unsigned char *)x)[0] ,\
   ((unsigned char *)y)[1] = ((unsigned char *)x)[1] ,\
   ((unsigned char *)y)[2] = ((unsigned char *)x)[2] ,\
   ((unsigned char *)y)[3] = ((unsigned char *)x)[3] )
   */

/* #define STORE2BYTE(x,y) (((unsigned char *)y)[0] = ((unsigned char *)x)[0] ,\
   ((unsigned char *)y)[1] = ((unsigned char *)x)[1] )
   */

  /*#else
    
    #include <sys/types.h>
    
    #define GET4BYTE(x) *((u_int32_t*) (x))
    
    #define GET2BYTE(x) *((u_int16_t*) (x)) */
  
  /* let x,y be ptrs to ints, copy *x into *y */
  /*#define STORE4BYTE(x,y) (*((u_int32_t*) (x)) = *y)
    
    #define STORE2BYTE(x,y) (*((u_int16_t*) (x)) = *y)
    
    #endif */

/* changing STORE4BYTE and STORE2BYTE functions as they break in 
   freeBSD; the old cmu version of STORE4BYTE used arrays to set the 
   value: 
   #define STORE4BYTE(x,y) (((unsigned char *)y)[0] = ((unsigned char *)x)[0] ,\
   ((unsigned char *)y)[1] = ((unsigned char *)x)[1] ,\
   ((unsigned char *)y)[2] = ((unsigned char *)x)[2] ,\
   ((unsigned char *)y)[3] = ((unsigned char *)x)[3] )
   which doesn;t work on freeBSD (with gcc version 2.7.2.1).
   the newer versions are tested to work in sunos/solaris/freeBSD  
   --Padma, 04/99 */

#define STORE4BYTE(x,y)  ((*((unsigned char *)y)) = ((*x) >> 24) & 255 ,\
			  (*((unsigned char *)y+1)) = ((*x) >> 16) & 255 ,\
			  (*((unsigned char *)y+2)) = ((*x) >> 8) & 255 ,\
			  (*((unsigned char *)y+3)) = (*x) & 255 )
	
#define STORE2BYTE(x,y)  ((*((unsigned char *)y)) = ((*x) >> 8) & 255 ,\
			  (*((unsigned char *)y+1)) = (*x) & 255 )
	
#define GET2BYTE(x)      (((*(unsigned char *)(x)) << 8) |\
			  (*(((unsigned char *)(x))+1)))
	
#define GET4BYTE(x)      (((*(unsigned char *)(x)) << 24) |\
			  (*(((unsigned char *)(x))+1) << 16) |\
			  (*(((unsigned char *)(x))+2) << 8) |\
			  (*(((unsigned char *)(x))+3)))

	
#endif /* _cmu_marshall_h_ */
