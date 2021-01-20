# Melissa Code\_Saturne 6 Example

The example has only been tested with Code\_Saturne version 6.

How to run the example locally with OpenMPI:

* Decide on an installation directry which is called `prefix` here.
* Build and install Melissa in `prefix`.
* Check if the Code\_Saturne executable can be found: execute `which code_saturne`. If no path is shown, you must locate the directory containing `code_saturne` and add this directory to the `PATH` environment variable: `export PATH="path/to/code-saturne/executable:$PATH"`.
* Check if the Melissa executable can be found: execute `which melissa-launcher`. If no path is shown, you must modify the `PATH` environment variable: `export PATH="$prefix/bin:$PATH"`.
* Create a working directory because Melissa and Code\_Saturne will create a large number of files: `mkdir workspace && cd workspace`.

You can run the example now:
```sh
melissa-launcher \
    openmpi \
    "$prefix/share/melissa/examples/Code_Saturne/options.py" \
    "$prefix/share/melissa/examples/Code_Saturne/run-code-saturne.py"
```
