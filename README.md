# io500

This is the C version of the IO500 benchmark.

## Preparation

The program interfaces directly with IOR/MDtest and pfind.
To retrieve the required packages and compile the library version, run

    $ ./prepare.sh

Then you can compile the io500 application running make.

## Usage

The benchmark requires a .ini file containing the options.
The .ini file is structured in sections depending on the phase.

Detailed help for the available options is provided when running:

    $ ./io500 -h
    Synopsis: ./io500 <INI file> [-v=<verbosity level>] [--dry-run]

The benchmark output the commands it would run (equivalence of command line invocations of ior/mdtest). Use --dry-run to not invoke any command.

In order to create a new INI file with all the options, you can execute:

    $ ./io500 --list > config-all.ini

The config-some illustrates the setting of various options. For more details, run ./io500 -h.

To see the currently active options, run:

    $ ./io500 <file.ini> -h


### Integrity check

After a run is completed, the score obtained and the configuration file can be verified to ensure that it wasn't accidentially modified.

You can either use the full-featured io500 application:

    $ ./io500 config-test-run.ini  --verify result.txt
    config-hash = 1065C0D
    score-hash  = C97CC873
    [OK] But this is an invalid run!

Or the lightweight verification tool which has fewer dependencies:

    $ ./io500-verify config-test-run.ini result.txt

## Output

  - The benchmark will output a default set of information in the INI format to simplify parsing. When setting verbosity to 5, you will receive more information.
  - It also stores the output files from IOR and MDTest in the results/ subdirectory with the timestamp of the run.

### Example output on the command line

The following is the minimal output when setting verbosity to 0

    $ mpiexec -np 2   ./io500 config-minimal.ini
    [RESULT]       ior-easy-write        0.186620 GiB/s  : time 0.027 seconds
    [RESULT]    mdtest-easy-write      103.300821 kIOPS : time 1.121 seconds
    [RESULT]       ior-hard-write        0.001313 GiB/s  : time 0.067 seconds
    [RESULT]    mdtest-hard-write       58.939081 kIOPS : time 1.021 seconds
    [RESULT]                 find     1486.435084 kIOPS : time 0.118 seconds
    [RESULT]        ior-easy-read        1.575557 GiB/s  : time 0.005 seconds
    [RESULT]     mdtest-easy-stat      839.392805 kIOPS : time 0.138 seconds
    [RESULT]        ior-hard-read        2.272671 GiB/s  : time 0.000 seconds
    [RESULT]     mdtest-hard-stat     1212.558124 kIOPS : time 0.050 seconds
    [RESULT]   mdtest-easy-delete      160.765642 kIOPS : time 0.753 seconds
    [RESULT]     mdtest-hard-read      275.011939 kIOPS : time 0.219 seconds
    [RESULT]   mdtest-hard-delete      132.015851 kIOPS : time 0.474 seconds
    [SCORE INVALID] Bandwidth 0.172092 GB/s : IOPS 292.625029 kiops : TOTAL 7.096374

This information is also saved in the file result_summary.txt in the respective results directory.

