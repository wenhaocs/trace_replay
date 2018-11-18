#!/usr/bin/python
import numpy as np
import sys,getopt
import subprocess
import ConfigParser
import random

iosize = 128      # KB
iosize_sectors = 256  # number of sectors
maximal_sectors = 100000000000000    # maximal number sectors
default_avg_iops = 30.0 * 1024 / iosize
default_stdv_iops = 5.0 * 1024 / iosize
rw = 0

def get_device():
    data = subprocess.check_output(['lsblk', '-o', 'NAME'])
    device_str = data.split('\n')[1:-1]

    for line_str in device_str:
        if line_str.find('\xe2\x94') != -1:
            devname = line_str.split()[1][1:-1]
            return '/dev/'+devname

def set_device(devname):
    config = ConfigParser.ConfigParser()
    config.read("config/config.ini")
    config.set('dev','device',devname)
    with open('config/config.ini', 'wb') as configfile:
    	config.write(configfile)

def generate(iops_array):
    filepath = 'trace/trace.txt'
    f = open(filepath, "w")
    
    offset = 0
    time_elapsed=0
    for iops in iops_array:
        interval = 1000.0 / iops
        timestamp = 0 # unit: ms
        while timestamp <= 1000.0:
            string = str(time_elapsed+timestamp)+ ' ' + str(offset) + ' ' + str(iosize_sectors) + ' ' + str(rw) + '\n'
            f.write(string)
            timestamp = timestamp + interval
            offset = (offset + iosize_sectors) % (maximal_sectors - iosize_sectors)
        time_elapsed = time_elapsed + 1000
    f.close()


def vary_array(iops_array, max_iops, stage_len, distrib):
    random.seed()
    if distrib == 'normal':
        # Second stage
        avg_iops = random.uniform(0, max_iops)
        stdv_iops = random.uniform(0, 0.5 * avg_iops)
        iops = np.random.normal(avg_iops, stdv_iops, stage_len)
        iops_array = np.append(iops_array, iops)
    
        # Third stage
        avg_iops = max_iops
        iops = np.random.normal(avg_iops, 0.1, stage_len)
        iops_array = np.append(iops_array, iops)

        # Fourth stage
        avg_iops = random.uniform(0, max_iops)
        stdv_iops = random.uniform(0, 0.5 * avg_iops)
        iops = np.random.normal(avg_iops, stdv_iops, stage_len)
        iops_array = np.append(iops_array, iops)

    elif distrib == 'poiss':
        # Second stage
        avg_iops = random.uniform(0, max_iops)
        iops = np.random.poisson(avg_iops, stage_len)
        iops_array = np.append(iops_array, iops)

        # Third stage
        avg_iops = max_iops
        iops = np.random.poisson(avg_iops, stage_len)
        iops_array = np.append(iops_array, iops)

        # Fourth stage
        avg_iops = random.uniform(0, max_iops)
        iops = np.random.poisson(avg_iops, stage_len)
        iops_array = np.append(iops_array, iops)

    return iops_array



if __name__ == '__main__':    
    opts, args = getopt.getopt(sys.argv[1:],"hm:s:l:",["distrib=", "max="])
    for opt, arg in opts:
        if opt == '-h':
            print 'python TraceGenerator.py -m <average> -s <stdv> -l <length in seconds> --distrib=<distribution>'
            sys.exit()
        elif opt == '-m':
            average = float(arg)    # in MB/s
            avg_iops = average * 1024 / iosize
        elif opt == '-s':
            stdv = float(arg)
            stdv_iops = stdv * 1024 / iosize
        elif opt == '-l':
            length = float(arg)
        elif opt == '--distrib':
            distrib = arg
        elif opt == '--max':
            max_tp = float(arg)
            max_iops = max_tp * 1024 / iosize

    stage_len = length / 4
    if distrib == 'normal':
        try:
            iops_array = np.random.normal(avg_iops,stdv_iops, stage_len)
        except Exception as e:
            print e, 'Using default avg and stdv iops.'
            avg_iops = default_avg_iops
            stdv_iops = default_stdv_iops
            iops_array = np.random.normal(avg_iops, stdv_iops, stage_len)
    elif distrib == 'poiss':
        try:
            iops_array = np.random.poisson(avg_iops, stage_len)
        except Exception as e:
            print e, 'Using default avg iops.'
            avg_iops = default_avg_iops
            iops_array = np.random.normal(avg_iops, stage_len)
    else:
        print 'Distribution %s unsupported' % distrib
        sys.exit()
    
    iops_array = vary_array(iops_array, max_iops, stage_len, distrib)

    devname = get_device()
    set_device(devname)
    generate(iops_array)      
