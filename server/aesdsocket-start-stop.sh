#!/bin/bash

if test ${#} -ne 1
then
    echo "Wrong number of parameters"
    exit 1
fi

case ${1} in
"start")
    echo "Running aesdsocket daemon"
    start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket
    ;;
"stop")
    echo "Stopping aesdsocket daemon"
    start-stop-daemon -K -n aesdsocket
    ;;
*)
    echo "Invalid option \"${1}\""
    exit 2
    ;;
esac

exit 0