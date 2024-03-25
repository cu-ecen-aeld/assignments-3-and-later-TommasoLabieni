#!/bin/sh

case $# in
    2)  
        FILESDIR=${1}
        SEARCHSTR=${2}
        ;;
    *)  echo "ERROR: Wrong number of params!"
        exit 1
        ;;
esac

if ! test -d ${FILESDIR}
then
    echo "ERROR: ${FILESDIR} does not exists!"
    exit 1
fi

NUMFILES=0
MATCH=0

for el in `find ${FILESDIR} -type f`
do
    NUMFILES=$((NUMFILES + 1))
    if grep ${SEARCHSTR} ${el} -q
    then
        MATCH=$((MATCH + 1))
    fi
done

echo "The number of files are ${NUMFILES} and the number of matching lines are ${MATCH}"