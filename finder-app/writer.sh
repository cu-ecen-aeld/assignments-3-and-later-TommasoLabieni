#!/bin/sh

case $# in
    2)  
        WRITEFILE=${1}
        WRITESTR=${2}
        ;;
    *)  echo "ERROR: Wrong number of params!"
        exit 1
        ;;
esac

DIR=`dirname ${WRITEFILE}`

if ! test -d ${DIR}
then
    mkdir -p ${DIR}
fi

echo ${WRITESTR} > ${WRITEFILE}