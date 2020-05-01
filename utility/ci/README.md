# Melissa CI/CD setup for GitLab

## Prerequisites

* Machine with root access

Whole setup was tested on Ubuntu Server 18.04 LTS. Any other distribution should work. This guide, however, assumes you use Debian/Ubuntu distribution with x86-64 architecture.


## Setting up GitLab Runner

In case of any problems or different distribution, whole [tutorial on installation](https://docs.gitlab.com/runner/install/linux-manually.html) and [tutorial on registering](https://docs.gitlab.com/runner/register/index.html) is on GitLab official page.

### Installing GitLab Runner


1. Download GitLab Runner binary
   
   ```bash
   sudo curl -L --output /usr/local/bin/gitlab-runner https://gitlab-runner-downloads.s3.amazonaws.com/latest/binaries/gitlab-runner-linux-amd64
   ```

2. Give it permissions to execute
   
   ```bash
   sudo chmod +x /usr/local/bin/gitlab-runner
   ```

3. Create a GitLab CI user, set password and add to sudoers group
   
   ```bash
   sudo useradd --comment 'GitLab Runner' --create-home gitlab-runner --shell /bin/bash
   sudo passwd gitlab-runner
   sudo adduser gitlab-runner sudo
   ```

4. Install and run as service
   
   ```bash
   sudo gitlab-runner install --user=gitlab-runner --working-directory=/home/gitlab-runner
   sudo gitlab-runner start
   ```

### Registering GitLab Runner

Visit your repository web page. In `Settings -> CI / CD -> Runners` find your registration token.

To register, use command
```bash
sudo gitlab-runner register
```

When asked for tags, type `Melissa`. This will allow to use tagged jobs. As for the executor, type `shell`

## Installing dependencies


### Installing Melissa's dependencies

```bash
sudo apt-get install -y pkg-config gfortran libopenmpi-dev openmpi-bin cmake cmake-curses-gui python3 pip
sudo pip install numpy
```

### Installing docker

```bash
curl -sSL https://get.docker.com/ | sh
sudo usermod -aG docker gitlab-runner
```

**Reboot the system and login on gitlab-runner account**

### Installing OAR-docker

```bash
cd $HOME
git clone https://github.com/oar-team/oar-docker.git
cd oar-docker
git checkout dev
sudo pip3 install -e .
cd ..
export LC_ALL=C.UTF-8
export LANG=C.UTF-8
oardocker init
cat << 'EOF' >  .oardocker/images/base/custom_setup.sh
#!/usr/bin/env bash

echo "Melissa Deps"
apt-get update
apt-get install -y pkg-config
apt-get install -y gfortran libopenmpi-dev openmpi-bin
apt-get install -y cmake cmake-curses-gui
# assumption: Python3 module is available in the Virtual Cluster
python3 -m pip install numpy

echo "Slurm"
apt-get install -y slurm-wlm
EOF
oardocker build
oardocker install http://oar-ftp.imag.fr/oar/2.5/sources/testing/oar-2.5.8.tar.gz
```

## Other setup

* If your `python` command doesn't point to python3, make a soft link.
    
    ```bash
    whereis python3 # find where is your python3 located. Usually it's /usr/bin
    cd /usr/bin
    sudo ln -s python3 python
    ```

* Move `CI-scripts` folder to gitlab-runner home direcory
  
* In your repository, swap `.gitlab-ci.yml` with the one inside this package

## Known Errors

* `UnixHTTPConnectionPool(host='localhost', port=None): Read timed out. (read timeout=60)`
  * Error appears randomly and goes away after another run or brick the system for good
  * There isn't any solution to fixing this problem besides restarting docker via `sudo service docker restart`. [Github issue is open since 2016...](https://github.com/docker/compose/issues/3927)
  * Setting `DOCKER_CLIENT_TIMEOUT=120` and `COMPOSE_HTTP_TIMEOUT=120` was supposed to help but I don't see much improvement

