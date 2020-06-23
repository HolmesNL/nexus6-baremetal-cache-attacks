import read_serial
import os
import ast
import matplotlib.pyplot as plt

INDEXES_TO_MONITOR = [x for x in range(2048)]

address_file = os.path.join(os.path.abspath(os.path.dirname(__file__)), 'include/address.h')
#output_dir = os.path.join(os.path.abspath(os.path.dirname(__file__)), '../../../thesis/plots/input/rsa_trustzone_replacement_cache_sets/')
#output_dir = os.path.join(os.path.abspath(os.path.dirname(__file__)), '../../../thesis/plots/input/rsa_trustzone_cache_sets/')
output_dir = os.path.join(os.path.abspath(os.path.dirname(__file__)), '../../../thesis/plots/input/rsa_trustzone_cache_sets_gaps/')
os.makedirs(output_dir, exist_ok=True)

def monitor_index(INDEX_TO_MONITOR, write=True):
    with open(address_file, 'w') as f:
        f.write(f"#define INDEX_TO_MONITOR {INDEX_TO_MONITOR}")
    read_serial.build_and_run()
    result = read_serial.get_result(b'p')
    if write:
        with open(os.path.join(output_dir, f"{INDEX_TO_MONITOR}.csv"), 'wb') as f:
            f.write(result)
    else:
        nums_string = str(result.splitlines()[1], 'utf-8')
        nums = ast.literal_eval(nums_string)
        plt.plot(nums)
        plt.show()

#for INDEX_TO_MONITOR in INDEXES_TO_MONITOR:
    #monitor_index(INDEX_TO_MONITOR)

for INDEX_TO_MONITOR in [1280, 1289, 298]:
    monitor_index(INDEX_TO_MONITOR, write=True)
