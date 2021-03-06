#!/bin/sh

NAME=wzplayer

cd /tmp
git clone 'https://github.com/wilbert2000/wzplayer.git' ${NAME}
cd $(NAME)
VERSION=`git describe --tags --abbrev=0`
cd ..
NAME_WITH_VERSION=${NAME}-${VERSION}
NAME_TAR=${NAME_WITH_VERSION}.tar.bz2

mv $(NAME) ${NAME_WITH_VERSION}
tar cvjf --exclude=.git ${NAME_TAR} ${NAME_WITH_VERSION}/
cat ${NAME_WITH_VERSION}/${NAME}.spec | sed -e 's/%define version [a-zA-Z0-9\.]*$/%define version '${VERSION}'/' > ${NAME}.spec
rm -r ${NAME_WITH_VERSION}

PCKGDIR=/usr/src/packages
if [ -e /etc/fedora-release ]; then
    PCKGDIR=/usr/src/redhat
fi
if [ -e /etc/mandrake-release ]; then
    PCKGDIR=/usr/src/rpm
fi
cp ${NAME_TAR} ${PCKGDIR}/SOURCES/
rpmbuild -bb --clean --rmsource ${NAME}.spec
