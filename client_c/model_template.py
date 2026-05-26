import argparse
import struct

### DEFINE MODEL HERE

def model(
        params: list[float],
        abscissa: list[float]):
    # Populate these lists with output data.
    signal: list[float] = []
    derived: list[float] = []
    
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