In the same directory, you will also find the result.txt file that contains more information and is stored using the INI file format.

    version         = SC20-testing
    config-hash     = 25C33C96
    result-dir      = ./results/
    ; START 2020-01-06 10:23:49
    ; ERROR INVALID stonewall-time != 300


    [ior-easy-write]
    t_start         = 2020-01-06 10:23:49
    exe             = ./ior -C -Q 1 -g -G 271 -k -e -o ./out//ior-easy/ior_file_easy -O stoneWallingStatusFile=./out//ior-easy/stonewall -O stoneWallingWearOut=1 -t 2m -b 2m -F -w -D 1 -a POSIX
    ; ERROR INVALID Write phase needed 0.020932s instead of stonewall 1s. Stonewall was hit at 0.0s
    throughput-stonewall = 0.37
    score           = 0.186620
    ; ERROR INVALID Runtime of phase (0.027088) is below stonewall time. This shouldn't happen!
    t_delta         = 0.0271
    t_end           = 2020-01-06 10:23:49

    [mdtest-easy-write]
    t_start         = 2020-01-06 10:23:49
    exe             = ./mdtest -n 1000000 -u -L -F -N 1 -d ./out//mdtest-easy -x ./out//mdtest-easy-stonewall -C -W 1 -a POSIX
    rate-stonewall  = 109.492799
    score           = 103.300821
    t_delta         = 1.1207
    t_end           = 2020-01-06 10:23:50

    [timestamp]
    t_start         = 2020-01-06 10:23:50
    t_delta         = 0.0000
    t_end           = 2020-01-06 10:23:50

    [ior-hard-write]
    t_start         = 2020-01-06 10:23:50
    exe             = ./ior -C -Q 1 -g -G 27 -k -e -o ./out//ior-hard/file -O stoneWallingStatusFile=./out//ior-hard/stonewall -O stoneWallingWearOut=1 -t 47008 -b 47008 -s 1 -w -D 1 -a POSIX
    ; ERROR INVALID Write phase needed 0.066709s instead of stonewall 1s. Stonewall was hit at 0.0s
    throughput-stonewall = 0.00
    score           = 0.001313
    ; ERROR INVALID Runtime of phase (0.067146) is below stonewall time. This shouldn't happen!
    t_delta         = 0.0671
    t_end           = 2020-01-06 10:23:50

    [mdtest-hard-write]
    t_start         = 2020-01-06 10:23:50
    exe             = ./mdtest -n 1000000 -t -w 3901 -e 3901 -N 1 -F -d ./out//mdtest-hard -x ./out//mdtest-hard-stonewall -C -W 1 -a POSIX
    rate-stonewall  = 59.263301
    score           = 58.939081
    t_delta         = 1.0207
    t_end           = 2020-01-06 10:23:51

    [find]
    t_start         = 2020-01-06 10:23:51
    exe             = ./pfind ./out/ -newer ./results//timestampfile -size 3901c -name *01* -C -H 1 -q 10000
    found           = 1596
    total-files     = 175761
    score           = 1486.435084
    t_delta         = 0.1184
    t_end           = 2020-01-06 10:23:51

    [ior-easy-read]
    t_start         = 2020-01-06 10:23:51
    exe             = ./ior -C -Q 1 -g -G 271 -k -e -o ./out//ior-easy/ior_file_easy -O stoneWallingStatusFile=./out//ior-easy/stonewall -O stoneWallingWearOut=1 -t 2m -b 2m -F -r -R -a POSIX
    score           = 1.575557
    t_delta         = 0.0054
    t_end           = 2020-01-06 10:23:51

    [mdtest-easy-stat]
    t_start         = 2020-01-06 10:23:51
    exe             = ./mdtest -n 1000000 -u -L -F -N 1 -d ./out//mdtest-easy -x ./out//mdtest-easy-stonewall -T -a POSIX
    score           = 839.392805
    t_delta         = 0.1381
    t_end           = 2020-01-06 10:23:51

    [ior-hard-read]
    t_start         = 2020-01-06 10:23:51
    exe             = ./ior -C -Q 1 -g -G 27 -k -e -o ./out//ior-hard/file -O stoneWallingStatusFile=./out//ior-hard/stonewall -O stoneWallingWearOut=1 -t 47008 -b 47008 -s 1 -r -R -a POSIX
    score           = 2.272671
    t_delta         = 0.0004
    t_end           = 2020-01-06 10:23:51

    [mdtest-hard-stat]
    t_start         = 2020-01-06 10:23:51
    exe             = ./mdtest -n 1000000 -t -w 3901 -e 3901 -N 1 -F -d ./out//mdtest-hard -x ./out//mdtest-hard-stonewall -T -a POSIX
    score           = 1212.558124
    t_delta         = 0.0499
    t_end           = 2020-01-06 10:23:51

    [mdtest-easy-delete]
    t_start         = 2020-01-06 10:23:51
    exe             = ./mdtest -n 1000000 -u -L -F -N 1 -d ./out//mdtest-easy -x ./out//mdtest-easy-stonewall -r -a POSIX
    score           = 160.765642
    t_delta         = 0.7534
    t_end           = 2020-01-06 10:23:52

    [mdtest-hard-read]
    t_start         = 2020-01-06 10:23:52
    exe             = ./mdtest -n 1000000 -t -w 3901 -e 3901 -N 1 -F -d ./out//mdtest-hard -x ./out//mdtest-hard-stonewall -E -X -a POSIX
    score           = 275.011939
    t_delta         = 0.2191
    t_end           = 2020-01-06 10:23:52

    [mdtest-hard-delete]
    t_start         = 2020-01-06 10:23:52
    exe             = ./mdtest -n 1000000 -t -w 3901 -e 3901 -N 1 -F -d ./out//mdtest-hard -x ./out//mdtest-hard-stonewall -r -a POSIX
    score           = 132.015851
    t_delta         = 0.4742
    t_end           = 2020-01-06 10:23:53

    [SCORE]
    MD              = 292.625029
    BW              = 0.172092
    SCORE           = 7.096374  [INVALID]
    hash            = 884F29B
    ; END 2020-01-06 10:23:53
