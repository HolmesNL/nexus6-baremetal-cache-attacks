import os
import read_serial

output_dir = '../../../thesis/plots/input'
filename = 'randomness.csv'
output_file = os.path.join(output_dir, filename)
os.makedirs(output_dir, exist_ok=True)

read_serial.build_and_run()

with open(output_file, 'wb') as f:
    f.write(read_serial.get_result(b'a'))
