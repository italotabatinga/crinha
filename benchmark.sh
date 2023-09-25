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
  times=20
  elapsed=0.0
  for (( i=1; i <= $times; i++ )); do
    it=$( { time $cmd $f 2> /dev/null ; } 2>&1 )
    elapsed=$(echo "$elapsed  $it" | awk '{printf "%f", $1 + $2}' )
  done
  echo "$elapsed $times" | awk '{printf "%f\n", $1 / $2}'
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
