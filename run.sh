#!/bin/bash

sleep 60
python TraceGenerator.py -m $1 -s $2 -l $3 --distrib=$4
./replay
