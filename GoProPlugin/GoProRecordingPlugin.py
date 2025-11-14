import datetime
import pathlib
import typing
from typing import Protocol
import asyncio

import open_gopro.network.wifi.mdns_scanner
import zeroconf.asyncio
from open_gopro import WiredGoPro
from open_gopro.models import constants, proto


class EDMOPythonPlugin(Protocol):
    def __init__(self, edmoPlugin):
        self.existingVideoList = None
        self.media_set_before = None
        self.gopros: list[WiredGoPro] = []

        self.edmoPlugin = edmoPlugin

        self.storageDirectory = f"{edmoPlugin.session.SessionStorageDirectory.ToString()}/Videos"

        # Let's ensure the directory is available
        pathlib.Path.mkdir(pathlib.Path(self.storageDirectory), parents=True, exist_ok=True)

        asyncio.run(self.initGoPro())

    async def initGoPro(self):
        connectedDevices = await self.getAllConnectedDevices()

        for device in connectedDevices:
            gopro = WiredGoPro(device)
            await gopro.open()
            assert (await gopro.http_command.load_preset_group(group=proto.EnumPresetGroup.PRESET_GROUP_ID_VIDEO)).ok

            # Ensure that we aren't in turbo mode, where we are prohibited from recording
            await gopro.http_command.set_turbo_mode(mode=constants.Toggle.DISABLE)

            # The GoPros are not guaranteed to preserve the actual time
            # This may be due to the lack of battery to preserve the clock
            # This is cheap and quick enough to set
            await gopro.http_command.set_date_time(date_time=datetime.datetime.now())

            self.gopros.append(gopro)

        # We attempt to stop the cameras just in case it is recording alread.
        await self.stopCameras()

        # Let's get the current state of all media on device
        self.existingVideoList = [(gp, [file.filename for file in (await gp.http_command.get_media_list()).data.files]) for gp in self.gopros]



    @staticmethod
    async def getAllConnectedDevices():
        """
        This is a near verbatim copy of what OpenGoPro library does to find connected devices, adjusted to yield all connected devices, rather than only the first.
        :return: A list of GoPro identifiers
        """
        listener = open_gopro.network.wifi.mdns_scanner.ZeroconfListener()
        names = []
        with zeroconf.Zeroconf(unicast=True) as zc:
            # Bloom note: This uses a protected member of WiredGoPro. I don't care
            zeroconf.asyncio.AsyncServiceBrowser(zc, WiredGoPro._MDNS_SERVICE_NAME, listener)

            while True:
                try:
                    name = await asyncio.wait_for(listener.urls.get(), 2)
                    names.append(name.split('.')[0])

                except TimeoutError:
                    break

        return names

    @staticmethod
    def getName() -> str:
        return "GoProPlugin"

    def sessionStarted(self):
        asyncio.run(self.startCameras())

    async def startCameras(self):
        async with asyncio.TaskGroup() as tg:
            for gp in self.gopros:
                # Bloom note: Stupid quirk of these functions, despite being async, the GoPro commands are blocking
                ## Run on another thread to mitigate, allowing for more synchronised capture
                tg.create_task(
                    asyncio.to_thread(asyncio.run, gp.http_command.set_shutter(shutter=constants.Toggle.ENABLE)))

    async def stopCameras(self):
        async with asyncio.TaskGroup() as tg:
            for gp in self.gopros:
                # Bloom note: Stupid quirk of these functions, despite being async, the GoPro commands are blocking
                ## Run on another thread to mitigate, allowing for more synchronised capture
                tg.create_task(
                    asyncio.to_thread(asyncio.run, gp.http_command.set_shutter(shutter=constants.Toggle.DISABLE)))

    def sessionEnded(self):
        asyncio.run(self.endSession())

    async def endSession(self):
        await self.stopCameras()

        # Let's get the latest captured media from all of the connected go pros

        # Let's get the current state of all media on device
        newVideoList = [(gp, [file.filename for file in (await gp.http_command.get_media_list()).data.files]) for gp in self.gopros]

        # and we find which files are now present, and take the newly created ones
        distinctVideoList = []
        for(gp, files) in newVideoList:
            oldFileList = [files for (gp2, files) in self.existingVideoList if gp2.identifier == gp.identifier][0]
            distinctVideoList.append((gp,[file for file in files if file not in oldFileList]))


        # Jot down the exact files and cameras involved. Just in case manual intervention is needed to collect the video
        fileList = open(f"{self.storageDirectory}/recordedFiles.txt", mode="w")

        for (gp, files) in distinctVideoList:
            fileList.write(f"{gp.identifier}:\n")
            fileList.writelines([f"\t{file}\n" for file in files])


        # Bloom note: We could also run the download tasks on individual threads
        ## I opt not to as the procedure is not exactly time sensitive
        ## Commented out to save time
        """ 
           async with asyncio.TaskGroup() as tg:
                for (gp, files) in newVideoList:
                    for file in files:
                        tg.create_task(self.downloadFrom(gp, file))
        """
                
        # Close everything properly
        for gopro in self.gopros:
            await gopro.close()

    async def downloadFrom(self, gopro, videoPath):
        # Let's turn on turbo mode to accelerate data transfers
        # This has an added side-effect of displaying the "Transferring data" screen on the GoPros themselves
        await gopro.http_command.set_turbo_mode(mode=constants.Toggle.ENABLE)

        print(f"Downloading {videoPath} from {gopro.identifier}")

        videoDownloadDelegate = lambda: gopro.http_command.download_file(camera_file=videoPath,
                                                                         local_file=pathlib.Path(
                                                                             f"{self.storageDirectory}/{gopro.identifier}.mp4"))

        # Try downloading. If we fail for any reason, we bail and not even try the metadata.
        if not await EDMOPythonPlugin.attemptAsync(videoDownloadDelegate):
            print(f"\tFailed to download video for {videoPath}.")
            await gopro.http_command.set_turbo_mode(mode=constants.Toggle.DISABLE)
            return

        gpmfDownloadDelegate = lambda: gopro.http_command.get_gpmf_data(camera_file=videoPath, local_file=pathlib.Path(
            f"{self.storageDirectory}/{gopro.identifier}.gpmf"
        ))

        if not await EDMOPythonPlugin.attemptAsync(gpmfDownloadDelegate):
            print(f"\tFailed to download GPMF for {videoPath}.")

        await gopro.http_command.set_turbo_mode(mode=constants.Toggle.DISABLE)

    @staticmethod
    async def attemptAsync(function: typing.Callable, attempts: int = 3) -> bool:
        """
        Attempts to run a delegate. If the delegate throws an exception, the delegate is re-run.
        :param function: The target delegate
        :param attempts: The maximum number of attempts before giving up
        :return: True if any attempt succeeded; False otherwise. 
        """
        while attempts > 0:
            try:
                await function()
                return True
            except:
                attempts = attempts - 1

        return False
