#!/bin/sh

LOCALDATA="../data/"

echo Installing into $DESTDIR

install -d ${LOCALDATA} ${DESTDIR}${DATADIR}

echo -e "Installing data into ${DESTDIR}${DATADIR}\n\n"

for dir in `find $LOCALDATA -type d` ; do
	NEWDIR=`basename $dir`
	echo "Dir --> ${DESTDIR}${DATADIR}${NEWDIR}"
	install -d ${DESTDIR}${DATADIR}${NEWDIR}
	
	for file in `find $dir -type f` ; do
		NEWFILE=`basename $file`
		echo " --> ${DESTDIR}${DATADIR}${NEWDIR}/${NEWFILE}"
		install $file ${DESTDIR}${DATADIR}${NEWDIR}/${NEWFILE}
	done
done

echo -e "\nInstalling Binary into ${DESTDIR}${BINDIR}"

install -d ${DESTDIR}${BINDIR}
install cdogs ${DESTDIR}${BINDIR}

echo "** Finished **"
