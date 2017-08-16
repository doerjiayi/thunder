#!/bin/bash

THUNDER_HOME=`dirname $0`
SCRIPT_NAME=`basename $0`
cd ${THUNDER_HOME}
THUNDER_HOME=`pwd`

echo "yes" | ${THUNDER_HOME}/stop.sh $@
${THUNDER_HOME}/start.sh $@

