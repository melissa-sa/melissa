# Setting up a gitlab runner on $SERVER using udocker

## install podman local to create the melissa-gitlab-runner image
- simply follow https://computingforgeeks.com/how-to-install-podman-on-ubuntu/<Paste>

## create the melissa-gitlab-runner image to have all dependencies installed in it
```sh
podman build -t melissa/melissa-gitlab-runner .
```
*BTW: to run the gitlab runner locally you can use*
```sh
podman run --rm -t -i -v $HOME/gitlab-runner/config:/etc/gitlab-runner melissa/melissa-gitlab-runner <gitlab-runner-command-line-options>
```
*now.*


## upload the image to $SERVER
```sh
podman save -o melissa-gitlab-runner.tar melissa/melissa-gitlab-runner
scp melissa-gitlab-runner.tar $SERVER:~/
```

## goto $SERVER
```sh
ssh $SERVER
```

The fastest and easiest way, not even requiring special access rights using udocker:

### install udocker:
(from https://github.com/indigo-dc/udocker/blob/master/doc/installation_manual.md)
```sh
curl https://raw.githubusercontent.com/indigo-dc/udocker/master/udocker.py > udocker
chmod u+rx ./udocker
./udocker install
```

### install the gitlab runner image
create the config dir:
```sh
mkdir -p $HOME/gitlab-runner/config
```

load the gitlab runner image:
```sh
./udocker load -i melissa-gitlab-runner.tar
```

put this in your .bashrc and source it!
```sh
alias gitlab-runner='$HOME/udocker run --rm -t -i -v $HOME/gitlab-runner/config:/etc/gitlab-runner localhost/melissa/melissa-gitlab-runner:latest'
```

*usage of the new gitlab-runner alias: as you would use any installed gitlab-runner command:*
`gitlab-runner <command-line-options>`


### register the runner:
```sh
gitlab-runner register
```
- as gitlab url use `https://gitlab.inria.fr/`
- get the token from https://gitlab.inria.fr/melissa/melissa/-/settings/ci_cd#js-runners-settings
- as tags specify `linux,local,melissa`
- as executer specify `shell`

### start the runner:
```sh
gitlab-runner run
```
*Starting as a service does not work with udocker as there is no detached mode!*

*As an alternative you can use the following script to run in background:*
```sh
#!/bin/bash

rm nohup.out

cmd="$HOME/udocker run --rm -t -i -v $HOME/gitlab-runner/config:/etc/gitlab-runner localhost/melissa/melissa-gitlab-runner:latest run"

echo starting gitlab_runner in background!
nohup $cmd &
```


## Troubleshooting
- Very dirty but does the job if the gitlab runner in the contianer has not enough permissions:
```sh
chmod a+wr -R $HOME/gitlab-runner/config
```
