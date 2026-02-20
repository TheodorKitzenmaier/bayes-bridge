import argparse
import math

parser = argparse.ArgumentParser()
parser.add_argument("-i", "--input") # input
parser.add_argument("-p", "--priors") # priors
parser.add_argument("-o", "--output") # output
parser.add_argument("-d", "--derived") # derived

args = parser.parse_args()

input_file = open(args.input, "r")
prior_file = open(args.priors, "r")
output_file = open(args.output, "w")
derived_file = open(args.derived, "w")

# Read data.
input_values = []
for value in input_file.read().split():
	input_values.append(float(value))
prior_values = []
for value in prior_file.read().split():
	prior_values.append(float(value))

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

derived_file.write(" ".join(str(derived_values)))
derived_file.close()

# Output values.
output_values = []
for value in input_values:
	output_values.append(math.exp(-decay_rate * value))
	output_values.append(1.0)

output_file.write(" ".join(str(output_values)))
output_file.close()
