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
 python3 python3-numpy libzmq5-dev pkg-config



# Do not run testcases as root as mpirun does not like this...
ARG userid=1000
RUN export uid=$userid gid=$userid && \
    mkdir -p /home/developer && \
    echo "docker:x:${uid}:${gid}:Docker,,,:/home/docker:/bin/bash" >> /etc/passwd && \
    echo "docker:x:${uid}:" >> /etc/group

RUN echo "docker:docker" | chpasswd
USER docker
