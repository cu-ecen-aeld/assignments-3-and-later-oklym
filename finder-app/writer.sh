#!/bin/sh
# Author: Oleh Klymenko

write_file=$1
write_str=$2


if [ -z "${write_file}" ]; then
  echo "ERROR: write_file is empty"
  exit 1
fi

if [ -z "${write_str}" ]; then
  echo "ERROR: write_str is empty"
  exit 1
fi


dir_name=$(dirname $write_file)
if [ ! -d "$dir_name" ]; then
  mkdir -p $dir_name
fi


echo "${write_str}" > "${write_file}"

