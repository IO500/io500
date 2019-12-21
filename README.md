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

### Example output on the command line

The following is the minimal output when setting verbosity to 0

$ mpiexec -np 2   ./io500 config-minimal.ini
result-dir      = ./results/2019.12.21-20.14.57
config-hash     = F9184F82
; WARNING stonewall-time != 300, this is an invalid run

[ior-easy-write]
exe             = ./ior -C -Q 1 -g -G 271 -k -e -o ./out/2019.12.21-20.14.57/ior_easy/ior_file_easy -O stoneWallingStatusFile=./out/2019.12.21-20.14.57/ior_easy/stonewall -O stoneWallingWearOut=1 -t 2m -b 9920000m -w -D 1 -a POSIX
score           = 0.183619

[mdtest-easy-write]
exe             = ./mdtest -n 1000000 -u -L -F -d ./out/2019.12.21-20.14.57/mdt_easy -x ./out/2019.12.21-20.14.57/mdt_easy-stonewall -C -W 1 -a POSIX
rate-stonewall  = 109.810713
score           = 102.680206

[timestamp]

[ior-hard-write]
exe             = ./ior -C -Q 1 -g -G 27 -k -e -o ./out/2019.12.21-20.14.57/ior_hard/file -O stoneWallingStatusFile=./out/2019.12.21-20.14.57/ior_hard/stonewall -O stoneWallingWearOut=1 -t 47008 -b 47008 -s 100000 -w -D 1 -a POSIX
score           = 0.406999

[mdtest-hard-write]
exe             = ./mdtest -n 1000000 -t -w 3901 -e 3901 -F -d ./out/2019.12.21-20.14.57/mdt_hard -x ./out/2019.12.21-20.14.57/mdt_hard-stonewall -C -W 1 -a POSIX
rate-stonewall  = 62.050735
score           = 61.706827

[find]
exe             = ./pfind ./out/2019.12.21-20.14.57 -newer ./out/2019.12.21-20.14.57/timestampfile -size 3901c -name *01* -C
found           = 1839
total-files     = 179359
score           = 15.989096

[ior-easy-read]
exe             = ./ior -C -Q 1 -g -G 271 -k -e -o ./out/2019.12.21-20.14.57/ior_easy/ior_file_easy -O stoneWallingStatusFile=./out/2019.12.21-20.14.57/ior_easy/stonewall -O stoneWallingWearOut=1 -t 2m -b 9920000m -r -R -a POSIX
score           = 4.292860

[mdtest-easy-stat]
exe             = ./mdtest -n 1000000 -u -L -F -d ./out/2019.12.21-20.14.57/mdt_easy -x ./out/2019.12.21-20.14.57/mdt_easy-stonewall -T -a POSIX
score           = 1022.168362

[ior-hard-read]
exe             = ./ior -C -Q 1 -g -G 27 -k -e -o ./out/2019.12.21-20.14.57/ior_hard/file -O stoneWallingStatusFile=./out/2019.12.21-20.14.57/ior_hard/stonewall -O stoneWallingWearOut=1 -t 47008 -b 47008 -s 100000 -r -R -a POSIX
score           = 7.497023

[mdtest-hard-stat]
exe             = ./mdtest -n 1000000 -t -w 3901 -e 3901 -F -d ./out/2019.12.21-20.14.57/mdt_hard -x ./out/2019.12.21-20.14.57/mdt_hard-stonewall -T -a POSIX
score           = 1284.803695

[mdtest-easy-delete]
exe             = ./mdtest -n 1000000 -u -L -F -d ./out/2019.12.21-20.14.57/mdt_easy -x ./out/2019.12.21-20.14.57/mdt_easy-stonewall -r -a POSIX
score           = 178.729250

[mdtest-hard-read]
exe             = ./mdtest -n 1000000 -t -w 3901 -e 3901 -F -d ./out/2019.12.21-20.14.57/mdt_hard -x ./out/2019.12.21-20.14.57/mdt_hard-stonewall -E -a POSIX
score           = 325.281411

[mdtest-hard-delete]
exe             = ./mdtest -n 1000000 -t -w 3901 -e 3901 -F -d ./out/2019.12.21-20.14.57/mdt_hard -x ./out/2019.12.21-20.14.57/mdt_hard-stonewall -r -a POSIX
score           = 148.773023

[SCORE]
MD              = 6.416
BW              = 2.941
SCORE           = 7.058 [INVALID]
