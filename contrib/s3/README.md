# io500-s3

The following explains how to use the IO500 to benchmark S3 compatible storage.
S3 runs are not fully compliant at the time of writing because find is not yet supported, but this might be useful for testing and comparing different S3 implementations. Some people might find it helpful since it is relatively complicated to set up. (Hopefully, in the long run, IO500 compliant runs would be possible with S3)

## Preparation

To retrieve the required packages and compile all needed software, including libS3, the library used to communicate with the S3 interface, please run:

```console
./contrib/s3/prepare-s3.sh
```

## Usage

The benchmark requires a .ini file containing the options.
The libS3 library should be dynamically linked before launching the benchmark.
A sample config-s3.ini is provided, please replace the variables with the corresponding S3 storage access info.

```console
$ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD/build/libs3/build/lib
./io500 ./contrib/s3/config-s3.ini
```

Detailed help for the available options for the S3 IOR/MDtest benchmark is provided when running:

```console
./bin/ior -a=S3-libs3 -h
```

### Example output on the command line

```console
 mpiexec -np 2 ./io500 config-s3.ini
IO500 version io500-isc22_v1 (standard)
[RESULT]       ior-easy-write        0.002189 GiB/s : time 303.523 seconds
[RESULT]    mdtest-easy-write        0.005847 kIOPS : time 302.666 seconds
[      ]            timestamp        0.000000 kIOPS : time 0.001 seconds
[RESULT]       ior-hard-write        0.000130 GiB/s : time 301.501 seconds
[RESULT]    mdtest-hard-write        0.001975 kIOPS : time 302.595 seconds
[RESULT]                 find        0.000000 kIOPS : time 0.000 seconds [INVALID]
[RESULT]        ior-easy-read        0.010056 GiB/s : time 66.307 seconds
[RESULT]     mdtest-easy-stat        0.006040 kIOPS : time 292.609 seconds
[RESULT]        ior-hard-read        0.000262 GiB/s : time 150.044 seconds
[RESULT]     mdtest-hard-stat        0.006040 kIOPS : time 100.022 seconds
[RESULT]   mdtest-easy-delete        0.002928 kIOPS : time 601.830 seconds
[RESULT]     mdtest-hard-read        0.003012 kIOPS : time 198.905 seconds
[RESULT]   mdtest-hard-delete        0.002956 kIOPS : time 203.265 seconds
[SCORE ] Bandwidth 0.000931 GiB/s : IOPS 0.003778 kiops : TOTAL 0.001875 [INVALID]
```
