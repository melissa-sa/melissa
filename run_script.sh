git submodule update
if [[ -d ./melissa/install]]
then
	source ./melissa/install/bin/melissa_set_env.sh
fi
if [[ -d ./simulators/mantaflow/build]]
then
	MANTAFLOW_PATH=$PWD/simulators/mantaflow/build
	export MANTAFLOW_PATH
fi
