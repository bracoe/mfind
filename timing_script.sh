#!/bin/bash
#timing_script
#Created on: 21 Oct 2018
#    Author: Bram Coenen

rm -f time_results
>&2 echo "-p 1"
time ./mfind -p 1 /pkg comsol >> /dev/null

>&2 echo "-p 2" >> time_results
time ./mfind -p 2 /pkg comsol >> /dev/null

>&2 echo "-p 3"
time ./mfind -p 3 /pkg comsol >> /dev/null

>&2 echo "-p 4"
time ./mfind -p 4 /pkg comsol >> /dev/null

>&2 echo "-p 5"
time ./mfind -p 5 /pkg comsol >> /dev/null

>&2 echo "-p 6"
time ./mfind -p 6 /pkg comsol >> /dev/null

>&2 echo "-p 7"
time ./mfind -p 7 /pkg comsol >> /dev/null

>&2 echo "-p 8"
time ./mfind -p 8 /pkg comsol >> /dev/null

>&2 echo "-p 9"
time ./mfind -p 9 /pkg comsol >> /dev/null

>&2 echo "-p 10"
time ./mfind -p 10 /pkg comsol >> /dev/null