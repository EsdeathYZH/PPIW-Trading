#!/bin/bash
boost="boost_1_67_0"
zeromq="zeromq-4.3.4"

root=/home/team-7/repo/awsome-10w

cd deps

# zeromq
mkdir -p "${zeromq}-install"
tar zxf "${zeromq}.tar.gz"
cd "${zeromq}"
./configure --prefix="${root}/deps/${zeromq}-install/"
make && make install
cd ..

# boost
mkdir -p "${boost}-install"
tar zxf "${boost}.tar.gz"
cd "${boost}"
./bootstrap.sh --prefix="${root}/deps/${boost}-install"
./b2 install
cd ..

cd ..