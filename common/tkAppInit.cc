/* 
 * tkAppInit.c --
 *
 *	Provides a default version of the main program and Tcl_AppInit
 *	procedure for Tcl applications (without Tk).
 *
 * Copyright (C) 2000 USC/ISI
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS: @(#) tkAppInit.c 1.21 96/03/26 16:47:07
 *
 *  pmsrve 29/June/2005 - Ported a new tkAppinit.cc for NS2, based from: 
 *      ns-2.28's common/tclAppinit.cc (....)
 *	    tk8.4.5's wish.c (v 1.7 2002/06/21 20:24:29 dgp Exp)
 *
 */

#include <tk.h>
#include "locale.h"
#include "config.h"

extern void init_misc(void);
extern EmbeddedTcl et_ns_lib;
extern EmbeddedTcl et_ns_ptypes;
extern EmbeddedTcl et_tk;

/* MSVC requires this global var declaration to be outside of 'extern "C"' */
#ifdef MEMDEBUG_SIMULATIONS
#include "mem-trace.h"
MemTrace *globalMemTrace;
#endif

#define NS_BEGIN_EXTERN_C	extern "C" {
#define NS_END_EXTERN_C		}

NS_BEGIN_EXTERN_C

#ifdef HAVE_FENV_H
#include <fenv.h>
#endif /* HAVE_FENV_H */

/*
 * The following variable is a special hack that is needed in order for
 * Sun shared libraries to be used for Tcl.
 */

#ifdef TK_TEST
extern int		Tktest_Init _ANSI_ARGS_((Tcl_Interp *interp));
#endif /* TK_TEST */



#include "bitmap/play.xbm"
#include "bitmap/stop.xbm"
#include "bitmap/rewind.xbm"
#include "bitmap/ff.xbm"

void loadbitmaps(Tcl_Interp* tcl)
{
	Tk_DefineBitmap(tcl, Tk_GetUid("play"),
			play_bits, play_width, play_height);
	Tk_DefineBitmap(tcl, Tk_GetUid("stop"),
			stop_bits, stop_width, stop_height);

	Tk_DefineBitmap(tcl, Tk_GetUid("rewind"),
			rewind_bits, rewind_width, rewind_height);
	Tk_DefineBitmap(tcl, Tk_GetUid("ff"),
			ff_bits, ff_width, ff_height);
}



/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	This is the main program for the application.
 *
 * Results:
 *	None: Tk_Main never returns here, so this procedure never
 *	returns either.
 *
 * Side effects:
 *	Whatever the application does.
 *
 *----------------------------------------------------------------------
 */

int
main(int argc, char **argv)
{
    /*
     * The following #if block allows you to change the AppInit
     * function by using a #define of TCL_LOCAL_APPINIT instead
     * of rewriting this entire file.  The #if checks for that
     * #define and uses Tcl_AppInit if it doesn't exist.
     */
    
#ifndef TK_LOCAL_APPINIT
#define TK_LOCAL_APPINIT Tcl_AppInit    
#endif
    extern int TK_LOCAL_APPINIT _ANSI_ARGS_((Tcl_Interp *interp));
    
    /*
     * The following #if block allows you to change how Tcl finds the startup
     * script, prime the library or encoding paths, fiddle with the argv,
     * etc., without needing to rewrite Tk_Main()
     */
    
#ifdef TK_LOCAL_MAIN_HOOK
    extern int TK_LOCAL_MAIN_HOOK _ANSI_ARGS_((int *argc, char ***argv));
    TK_LOCAL_MAIN_HOOK(&argc, &argv);
#endif

    Tk_Main(argc, argv, TK_LOCAL_APPINIT);
    return 0;			/* Needed only to prevent compiler warning. */
}


#if defined(__i386__) && defined(__GNUC__)

#define HAVE_NS_SETUP_FPU	/* convenience flag to check on later */
	
/* This function is supposed to set up a uniform FPU state on all i386
 * platforms.  It may (should) be called instead of functions in the
 * fe- family.
 */
