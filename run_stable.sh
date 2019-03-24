#!/bin/bash

sleep 60
python TraceGenerator_stable.py -w $1 -f $2 -s $3 --devsize=$4
./replay
