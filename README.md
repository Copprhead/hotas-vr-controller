# hotas-vr-controller
An OpenVR driver that allows the hands-free use of your regular VR controllers, so that you can keep your Hands On Throttle And Stick (HOTAS). 


# Dependencies
* Use AutoHotkey + AutoHotkeyInterceptor to bind the left and right mouse click of the finger mice to a keyboard button. Use the "subscription" mode and register to the mouse click events. Use different keys for mouse click down and up. Within the event method the keyboard key needs to be pressed for down, sleep for 50-100ms and then can be released (up). This is necessary so that the key press is detected within the OpenVR driver.
* Turn physical controller off until DCS is running. When DCS is running, turn on the left controller and wait until recongition finished and the left virtual controller is moving. Then turn on the right controller. If both controllers are turned on at the same time they might overrule the virtual controller.
* Disable all bindings of the physical controller in SteamVR. Use the "Manage Controller Bindings" option in SteamVR. DCS needs to be running so that the "empty" binding can be assigned to DCS and is from then on automatically loaded. This avoids the physical controller to overrule the virtual controller (which can happen when you accidently press a button on the physical controller).
