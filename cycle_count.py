import os
import read_serial

start = 0
end = 5000
output_dir = '../../../thesis/plots/input'
filename = "cycle_count_{}_{}.csv".format(start, end)
output_file = os.path.join(output_dir, filename)
os.makedirs(output_dir, exist_ok=True)

read_serial.build_and_run()

with open(output_file, 'wb') as f:
    f.write(read_serial.get_result(b'u'))
