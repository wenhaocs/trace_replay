#!/bin/bash

sleep 60
python TraceGenerator_2stage.py -m $1 -s $2 -l $3 --distrib=$4 --max=$5
./replay
