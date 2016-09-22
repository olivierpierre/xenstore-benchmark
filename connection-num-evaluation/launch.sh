#!/bin/bash
set -euo pipefail

BENCHMARK="../xenstore-benchmark"

echo "#connections;read;write;get_domain_path"

for i in 0 1 2 4 8 16 32 64 128 256 512; do
	sed "s/connection_num 0/connection_num $i/" config.cfg > tmp.cfg
	$BENCHMARK tmp.cfg > out
	read_val=`cat out | grep read | cut -f 2 -d ";"`
	write_val=`cat out | grep write | cut -f 2 -d ";"`
	get_domain_path_val=`cat out | grep get_domain_path | cut -f 2 -d ";"`
	
	echo "$i;$read_val;$write_val;$get_domain_path_val"
done

rm out tmp.cfg
