#include "ServerTrackedDeviceProvider.h"
#include "Logging.h"
#include "InterfaceHookInjector.h"

#if defined( _WINDOWS )
#include <windows.h>
#endif


vr::EVRInitError ServerTrackedDeviceProvider::Init(vr::IVRDriverContext *pDriverContext)
{
	TRACE("ServerTrackedDeviceProvider::Init()");
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);

	memset(transforms, 0, vr::k_unMaxTrackedDeviceCount * sizeof DeviceTransform);

	InjectHooks(this, pDriverContext);


	auto driverInput = vr::VRDriverInput();
	
	return vr::VRInitError_None;
}

void ServerTrackedDeviceProvider::Cleanup()
{
	TRACE("ServerTrackedDeviceProvider::Cleanup()");
	DisableHooks();
	VR_CLEANUP_SERVER_DRIVER_CONTEXT();
}

void ServerTrackedDeviceProvider::RunFrame()
{
	if (handleTrigger != nullptr && handleGrip != nullptr)
	{
		if (((0x8000 & GetAsyncKeyState(0x4B)) != 0)) // K
		{
			m_rightGrip = true;
			m_rightTrigger = true;
		}
		if (((0x8000 & GetAsyncKeyState(0x49)) != 0)) // I
		{
			m_rightTrigger = false;
			m_rightGrip = false;
		}

		if (((0x8000 & GetAsyncKeyState(0x4C)) != 0)) // L
		{
			m_rightGrip = true;
		}
		if (((0x8000 & GetAsyncKeyState(0x4F)) != 0)) // O
		{
			m_rightGrip = false;
		}


		if (m_rightTrigger)
		{
			TRACE("ServerTrackedDeviceProvider::TriggerPressed");
			vr::VRDriverInput()->UpdateScalarComponent(*handleTrigger, (float)1.00, 0);
		}
		else
		{
			TRACE("ServerTrackedDeviceProvider::TriggerReleased");
			vr::VRDriverInput()->UpdateScalarComponent(*handleTrigger, 0.0, 0);
		}

		if (m_rightGrip)
		{
			TRACE("ServerTrackedDeviceProvider::GripPressed");
			vr::VRDriverInput()->UpdateScalarComponent(*handleGrip, (float)1.00, 0);
		}
		else
		{
			TRACE("ServerTrackedDeviceProvider::GripReleased");
			vr::VRDriverInput()->UpdateScalarComponent(*handleGrip, 0.0, 0);
		}
	}
}


inline vr::HmdQuaternion_t operator*(const vr::HmdQuaternion_t &lhs, const vr::HmdQuaternion_t &rhs) {
	return {
		(lhs.w * rhs.w) - (lhs.x * rhs.x) - (lhs.y * rhs.y) - (lhs.z * rhs.z),
		(lhs.w * rhs.x) + (lhs.x * rhs.w) + (lhs.y * rhs.z) - (lhs.z * rhs.y),
		(lhs.w * rhs.y) + (lhs.y * rhs.w) + (lhs.z * rhs.x) - (lhs.x * rhs.z),
		(lhs.w * rhs.z) + (lhs.z * rhs.w) + (lhs.x * rhs.y) - (lhs.y * rhs.x)
	};
}

inline vr::HmdVector3d_t quaternionRotateVector(const vr::HmdQuaternion_t& quat, const double(&vector)[3]) {
	vr::HmdQuaternion_t vectorQuat = { 0.0, vector[0], vector[1] , vector[2] };
	vr::HmdQuaternion_t conjugate = { quat.w, -quat.x, -quat.y, -quat.z };
	auto rotatedVectorQuat = quat * vectorQuat * conjugate;
	return { rotatedVectorQuat.x, rotatedVectorQuat.y, rotatedVectorQuat.z };
}

bool ServerTrackedDeviceProvider::HandleDevicePoseUpdated(uint32_t openVRID, vr::DriverPose_t &pose)
{	
	auto props = vr::VRProperties()->TrackedDeviceToPropertyContainer(openVRID);
	std::int32_t role = vr::VRProperties()->GetInt32Property(props, vr::ETrackedDeviceProperty::Prop_ControllerRoleHint_Int32);

	bool m_leftTrigger = false;
	bool m_leftGrip = false;
	bool m_rightTrigger = false;
	bool m_rightGrip = false;

	auto& tf = transforms[openVRID];
	if (role == vr::ETrackedControllerRole::TrackedControllerRole_LeftHand)
	{
		tf.enabled = true;
 
		tf.translation.v[0] = (float)-0.10;
		tf.translation.v[1] = (float)-0.075;
		tf.translation.v[2] = (float)0.05;
	}
	else if (role == vr::ETrackedControllerRole::TrackedControllerRole_RightHand)
	{
		tf.enabled = true;

		tf.translation.v[0] = (float)-0.10;
		tf.translation.v[1] = (float)-0.075;
		tf.translation.v[2] = (float)0.075;
	}

	if (tf.enabled)
	{
		// disable rotation until tf has an actual value for the rotation
		//pose.qWorldFromDriverRotation = tf.rotation * pose.qWorldFromDriverRotation;

		vr::HmdVector3d_t rotatedTranslation = quaternionRotateVector(tf.rotation, pose.vecWorldFromDriverTranslation);
		pose.vecWorldFromDriverTranslation[0] = rotatedTranslation.v[0] + tf.translation.v[0];
		pose.vecWorldFromDriverTranslation[1] = rotatedTranslation.v[1] + tf.translation.v[1];
		pose.vecWorldFromDriverTranslation[2] = rotatedTranslation.v[2] + tf.translation.v[2];
	}
	return true;
}

bool ServerTrackedDeviceProvider::HandleTrackedDeviceAdded(const char* pchDeviceSerialNumber, vr::ETrackedDeviceClass eDeviceClass, vr::ITrackedDeviceServerDriver* pDriver)
{
	TRACE("ServerTrackedDeviceProvider::HandleTrackedDeviceAdded(%s)", pchDeviceSerialNumber);

	return true;
}

void ServerTrackedDeviceProvider::HandleCreateBooleanComponent(vr::PropertyContainerHandle_t ulContainer, const char* pchName, vr::VRInputComponentHandle_t* pHandle)
{
	TRACE("ServerTrackedDeviceProvider::HandleCreateBooleanComponent(%s)", pchName);
}

void ServerTrackedDeviceProvider::HandleCreateScalarComponent(vr::PropertyContainerHandle_t ulContainer, const char* pchName, vr::VRInputComponentHandle_t* pHandle, vr::EVRScalarType eType, vr::EVRScalarUnits eUnits)
{
	TRACE("ServerTrackedDeviceProvider::HandleCreateScalarComponent(%s)", pchName);

	std::string componentName(pchName);
	if (componentName == "/input/grip/value")
	{
		TRACE("ServerTrackedDeviceProvider::handleGrip assigned");
		handleGrip = pHandle;
	}
	else if (componentName == "/input/trigger/value")
	{
		TRACE("ServerTrackedDeviceProvider::handleTrigger assigned");
		handleTrigger = pHandle;
	}
}
