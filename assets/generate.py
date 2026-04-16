import math
import argparse

from typing import Optional

def parse_args(args: Optional[str] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--config", help="Config file to load.")
    parser.add_argument("-o", "--output", help="File to output values to.")
    parser.add_argument("-r", "--range", nargs=2, type=float, help="Min and max values to evaluate function over.")
    parser.add_argument("-s", "--steps", type=int, default=15, help="Number of points to evaluate the function at.")
    parser.add_argument("-a", "--coefficients", nargs="+", type=float, help="Coefficients of terms.")
    parser.add_argument("-b", "--exponents", nargs="+", type=float, help="Exponent coefficient of terms.")
    result = parser.parse_args(args)
    if(result.config):
        with open(result.config, "r") as file:
            result = parse_args(file.read())
    return result

def calculate_values(
        min: float,
        max: float,
        steps: int,
        coefficients: list[float],
        exponents: list[float]) -> tuple[list[float], list[float]]:
    inputs = []
    outputs = []
    for value in [min + (max - min) / (steps - 1) * i for i in range(steps)]:
        inputs.append(value)
        result = 0
        for coefficient, exponent in zip(coefficients, exponents):
            result += coefficient * math.exp(-exponent * value)
        outputs.append(result)
    return inputs, outputs

def output_values(
        filename: str,
        inputs: list[float],
        outputs: list[float]):
    with open(filename, "w") as outfile:
        for input, output in zip(inputs, outputs):
            outfile.write(f"{input} {output}\n")

args = parse_args()
print(args)

inputs, outputs = calculate_values(
    min(args.range),
    max(args.range),
    args.steps,
    args.coefficients,
    args.exponents)

output_values(args.output, inputs, outputs)
