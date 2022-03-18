#!/bin/bash

root=/home/team-7/repo/awsome-10w

cat mpd.hosts | while read machine
do
	echo mkdir on $machine
    ssh -p 40724 team-7@${machine} "mkdir -p ${root}/deps/; mkdir -p ${root}/build/"
done