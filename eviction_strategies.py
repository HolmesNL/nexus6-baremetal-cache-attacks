import serial
import io
import os
import sys
import subprocess
import time

ser = serial.Serial('/dev/ttyUSB0', 115200)

ES_EVICTION_COUNTERS = [x for x in range(7,17)] + [50]
ES_NUMBER_OF_ACCESSES_IN_LOOPS = range(1,7)
ES_DIFFERENT_ADDRESSES_IN_LOOPS = [1]

strategy_dir = os.path.join(os.path.abspath(os.path.dirname(__file__)), 'include/strategies')
#output_dir = '../../../thesis/plots/input/histograms/prime_probe3/'
output_dir = '../../../thesis/plots/input/histograms/evict_reload1/'
os.makedirs(strategy_dir, exist_ok=True)
os.makedirs(output_dir, exist_ok=True)

def execute_command(command):
    proc = subprocess.Popen(command, shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    debug = True
    if debug:
        for l in io.TextIOWrapper(proc.stdout, encoding='utf-8'):
            print(l, end="")
        for l in io.TextIOWrapper(proc.stderr, encoding='utf-8'):
            print(l, end="")


for ES_EVICTION_COUNTER in ES_EVICTION_COUNTERS:
    for ES_NUMBER_OF_ACCESSES_IN_LOOP in ES_NUMBER_OF_ACCESSES_IN_LOOPS:
        for ES_DIFFERENT_ADDRESSES_IN_LOOP in ES_DIFFERENT_ADDRESSES_IN_LOOPS:
            strategy = "{}-{}-{}".format(ES_EVICTION_COUNTER, ES_NUMBER_OF_ACCESSES_IN_LOOP, ES_DIFFERENT_ADDRESSES_IN_LOOP)
            strategy_file = os.path.join(strategy_dir, strategy + '.h')
            output_file = os.path.join(output_dir, strategy + '.txt')
            with open(strategy_file, 'w') as f:
                f.write(f"#define ES_EVICTION_COUNTER {ES_EVICTION_COUNTER}\n")
                f.write(f"#define ES_NUMBER_OF_ACCESSES_IN_LOOP {ES_NUMBER_OF_ACCESSES_IN_LOOP}\n")
                f.write(f"#define ES_DIFFERENT_ADDRESSES_IN_LOOP {ES_DIFFERENT_ADDRESSES_IN_LOOP}\n")
            execute_command([
                'make',
                f"STRATEGY={strategy_file}",
            ])
            execute_command([
                "fastboot",
                "boot",
                "kernel.img",
            ])
            ser.write(b't')
            result = ser.read_until(b'EOF')
            ser.write(b'r')
            start = result.find(b'System Booted')
            if start == -1:
                start = 0
            with open(output_file, 'wb') as f:
                f.write(result[start:])
