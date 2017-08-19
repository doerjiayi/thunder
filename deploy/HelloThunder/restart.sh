#!/bin/bash

SERVER_HOME=`dirname $0`
SCRIPT_NAME=`basename $0`
cd ${SERVER_HOME}
SERVER_HOME=`pwd`

${SERVER_HOME}/stop.sh
${SERVER_HOME}/start.sh

