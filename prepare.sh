#!/bin/bash -e

set -e

echo This script downloads the code for the benchmarks
echo It will also attempt to build the benchmarks
echo It will output OK at the end if builds succeed
echo

IOR_HASH=bd76b45ef9db
PFIND_HASH=9d77056adce6

INSTALL_DIR=$PWD
BIN=$INSTALL_DIR/bin
BUILD=$PWD/build
MAKE="make -j4"

function main {
  # listed here, easier to spot and run if something fails
  setup

  get_ior
  get_pfind

  build_ior
  build_pfind
  build_io500

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
  local repo=$1
  local dir=$2
  local tag=$3

  pushd $BUILD
  [ -d "$dir" ] || git clone $repo $dir
  cd $dir
  # turning off the hash thing for now because too many changes happening too quickly
  git fetch
  git checkout $tag
  popd
}

###### GET FUNCTIONS
function get_ior {
  echo "Getting IOR and mdtest"
  git_co https://github.com/hpc/ior.git ior $IOR_HASH
}

function get_pfind {
  echo "Preparing parallel find"
  git_co https://github.com/VI4IO/pfind.git pfind $PFIND_HASH
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

function build_pfind {
  pushd $BUILD/pfind
  ./prepare.sh
  ./compile.sh
  ln -sf $BUILD/pfind/pfind $BIN/pfind
  echo "Pfind: OK"
  echo
  popd
}

function build_io500 {
  make
  echo "io500: OK"
  echo
}

###### CALL MAIN
main
