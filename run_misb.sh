#!/bin/bash

sleep 60
python TraceGenerator_misb.py -l $1 -f $2 -m $3
./replay
