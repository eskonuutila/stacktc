#!/usr/bin/env python
import sys
import argparse
from pathlib import Path


def assign_label_num(label, label_num_map, num_label_map):
    num = label_num_map.get(label, None)
    if num is None:
        num = len(label_num_map)
        label_num_map[label] = num
        num_label_map[num] = label
    return num


def labels_to_nums(inp, outp):
    lineno = 0
    label_num_map = dict()
    num_label_map = dict()
    inp.readline()
    for line in inp:
        parts = line.split(",")
        if len(parts) == 2:
            assign_label_num(parts[0].strip(), label_num_map, num_label_map)
            lineno += 1
        elif len(line) > 0 and line[0] == "#" or line.isspace():
            continue
        else:
            print(f'Illegal input at line {lineno}')
            sys.exit(1)
    inp.seek(0)
    inp.readline()
    outp.write("from,to\n")
    for line in inp:
        parts = line.split(",")
        if len(parts) == 2:
            v_from = label_num_map[parts[0].strip()]
            v_to = assign_label_num(parts[1].strip(), label_num_map, num_label_map)
            outp.write(f'{v_from},{v_to}\n')
        elif len(line) > 0 and line[0] == "#" or line.isspace():
            continue

    return label_num_map, num_label_map


def main(input, output=None, output_mappings=False, **kwargs):
    if input == "-":
        inp = sys.stdin
    else:
        inp = open(input, "r")
    if output is None or output == '-':
        outp = sys.stdout
    else:
        outp = open(output, "w")
    l2n, n2l = labels_to_nums(inp, outp)
    if inp != sys.stdin:
        inp.close()
    if outp != sys.stdout:
        outp.close()
    if output_mappings:

        def write_dict_sorted(d, from_name, to_name, output_file):
            with open(output_file, "w") as outp:
                outp.write(f'{from_name},{to_name}\n')
                for f, t in sorted(list(d.items())):
                    outp.write(f'{f},{t}\n')

        write_dict_sorted(l2n, "label", "number", "l2n.csv" if outp == sys.stdout else Path(output).with_suffix('.l2n.csv'))
        write_dict_sorted(n2l, "number", "label", "n2l.csv" if outp == sys.stdout else Path(output).with_suffix('.n2l.csv'))


def parse_args(arglist=None):
    parser = argparse.ArgumentParser(description="""Convert the textual vertice names of a graph into numeric ones.""")
    parser.add_argument("-m",
                        "--output_mappings",
                        action="store_true",
                        help="""Output the mappings label to number and number to label to files. The file names are the <OUTPUT>l2n.csv
                        and >OUTPUT>n2l.csv, where OUTPUT is then name of the output file without its suffix or an emptry string
                        if output file name is omitted."""),
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
