#
# Makefile for vint docs
#

SRCDOCS=	everything.tex
DVIOUT=		everything.dvi
POSTOUT=	everything.ps
CONVERT=	fig2dev
PICTURES= \
	agent.eps \
	mcast_classes.eps \
	multinode.eps \
	node.eps packet.eps


LATEX=		latex
DVIPS=		dvips

.SUFFIXES:	.dvi .eps .tex .fig .ps
##.PRECIOUS:	$(DVIOUT)

all: doit $(PICTURES) $(POSTOUT)
$(POSTOUT): $(DVIOUT)
	
.tex.dvi:
	$(LATEX) $*
	rm -f $*.b[bl]
	$(LATEX) $*
	$(LATEX) $*

.dvi.ps:
	$(DVIPS) -o $@ $<

.fig.eps:
	rm -f $@
	$(CONVERT) -L pstex $< > $@

doit:
	rm -f $(POSTOUT)
	rm -f *.bbl *.blg

clean: doit

veryclean: clean
	rm -f *.aux
	rm -f $(PICTURES) $(DVIOUT)
