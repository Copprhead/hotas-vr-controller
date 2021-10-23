# hotas-vr-controller
An OpenVR driver that allows the hands-free use of your regular VR controllers, so that you can keep your Hands On Throttle And Stick (HOTAS). 

Your hands are the most intuitive way to interact with a virtual clickable cockpit in VR. A regular VR controller is a good way to click buttons, flick switches or rotate a knob. But when it comes to flight controls the use of a physical joystick and throttle are vital for precise flying. But it's tedious to constantly switch your hands between the VR controllers and the physical joystick or throttle as you can't hold them at the same time. True hand tracking would be the solution, but unfortunately, this is in most cases not (yet) reliable and for most of us also not an obtainable solution. 

hotas-vr-controller is an OpenVR driver that allows modifying the position and the rotation of your regular VR controllers. This makes it possible to strap the regular VR controllers to the back of your hands or your lower arm, while the virtual hands still appear in the correct position and orientation. hotas-vr-controller captures mouse clicks and emulates the grip and the trigger inputs of the VR controllers. "Finger mouse" devices (small mouse devices typically used for presentations) can be worn on the index fingers of both hands to trigger these actions.

This configuration enables a fluid transition of your hands between a physical joystick or throttle and the hand interactions with the virtual clickable cockpit. 

# Supported flight simulations
* DCS World 2.7

# Supported VR controllers
* Should work with any OpenVR compatible tracked device associated with the left hand or right hand and with a grip and trigger input.
* Tested so far only with HP Reverb G2.

# Prerequisites
* Two VR controllers
* Two "Finger Mouse" devices
* Strapping material (e.g. old gloves and zip ties)
* SteamVR 1.16.8 or higher
* Interception driver https://github.com/oblitum/Interception (included in the release package)

# Installation
1. Download the latest hotas-vr-controller release package. 
2. Unzip the release package in a folder of your choice. For example: `C:\abc\hotas-vr-controller`.
3. Install the Interception driver. The command-line installer `install-interception.exe` of the driver is included in `bin` folder of the release package. **IMPORTANT: Do not double-click it. The installer must be run within a Windows Command Prompt (cmd.exe) with administrative rights.** To do this, click the "Start" button and type in "Command Prompt". Right-click on the "Command Prompt" icon and select "Run as administrator". Then use the `cd` command to change to the directory of the installer and then execute `install-interception.exe /install`. For example:
```
cd C:\abc\hotas-vr-controller\bin
install-interception.exe /install
```
4. Register the OpenVR driver. SteamVR has the command line tool `vrpathreg.exe` for that. Open a command prompt (click the "Start" button and type in "Command Prompt"). The actual paths depend on where your Steam apps are installed. For example:
```
"C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win64\vrpathreg.exe" adddriver "C:\abc\hotas-vr-controller\hotas"
```

5. Run SteamVR and continue with the configuration.

# Configuration
hotas-vr-controller is configured via the `C:\abc\hotas-vr-controller\bin\win64\driver_hotas.ini` file:

```
[LeftControllerOffset]
x=-0.125
y=-0.1
z=0.05
rx=-22.5
ry=45
rz=-100

[LeftDevice]
hardware=HID\VID_0250&PID_3412&REV_1001
index=1

[RightControllerOffset]
x=-0.16
y=-0.05
z=0.05
rx=-45
ry=55
rz=-55

[RightDevice]
hardware=HID\VID_0250&PID_3412&REV_1001
index=0
```

1. Strap the VR controllers to the outside of your hand or lower arm. Choose a position that allows you to grab your joystick and throttle.
2. Modify the offset (x, y, z) and rotation (rx, ry, rz) values until the positions of the virtual hands match with the position of your actual hands. This can be done while SteamVR is running, the values are updated every 10 sec.
3. Strack on the "Finger Mouse" devices. To determine the hardware id of your click device perform a left or right-click with every mouse device while SteamVR is running. The hardware id is written to the `C:\abc\hotas-vr-controller\bin\win64driver_hotas.log` file. For example:
```
[hh:mm:ss] DeviceId: 14 HardwareId: HID\VID_046D&PID_C01E&REV_2200
```
3. Change the device index in case both devices have the same hardware id. The index defines which of the two devices will be detected first after SteamVR was started. So in the example above, do a left-click or right-click with the device of your right hand before you left-click or right-click with the device of your left hand.
4. Restart SteamVR after changing the hardware id and the device index.

# Uninstallation
1. Deregister the Open VR driver. SteamVR has the command line tool `vrpathreg.exe` for that. Open a command prompt (click the "Start" button and type in "Command Prompt"). The actual paths depend on where your Steam apps are installed. For example:
```
"C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win64\vrpathreg.exe" removedriver "C:\abc\hotas-vr-controller\hotas"
```

2. Uninstall the Interception driver. The command-line installer `install-interception.exe` of the driver is included in `bin` folder of the release package. **IMPORTANT: Do not double-click it. The installer must be run within a Windows Command Prompt (cmd.exe) with administrative rights.** To do this, click the "Start" button and type in "Command Prompt". Right-click on the "Command Prompt" icon and select "Run as administrator". Then use the `cd` command to change to the directory of the installer and then execute `install-interception.exe /install`. For example:
```
cd C:\abc\hotas-vr-controller\bin
install-interception.exe /uninstall
```
3. Delete the folder with the files from the release package. For example: `C:\abc\hotas-vr-controller`.
