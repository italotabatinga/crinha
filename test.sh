#!/bin/bash

function setup() {
  echo -n "building..."

  mkdir -p tmp
  (make clean && make) |> /dev/null
  cp build/main tmp/crinha
  echo "done!"
  echo
}

setup
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

if (($e > 0)); then
  echo
  echo "$e errors found"
fi

exit $e