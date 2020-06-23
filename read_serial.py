import serial
import subprocess
import io

ser =  serial.Serial('/dev/ttyUSB0', 115200)

def execute_command(command):
    proc = subprocess.Popen(command, shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    debug = True
    if debug:
        for l in io.TextIOWrapper(proc.stdout, encoding='utf-8'):
            print(l, end="")
        for l in io.TextIOWrapper(proc.stderr, encoding='utf-8'):
            print(l, end="")

def get_result(c):
    ser.write(c)
    result = ser.read_until(b'EOF')
    ser.write(b'r')
    start = result.find(b'System Booted')
    if start == -1:
        start = 0
    return result[start:]


def build_and_run():
    execute_command([
        'make',
    ])
    execute_command([
        "fastboot",
        "boot",
        "kernel.img",
    ])
