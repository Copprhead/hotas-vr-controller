#pragma once

#include <openvr_driver.h>

class ServerTrackedDeviceProvider : public vr::IServerTrackedDeviceProvider
{
public:
	////// Start vr::IServerTrackedDeviceProvider functions

	/** initializes the driver. This will be called before any other methods are called. */
	virtual vr::EVRInitError Init(vr::IVRDriverContext *pDriverContext) override;

	/** cleans up the driver right before it is unloaded */
	virtual void Cleanup() override;

	/** Returns the version of the ITrackedDeviceServerDriver interface used by this driver */
	virtual const char * const *GetInterfaceVersions() { return vr::k_InterfaceVersions; }

	/** Allows the driver do to some work in the main loop of the server. */
	virtual void RunFrame() override;

	/** Returns true if the driver wants to block Standby mode. */
	virtual bool ShouldBlockStandbyMode() { return false; }

	/** Called when the system is entering Standby mode. The driver should switch itself into whatever sort of low-power
	* state it has. */
	virtual void EnterStandby() { }

	/** Called when the system is leaving Standby mode. The driver should switch itself back to
	full operation. */
	virtual void LeaveStandby() { }

	////// End vr::IServerTrackedDeviceProvider functions

	ServerTrackedDeviceProvider() { }

	bool HandleDevicePoseUpdated(uint32_t openVRID, vr::DriverPose_t &pose);
	bool HandleTrackedDeviceAdded(const char* pchDeviceSerialNumber, vr::ETrackedDeviceClass eDeviceClass, vr::ITrackedDeviceServerDriver* pDriver);
	void HandleCreateBooleanComponent(vr::PropertyContainerHandle_t ulContainer, const char* pchName, vr::VRInputComponentHandle_t* pHandle);
	void HandleCreateScalarComponent(vr::PropertyContainerHandle_t ulContainer, const char* pchName, vr::VRInputComponentHandle_t* pHandle, vr::EVRScalarType eType, vr::EVRScalarUnits eUnits);

private:
	struct DeviceTransform
	{
		bool enabled = false;
		vr::HmdVector3d_t translation;
		vr::HmdQuaternion_t rotation;
	};

	DeviceTransform transforms[vr::k_unMaxTrackedDeviceCount];

	vr::VRInputComponentHandle_t* handleTriggerLeft = nullptr;
	vr::VRInputComponentHandle_t* handleGripLeft = nullptr;
	vr::VRInputComponentHandle_t* handleTriggerRight = nullptr;
	vr::VRInputComponentHandle_t* handleGripRight = nullptr;

	bool triggerLeft = false;
	bool gripLeft = false;
	bool triggerRight = false;
	bool gripRight = false;
};