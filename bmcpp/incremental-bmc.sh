#!/bin/sh
die () {
    echo >&2 "$1"
    rm /tmp/tmp;
    exit 0
}

[ "$#" -eq 2 ] || die "2 arguments are required: <filename> <k>, but only $# were provided"

k=0
maxK=$2
while [[ $k -le $maxK ]]; do
	./bmc $1 $k > /tmp/tmp
	output=$(grep -c "^Incorrect" /tmp/tmp)
	[ "$output" -eq 0 ] || die "Incorrect"
	let k=$k+1
done
echo "Correct"
rm /tmp/tmp
