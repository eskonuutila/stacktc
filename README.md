# stacktc -- Efficient Transitive Closure Computation

The `stacktc` transitive closure algorithm and the interval-based successor set representation presented in [Nuutila, E., Efficient Transitive Closure Computation in Large Digraphs. Acta Polytechnica Scandinavica, Mathematics and Computing in Engineering Series No. 74, Helsinki 1995, 124 pages. Published by the Finnish Academy of Technology. ISBN 951-666-451-2, ISSN 1237-2404, UDC 681.3.](docs/thesis.pdf).

**NOTE!** This version has not been extensively tested. Please contact the author, if you find bugs.

## Building

To build the binary:

```
git clone git@github.com:eskonuutila/stacktc.git
cd stacktc/src/c
./build.sh
```

Optionally you can install the program into `/usr/local/bin` by

```
make install
```

## Running

You run the program by

```
./stacktc OPTIONS [INPUT [OUTPUT]]
```

To see all parameters and a help text, run:

```
./stacktc -h
```

The input file should be a CSV file with a header line, and the rest of the lines should be edges in format FROM,TO. FROM and TO should be numeric vertex identifiers.

There are example files in subdirectory `examples`. The files `*-nums.csv` are in correct numeric format; the other files contain graphs
with non-numeric vertice names. To convert graphs with non-numeric vertex names into numeric ones use the script python3 script
`tools/graph_labels_to_numbers.py`.

An example run:

```
./stacktc -w -i ../../examples/thesis-fig-3.2-nums.csv
```
