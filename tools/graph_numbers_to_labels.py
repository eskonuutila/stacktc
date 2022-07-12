#!/usr/bin/env python
import sys
import argparse


def nums_to_labels(num_to_label, inp, outp):
    inp.readline()
    outp.write("from,to\n")
    lineno = 0
    for line in inp:
        parts = line.split(",")
        if len(parts) == 2:
            v_from = num_to_label[parts[0].strip()]
            v_to = num_to_label[parts[1].strip()]
            outp.write(f'{v_from},{v_to}\n')
        elif len(line) > 0 and line[0] == "#" or line.isspace():
            continue
        else:
            print(f'Illegal input at line {lineno}')
            sys.exit(1)


def main(input, output=None, map=None, **kwargs):
    num_to_label = dict()
    with open(map, "r") as inp:
        inp.readline()
        lineno = 0
        for line in inp:
            parts = line.split(",")
            if len(parts) == 2:
                num = parts[0].strip()
                label = parts[1].strip()
                num_to_label[num] = label
            elif len(line) > 0 and line[0] == "#" or line.isspace():
                continue
            else:
                print(f'Illegal input at line {lineno}')
                sys.exit(1)
    if input == "-":
        inp = sys.stdin
    else:
        inp = open(input, "r")
    if output is None or output == '-':
        outp = sys.stdout
    else:
        outp = open(output, "w")
    nums_to_labels(num_to_label, inp, outp)
    if inp != sys.stdin:
        inp.close()
    if outp != sys.stdout:
        outp.close()


def parse_args(arglist=None):
    parser = argparse.ArgumentParser(description="""Convert the vertice numbers of a graph into labels.""")
    parser.add_argument("-m", "--map", help="""A CSV file containing a header and lines FROM,TO""", required=True),
    parser.add_argument("input",
                        help="A CSV file containing the edges as pairs of labels FROM,TO. You can give '-' to read from standard input.")
    parser.add_argument("output",
                        help="An optional output CSV file. If missing or '-', the output is written to the standard output.",
                        nargs="?",
                        default=None)
    args = parser.parse_args() if arglist is None else parser.parse_args(arglist)
    kwargs = vars(args)
    return kwargs


if __name__ == "__main__":
    main(**parse_args())
