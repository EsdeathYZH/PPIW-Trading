#!/bin/bash
#e.g. ./deps_sync.sh

root=/home/team-7/repo/awsome-10w

cat mpd.hosts | while read machine
do
	echo sync to $machine
	#Don't copy things like Makefile, CMakeFiles, etc in build directory.
	scp -r -P 40724 ./deps/zeromq-4.3.4-install/ team-7@${machine}:${root}/deps/
	scp -r -P 40724 ./deps/boost_1_67_0-install/ team-7@${machine}:${root}/deps/
done
