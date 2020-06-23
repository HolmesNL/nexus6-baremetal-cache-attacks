import os
import read_serial

num_samples = 100
num_profiles = 1000

output_dir = '../../../thesis/plots/input'
#filename = 'openssl_flush_reload_spread_{}_{}.csv'.format(num_samples, num_profiles)
#filename = 'openssl_flush_reload_{}_{}.csv'.format(num_samples, num_profiles)
#filename = 'openssl_flush_reload_spread_{}.csv'.format(num_profiles)
#filename = 'openssl_execution_time_second_cpu.csv'
#filename = 'openssl_prime_probe_{}_{}.csv'.format(num_samples, num_profiles)
#filename = 'openssl_cache_heatmap_10.txt'
#filename = 'rsa_execution_time.csv'
#filename = 'rsa_execution_time_second_cpu.csv'
#filename = 'openssl_rsa_BN_mod_exp_mont.csv'
#filename = 'openssl_rsa_BN_mod_exp_mont_loop.csv'
#filename = 'histogram_flush_reload_instruction_with_flush_instruction.csv'
#filename = 'histogram_prime_probe_instruction.csv'
#filename = 'histogram_flush_reload.csv'
#filename = 'histogram_flush_reload_instruction_1000kb_nop.csv'
filename = 'histogram_prime_probe_instruction_1000kb_nop.csv'
output_file = os.path.join(output_dir, filename)
os.makedirs(output_dir, exist_ok=True)

read_serial.build_and_run()

with open(output_file, 'wb') as f:
    f.write(read_serial.get_result(b't'))
