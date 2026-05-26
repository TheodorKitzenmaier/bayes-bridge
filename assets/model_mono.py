import argparse
import struct
import math

### DEFINE MODEL HERE

def model(
        params: list[float],
        abscissa: list[float]):
    # Populate these lists with output data.
    signal: list[float] = []
    derived: list[float] = []

    decay_rate = params[0]

    for _ in range(3):
        derived.append(decay_rate)
    for _ in range(3):
        derived.append(0.0 if decay_rate == 0.0 else 1.0/decay_rate)

    error = True

    for value in abscissa:
        result = math.exp(-decay_rate * value)
        signal.append(result)
        if result != 0.0:
            error = False

    if error:
        signal[0] = 1.0

    # Return as tuple.
    return signal, derived

### END MODEL DEFINITION

def get_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--input") # input
    parser.add_argument("-p", "--priors") # priors
    parser.add_argument("-o", "--output") # output
    parser.add_argument("-d", "--derived") # derived

    return parser.parse_args()

def read_file(filename: str) -> list[float]:
    file = open(filename, "rb")
    values = []
    while raw := file.read(8):
        values.append(struct.unpack("=d", raw)[0])
    file.close()
    return values

def write_file(filename: str, values: list[float]) -> None:
    file = open(filename, "wb")
    for value in values:
        file.write(struct.pack("=d", value))
    file.close()

def run():
    args = get_args()
    input = read_file(args.input)
    priors = read_file(args.priors)
    signal, derived = model(priors, input)
    write_file(args.derived, derived)
    write_file(args.output, signal)

run()
