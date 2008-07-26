bin_PROGRAMS = vrack

vrack_SOURCES = \
	main.c main.h \
	gui.c gui.h \
	vnc.c vnc.h \
	rack.c rack.h \
	display.c display.h \
	kvm.c kvm.h \
	switch.c switch.h \
	qmach.c qmach.h \
	misc.c misc.h \
	cmdq.c cmdq.h \
	tempdir.c tempdir.h

vrack_CFLAGS = \
	-I$(top_srcdir) -I$(top_builddir) \
	-Wall -ansi \
	@DBG_CFLAGS@ \
	@GLIB_CFLAGS@ @GTK_CFLAGS@ @GTKVNC_CFLAGS@ \
	-DDEBUG=@DEBUG@

vrack_LDFLAGS = \
	@GTK_LIBS@ @GTKVNC_LIBS@
