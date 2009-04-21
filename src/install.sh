#!/bin/bash

function wantset () {
	local var=$1

	eval "[ -z \"\$$var\" ] && echo \"$var needs to be set!\" && exit 1"
}

function die () {
	echo "$1"
	exit 1
}

wantset DESTDIR
wantset BINDIR
wantset DOCDIR
wantset PROG

wantset LOCALDATA
wantset LOCALDOCS

[ ! -d $LOCALDATA ]	&& die "$LOCALDATA not a directory!"
[ ! -d $LOCALDOCS ]	&& die "$LOCALDOCS not a directory!"
[ ! -f $PROG ]		&& die "$PROG not a file!"

echo "### Installing into $DESTDIR ###"
echo "# Data dir: ${DATADIR}"
echo "# Docs dir: ${DOCDIR}"
echo "# Bin dir:  ${BINDIR}" 

echo "** Installing binary **"
binpath="${DESTDIR}/${BINDIR}/"
mkdir -p "${binpath}" && cp ${PROG} "${binpath}/${PROG}" || die "Couldn't install binary!"

echo "** Installing data **"
datapath="${DESTDIR}/${DATADIR}/"
mkdir -p "${datapath}" && cp -R ${LOCALDATA}/* "${datapath}/" || die "Couldn't install data!"

echo "** Installing docs **"
docpath="${DESTDIR}/${DOCDIR}/"
mkdir -p ${docpath} && cp -R ${LOCALDOCS}/* "${docpath}/" || die "Couldn't install docs!"

echo "** Finished **"
