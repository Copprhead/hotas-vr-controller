#include "ServerTrackedDeviceProvider.h"
#include "Logging.h"
#include "InterfaceHookInjector.h"
#include "inipp.h"
#include "spsc_queue.h"

#if defined( _WINDOWS )
#include <windows.h>
#endif

bool gIsExiting = false;
spsc_queue<int> gQueue;

void InterceptionThreadFunction()
{
	InterceptionContext context;
	InterceptionDevice device;
	InterceptionStroke stroke;

	context = interception_create_context();
	interception_set_filter(context, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_LEFT_BUTTON_DOWN | INTERCEPTION_FILTER_MOUSE_LEFT_BUTTON_UP | INTERCEPTION_FILTER_MOUSE_RIGHT_BUTTON_DOWN | INTERCEPTION_FILTER_MOUSE_RIGHT_BUTTON_UP);

	while (!gIsExiting)
	{
		if (interception_receive(context, device = interception_wait(context), &stroke, 1) > 0)
		{
			if (interception_is_mouse(device))
			{				
				//TRACE("Device: %i", device - INTERCEPTION_MOUSE(0));

				//wchar_t hardware_id[500];
				//size_t length = interception_get_hardware_id(context, device, hardware_id, sizeof(hardware_id));

				//if (length > 0 && length < sizeof(hardware_id))
				//{
				//	TRACE("hardware_id found: %S", hardware_id);
				//}

				InterceptionMouseStroke& mousestroke = *(InterceptionMouseStroke*)&stroke;
				if (device - INTERCEPTION_MOUSE(0) == 1)
				{
					if (mousestroke.state == INTERCEPTION_FILTER_MOUSE_LEFT_BUTTON_DOWN)
					{
						gQueue.enqueue(LEFT_GRIP_DOWN);
					}
					else if (mousestroke.state == INTERCEPTION_FILTER_MOUSE_LEFT_BUTTON_UP)
					{
						gQueue.enqueue(LEFT_GRIP_UP);
					}
					else if (mousestroke.state == INTERCEPTION_FILTER_MOUSE_RIGHT_BUTTON_DOWN)
					{
						gQueue.enqueue(LEFT_TRIGGER_DOWN);
					}					
					else if (mousestroke.state == INTERCEPTION_FILTER_MOUSE_RIGHT_BUTTON_UP)
					{
						gQueue.enqueue(LEFT_TRIGGER_UP);
					}
				}
				else if (device - INTERCEPTION_MOUSE(0) == 2)
				{
					if (mousestroke.state == INTERCEPTION_FILTER_MOUSE_LEFT_BUTTON_DOWN)
					{
						gQueue.enqueue(RIGHT_TRIGGER_DOWN);
					}
					else if (mousestroke.state == INTERCEPTION_FILTER_MOUSE_LEFT_BUTTON_UP)
					{
						gQueue.enqueue(RIGHT_TRIGGER_UP);
					}
					else if (mousestroke.state == INTERCEPTION_FILTER_MOUSE_RIGHT_BUTTON_DOWN)
					{
						gQueue.enqueue(RIGHT_GRIP_DOWN);
					}
					else if (mousestroke.state == INTERCEPTION_FILTER_MOUSE_RIGHT_BUTTON_UP)
					{
						gQueue.enqueue(RIGHT_GRIP_UP);
					}
				}
				else
				{
					interception_send(context, device, &stroke, 1);
				}
			}
			else
			{
				interception_send(context, device, &stroke, 1);
			}
		}
	}

	interception_destroy_context(context);
}



vr::EVRInitError ServerTrackedDeviceProvider::Init(vr::IVRDriverContext *pDriverContext)
{	
	TRACE("ServerTrackedDeviceProvider::Init()");
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);

	memset(transforms, 0, vr::k_unMaxTrackedDeviceCount * sizeof DeviceTransform);

	InjectHooks(this, pDriverContext);

	auto driverInput = vr::VRDriverInput();

	gIsExiting = false;
	interceptionThread = new std::thread( InterceptionThreadFunction );
	
	return vr::VRInitError_None;
}

void ServerTrackedDeviceProvider::Cleanup()
{
	TRACE("ServerTrackedDeviceProvider::Cleanup()");	

	gIsExiting = true;

	DisableHooks();
	VR_CLEANUP_SERVER_DRIVER_CONTEXT();
}

