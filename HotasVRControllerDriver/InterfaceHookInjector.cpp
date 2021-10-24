#include "Logging.h"
#include "Hooking.h"
#include "InterfaceHookInjector.h"
#include "ServerTrackedDeviceProvider.h"

static ServerTrackedDeviceProvider *Driver = nullptr;

// Hooks for IVRDriverContext
static Hook<void*(*)(vr::IVRDriverContext *, const char *, vr::EVRInitError *)> 
	GetGenericInterfaceHook("IVRDriverContext::GetGenericInterface");

// Hooks for IVRServerDriverHost
static Hook<void(*)(vr::IVRServerDriverHost *, uint32_t, const vr::DriverPose_t &, uint32_t)>
	TrackedDevicePoseUpdatedHook005("IVRServerDriverHost005::TrackedDevicePoseUpdated");

static Hook<void(*)(vr::IVRServerDriverHost *, uint32_t, const vr::DriverPose_t &, uint32_t)>
	TrackedDevicePoseUpdatedHook006("IVRServerDriverHost006::TrackedDevicePoseUpdated");

static Hook<bool(*)(vr::IVRServerDriverHost*, const char*, vr::ETrackedDeviceClass, vr::ITrackedDeviceServerDriver*)>
	TrackedDeviceAddedHook005("IVRServerDriverHost005::TrackedDeviceAdded");

static Hook<bool(*)(vr::IVRServerDriverHost*, const char*, vr::ETrackedDeviceClass, vr::ITrackedDeviceServerDriver*)>
	TrackedDeviceAddedHook006("IVRServerDriverHost006::TrackedDeviceAdded");

// Hooks for IVRDriverInput
static Hook<vr::EVRInputError(*)(vr::IVRDriverInput*, vr::PropertyContainerHandle_t, const char*, vr::VRInputComponentHandle_t*)>
	CreateBooleanComponentHook("IVRDriverInput::CreateBooleanComponent");
static Hook<vr::EVRInputError(*)(vr::IVRDriverInput*, vr::PropertyContainerHandle_t, const char*, vr::VRInputComponentHandle_t*, vr::EVRScalarType, vr::EVRScalarUnits)>
	CreateScalarComponentHook("IVRDriverInput::CreateScalarComponent");


void HandleCreateScalarComponent();


static void DetourTrackedDevicePoseUpdated005(vr::IVRServerDriverHost *_this, uint32_t unWhichDevice, const vr::DriverPose_t &newPose, uint32_t unPoseStructSize)
{
	//TRACE("ServerTrackedDeviceProvider::DetourTrackedDevicePoseUpdated(%d)", unWhichDevice);
	auto pose = newPose;
	if (Driver->HandleDevicePoseUpdated(unWhichDevice, pose))
	{
		TrackedDevicePoseUpdatedHook005.originalFunc(_this, unWhichDevice, pose, unPoseStructSize);
	}
}

static void DetourTrackedDevicePoseUpdated006(vr::IVRServerDriverHost *_this, uint32_t unWhichDevice, const vr::DriverPose_t &newPose, uint32_t unPoseStructSize)
{
	//TRACE("ServerTrackedDeviceProvider::DetourTrackedDevicePoseUpdated(%d)", unWhichDevice);
	auto pose = newPose;
	if (Driver->HandleDevicePoseUpdated(unWhichDevice, pose))
	{
		TrackedDevicePoseUpdatedHook006.originalFunc(_this, unWhichDevice, pose, unPoseStructSize);
	}
}

static bool DetourTrackedDeviceAdded005(vr::IVRServerDriverHost* _this, const char* pchDeviceSerialNumber, vr::ETrackedDeviceClass eDeviceClass, vr::ITrackedDeviceServerDriver* pDriver)
{
	TRACE("ServerTrackedDeviceProvider::DetourTrackedDeviceAdded(%s)", pchDeviceSerialNumber);
	return TrackedDeviceAddedHook005.originalFunc(_this, pchDeviceSerialNumber, eDeviceClass, pDriver);
}

static bool DetourTrackedDeviceAdded006(vr::IVRServerDriverHost* _this, const char* pchDeviceSerialNumber, vr::ETrackedDeviceClass eDeviceClass, vr::ITrackedDeviceServerDriver* pDriver)
{
	TRACE("ServerTrackedDeviceProvider::DetourTrackedDeviceAdded(%s)", pchDeviceSerialNumber);
	return TrackedDeviceAddedHook006.originalFunc(_this, pchDeviceSerialNumber, eDeviceClass, pDriver);
}