static void ns_setup_fpu() {

static const int NS_FPU_CW_IC  = 0x1000; /* Infty control(12): support +/- infinity */
static const int NS_FPU_CW_RC  = 0x0000; /* Round control(11,10): to nearest */
static const int NS_FPU_CW_PC  = 0x0200; /* Precision control(9,8): 53 bits */
static const int NS_FPU_CW_IEM = 0x0000; /* Interrupt enable mask(7): enabled */
static const int NS_FPU_CW_B6  = 0x0040; /* undefined, set to one in my FPU */
static const int NS_FPU_CW_PM  = 0x0020; /* Precision mask(5), inexact exception: disabled */
static const int NS_FPU_CW_UM  = 0x0010; /* Underflow mask(4): disabled */
static const int NS_FPU_CW_OM  = 0x0000; /* Overflow mask(3): enabled */
static const int NS_FPU_CW_ZM  = 0x0000; /* Zero divide mask(2): enabled */
static const int NS_FPU_CW_DM  = 0x0002; /* Denormalized operand(1): disabled */
static const int NS_FPU_CW_IM  = 0x0000; /* Invalid operation mask(0): enabled */

static const int NS_FPU_CW    = NS_FPU_CW_IC
				| NS_FPU_CW_RC
				| NS_FPU_CW_PC  
				| NS_FPU_CW_IEM 
				| NS_FPU_CW_B6  
				| NS_FPU_CW_PM  
				| NS_FPU_CW_UM  
				| NS_FPU_CW_OM  
				| NS_FPU_CW_ZM	
				| NS_FPU_CW_DM  
				| NS_FPU_CW_IM;

	unsigned short _cw = NS_FPU_CW;
	asm ("fldcw %0" : : "m" (*&_cw));
}
#endif /* !HAVE_NS_SETUP_FPU && __i386__ && __GNUC__ */

#if !defined(HAVE_FESETPRECISION) && defined(__i386__) && defined(__GNUC__)
// use our own!
#define HAVE_FESETPRECISION
/*
 * From:
 |  Floating-point environment <fenvwm.h>                                    |
 | Copyright (C) 1996, 1997, 1998, 1999                                      |
 |                     W. Metzenthen, 22 Parker St, Ormond, Vic 3163,        |
 |                     Australia.                                            |
 |                     E-mail   billm@melbpc.org.au                          |
 * used here with permission.
 */
#define FE_FLTPREC       0x000
#define FE_INVALIDPREC   0x100
#define FE_DBLPREC       0x200
#define FE_LDBLPREC      0x300
/*
 * From:
 * fenvwm.c
 | Copyright (C) 1999                                                        |
 |                     W. Metzenthen, 22 Parker St, Ormond, Vic 3163,        |
 |                     Australia.  E-mail   billm@melbpc.org.au              |
 * used here with permission.
 */
/*
  Set the precision to prec if it is a valid
  floating point precision macro.
  Returns 1 if precision set, 0 otherwise.
  */
static inline int fesetprecision(int prec)
{
  if ( !(prec & ~FE_LDBLPREC) && (prec != FE_INVALIDPREC) )
    {
      unsigned short cw;
      asm ("fnstcw %0":"=m" (*&cw));
      asm ("fwait");

      cw = (cw & ~FE_LDBLPREC) | (prec & FE_LDBLPREC);

      asm volatile ("fldcw %0" : /* Don't push these colons together */ : "m" (*&cw));
      return 1;
    }
  else
    return 0;
}
#endif /* !HAVE_FESETPRECISION && __i386__ && __GNUC__ */


/*
 * setup_floating_point_environment
 *
 * Set up the floating point environment to be as standard as possible.
 *
 * For example:
 * Linux i386 uses 60-bit floats for calculation,
 * not 56-bit floats, giving different results.
 * Fix that.
 *
 * See <http://www.linuxsupportline.com/~billm/faq.html>
 * for why we do this fix.
 *
 * This function is derived from wmexcep
 *
 */
