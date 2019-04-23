FROM oardocker/debian9:2018.04.15
MAINTAINER Salem Harrache "salem.harrache@inria.fr"


ENV container=docker \
    LC_CTYPE=en_US.UTF-8 \
    LANG=en_US.UTF-8 \
    LC_ALL=en_US.UTF-8 \
    TERM=xterm \
    DEBIAN_FRONTEND=noninteractive \
    DEBIAN_PRIORITY=critical \
    DEBCONF_NOWARNINGS=yes \
    COLOR=green

RUN echo "export COLOR=green" > /etc/hostname.color

RUN echo "update"
RUN apt-get update
RUN apt-get install -y pkg-config
RUN apt-get install -y gfortran libopenmpi-dev openmpi-bin
RUN apt-get install -y cmake cmake-curses-gui
RUN apt-get install -y python3
RUN python3 -m pip install numpy

## rsyslog sender
COPY config/rsyslog.conf /etc/rsyslog.conf

## configure sshd daemon
ADD config/sshd_config /etc/ssh/sshd_config

## add helpers scripts:
ADD sbin/ /usr/local/sbin/
RUN chmod +x /usr/local/sbin/*
ADD bin/ /usr/local/bin/
RUN chmod 755 /usr/local/bin/*

## tmpfiles.d — Configuration for creation, deletion and cleaning of volatile and temporary files
ADD config/tmpfiles_oardocker.conf /etc/tmpfiles.d/oardocker.conf

## systemd services
RUN rm -rf /etc/systemd/system/*
ADD systemd /etc/systemd/system
RUN systemctl unmask systemd-tmpfiles-setup.service ; \
    systemctl set-default oardocker.target ; \
    chmod 644 /etc/systemd/system/* ; \
    systemctl enable oardocker-sshd.socket oardocker-ssh-keys oardocker-mount-cow systemd-tmpfiles-setup oardocker-rsyslog.service

ADD generate_ssh_keys.sh /tmp/generate_ssh_keys.sh
RUN bash /tmp/generate_ssh_keys.sh

ADD create_users.sh /tmp/create_users.sh
RUN bash /tmp/create_users.sh

ADD . /tmp
RUN echo "Running custom setup script..." && /bin/bash /tmp/custom_setup.sh  && rm -rf /tmp/*

VOLUME /sys/fs/cgroup
VOLUME /run

STOPSIGNAL SIGRTMIN+3

CMD ["/usr/local/sbin/container-init"]
