#!/bin/bash

function setup() {
  echo -n "building..."

  rm -rf tmp
  mkdir -p tmp
  make clean |> /dev/null
  make |> /dev/null
  cp build/main tmp/crinha
  echo "done!"
  echo
}

function tests() {
  e=0
  for f in tests/*.rinha; do
    filename=$(basename $f)
    expected="$f.out"
    result="tmp/$filename.out"

    printf %-30s $filename | tr ' ' .

    tmp/crinha $f > $result
    if cmp -s $expected $result; then
      echo OK
    else
      ((e+=1))
      echo ERROR
      echo "    expected: $(cat $expected)"
      echo "    got:      $(cat $result)"
    fi
  done

  return $e
}

setup
tests

errors=$?
if (($errors > 0)); then
  echo
  echo "$errors errors found"
fi

exit $errors