#!/bin/bash

set -e

echo This script downloads some code needed for the benchmarks
echo It will also attempt to build the benchmarks
echo It will output OK at the end if builds succeed
echo

LIBS3_HASH=287e4bee6fd430
# tested with the following mpi implementation
# MPILib="openmpi/2.0.2p2_hpcx-nag62"

# shellcheck source=/dev/null
source "$(dirname "$0")/../../prepare.sh"
function main_s3 {
  # listed here, easier to spot and run if something fails
  setup
  # Prerequistics
  echo "Please make sure that you have the required packages: curl libxml2 openssl libiconv gcc automake autoconf openmpi(or another mpi implementation) cmake"
  # module load curl libxml2 openssl libiconv gcc automake autoconf $MPILib cmake
  echo "Tested on Ubuntu: apt install -y gcc cmake autoconf openssl libssl-dev libcurl4-openssl-dev libxml2-dev openmpi-bin libopenmpi-dev"
  echo "Tested on Centos/Fedora: dnf install -y git gcc cmake automake autoconf openssl libxml2 libxml2-devel libcurl-devel openssl-devel openmpi"
  sleep 1
  get_schema_tools
  get_libs3
  get_ior
  get_pfind
  build_ior_s3
  build_pfind
  build_io500
  echo
  echo "OK: All required software packages are now prepared"
  ls "$BIN"
}

###### GET FUNCTION
function get_libs3 {
  echo "Getting libs3"
  git_co https://github.com/bji/libs3.git libs3 $LIBS3_HASH
  pushd "$BUILD"/libs3
  $MAKE clean
  DESTDIR=$INSTALL_DIR $MAKE install
  popd
}
###### BUILD FUNCTION
function build_ior_s3 {
  pushd "$BUILD"/ior
  ./bootstrap
  ./configure --prefix="$INSTALL_DIR" --with-S3-libs3 CPPFLAGS=-I"$INSTALL_DIR"/include LDFLAGS=-L"$INSTALL_DIR"/lib
  cd src
  $MAKE clean
  $MAKE install
  echo "IOR: OK"
  echo
  popd
}
###### CALL MAIN
main_s3
