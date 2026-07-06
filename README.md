# io500

This is the C version of the IO500 benchmark.

## Install and Build Packages

Before compiling, you should review and edit the `prepare.sh` file to customize the build configuration for your system. 

* **Custom Build Flags**: You can edit the variables at the top of `prepare.sh` (or set them as environment variables) to pass custom flags:
  * `IOR_EXTRA_CONFIGURE_FLAGS`: Extra arguments to pass to IOR's `./configure` (e.g., `--with-aio`).
  * `IO500_EXTRA_CFLAGS` / `IO500_EXTRA_LDFLAGS`: Custom compiler/linker flags for `io500` (e.g., for custom storage libraries like DAOS).

Once you have configured the script, run it to download and compile all required packages (IOR, mdtest, pfind):

    $ ./prepare.sh

Note that `prepare.sh` installs these packages locally in the `io500` directory to ensure the correct versions are utilized for the benchmark, even if they are already installed globally on your system.

## Customize configuration
Be advised that any modifications to IO500 MUST be made according to the [IO500 rules](https://io500.org/rules/submission) if the results are intended for official submission.

Please remember that for a valid submission, the test must include all mandatory phases (`ior-easy`, `ior-hard`, `mdtest-easy`, `mdtest-hard`, `find`) with a minimum 300s test duration for *write* phases (stonewall timer). A run for an official submission can take up to 2 hours, and so if you are not trying to
generate results for an official submission, then it is ok for debugging purposes to
reduce the stonewall timer below 300s.

### Edit io500.sh script
Modify the existing `io500.sh` script to include information necessary for your MPI job scheduler. 

* The variables `io500_mpirun` and `io500_mpiargs` should be set according to your system and test case.
* The `setup()` section may optionally contain commands to create *only the top-level*
test directories under `$workdir` for the `ior-hard/ior-easy` and `mdtest-hard/mdtest-easy` test phases.

### Create an .ini file to specify phases and file system options
An .ini file containing all execution phase parameters is required.

Detailed help for the available options is provided when running:

```text
$ ./io500 -h
Synopsis:
  ./io500 <INI file> [options]                     Run the benchmark
  or
  ./io500 <INI file> --verify <output file>        Verify that the output matches the configuration
  or
  ./io500 --help | -h                              Show this help message
  or
  ./io500 --list | -l                              List all configuration options for INI file
  or
  ./io500 --list-mandatory | -lm                   List only mandatory configuration options for INI file

Options:
  --config-hash          Compute the configuration hash
  --cleanup              Run only the delete phases (useful to clean up failed runs)
  --dry-run              Show executed IO benchmark arguments but do not run them
  --mode=standard|extended
                         Define the mode to run the benchmark (default: standard)
  --timestamp=<string>   Use <string> for the output directory name

  -v                     Increase the verbosity level (can be repeated, e.g., -v -v).
  --verbose=<level>      Set the verbosity level (1-10).
```

For most users, it is best to generate an .ini file of the mandatory phases required
for an official submission.

    $ ./io500 --list-mandatory > config-mandatory.ini

Note that an existing template `config-minimal.ini` is also provided with the
minimum options for an official run. 

Further, a `config-debug-run.ini` has also been
provided to demonstrate how to configure a 1 second run for each phase that can be
used to initially verify proper operation of all test phases. Note that verifying a debug run will report `[OK] But this is an invalid run!` and return an exit code of `1` because the stonewall timer was below the mandatory 300 seconds.

### Edit the generated .ini file for your storage system and environment
Edit the config-mandatory.ini file. The file is organized with global variables
at the top and then variables for each section below in the order they are executed.

The following parameters are critical to set correctly:
* `[global] datadir` (directory where test files will be created)
* `[global] resultdir` (directory where result files will be written)
* `[global] api` (storage interface to use, if not POSIX)

The following variables can also be adjusted. This is sometimes required in order to
achieve the minimum 300 seconds execution time of each write phase:
* `[ior-easy] blocksize` (maximum data written per rank per I/O to a separate file)
* `[ior-hard] segmentCount` (maximum segments written per rank)
* `[mdtest-easy] n` (maximum number of files created per rank)
* `[mdtest-hard] n` (maximum number of files created per rank)

Note `[global] verbosity` can be set from 1 (least output) to 5 (highest output).


## Run benchmark and evaluate results
Execute io500 directly or via a batch job submission.
E.g.,
```text
    $ ./io500 config-mandatory.ini
```

Once a run has been completed, you can manually validate if it was successful by reviewing
the output in `$resultdir/result_summary.txt`:
* Ensure that none of the results are marked `[INVALID]` due to a write phase
execution time below 300s or some other error.
* Ensure there is a `SCORE` line at the end of the output that is not marked `[INVALID]`.

The run can be verified for correctness as well by running either:
```text
    $ ./io500 config-mandatory.ini  --verify ./results/result.txt
```

Or the lightweight verification tool which has fewer dependencies:
```text
    $ ./io500-verify config-mandatory.ini ./results/result.txt
```

## Submit your results
Submit your results using the instructions on the [IO500 website](https://io500.org/submission).

## Appendix A: Gather additional system information for submission

At any time, you may create a description for your hardware and software environment and save it using the [info-creator tool](https://www.google.com/search?q=https://www.vi4io.org/io500-info-creator/). You will be asked at submission time to provide the information and can make any final changes.

We are testing scripts to automatically capture information from your live system and integrate it into the system description. After you run `./prepare.sh` you will find in the directory `schema-tools` all the available scripts. These can be run on your JSON file, for example:
```bash
./schema-tools/cdcl_add_osinfo.py system-information.json
```
This will add the information about your operating system to the `system-information.json` that you may want to submit. You can upload or download the JSON to or from the info creator at any time, verifying the changes you make. A script may support configuring a specific file system or node, which is particularly useful if you have multiple different node configurations.

## Appendix B: Example output on the command line

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
