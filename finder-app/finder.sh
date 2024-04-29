#!/bin/sh
# Author: Oleh Klymenko

filesdir=$1
searchstr=$2


if [ -z "${filesdir}" ]; then
  echo "ERROR: filesdir is empty"
  exit 1
fi

if [ -z "${searchstr}" ]; then
  echo "ERROR: searchstr is empty"
  exit 1
fi

if [ ! -d "${filesdir}" ]; then
  echo "ERROR: filesdir is not a directory"
  exit 1
fi


grep_result=$( grep -Rc ${searchstr} ${filesdir})

files_cnt=0
match_total=0

for line in $grep_result; do
  match_in_file="$((${line#*:}))"
  if [ "$match_in_file" -gt 0 ]; then
    files_cnt=$((files_cnt + 1))
    match_total=$((match_total + match_in_file))
  fi
done


echo "The number of files are ${files_cnt} and the number of matching lines are ${match_total}"

