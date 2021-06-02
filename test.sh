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

LIMIT=100

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
	cat "$1" | curl --silent --output "$2" --data-binary @- "$ADDR:$PORT"
}

# get_testdata: fetches test data for comparison
function get_testdata
{
	curl --silent --output "$2.rdat" "$1"
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
	mk_testdata $BASENAME
	upload_testdata "$BASENAME.dat" "$BASENAME.url"
	URL=$(cat "$BASENAME.url")
	get_testdata "$URL" "$BASENAME.rdat"
	blob_compare "$BASENAME.dat" "$BASENAME.rdat"
}

# make all of the data
for i in $(seq 1 1 $LIMIT); do
	mk_testdata $i &
done

echo $(jobs -rp)

wait $(jobs -rp)

# # paste all of the pastes
# for f in $(find "$DATADIR" -name "blob*.dat"); do
# 	NAKED=${f%.dat}
# 	upload_testdata "$NAKED"
# done
# 
# # now, for every url, do a GET on that url
# for f in $(find "$DATADIR" -name "blob*.url"); do
# 	URL=$(cat $f)
# 	NAKED="${f%.url}"
# 	get_testdata "$URL" "$NAKED"
# done
# 
# # then, compare all of them
# for f in $(find "$DATADIR" -name "blob*.dat"); do
# 	NAKED="${f%.dat}"
# 	cmp -l "$NAKED.dat" "$NAKED.rdat" \
# 		| gawk '{printf "%08X %02X %02X\n", $1, strtonum(0$2), strtonum(0$3)}'
# done

rm -rf "$DATADIR"

