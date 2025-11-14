from time import sleep

from GoProRecordingPlugin import EDMOPythonPlugin

# Mock interfaces for the sessions that we obtain from the C# server 
class CSString:
    def ToString(self): return "/home/derrick/gp/";
class session:
    SessionStorageDirectory = CSString()

class plugin:
    session = session()


# Test whether our module works as intended
if __name__ == '__main__':
    pythonPlugin = EDMOPythonPlugin(plugin())
    
    pythonPlugin.sessionStarted()
    
    sleep(10)
    pythonPlugin.sessionEnded()

