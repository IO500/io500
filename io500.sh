#!/bin/bash
#
# INSTRUCTIONS:
# This script takes its parameters from the same .ini file as io500 binary.

function setup_paths {
  # Set the paths to the binaries and how to launch MPI jobs.
  # If you ran ./utilities/prepare.sh successfully, then binaries are in ./bin/
  io500_ior_cmd=$PWD/bin/ior
  io500_mdtest_cmd=$PWD/bin/mdtest
  io500_mdreal_cmd=$PWD/bin/md-real-io
  io500_mpirun="mpiexec"
  io500_mpiargs="-np 2"
}

function setup_directories {
  local workdir
  local resultdir
  local ts

  # set directories where benchmark files are created and where the results go
  # If you want to set up stripe tuning on your output directories or anything
  # similar, then this is the right place to do it.  This creates the output
  # directories for both the app run and the script run.

  timestamp=$(date +%Y.%m.%d-%H.%M.%S)           # create a uniquifier
  [ $(get_ini_global_param timestamp-datadir True) != "False" ] &&
	ts="$timestamp" || ts="io500"
  # directory where the data will be stored
  workdir=$(get_ini_global_param datadir $PWD/datafiles)/$ts
  io500_workdir=$workdir-scr
  [ $(get_ini_global_param timestamp-resultdir True) != "False" ] &&
	ts="$timestamp" || ts="io500"
  # the directory where the output results will be kept
  resultdir=$(get_ini_global_param resultdir $PWD/results)/$ts
  io500_result_dir=$resultdir-scr

  mkdir -p $workdir-{scr,app} $resultdir-{scr,app}
}

# you should not edit anything below this line
set -eo pipefail  # better error handling

io500_ini="${1:-""}"
if [[ -z "$io500_ini" ]]; then
  echo "error: ini file must be specified.  usage: $0 <config.ini>"
  exit 1
fi
if [[ ! -s "$io500_ini" ]]; then
  echo "error: ini file '$io500_ini' not found or empty"
  exit 2
fi

function get_ini_section_param() {
  local section="$1"
  local param="$2"
  local inside=false

  while read LINE; do
    LINE=$(sed -e 's/ *#.*//' -e 's/ *= */=/' <<<$LINE)
    $inside && [[ "$LINE" =~ "[.*]" ]] && inside=false && break
    [[ -n "$section" && "$LINE" =~ "[$section]" ]] && inside=true && continue
    ! $inside && continue
    echo $LINE | awk -F = "/^$param/ { print \$2 }"
    [[ "$LINE" =~ "$param" ]] && break
  done < $io500_ini
}

function get_ini_param() {
  local section="$1"
  local param="$2"
  local default="$3"

  # try and get the most-specific param first, then more generic params
  val=$(get_ini_section_param $section $param)
  [ -n "$val" ] || val="$(get_ini_section_param ${section%-*} $param)"
  [ -n "$val" ] || val="$(get_ini_section_param global $param)"

  echo "${val:-$default}" | sed -e 's/FALSE/False/' -e 's/TRUE/True/'
}

function get_ini_run_param() {
  local section="$1"
  local default="$2"
  local val

  val=$(get_ini_section_param $section noRun)

  # logic is reversed from "noRun=TRUE" to "run=False"
  [[ $val = TRUE ]] && echo "False" || echo "$default"
}

function get_ini_global_param() {
  local param="$1"
  local default="$2"
  local val

  val=$(get_ini_section_param global $param |
	sed -e 's/FALSE/False/' -e 's/TRUE/True/')

  echo "${val:-$default}"
}

# does the write phase and enables the subsequent read
io500_run_ior_easy="$(get_ini_run_param ior-easy True)"
# does the creat phase and enables the subsequent stat
io500_run_md_easy="$(get_ini_run_param mdtest-easy True)"
# does the write phase and enables the subsequent read
io500_run_ior_hard="$(get_ini_run_param ior-hard True)"
# does the creat phase and enables the subsequent read
io500_run_md_hard="$(get_ini_run_param mdtest-hard True)"
io500_run_find="$(get_ini_run_param find True)"
io500_run_ior_easy_read="$(get_ini_run_param ior-easy-read True)"
io500_run_md_easy_stat="$(get_ini_run_param mdtest-easy-stat True)"
io500_run_ior_hard_read="$(get_ini_run_param ior-hard-read True)"
io500_run_md_hard_stat="$(get_ini_run_param mdtest-easy-stat True)"
io500_run_md_hard_read="$(get_ini_run_param mdtest-easy-stat True)"
# turn this off if you want to just run find by itself
io500_run_md_easy_delete="$(get_ini_run_param mdtest-easy-delete True)"
# turn this off if you want to just run find by itself
io500_run_md_hard_delete="$(get_ini_run_param mdtest-hard-delete True)"
io500_run_md_hard_delete="$(get_ini_run_param mdtest-hard-delete True)"
io500_run_mdreal="$(get_ini_run_param mdreal False)"
# attempt to clean the cache after every benchmark, useful for validating the performance results and for testing with a local node; it uses the io500_clean_cache_cmd (can be overwritten); make sure the user can write to /proc/sys/vm/drop_caches
io500_clean_cache="$(get_ini_global_param drop-caches False)"
io500_clean_cache_cmd="$(get_ini_global_param drop-caches-cmd)"
io500_cleanup_workdir="$(get_ini_run_param cleanup)"
# Stonewalling timer, set to 300 to be an official run; set to 0, if you never want to abort...
io500_stonewall_timer=$(get_ini_param debug stonewall-time 300)
# Choose regular for an official regular submission or scc for a Student Cluster Competition submission to execute the test cases for 30 seconds instead of 300 seconds
io500_rules="regular"