static inline void
setup_floating_point_environment()
{
#ifdef HAVE_NS_SETUP_FPU

	ns_setup_fpu();

#else /* !HAVE_NS_SETUP_FPU */

	// In general, try to use the C99 standards to set things up.
	// If we can't do that, do nothing and hope the default is right.
#ifdef HAVE_FESETPRECISION
	fesetprecision(FE_DBLPREC);
#endif

#ifdef HAVE_FEENABLEEXCEPT
	/*
	 * In general we'd like to catch some serious exceptions (div by zero)
	 * and ignore the boring ones (overflow/underflow).
	 * We set up that up here.
	 * This depends on feenableexcept which is (currently) GNU
	 * specific.
	 */
	int trap_exceptions = 0;
#ifdef FE_DIVBYZERO
	trap_exceptions |= FE_DIVBYZERO;
#endif
#ifdef FE_INVALID
	trap_exceptions |= FE_INVALID;
#endif
#ifdef FE_OVERFLOW
	trap_exceptions |= FE_OVERFLOW;
#endif
//#ifdef FE_UNDERFLOW
//	trap_exceptions |= FE_UNDERFLOW;
//#endif
	
	feenableexcept(trap_exceptions);
#endif /* HAVE_FEENABLEEXCEPT */
#endif /* !HAVE_NS_SETUP_FPU */
}




/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppInit --
 *
 *	This procedure performs application-specific initialization.
 *	Most applications, especially those that incorporate additional
 *	packages, will have their own version of this procedure.
 *
 * Results:
 *	Returns a standard Tcl completion code, and leaves an error
 *	message in interp->result if an error occurs.
 *
 * Side effects:
 *	Depends on the startup script.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_AppInit(Tcl_Interp *interp)
{
#ifdef MEMDEBUG_SIMULATIONS
        extern MemTrace *globalMemTrace;
        globalMemTrace = new MemTrace;
#endif

	setup_floating_point_environment();
       
	if (Tcl_Init(interp) == TCL_ERROR ||
	    Otcl_Init(interp) == TCL_ERROR)
		return TCL_ERROR;

    if (Tk_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }


    Tcl_StaticPackage(interp, "Tk", Tk_Init, Tk_SafeInit);
#ifdef TK_TEST
    if (Tktest_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "Tktest", Tktest_Init,
            (Tcl_PackageInitProc *) NULL);
#endif /* TK_TEST */


    /*
     * Call the init procedures for included packages.  Each call should
     * look like this:
     *
     * if (Mod_Init(interp) == TCL_ERROR) {
     *     return TCL_ERROR;
     * }
     *
     * where "Mod" is the name of the module.
     */
#ifdef HAVE_LIBTCLDBG
	extern int Tcldbg_Init(Tcl_Interp *);   // hackorama
	if (Tcldbg_Init(interp) == TCL_ERROR) {
		return TCL_ERROR;
	}
#endif



	// copied from ns2.28's tclAppinit.cc
	Tcl_SetVar(interp, "tcl_rcFileName", "~/.ns.tcl", TCL_GLOBAL_ONLY);
	Tcl::init(interp, "ns");
	init_misc();
    et_ns_ptypes.load();
	et_ns_lib.load();

	// copied from ns2.28's old tkAppinit.cc

	// it seems that using tk8.4.5 this line isnt' needed anymore. enabling it produces an error.
	// HOWEVER, do enable it if using tk8.3 and below!
	// et_tk.load();                  //      <<<<-------- this line
	
    Tcl::instance().tkmain(Tk_MainWindow(interp));
    loadbitmaps(interp);


#ifdef TK_TEST
	if (Tcltest_Init(interp) == TCL_ERROR) {
		return TCL_ERROR;
	}
	Tcl_StaticPackage(interp, "Tcltest", Tcltest_Init,
			  (Tcl_PackageInitProc *) NULL);
#endif /* TK_TEST */

	return TCL_OK;
}

#ifndef WIN32
void
abort()
{
	Tcl& tcl = Tcl::instance();
	tcl.evalc("[Simulator instance] flush-trace");
#ifdef abort
#undef abort
	abort();
#else
	exit(1);
#endif /*abort*/
	/*NOTREACHED*/
}
#endif

NS_END_EXTERN_C




