#!/bin/bash
#e.g. ./sync.sh

root=/home/team-7/repo/awsome-10w/

cat mpd.hosts | while read machine
do
	echo sync to $machine
	#Don't copy things like Makefile, CMakeFiles, etc in build directory.
	scp -r -P 40724 ./build/exchange team-7@${machine}:${root}/build/
	scp -r -P 40724 ./build/trader team-7@${machine}:${root}/build/
    scp -r -P 40724 config.template team-7@${machine}:${root}
    scp -r -P 40724 network.host team-7@${machine}:${root}
done
