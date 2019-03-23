#!/usr/bin/python
import numpy as np
import sys,getopt
import subprocess
import ConfigParser
import random

iosize = 128      # KB
iosize_sectors = 256  # number of sectors
default_avg_iops = 30.0 * 1024 / iosize
default_stdv_iops = 5.0 * 1024 / iosize
rw = 0

def get_device():
    data = subprocess.check_output(['lsblk', '-o', 'NAME,SIZE'])
    device_str = data.split('\n')[1:-1]

    for line_str in device_str:
        if line_str.find('\xe2\x94') != -1 or line_str.find('`') != -1:
            devname = line_str.split()[1][1:-1]
            size = line_str.split()[2]
            size = size[0:size.find('G')]
            return '/dev/'+devname, int(size)

def set_device(devname):
    config = ConfigParser.ConfigParser()
    config.read("config/config.ini")
    config.set('dev','device',devname)
    with open('config/config.ini', 'wb') as configfile:
    	config.write(configfile)

def generate(iops_array, maximal_sectors):
    filepath = 'trace/trace.txt'
    f = open(filepath, "w")
    
    offset = 0
    time_elapsed=0
    for iops in iops_array:
        if iops == 0:        # iops cannot be 0
            continue
        interval = 1000.0 / abs(iops)         # iops and interval must be positive
        timestamp = 0 # unit: ms
        while timestamp <= 1000:
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
        #avg_iops = random.uniform(0, max_iops)
        #stdv_iops = random.uniform(0, 0.5 * avg_iops)
        #iops = np.random.normal(avg_iops, stdv_iops, stage_len)
        #iops_array = np.append(iops_array, iops)
    
        # Third stage
        #avg_iops = max_iops
        #iops = np.random.normal(avg_iops, 0.1, stage_len)
        #iops_array = np.append(iops_array, iops)

        # Fourth stage
        avg_iops = random.uniform(0, max_iops)
        stdv_iops = random.uniform(0, 0.5 * avg_iops)
        iops = np.random.normal(avg_iops, stdv_iops, stage_len)
        iops_array = np.append(iops_array, iops)

    elif distrib == 'poiss':
        # Second stage
        #avg_iops = random.uniform(0, max_iops)
        #iops = np.random.poisson(avg_iops, stage_len)
        #iops_array = np.append(iops_array, iops)

        # Third stage
        #avg_iops = max_iops
        #iops = np.random.poisson(avg_iops, stage_len)
        #iops_array = np.append(iops_array, iops)

        # Fourth stage
        avg_iops = random.uniform(0, max_iops)
        iops = np.random.poisson(avg_iops, stage_len)
        iops_array = np.append(iops_array, iops)

    return iops_array



if __name__ == '__main__':    
    opts, args = getopt.getopt(sys.argv[1:],"hw:f:s:",["distrib=", "max=", "start="])
    for opt, arg in opts:
        if opt == '-h':
            print 'python TraceGenerator.py -w <warmup> -f <full> -s <stdv>'
            sys.exit()
        elif opt == '-w':
            warmup = float(arg)    # in MB/s
            warmup_iops = warmup * 1024 / iosize
        elif opt == '-f':
            full = float(arg)    # in MB/s
            full_iops = full * 1024 / iosize
        elif opt == '-s':
            stdv = float(arg)
            stdv_iops = stdv * 1024 / iosize

    #warmup_len = 300   #5min
    #stable_len = 600   #10min
    iops_array = np.random.normal(full_iops,stdv_iops,900)

    #iops = np.random.normal(full_iops,stdv_iops,stable_len)
    #iops_array = np.append(iops_array, iops)

    devname,size = get_device()
    print 'Device name %s. Size: %d' % (devname,size)
    set_device(devname)
    maximal_sectors = size * 1024 * 1024 * 2
    generate(iops_array, maximal_sectors)      
