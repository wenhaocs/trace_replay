#!/usr/bin/python
import numpy as np
import sys,getopt
import subprocess
import ConfigParser
import random


def get_device(devsize):
    data = subprocess.check_output(['lsblk', '-o', 'NAME,SIZE'])
    device_str = data.split('\n')[1:-1]

    for line_str in device_str:
        if line_str.find('\xe2\x94') != -1 or line_str.find('`') != -1:
            devname = line_str.split()[1][1:-1]
            size = line_str.split()[2]
            size = size[0:size.find('G')]
            if size == devsize:        # make sure it is the device the application requests
                return '/dev/'+devname, int(size)

def set_device(devname):
    config = ConfigParser.ConfigParser()
    config.read("config/config.ini")
    config.set('dev','device',devname)
    with open('config/config.ini', 'wb') as configfile:
    	config.write(configfile)


if __name__ == '__main__':
    opts, args = getopt.getopt(sys.argv[1:],"hm:s:l:",["distrib=", "max=", "devsize="])
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
        elif opt == '--devsize':
            devsize = arg

    devname,size = get_device(devsize)
    print 'Device name %s. Size: %d' % (devname,size)
    set_device(devname)
