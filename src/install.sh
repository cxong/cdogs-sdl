#!/bin/sh

function wantset () {
	local var=$1

	eval "[ -z \"\$$var\" ] && echo \"$var needs to be set!\" && exit 1"
}

wantset DESTDIR
wantset BINDIR
wantset DOCDIR
wantset PROG

wantset LOCALDATA
wantset LOCALDOCS

[ ! -d $LOCALDATA ]	&& echo "$LOCALDATA not a directory!" && exit 1
[ ! -d $LOCALDOCS ]	&& echo "$LOCALDOCS not a directory!" && exit 1
[ ! -f $PROG ]		&& echo "$PROG not a file!" && exit 1 

echo "### Installing into $DESTDIR ###"
echo "# Data dir: ${DATADIR}"
echo "# Docs dir: ${DOCDIR}"
echo "# Bin dir:  ${BINDIR}" 

echo "** Installing data **"
datapath="${DESTDIR}/${DATADIR}"
mkdir -p "${datapath}"
cp -R "${LOCALDATA}" "${datapath}/"

echo "** Installing docs **"
docpath="${DESTDIR}/${DOCDIR}/"
mkdir -p "${docpath}"
cp -R "${LOCALDOCS}" "${docpath}"

echo "** Installing binary **"
binpath="${DESTDIR}/${DOCDIR}/"
mkdir -p "${binpath}"
cp "${PROG}" "${binpath}/${PROG}"
chmod +x "${binpath}/${PROG}"

echo "** Finished **"
