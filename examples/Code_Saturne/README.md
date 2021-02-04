# Melissa Code\_Saturne 6 Example

The example has only been tested with Code\_Saturne version 6.0.

How to run the example locally with OpenMPI:

* Decide on an installation directry which is called `$prefix` here.
* Build and install Melissa in `$prefix`.
* Update environment variables to make sure the Melissa launcher and server can be started: `source "$prefix/bin/melissa-set-env.sh"`
* Check if the Code\_Saturne executable can be found: execute `which code_saturne`. If no path is shown, you must locate the directory containing `code_saturne` and add this directory to the `PATH` environment variable: `export PATH="path/to/code-saturne/executable:$PATH"`.
* Create a working directory because Melissa and Code\_Saturne will create a large number of files: `mkdir workspace && cd workspace`.

You can run the example now:
```sh
melissa-launcher \
    --with-simulation-setup
    openmpi \
    "$prefix/share/melissa/examples/Code_Saturne/options.py" \
    "$prefix/share/melissa/examples/Code_Saturne/run-code-saturne.py"
```
