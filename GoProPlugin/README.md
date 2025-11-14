# GoPro integration plugin
This plugin provides GoPro integration to an EDMOSession. 

## How it works.
When a session starts, all GoPro cameras connected to the hosting computer will begin recording in a synchronised manner. Likewise, when a session ends, all GoPros that are recording will be stopped simultaneously.

After stopping the recording, the video files will be downloaded from the cameras and placed in SessionRoot. The transfer process is done on one camera at a time. During this process, the session is blocked from closing, which avoids new sessions being started. Once transfer is complete, a new session can be started.

## Tested devices
* HERO 13

## Dependencies
* [Python 3.12](https://www.python.org/downloads/)
    + This is the python version supported by `PythonNet`
* [Open GoPro library](https://pypi.org/project/open_gopro/)

## Installation instructions

* [Download the python script](https://github.com/TeamEDMO/GoProPlugin/blob/master/GoProRecordingPlugin.py)
* Install OpenGoPro library into the global environment
    + This must be installed into the global environment in order to ensure the Python plugin loader can correctly import the library
 ```sh
python3.12 -m pip install open_gopro
```
* Place the downloaded python script into the plugins folder of the EDMO Server

## Usage instructions
* Connect any number of GoPro cameras to the computer using USB-C
    + Ensure that sufficient power is provided to the GoPro Cameras if you intend to use the cameras without a battery.
* Turn on the GoPro
* Run EDMOSessions as usual.