void ServerTrackedDeviceProvider::checkLeft()
{
	triggerLeft = false;
	gripLeft = false;

	//if (((0x8000 & GetAsyncKeyState(0x5A)) != 0)) // Z
	//{
	//	gripLeft = true;
	//}
	//if (((0x8000 & GetAsyncKeyState(0x55)) != 0)) // U
	//{
	//	triggerLeft = true;
	//	gripLeft = true;
	//}

}

void ServerTrackedDeviceProvider::checkRight()
{
	triggerRight = false;
	gripRight = false;

	//if (((0x8000 & GetAsyncKeyState(0x49)) != 0)) // I
	//{
	//	gripRight = true;
	//	triggerRight = true;
	//}
	//if (((0x8000 & GetAsyncKeyState(0x4F)) != 0)) // O
	//{
	//	gripRight = true;
	//}

}


void ServerTrackedDeviceProvider::RunFrame()
{
	//checkLeft();
	//checkRight();

	int state = -1;
	if (gQueue.dequeue(state))
	{
		if (state == LEFT_GRIP_DOWN)
		{
			gripLeft = true;
			TRACE("LEFT_GRIP_DOWN");
		}
		else if (state == LEFT_GRIP_UP)
		{
			gripLeft = false; 
			TRACE("LEFT_GRIP_UP");
		}
		else if (state == LEFT_TRIGGER_DOWN)
		{
			triggerLeft = true;
			gripLeft = true;
			TRACE("LEFT_TRIGGER_DOWN");
		}
		else if (state == LEFT_TRIGGER_UP)
		{
			triggerLeft = false;
			gripLeft = false;
			TRACE("LEFT_TRIGGER_UP");
		}


		else if (state == RIGHT_GRIP_DOWN)
		{
			gripRight = true;
			TRACE("RIGHT_GRIP_DOWN");
		}
		else if (state == RIGHT_GRIP_UP)
		{
			gripRight = false;
			TRACE("RIGHT_GRIP_UP");
		}
		else if (state == RIGHT_TRIGGER_DOWN)
		{
			triggerRight = true;
			gripRight = true;
			TRACE("RIGHT_TRIGGER_DOWN");
		}
		else if (state == RIGHT_TRIGGER_UP)
		{
			triggerRight = false;
			gripRight = false;
			TRACE("RIGHT_TRIGGER_UP");
		}
	}
		
	if (handleTriggerLeft != nullptr && handleGripLeft != nullptr)
	{
		if (triggerLeft)
		{
			vr::VRDriverInput()->UpdateScalarComponent(*handleTriggerLeft, (float)1.00, 0);
		}
		else
		{
			vr::VRDriverInput()->UpdateScalarComponent(*handleTriggerLeft, (float)0.0, 0);
		}

		if (gripLeft)
		{
			vr::VRDriverInput()->UpdateScalarComponent(*handleGripLeft, (float)1.0, 0);
		}
		else
		{
			vr::VRDriverInput()->UpdateScalarComponent(*handleGripLeft, (float)0.0, 0);
		}
	}
	
	if (handleTriggerRight != nullptr && handleGripRight != nullptr)
	{	
		if (triggerRight)
		{
			vr::VRDriverInput()->UpdateScalarComponent(*handleTriggerRight, (float)1.0, 0);
		}
		else
		{
			vr::VRDriverInput()->UpdateScalarComponent(*handleTriggerRight, (float)0.0, 0);
		}

		if (gripRight)
		{
			vr::VRDriverInput()->UpdateScalarComponent(*handleGripRight, (float)1.0, 0);
		}
		else
		{
			vr::VRDriverInput()->UpdateScalarComponent(*handleGripRight, (float)0.0, 0);
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

vr::HmdQuaternion_t create_from_axis_angle(const double& xx, const double& yy, const double& zz, const double& a)
{
	vr::HmdQuaternion_t result;

	// Here we calculate the sin( theta / 2) once for optimization
	double factor = sin(a / 2.0);

	// Calculate the x, y and z of the quaternion
	double x = xx * factor;
	double y = yy * factor;
	double z = zz * factor;

	// Calcualte the w value by cos( theta / 2 )
	double w = cos(a / 2.0);

	result.w = w;
	result.x = x;
	result.y = y;
	result.z = z;

	return result;
}

vr::HmdQuaternion_t QuatMultiply(const vr::HmdQuaternion_t* q1, const vr::HmdQuaternion_t* q2)
{
	vr::HmdQuaternion_t result;
	result.x = q1->w * q2->x + q1->x * q2->w + q1->y * q2->z - q1->z * q2->y;
	result.y = q1->w * q2->y - q1->x * q2->z + q1->y * q2->w + q1->z * q2->x;
	result.z = q1->w * q2->z + q1->x * q2->y - q1->y * q2->x + q1->z * q2->w;
	result.w = q1->w * q2->w - q1->x * q2->x - q1->y * q2->y - q1->z * q2->z;
	return result;
}

bool ServerTrackedDeviceProvider::HandleDevicePoseUpdated(uint32_t openVRID, vr::DriverPose_t &pose)
{	
	auto props = vr::VRProperties()->TrackedDeviceToPropertyContainer(openVRID);
	std::int32_t role = vr::VRProperties()->GetInt32Property(props, vr::ETrackedDeviceProperty::Prop_ControllerRoleHint_Int32);

	auto rotation = pose.qRotation;

	double baseOffset[3];
	baseOffset[0] = 0;
	baseOffset[1] = 0;
	baseOffset[2] = 0;

	double r_x = 0;
	double r_y = 0;
	double r_z = 0;
	
	bool hasOffset = false;
	bool hasRotation = false;

	if (role == vr::ETrackedControllerRole::TrackedControllerRole_LeftHand)
	{
		baseOffset[0] = (float)-0.10;
		baseOffset[1] = (float)-0.075;
		baseOffset[2] = (float)0.05;

		r_x = -0.25;
		r_y = 0.5;
		r_z = -0.9;

		hasOffset = true;
		hasRotation = true;
	}
	if (role == vr::ETrackedControllerRole::TrackedControllerRole_RightHand)
	{
		baseOffset[0] = (float)-0.10;
		baseOffset[1] = (float)-0.075;
		baseOffset[2] = (float)0.075;

		r_x = -0.25;
		r_y = 0.5;
		r_z = -0.9;

		hasOffset = true;
		hasRotation = true;
	}	

	if (hasOffset == true)
	{
		auto offset = quaternionRotateVector(rotation, baseOffset);

		pose.vecPosition[0] = pose.vecPosition[0] + offset.v[0];
		pose.vecPosition[1] = pose.vecPosition[1] + offset.v[1];
		pose.vecPosition[2] = pose.vecPosition[2] + offset.v[2];
	}

	if (hasRotation == true)
	{
		auto rotate_x = create_from_axis_angle(1, 0, 0, r_x * 3.13 / 2);
		auto rotate_y = create_from_axis_angle(0, 1, 0, r_y * 3.13 / 2);		
		auto rotate_z = create_from_axis_angle(0, 0, 1, r_z * 3.13 / 2);

		rotation = QuatMultiply(&rotation, &rotate_x);
		rotation = QuatMultiply(&rotation, &rotate_y);
		rotation = QuatMultiply(&rotation, &rotate_z);

		pose.qRotation = rotation;
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

	std::int32_t role = vr::VRProperties()->GetInt32Property(ulContainer, vr::ETrackedDeviceProperty::Prop_ControllerRoleHint_Int32);
	if (role == vr::ETrackedControllerRole::TrackedControllerRole_LeftHand)
	{
		std::string componentName(pchName);
		if (componentName == "/input/grip/value")
		{
			TRACE("ServerTrackedDeviceProvider::handleGripLeft assigned");
			handleGripLeft = pHandle;
		}
		else if (componentName == "/input/trigger/value")
		{
			TRACE("ServerTrackedDeviceProvider::handleTriggerLeft assigned");
			handleTriggerLeft = pHandle;
		}
	}
	else if (role == vr::ETrackedControllerRole::TrackedControllerRole_RightHand)
	{
		std::string componentName(pchName);
		if (componentName == "/input/grip/value")
		{
			TRACE("ServerTrackedDeviceProvider::handleGripRight assigned");
			handleGripRight = pHandle;
		}
		else if (componentName == "/input/trigger/value")
		{
			TRACE("ServerTrackedDeviceProvider::handleTriggerRight assigned");
			handleTriggerRight = pHandle;
		}
	}
}
