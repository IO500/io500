#!/bin/bash -e

set -e

echo This script downloads the code for the benchmarks
echo It will also attempt to build the benchmarks
echo It will output OK at the end if builds succeed
echo
IOR_HASH=8ab8f69b32b919
PFIND_HASH=aaba722a178

INSTALL_DIR=$PWD
BIN=$INSTALL_DIR/bin
BUILD=$PWD/build
MAKE="make -j${NPROC:-$(nproc 2> /dev/null || echo 4)}"	# handle missing nproc

function main {
  # listed here, easier to spot and run if something fails
  setup

  get_schema_tools
  get_ior
  get_pfind

  build_ior
  build_pfind
  build_io500

  echo
  echo "OK: All required software packages are now prepared"
  ls "$BIN"
}

function setup {
  #rm -rf $BUILD $BIN
  mkdir -p "$BUILD" "$BIN"
  #cp utilities/find/mmfind.sh $BIN
}

function git_co {
  local repo=$1
  local dir=$2
  local tag=$3

  pushd "$BUILD"
  [ -d "$dir" ] || git clone "$repo" "$dir"
  cd "$dir"
  git fetch
  if [ -n "$tag" ]; then git checkout "$tag"; fi
  popd
}

###### GET FUNCTIONS
function get_ior {
  local ior_dir="ior"
  if [ -d "$ior_dir" ]; then
    echo "IOR already exists. Skipping download."
  else
    echo "Getting IOR and mdtest"
    git_co https://github.com/hpc/ior.git "$ior_dir" $IOR_HASH
  fi
}

function get_pfind {
  local pfind_dir="pfind"
  if [ -d "$pfind_dir" ]; then
    echo "Parallel find already exists. Skipping download."
  else
    echo "Preparing parallel find"
    git_co https://github.com/VI4IO/pfind.git "$pfind_dir" $PFIND_HASH
  fi
}

function get_schema_tools {
  local schema_build_dir="build/cdcl-schema-tools"
  local schema_dir="schema-tools"
  if [ -d "$schema_build_dir" ] && [ -d "$schema_dir" ]; then
    echo "Schema tools already exist. Skipping download."
  else
    echo "Downloading supplementary schema tools"
    git_co https://github.com/VI4IO/cdcl-schema-tools.git cdcl-schema-tools
    [ -d "$dir" ] || ln -sf "$PWD"/build/cdcl-schema-tools schema-tools
  fi
}

###### BUILD FUNCTIONS
function build_ior {
  pushd "$BUILD"/ior
  ./bootstrap
  # Add here extra flags
  ./configure --prefix="$INSTALL_DIR"
  cd src
  $MAKE clean
  $MAKE install
  echo "IOR: OK"
  echo
  popd
}

function build_pfind {
  pushd "$BUILD"/pfind
  ./prepare.sh
  ./compile.sh
  ln -sf "$BUILD"/pfind/pfind "$BIN"/pfind
  echo "Pfind: OK"
  echo
  popd
}

function build_io500 {
  $MAKE
  echo "io500: OK"
  echo
}

###### CALL MAIN
main
