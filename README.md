# hotas-vr-controller
An OpenVR driver that allows the hands-free use of your regular VR controllers, so that you can keep your Hands On Throttle And Stick (HOTAS). 

Your hands are the the most intuitive way to interact with a virtual clickable cockpit in VR. A regular VR controller is a good way to click buttons, flick switches or rotate a knob. But when it comes to the flight controls the use of physical joystick and throttle are vital for precise flying. But it's tidious to constantly switch your hands between the VR controllers and the physical joystick or throttle as you can't hold them at the same time. True hand tracking would be the solution, but unfortunatley this is not (yet) a reliable and for most of us also not an obtainable solution. 

hotas-vr-controller is a OpenVR driver that allows to modify the position and the rotation of your regular VR controllers. This makes it possible to strap the regular VR controllers to the back of your hands or your lower arm, while the virtual hands still appear in the correct position and orientation. hotas-vr-controller captures mouse clicks and emulate the grip and the trigger inputs of the VR controllers. "Finger mouse" devices (small mouse device typically used for presentations) can be worn on the index fingers of both hands to trigger these actions.

This configuration enables a fluid transition of your hands between a physical joystick or throttle and the hand interactions with the virtual clickable cockpit. Key benefits:
* Tracking is based on regular VR controllers. It is reliable and the controllers are usually included with the VR headsets.
* Strapping material (e.g. old gloves and zipties) is cheap and readily available.
* "Finger mouse" devices are cheap and available world-wide with short delivery time.

# Supported flight simulations
* Tested with Digital Combat Simulator (DCS).

# Supported VR controllers
* In theory any OpenVR compatible tracked device associated with the left hand or right hand and with a grip and trigger input.
* Tested so far only with HP Reverb G2.

# Prerequisites
* SteamVR 1.16.8 or higher
* Interception driver https://github.com/oblitum/Interception. The installer is included.

# Installation
1. Download the latest release of the hotas-vr-controller driver
2. Unzip the archive in a folder of your choice
3. Install the Interception driver ...
4. Register the OpenVR driver ...
5. Start SteamVR

# Configuration
1. Modify the offsets and rotation in the driver_hotas.ini until the virtual hand position and rotation matches the actual hands
2. This can be done while SteamVR is running, the offsets are updated every 10 sec.

