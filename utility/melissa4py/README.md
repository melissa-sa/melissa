# Melissa4py
*A python based version of the melissa server*

The melissa4py package includes an implementation of the melissa server in pure python. This was originaly intender for easy development/prototyping.

## Installation
To use melissa4py you need to install the python package. This can be done when building/installing melissa with extra cmake options or manualy using pip.

1. Installing with CMake
   
   The following options can be specified when building melissa
   - `MELISSA4PY_INSTALL`: install the melissa4py package
   - `MELISSA4PY_REQUIREMENTS`: install the melissa4py package requirements.
   - `MELISSA4PY_DEVELOP_MODE`: install the melissa4py package on develop mode.
   - `MELISSA4PY_USER_MODE`: install the melissa4py on user mode.

2. Installing with PIP
   
   This method allows for only the melissa4py package to be installed. From the melissa4py directory the following commands can be issued
   ```bash
   # Install requirements
   pip install -r requirements.txt
   # Install package
   pip install .
   ```

## Usage

The main functionality in provided by melissa4py is in the `MelissaServer` object in the server.py module. The object allows to abstract the user from the communication layer of the melissa protocol. That is, the `MelissaServer` will connect to the launcher, handle connection and data messages from simulations and provide some of the fault tolerance mechanisms of the c based melissa server.

Another module that might concern the user is the `message` module. This module contains objects that map to some of the messages of the melissa protocol. Most likely it will be the case that the user would like to process incomming data form the simulations. For this case the server will deserialize incomming data messages from simulations into the `SimulationData` object and the user can then access the message parts using the attributes of this object.

So a simple application using the `MelissaServer` could look like:

```python
from melissa4py.message import SimulationData
from melissa4py.server import MelissaServer


def do_stuff(data, parameters, timestep):
    print(data, parameters, timestep)


def main(args):
    # 1. Configure the and initlaize server
    server = MelissaServer(args.timesteps,
                           args.nb_proc_server,
                           args.nb_parameters_simulation,
                           text_pull_port=args.txt_pull_port,
                           text_push_port=args.txt_push_port,
                           text_request_port=args.txt_req_port,
    			           launcher_node_name=args.launcher_node_name,
                           fields=['heat1'])
    samples = 0
    while True:
        # 2. Check for messages/events
        status = server.run(timeout=100)
        if isinstance(status, SimulationData):
            # 3. Handle/process simulation data messages
            do_stuff(status.data, status.parameters, status.timestep)
            samples += 1
            if samples >= int(args.sample_size * args.timesteps):
                print('Recieved all samples. Stopping..')
                break
    # 4. Stop server
    server.stop()
```

### Examples

A full working example can be found on `melissa/examples/heat_example/python_server`. 