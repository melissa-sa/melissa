git submodule update
cd melissa/
#mkdir build/
cd build
cmake ..  -DINSTALL_ZMQ=ON -DBUILD_DOCUMENTATION=OFF -DBUILD_TESTING=OFF
# cmake ..  -DINSTALL_ZMQ=ON  -DCMAKE_BUILD_TYPE=DEBUG -DBUILD_DOCUMENTATION=OFF -DBUILD_TESTING=OFF
# make -j4
make install -j4
source ../install/bin/melissa_set_env.sh
cd ../../
cd simulators/mantaflow
mkdir build
cd build
cmake .. -DGUI=OFF -DOPENMP=OFF -DNUMPY=ON
make -j4
MANTAFLOW_PATH=$PWD
export MANTAFLOW_PATH
