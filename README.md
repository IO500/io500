# io500

This is the C version of the IO500 benchmark.

## Preparation

The program interfaces directly with IOR/MDtest and pfind.
To retrieve the required packages and compile the library version, run
$ ./prepare.sh

Then you can compile the io500 application running make.

## Usage

The benchmark requires a INI file containing the options.
The INI file is structured in sections depending on the phase.

Detailed help for the available options is provided when running:
$ ./io500 -h
Synopsis: ./io500 <INI file> [-v=<verbosity level>] [--dry-run]

The benchmark output the commands it would run (equivalence of command line invocations of ior/mdtest). Use --dry-run to not invoke any command.

Two INI files are provided, the config-minimal.ini and the config-some.ini.
  - The minimal example contains the only mandatory parameter, the data directory. It also sets the stonewalling to 1 for testing.
  - The config-some illustrates the setting of various options. For more details, run ./io500 -h.

To see the currently active options, run:
$ ./io500 <INI file> -h

## Output

  - The benchmark will output a default set of information in the INI format to simplify parsing. When setting verbosity to 5, you will receive more information.
  - It also stores the output files from IOR and MDTest in the results/ subdirectory with the timestamp of the run.
