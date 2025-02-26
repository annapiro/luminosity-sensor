"""
Convert serial monitor data from TSL2591:
10:31:58.809 -> 1049268,19373,10005,18482.34,935.77,48.63,27.56

To csv:
10:31:58.809,1049268,120,19373,10005,18482.34,935.77,48.63,27.56

Assume the measurements are made under a solar simulator
and the intensity of the spectrum is stated on a separate line.
"""

import argparse
import csv
import os

HEADER = 'Time,ArduinoTime,Intensity,CH0,CH1,Lux,Irradiance,Error,Temperature'


def process_input(filename: str) -> list[list[str]]:
    current_intensity = -1
    output_lines = list()
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.isdigit():
                current_intensity = int(line)
            else:
                line = line.replace(' -> ', ',')
                line_split = line.split(',')
                line_split.insert(2, str(current_intensity))
                output_lines.append(line_split)
    return output_lines


def save_output(lines: list[list[str]], filename: str):
    filename = os.path.splitext(filename)[0] + '.csv'
    header = HEADER.split(',')
    with open(filename, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile, delimiter=',')
        writer.writerow(header)
        writer.writerows(lines)


def main(filename: str):
    output_lines = process_input(filename)
    save_output(output_lines, filename)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Convert serial monitor output to csv.')
    parser.add_argument('filename', help='text file with the raw data')
    args = parser.parse_args()
    main(args.filename)
