#pragma once

#include <openvr_driver.h>

class ServerTrackedDeviceProvider;
/*
static void DetourTrackedDevicePoseUpdated(vr::IVRServerDriverHost * _this, uint32_t unWhichDevice, const vr::DriverPose_t & newPose, uint32_t unPoseStructSize);

static bool DetourTrackedDeviceAdded(const char* pchDeviceSerialNumber, vr::ETrackedDeviceClass eDeviceClass, vr::ITrackedDeviceServerDriver* pDriver);

static vr::EVRInputError DetourCreateBooleanComponent(vr::PropertyContainerHandle_t ulContainer, const char* pchName, vr::VRInputComponentHandle_t* pHandle);
*/

void InjectHooks(ServerTrackedDeviceProvider *driver, vr::IVRDriverContext *pDriverContext);

void DisableHooks();