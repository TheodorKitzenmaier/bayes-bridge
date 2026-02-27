import argparse
import struct
import math

parser = argparse.ArgumentParser()
parser.add_argument("-i", "--input") # input
parser.add_argument("-p", "--priors") # priors
parser.add_argument("-o", "--output") # output
parser.add_argument("-d", "--derived") # derived

args = parser.parse_args()

input_file = open(args.input, "rb")
prior_file = open(args.priors, "rb")
output_file = open(args.output, "wb")
derived_file = open(args.derived, "wb")

# Read data.
input_values = []
while raw := input_file.read(8):
	input_values.append(struct.unpack("=d", raw)[0])
prior_values = []
while raw := prior_file.read(8):
	prior_values.append(struct.unpack("=d", raw)[0])

input_file.close()
prior_file.close()

# Derived values.
derived_values = []
decay_rate = prior_values[0]
if decay_rate == 0:
	for _ in range(6): derived_values.append(0.0)
else:
	for _ in range(3): derived_values.append(decay_rate)
	for _ in range(3): derived_values.append(1.0 / decay_rate)

for value in derived_values:
	derived_file.write(struct.pack("@d", value))
derived_file.close()

# Output values.
output_values = []
for value in input_values:
	output_values.append(math.exp(-decay_rate * value))
for _ in input_values:
	output_values.append(1.0)

for value in output_values:
	output_file.write(struct.pack("@d", value))
output_file.close()
