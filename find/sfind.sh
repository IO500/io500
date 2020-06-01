#! /bin/bash

# this script takes arguments just like the normal GNU find command
# in fact, it just passes them directly on

# you can replace this with whatever you want.  You will almost surely want to because this will be really slow for large number of files.
# Whatever you do, you just need to accept something that looks like normal find parameters:
# -name *01* -size 3901c -newer some_file 
# When you are done pring 'x/y' where x is matched files and y is total files searched.
# There is a parallel version also install in bin that might be better

function parse_rates {
  #find -D rates gives a weird thing like this:
  #Predicate success rates after completion
  # (  ( -name *01* [0.8] [602/30415=0.0197929] -a [0.008] [308/30415=0.0101266] [call stat] [need type] -newer /Users/jbent/io-500-dev/datafiles/io500.2017.10.21-21.05.56/timestampfile [0.01] [308/602=0.511628]  ) -a [8e-05] [308/30415=0.0101266] [call stat] -size 3900 [0.01] [308/308=1]  ) -a [8e-05] [308/30415=0.0101266] -print [1] [308/308=1] 
  # so if we parse out the first 602/30415, we can get 30415 as the total number of files searched
  # and if we parse out the final 308/308, we can get 308 as the total number of files matched 
  echo $rates | tr " " "\n" | grep '/' | $1 -1 | cut -d \/ -f 2 | cut -d = -f 1
}

rates=`find -D rates $* 2>&1 | grep -A1 Predicate | tail -1` 
total_files=$(parse_rates 'head')
match_files=$(parse_rates 'tail')
echo "MATCHED $match_files/$total_files"
