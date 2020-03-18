FROM registry.hub.docker.com/gitlab/gitlab-runner
RUN apt-get -qq update
RUN apt-get install -y \
 psmisc \
 bc \
 gfortran \
 git \
 autoconf \
 openmpi-bin libopenmpi-dev openmpi-common \
 build-essential gcc g++ make cmake \
 python3 python3-numpy libzmq5-dev
