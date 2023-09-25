#!/bin/bash

function setup() {
  TIMEFORMAT=%R
  echo -n "building....."

  rm -rf tmp
  mkdir -p tmp
  make clean &> /dev/null
  make &> /dev/null
  if (($? > 0)); then
    echo "error"
    exit 1
  fi
  cp build/main tmp/crinha
  echo "ok!"
  echo
}

function benchmark() {
  cmd=$1
  f=$2
  filename=$(basename $f)
  printf %-30s $filename | tr ' ' .
  elapsed=$( { time $cmd $f 2> /dev/null ; } 2>&1 )
  echo $elapsed
}

function benchmarks() {
  e=0
  for f in benchmarks/*.rinha; do
    filename=$(basename $f)

    benchmark tmp/crinha $f
    for b in benchmarks/${filename%.*}.*; do
      ext=${b##*.}

      case $ext in
        py)
          benchmark python3 $b
          ;;
        rb)
          benchmark ruby $b
          ;;
        c)
          gcc -o tmp/benchmark.out $b
          benchmark tmp/benchmark.out $b
          ;;
      esac
    done

    echo
  done

  return $e
}

setup
benchmarks | tee benchmarks/benchmarks.txt