# to run this benchmark, find and edit each of these functions.  Please also
# also edit 'extra_description' function to help us collect the required data.
function main {
  setup_directories
  setup_paths
  setup_ior_easy # required if you want a complete score
  setup_ior_hard # required if you want a complete score
  setup_mdt_easy # required if you want a complete score
  setup_mdt_hard # required if you want a complete score
  setup_find     # required if you want a complete score
  setup_mdreal   # optional
  run_benchmarks

  if [[ -s "system-information.txt" ]]; then
    echo "Warning: please create a system-information.txt description by"
    echo "copying the information from https://vi4io.org/io500-info-creator/"
  else
    cp "system-information.txt" $io500_result_dir
  fi

  create_tarball
}

function setup_ior_easy {
  local params

  io500_ior_easy_size=$(get_ini_param ior-easy blockSize 9920000m | tr -d m)
  val=$(get_ini_param ior-easy API POSIX)
  [ -n "$val" ] && params+=" -a $val"
  val="$(get_ini_param ior-easy transferSize)"
  [ -n "$val" ] && params+=" -t $val"
  val="$(get_ini_param ior-easy hintsFileName)"
  [ -n "$val" ] && params+=" -U $val"
  val="$(get_ini_param ior-easy posix.odirect)"
  [ "$val" = "True" ] && params+=" --posix.odirect"
  val="$(get_ini_param ior-easy verbosity)"
  [ -n "$val" ] && params+=" $(yes ' -v' | head -$val)"
  io500_ior_easy_params="$params"
  echo -n ""
}

function setup_mdt_easy {
  io500_mdtest_easy_params="-u -L" # unique dir per thread, files only at leaves

  val=$(get_ini_param mdtest-easy n 1000000)
  [ -n "$val" ] && io500_mdtest_easy_files_per_proc="$val"
  val=$(get_ini_param mdtest-easy API POSIX)
  [ -n "$val" ] && io500_mdtest_easy_params+=" -a $val"
  val=$(get_ini_param mdtest-easy posix.odirect)
  [ "$val" = "True" ] && io500_mdtest_easy_params+=" --posix.odirect"
  echo -n ""
}

function setup_ior_hard {
  local params

  io500_ior_hard_api=$(get_ini_param ior-hard API POSIX)
  io500_ior_hard_writes_per_proc="$(get_ini_param ior-hard segmentCount 10000000)"
  val="$(get_ini_param ior-hard hintsFileName)"
  [ -n "$val" ] && params+=" -U $val"
  val="$(get_ini_param ior-hard posix.odirect)"
  [ "$val" = "True" ] && params+=" --posix.odirect"
  val="$(get_ini_param ior-easy verbosity)"
  [ -n "$val" ] && params+="$(yes ' -v' | head -$val)"
  io500_ior_hard_api_specific_options="$params"
  echo -n ""
}

function setup_mdt_hard {
  val=$(get_ini_param mdtest-hard n 1000000)
  [ -n "$val" ] && io500_mdtest_hard_files_per_proc="$val"
  io500_mdtest_hard_api="$(get_ini_param mdtest-hard API POSIX)"
  io500_mdtest_hard_api_specific_options=""
  echo -n ""
}

function setup_find {
  val="$(get_ini_param find external-script)"
  [ -z "$val" ] && io500_find_mpi="True" && io500_find_cmd="$PWD/bin/pfind" ||
    io500_find_cmd="$val"
  # uses stonewalling, run pfind
  io500_find_cmd_args="$(get_ini_param find external-args)"
  echo -n ""
}

function setup_mdreal {
  echo -n ""
}

function run_benchmarks {
  local app_first=$((RANDOM % 100))
  local app_rc=0

  # run the app and C version in random order to try and avoid bias
  (( app_first >= 50 )) && $io500_mpirun $io500_mpiargs $PWD/io500 $io500_ini --timestamp $timestamp || app_rc=$?

  # Important: source the io500_fixed.sh script.  Do not change it. If you
  # discover a need to change it, please email the mailing list to discuss.
  source build/io500-dev/utilities/io500_fixed.sh 2>&1 |
    tee $io500_result_dir/io-500-summary.$timestamp.txt

  (( $app_first >= 50 )) && return $app_rc

  # run the app and C version in random order to try and avoid bias
  $io500_mpirun $io500_mpiargs $PWD/io500 $io500_ini --timestamp $timestamp
}

create_tarball() {
  local sourcedir=$(dirname $io500_result_dir)
  local fname=$(basename ${io500_result_dir%-scr})
  local tarball=$sourcedir/io500-$HOSTNAME-$fname.tgz

  cp -v $0 $io500_ini $io500_result_dir
  tar czf $tarball -C $sourcedir $fname-{app,scr}
  echo "Created result tarball $tarball"
}

# Information fields; these provide information about your system hardware
# Use https://vi4io.org/io500-info-creator/ to generate information about
# your hardware that you want to include publicly!
function extra_description {
  # UPDATE: Please add your information into "system-information.txt" pasting the output of the info-creator
  # EXAMPLE:
  # io500_info_system_name='xxx'
  # DO NOT ADD IT HERE
  :
}

main
