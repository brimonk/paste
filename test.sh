#!/usr/bin/env bash
# Brian Chrzanowski
# 2021-06-02 09:38:56
#
# Tests for the pastebin server.
#
# Basically, we roll through 1000 pastes and we see if we get everything back
# that we send.

ADDR="http://localhost"
PORT=5000

LIMIT=5000
BATCH=100

DATADIR=$(mktemp -d -t paste-XXXXXXXX)

mkdir -pv "$DATADIR"

function get_barename
{
	echo "$DATADIR/blob-$1"
}

# testdata: puts some test data in a file
function mk_testdata
{
	dd if=/dev/random of="$1" bs=4096 count=1 &> /dev/null
}

# upload_testdata: uploads test data
function upload_testdata
{
	cat "$1" | curl --silent --data-binary @- "$ADDR:$PORT" > "$2"
}

# get_testdata: fetches test data for comparison
function get_testdata
{
	curl --silent --output "$2" "$1"
}

# blob_compare: compares the two blobs
function blob_compare
{
	cmp -l $1 $2 | wc -c &> /dev/null
}

# single_paste: goes through the motions of running the e2e suite for one paste
function single_paste
{
	BASENAME=$(get_barename $1)

	mk_testdata "$BASENAME.dat"

	upload_testdata "$BASENAME.dat" "$BASENAME.url"

	URL=$(cat "$BASENAME.url")

	echo -e "$1\t$URL"

	get_testdata "$URL" "$BASENAME.rdat"

	blob_compare "$BASENAME.dat" "$BASENAME.rdat"
}

for i in $(seq 0 $BATCH $((LIMIT - 1))); do
	for j in $(seq $i 1 $(($i + $BATCH - 1))); do
		single_paste $j &
	done

	wait $(jobs -rp)
done

rm -rf "$DATADIR"

