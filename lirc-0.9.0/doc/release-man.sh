#!/bin/sh
select_vars ()
{
	PROG=$1
	SECTION=1
	case $PROG in
	irpty)
		MANPAGE=$PROG.$SECTION
		DIR=$TOPDIR/tools/
	;;
	irw)
		MANPAGE=$PROG.$SECTION
		DIR=$TOPDIR/tools/
	;;
	irexec)
		MANPAGE=$PROG.$SECTION
		DIR=$TOPDIR/tools/
	;;
	ircat)
		MANPAGE=$PROG.$SECTION
		DIR=$TOPDIR/tools/
	;;
	irxevent)
		MANPAGE=$PROG.$SECTION
		DIR=$TOPDIR/tools/
	;;
	mode2)
		MANPAGE=$PROG.$SECTION
		DIR=$TOPDIR/tools/
		PROG_PRE_PARAMS="--include ${SRCDIR}/man-source/mode2-common.inc"
	;;
	lircrcd)
		MANPAGE=$PROG.$SECTION
		DIR=$TOPDIR/tools/
	;;
	smode2)
		MANPAGE=$PROG.$SECTION
		DIR=$TOPDIR/tools/
		PROG_PRE_PARAMS="--include ${SRCDIR}/man-source/mode2-common.inc"
	;;
	xmode2)
		MANPAGE=$PROG.$SECTION
		DIR=$TOPDIR/tools/
		PROG_PRE_PARAMS="--include ${SRCDIR}/man-source/mode2-common.inc"
	;;
	irsend)
		MANPAGE=$PROG.$SECTION
		DIR=$TOPDIR/tools/
	;;
	irrecord)
		MANPAGE=$PROG.$SECTION
		DIR=$TOPDIR/daemons/
	;;
	lircd)
		SECTION=8
		MANPAGE=$PROG.$SECTION
		DIR=$TOPDIR/daemons/
		PROG_PARAMS="--include ${SRCDIR}/man-source/daemons.inc"

	;;
	lircmd)
		SECTION=8
		MANPAGE=$PROG.$SECTION
		DIR=$TOPDIR/daemons/
		PROG_PARAMS="--include ${SRCDIR}/man-source/daemons.inc"
	;;
	esac

	if [ ! -x $DIR$PROG ]
	# binary is not built or we are not in the source tree. Lets hope it
	# is on the PATH
	then
		DIR=""
	fi
}

PATH="$PATH:/usr/local/sbin:/sbin:/usr/sbin"

TOPDIR=${top_builddir:-..}
SRCDIR=${srcdir:-.}
BUILDDIR=${builddir:-.}

##########################
## Start

install -d man-html

HELP2MAN=help2man
MAN2HTML=${BUILDDIR}/man2html

for PROG in irpty irexec ircat irw mode2 smode2 xmode2 irsend irrecord lircd lircmd lircrcd irxevent
do
	PROG_PARAMS=""
	PROG_PRE_PARAMS=""
	select_vars $PROG

	#make the manpage
	$HELP2MAN \
		$PROG_PRE_PARAMS \
		--section $SECTION \
		--no-info \
		--include ${SRCDIR}/man-source/help2man.inc \
		--opt-include ${SRCDIR}/man-source/$PROG.inc \
		$PROG_PARAMS \
		$DIR$PROG -o man/$MANPAGE

	# since libtool is used help2man prepends "lt-" to some executable
	# names, we could require lirc to be installed and this wouldn't
	# happen
	sed \
		-e 's/lt-irexec/irexec/' \
		-e 's/lt-ircat/ircat/' \
		-e 's/LT-IRPTY/IRPTY/' \
		-e 's/lt-irpty/irpty/' \
		-e 's/lt-irxevent/irxevent/' \
		-e 's/LT-IRXEVENT/IRXEVENT/' \
		-e 's/\([0-9]\)-CVS/\1/' \
		-e 's/\([0-9]\)-git/\1/' \
			man/$MANPAGE > man/$MANPAGE.tmp

	mv man/$MANPAGE.tmp man/$MANPAGE


	#make the html page
	$MAN2HTML $PWD/man/$MANPAGE | sed \
					-e '1,/<BODY>/d' \
					-e '/<\/BODY>/,$d' \
					-e '/HREF="#index"/d' \
					-e '/NAME="index"/,/^Time:/d' \
					-e '/SEE ALSO/,/^<HR>/d' \
						> man-html/$PROG.html
done

