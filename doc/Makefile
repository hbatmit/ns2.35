#
# $Id: Makefile,v 1.38 2010/05/09 21:55:55 tom_henderson Exp $
#

# by default, just assume latex2html is in the user's path
LATEX2HTML=	latex2html
L2H_WORKING_DIR = ./l2h
WEB_TARGET_DIR = /nfs/web/isi.edu/htdocs/division7/nsnam/ns/doc

LATEX=          latex
BIBTEX=         bibtex
DVIPS=          dvips
CP=		/bin/cp
RM=		/bin/rm

FILES= \
	asim.tex \
	agents.tex \
	applications.tex \
	worm-model.tex \
	basic.tex \
	delays.tex \
	dynamics.tex \
	emulation.tex \
	error_model.tex \
	everything.tex \
	lan.tex \
	links.tex \
	math.tex \
	multicast.tex \
	nodes.tex \
	otcl.tex \
	packet_format.tex \
	queue_mgmt.tex \
	session.tex \
	sim.tex \
	srm.tex \
	tcp.tex \
	timers.tex \
	trace.tex \
	udp.tex \
	undocumented.tex \
	unicast.tex \
	webcache.tex \
	address.tex \
	hier-rtg.tex \
	mobility.tex \
	newnode.tex \
	debug.tex \
	namtrace.tex \
	energymodel.tex \
	plm.tex \
	propagation.tex \
	testsuite.tex \
	diffserv.tex \
	educational.tex \
	codestyle.tex \
	diffusion.tex \
	sctp.tex \
	xcp.tex

all: everything.dvi everything.ps.gz everything.html everything.pdf

html-pre-process:
	$(RM) -rf $(L2H_WORKING_DIR)
	mkdir $(L2H_WORKING_DIR)
	$(CP) -r *.sty *.tex *.aux *.bib *.bbl *.log figures/*.eps $(L2H_WORKING_DIR)
	cd $(L2H_WORKING_DIR); 	../pre-process.pl .

html-process:
	cd $(L2H_WORKING_DIR); $(LATEX2HTML) -local_icons -toc_depth 6 -show_section_numbers everything.tex

html-post-process:
	cd $(L2H_WORKING_DIR)/everything; ../../post-process.pl .

everything.html: html-pre-process html-process html-post-process
	@echo html results are in  $(L2H_WORKING_DIR)/everything


everything.ps: $(FILES) everything.dvi
	dvips -f everything >everything.ps

everything.ps.gz: $(FILES) everything.dvi everything.ps
	gzip < everything.ps >everything.ps.gz

everything.pdf: $(FILES) everything.ps
	ps2pdf everything.ps

everything.dvi: $(FILES)
	$(LATEX) everything
	@if egrep 'Rerun to get' everything.log; then echo 'Relatexing for cross-references.'; $(LATEX) everything; fi

gv: everything.ps $(FILES)
	gv everything.ps

# xxx: doesn't redo bibliography
clean:
	rm -f *.aux *.log *.toc everything.dvi everything.ps everything.pdf

ns-doc-auto:
	$(MAKE) everything.ps.gz
	$(MAKE) everything.pdf
	$(MAKE) everything.html
	cp everything.ps.gz  $(WEB_TARGET_DIR)/ns_doc.ps.gz
	cp everything.pdf $(WEB_TARGET_DIR)/ns_doc.pdf
	cp $(L2H_WORKING_DIR)/everything/*.{html,css} $(WEB_TARGET_DIR)
	cp $(L2H_WORKING_DIR)/everything/*.png $(WEB_TARGET_DIR)


