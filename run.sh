#!/bin/csh

####################  Usage  ########################
# ./run.sh <trace-file-directory>
# 1. The script will print the IPC of the User implemented policy on the terminal along with trace file name. The script will also print the Speedup once all the
#    traces have been executed.
# 2. The script will also generate a csv named "efectiu_results.csv" which will contain the trace file name, IPC from LRU, IPC from User policy and Individual 
#    Speedup.
#####################################################

setenv tracefile $1
if ( ${tracefile} == "" ) then
	printf "Usage: $0 <trace-file-directory>\n"
	exit 1
endif

if ( ! { make -q } ) then
	printf "efectiu program is not up to date.\n"
endif

if ( ! -e efectiu) then
	printf "efectiu program is not built.\n"
	exit 1
endif

set trace_list = `find ${tracefile} -name '*.trace.*' | sort`
set acc_product = 1
set n = 0
set log_file = "efectiu_results.csv"

printf "Trace File,IPC_LRU,IPC_User,SpeedUp\n" > $log_file

foreach i ( $trace_list )
  #Printing the trace file name
	printf "%-40s" $i | tee -a $log_file
	printf "," >> $log_file 

  #Computing the LRU IPC here
  setenv DAN_POLICY 0
	set IPC_lru = `./efectiu $i| & grep "^core " | sed -e 's/ IPC$//'|sed -e 's/^.*\s\+//'`
  printf "%19.4f," $IPC_lru >> $log_file

  #Computing the USER IPC here
  setenv DAN_POLICY 2
	set IPC_custom = `./efectiu $i| & grep "^core " | sed -e 's/ IPC$//'|sed -e 's/^.*\s\+//'`
  printf "%16.4f" $IPC_custom | tee -a $log_file
  printf "," >> $log_file

  #Computing the individual SpeedUp
	set SpeedUp_ratio = `printf "$IPC_custom / $IPC_lru\n" | bc -l`
	set acc_product = `printf "$acc_product * $SpeedUp_ratio\n" | bc -l`
  printf "%16.5f" $SpeedUp_ratio >> $log_file
  printf "\n" | tee -a $log_file
	@ n = $n + 1 #casn1h
end

printf "Geometric Mean SpeedUp is :"| tee -a "$log_file"
awk 'BEGIN { print (ARGV[1] ^ (1.0 / ARGV[2])) }' $acc_product $n| tee -a "$log_file"
exit 0
