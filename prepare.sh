#!/bin/bash -e

set -e

echo This script downloads the code for the benchmarks
echo It will also attempt to build the benchmarks
echo It will output OK at the end if builds succeed
echo

IOR_HASH=8f14166a7939d644e
IO500_HASH=io500-isc20
MDREAL_HASH=io500-isc20

INSTALL_DIR=$PWD
BIN=$INSTALL_DIR/bin
BUILD=$PWD/build
MAKE="make -j4"

function main {
  # listed here, easier to spot and run if something fails
  setup

  get_ior
  get_pfind
  get_io500_dev
  #get_mdrealio || true  # this failed on RHEL 7.4 so turning off until fixed

  build_ior
  build_pfind
  build_io500_dev
  build_io500_app
#  build_mdrealio || true  # this failed on RHEL 7.4 so turning off until fixed

  echo
  echo "OK: All required software packages are now prepared"
  ls $BIN
}

function setup {
  #rm -rf $BUILD $BIN
  mkdir -p $BUILD $BIN
  #cp utilities/find/mmfind.sh $BIN
}

function git_co {
  pushd $BUILD
  [ -d "$2" ] || git clone $1 $2
  cd $2
  # turning off the hash thing for now because too many changes happening too quickly
  git fetch
  git checkout $3
  popd
}

###### GET FUNCTIONS
function get_ior {
  echo "Getting IOR and mdtest"
  git_co https://github.com/hpc/ior.git ior $IOR_HASH
}

function get_pfind {
  echo "Preparing parallel find"
  git_co https://github.com/VI4IO/pfind.git pfind
}

function get_io500_dev {
  echo "Getting IO500-dev"
  git_co https://github.com/VI4IO/io-500-dev io500-dev $IO500_HASH
}

function get_mdrealio {
  echo "Preparing MD-REAL-IO"
  git_co https://github.com/JulianKunkel/md-real-io md-real-io $MDREAL_HASH
}

###### BUILD FUNCTIONS
function build_ior {
  pushd $BUILD/ior
  ./bootstrap
  ./configure --prefix=$INSTALL_DIR
  cd src
  $MAKE clean
  $MAKE install
  echo "IOR: OK"
  echo
  popd
}

function build_io500_dev {
  mkdir -p $BUILD/io500-dev/build
  pushd $BUILD/io500-dev/build
  ln -sf $BUILD/ior
  ln -sf $BUILD/pfind
  echo "io500-dev: OK"
  echo
  popd
}

function build_pfind {
  pushd $BUILD/pfind
  ./prepare.sh
  ./compile.sh
  ln -sf $BUILD/pfind/pfind $BIN/pfind
  echo "Pfind: OK"
  echo
  popd
}

function build_io500_app {
  make
  echo "io500-app: OK"
  echo
}

function build_mdrealio {
  pushd $BUILD/md-real-io
  ./configure --prefix=$INSTALL_DIR --minimal
  $MAKE install
  #mv src/md-real-io $BIN
  echo "MD-REAL-IO: OK"
  echo
  popd
}

###### CALL MAIN
main
