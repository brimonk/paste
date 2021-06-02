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

LIMIT=1000

DATADIR=$(mktemp -d -t paste-XXXXXXXX)

mkdir -pv "$DATADIR"

# testdata: puts some test data in a file
function mk_testdata
{
	dd if=/dev/random of="$DATADIR/blob-$1.dat" bs=4096 count=1 &> /dev/null
}

# upload_testdata: uploads test data
function upload_testdata
{
	cat "$1.dat" | curl --silent --output "$1.url" --data-binary @- "$ADDR:$PORT"
}

# get_testdata: fetches test data for comparison
function get_testdata
{
	curl --silent --output "$2.rdat" "$1"
}

# make all of the data
for i in $(seq 1 1 $LIMIT); do
	mk_testdata $i
done

# paste all of the pastes
for f in $(find "$DATADIR" -name "blob*.dat"); do
	NAKED=${f%.dat}
	upload_testdata "$NAKED"
done

# now, for every url, do a GET on that url
for f in $(find "$DATADIR" -name "blob*.url"); do
	URL=$(cat $f)
	NAKED="${f%.url}"
	get_testdata "$URL" "$NAKED"
done

# then, compare all of them
for f in $(find "$DATADIR" -name "blob*.dat"); do
	NAKED="${f%.dat}"
	cmp -l "$NAKED.dat" "$NAKED.rdat" \
		| gawk '{printf "%08X %02X %02X\n", $1, strtonum(0$2), strtonum(0$3)}'
done

rm -rf "$DATADIR"

