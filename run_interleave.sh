#!/bin/bash

sleep 60
python TraceGenerator_interleave.py -s $1 -t $2 -b $3 --start=$4
./replay