static vr::EVRInputError DetourCreateBooleanComponent(vr::IVRDriverInput* _this, vr::PropertyContainerHandle_t ulContainer, const char* pchName, vr::VRInputComponentHandle_t* pHandle)
{
	TRACE("ServerTrackedDeviceProvider::DetourCreateBooleanComponent(%s)", pchName);

	vr::EVRInputError inputError = CreateBooleanComponentHook.originalFunc(_this, ulContainer, pchName, pHandle);

	Driver->HandleCreateBooleanComponent(ulContainer, pchName, pHandle);

	return inputError;
}

static vr::EVRInputError DetourCreateScalarComponent(vr::IVRDriverInput* _this, vr::PropertyContainerHandle_t ulContainer, const char* pchName, vr::VRInputComponentHandle_t* pHandle, vr::EVRScalarType eType, vr::EVRScalarUnits eUnits)
{
	TRACE("ServerTrackedDeviceProvider::DetourCreateScalarComponent(%s)", pchName);

	vr::EVRInputError inputError = CreateScalarComponentHook.originalFunc(_this, ulContainer, pchName, pHandle, eType, eUnits);

	Driver->HandleCreateScalarComponent(ulContainer, pchName, pHandle, eType, eUnits);

	return inputError;
}

static void* DetourGetGenericInterface(vr::IVRDriverContext* _this, const char* pchInterfaceVersion, vr::EVRInitError* peError)
{
	TRACE("ServerTrackedDeviceProvider::DetourGetGenericInterface(%s)", pchInterfaceVersion);
	auto originalInterface = GetGenericInterfaceHook.originalFunc(_this, pchInterfaceVersion, peError);

	std::string iface(pchInterfaceVersion);
	if (iface == "IVRServerDriverHost_005")
	{
		if (!IHook::Exists(TrackedDeviceAddedHook005.name))
		{
			TrackedDeviceAddedHook005.CreateHookInObjectVTable(originalInterface, 0, &DetourTrackedDeviceAdded005);
			IHook::Register(&TrackedDeviceAddedHook005);
		}

		if (!IHook::Exists(TrackedDevicePoseUpdatedHook005.name))
		{
			TrackedDevicePoseUpdatedHook005.CreateHookInObjectVTable(originalInterface, 1, &DetourTrackedDevicePoseUpdated005);
			IHook::Register(&TrackedDevicePoseUpdatedHook005);
		}
	}
	else if (iface == "IVRServerDriverHost_006")
	{
		if (!IHook::Exists(TrackedDeviceAddedHook006.name))
		{
			TrackedDeviceAddedHook006.CreateHookInObjectVTable(originalInterface, 0, &DetourTrackedDeviceAdded006);
			IHook::Register(&TrackedDeviceAddedHook006);
		}

		if (!IHook::Exists(TrackedDevicePoseUpdatedHook006.name))
		{
			TrackedDevicePoseUpdatedHook006.CreateHookInObjectVTable(originalInterface, 1, &DetourTrackedDevicePoseUpdated006);
			IHook::Register(&TrackedDevicePoseUpdatedHook006);
		}
	}
	else if (iface == "IVRDriverInput_003")
	{
		if (!IHook::Exists(CreateBooleanComponentHook.name))
		{
			CreateBooleanComponentHook.CreateHookInObjectVTable(originalInterface, 0, &DetourCreateBooleanComponent);
			IHook::Register(&CreateBooleanComponentHook);
		}

		if (!IHook::Exists(CreateScalarComponentHook.name))
		{
			CreateScalarComponentHook.CreateHookInObjectVTable(originalInterface, 2, &DetourCreateScalarComponent);
			IHook::Register(&CreateScalarComponentHook);
		}
	}

	return originalInterface;
}

void InjectHooks(ServerTrackedDeviceProvider* driver, vr::IVRDriverContext* pDriverContext)
{
	Driver = driver;

	auto err = MH_Initialize();
	if (err == MH_OK)
	{
		GetGenericInterfaceHook.CreateHookInObjectVTable(pDriverContext, 0, &DetourGetGenericInterface);
		IHook::Register(&GetGenericInterfaceHook);
	}
	else
	{
		LOG("MH_Initialize error: %s", MH_StatusToString(err));
	}
}


void DisableHooks()
{
	IHook::DestroyAll();
	MH_Uninitialize();
}