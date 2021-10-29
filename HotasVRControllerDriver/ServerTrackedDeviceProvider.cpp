#include "ServerTrackedDeviceProvider.h"
#include "Logging.h"
#include "InterfaceHookInjector.h"
#include "inipp.h"
#include "spsc_queue.h"
#include <fstream>
#define _USE_MATH_DEFINES
#include <math.h>

#if defined( _WINDOWS )
#include <windows.h>
#endif

bool gIsExiting = false;
spsc_queue<ControllerState> gQueue;

unsigned long gFrames = 0;

unsigned int leftDeviceIndex = 0;
unsigned int rightDeviceIndex = 0;
std::wstring leftDeviceHardware = L"";
std::wstring rightDeviceHardware = L"";

unsigned int wheelDuration = 20;
float wheelExtension = (float)0.5;
float wheelProgression = (float)0.1;

std::map<InterceptionDevice, DeviceType> gDeviceMapping;


template<typename K, typename V>
bool findByValue(std::vector<K>& vec, std::map<K, V> mapOfElemen, V value)
{
	bool bResult = false;
	auto it = mapOfElemen.begin();
	// Iterate through the map
	while (it != mapOfElemen.end())
	{
		// Check if value of this entry matches with given value
		if (it->second == value)
		{
			// Yes found
			bResult = true;
			// Push the key in given map
			vec.push_back(it->first);
		}
		// Go to next entry in map
		it++;
	}
	return bResult;
}

void InterceptionThreadFunction()
{
	InterceptionContext context;
	InterceptionDevice device;
	InterceptionStroke stroke;

	context = interception_create_context();
	interception_set_filter(context, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_LEFT_BUTTON_DOWN | INTERCEPTION_FILTER_MOUSE_LEFT_BUTTON_UP | INTERCEPTION_FILTER_MOUSE_RIGHT_BUTTON_DOWN | INTERCEPTION_FILTER_MOUSE_RIGHT_BUTTON_UP | INTERCEPTION_FILTER_MOUSE_WHEEL );

	while (!gIsExiting)
	{
		if (interception_receive(context, device = interception_wait(context), &stroke, 1) > 0)
		{
			if (interception_is_mouse(device))
			{	
				// Check if the current device was already detected.
				if (gDeviceMapping.count(device) == 0)
				{
					// Check hardware id.
					wchar_t hardware_id[500] = { 0 };
					size_t length = interception_get_hardware_id(context, device, hardware_id, sizeof(hardware_id));
					if (length > 0 && length < sizeof(hardware_id))
					{
						if (wcscmp(hardware_id, leftDeviceHardware.c_str()) != 0 && wcscmp(hardware_id, rightDeviceHardware.c_str()) != 0)
						{
							// If device is not mapped, write the hardware id to the log file
							LOG("Unmapped device: %i HardwareId: %S", device, hardware_id);
						}
						else if ((wcscmp(leftDeviceHardware.c_str(), rightDeviceHardware.c_str()) == 0) && (wcscmp(hardware_id, leftDeviceHardware.c_str()) == 0 || wcscmp(hardware_id, rightDeviceHardware.c_str()) == 0))
						{
							// same device type for both sides. compare device index
							if (leftDeviceIndex < rightDeviceIndex)
							{
								// left device comes first.
								std::vector<InterceptionDevice> vec;
								bool result = findByValue(vec, gDeviceMapping, DeviceType::LEFT_DEVICE);
								if (result == true)
								{
									// There is already a left device. So this must be the right device.
									gDeviceMapping[device] = DeviceType::RIGHT_DEVICE;								
								}
								else
								{
									// There is no left device yet. So this must be the left device.
									gDeviceMapping[device] = DeviceType::LEFT_DEVICE;
								}
							}
							else
							{
								// right device comes first.							
								std::vector<InterceptionDevice> vec;
								bool result = findByValue(vec, gDeviceMapping, DeviceType::RIGHT_DEVICE);
								if (result == true)
								{
									// There is already a right device. So this must be the left device.
									gDeviceMapping[device] = DeviceType::LEFT_DEVICE;
								}
								else
								{
									// There is no right device yet. So this must be the left device.
									gDeviceMapping[device] = DeviceType::RIGHT_DEVICE;
								}
							}
						}
						else if (wcscmp(hardware_id, leftDeviceHardware.c_str()) == 0)
						{
							gDeviceMapping[device] = DeviceType::LEFT_DEVICE;
						}
						else if (wcscmp(hardware_id, rightDeviceHardware.c_str()) == 0)
						{
							gDeviceMapping[device] = DeviceType::RIGHT_DEVICE;
						}
						else
						{
							// if device is not mapped to left or right controller, add it to ignore list
							gDeviceMapping[device] = DeviceType::IGNORE_DEVICE;
						}
					}
					else
					{
						// if there is no hardware_id, add device to ignore list
						gDeviceMapping[device] = DeviceType::IGNORE_DEVICE;
					}
				}
				
				InterceptionMouseStroke& mousestroke = *(InterceptionMouseStroke*)&stroke;

				DeviceType type = gDeviceMapping[device];
				if(type == DeviceType::LEFT_DEVICE)
				{
					if (mousestroke.state == INTERCEPTION_FILTER_MOUSE_LEFT_BUTTON_DOWN)
					{
						gQueue.enqueue(ControllerState::LEFT_TRIGGER_DOWN);
					}
					else if (mousestroke.state == INTERCEPTION_FILTER_MOUSE_LEFT_BUTTON_UP)
					{
						gQueue.enqueue(ControllerState::LEFT_TRIGGER_UP);
					}
					else if (mousestroke.state == INTERCEPTION_FILTER_MOUSE_RIGHT_BUTTON_DOWN)
					{
						gQueue.enqueue(ControllerState::LEFT_GRIP_DOWN);
						
					}					
					else if (mousestroke.state == INTERCEPTION_FILTER_MOUSE_RIGHT_BUTTON_UP)
					{
						gQueue.enqueue(ControllerState::LEFT_GRIP_UP);
						
					}
					else if (mousestroke.state == INTERCEPTION_FILTER_MOUSE_WHEEL)
					{
						if (mousestroke.rolling > 0)
						{
							gQueue.enqueue(ControllerState::LEFT_WHEEL_UP);
						}
						else if (mousestroke.rolling < 0)
						{
							gQueue.enqueue(ControllerState::LEFT_WHEEL_DOWN);
						}
					}
				}
				else if (type == DeviceType::RIGHT_DEVICE)
				{
					if (mousestroke.state == INTERCEPTION_FILTER_MOUSE_LEFT_BUTTON_DOWN)
					{
						gQueue.enqueue(ControllerState::RIGHT_GRIP_DOWN);
					}
					else if (mousestroke.state == INTERCEPTION_FILTER_MOUSE_LEFT_BUTTON_UP)
					{
						gQueue.enqueue(ControllerState::RIGHT_GRIP_UP);
					}
					else if (mousestroke.state == INTERCEPTION_FILTER_MOUSE_RIGHT_BUTTON_DOWN)
					{
						gQueue.enqueue(ControllerState::RIGHT_TRIGGER_DOWN);
						
					}
					else if (mousestroke.state == INTERCEPTION_FILTER_MOUSE_RIGHT_BUTTON_UP)
					{
						gQueue.enqueue(ControllerState::RIGHT_TRIGGER_UP);
						
					}
					else if (mousestroke.state == INTERCEPTION_FILTER_MOUSE_WHEEL)
					{
						if (mousestroke.rolling > 0)
						{
							gQueue.enqueue(ControllerState::RIGHT_WHEEL_UP);
						}
						else if(mousestroke.rolling < 0)
						{ 
							gQueue.enqueue(ControllerState::RIGHT_WHEEL_DOWN);
						}
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

void ServerTrackedDeviceProvider::loadConfig(std::wstring configPath)
{
	inipp::Ini<char> ini;
	std::ifstream is(configPath);
	ini.parse(is);

	inipp::get_value(ini.sections["LeftControllerOffset"], "x", leftControllerOffset.x);
	inipp::get_value(ini.sections["LeftControllerOffset"], "y", leftControllerOffset.y);
	inipp::get_value(ini.sections["LeftControllerOffset"], "z", leftControllerOffset.z);
	inipp::get_value(ini.sections["LeftControllerOffset"], "rx", leftControllerOffset.rx);
	inipp::get_value(ini.sections["LeftControllerOffset"], "ry", leftControllerOffset.ry);
	inipp::get_value(ini.sections["LeftControllerOffset"], "rz", leftControllerOffset.rz);

	inipp::get_value(ini.sections["RightControllerOffset"], "x", rightControllerOffset.x);
	inipp::get_value(ini.sections["RightControllerOffset"], "y", rightControllerOffset.y);
	inipp::get_value(ini.sections["RightControllerOffset"], "z", rightControllerOffset.z);
	inipp::get_value(ini.sections["RightControllerOffset"], "rx", rightControllerOffset.rx);
	inipp::get_value(ini.sections["RightControllerOffset"], "ry", rightControllerOffset.ry);
	inipp::get_value(ini.sections["RightControllerOffset"], "rz", rightControllerOffset.rz);

	inipp::get_value(ini.sections["Settings"], "wheelDuration", wheelDuration);
	inipp::get_value(ini.sections["Settings"], "wheelExtension", wheelExtension);
	inipp::get_value(ini.sections["Settings"], "wheelProgression", wheelProgression);
}

vr::EVRInitError ServerTrackedDeviceProvider::Init(vr::IVRDriverContext *pDriverContext)
{	
	TRACE("ServerTrackedDeviceProvider::Init()");

	// get dll path
	wchar_t path[MAX_PATH] = { 0 };
	HMODULE hm = NULL;
	if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&InterceptionThreadFunction, &hm) == 0)
	{
		int ret = GetLastError();
		LOG("GetModuleHandle failed, error = %d\n", ret);
		// Return or however you want to handle an error.
		return vr::VRInitError_Init_NoConfigPath;
	}
	if (GetModuleFileName(hm, path, sizeof(path)) == 0)
	{
		int ret = GetLastError();
		LOG("GetModuleFileName failed, error = %d\n", ret);
		// Return or however you want to handle an error.
		return vr::VRInitError_Init_NoConfigPath;
	}
		
	TRACE("Path: %S", path);

	std::wstring filename(path);
	std::wstring directory;
	const size_t last_slash_idx = filename.rfind('\\');
	if (std::string::npos != last_slash_idx)
	{
		directory = filename.substr(0, last_slash_idx);
	}

	configPath = directory + std::wstring(L"\\driver_hotas.ini");

	// Load device hardware and index
	inipp::Ini<char> ini;
	std::ifstream is(configPath);
	ini.parse(is);

	std::string leftTemp = "";
	std::string rightTemp = "";

	inipp::get_value(ini.sections["LeftDevice"], "index", leftDeviceIndex);
	inipp::get_value(ini.sections["LeftDevice"], "hardware", leftTemp);
	inipp::get_value(ini.sections["RightDevice"], "index", rightDeviceIndex);	
	inipp::get_value(ini.sections["RightDevice"], "hardware", rightTemp);

	// it's safe as the ini file only contains ascii
	leftDeviceHardware = std::wstring(leftTemp.begin(), leftTemp.end());
	rightDeviceHardware = std::wstring(rightTemp.begin(), rightTemp.end());

	// Load controller offsets once during init. The config updated periodically to allow offset changes during runtime.
	loadConfig(configPath);
	
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
	
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

void ServerTrackedDeviceProvider::RunFrame()
{		
	++gFrames;
	if (gFrames >= 600) 
	{
		// Load the config every 600 frames, which is about every 10 sec (in 60 HZ mode)
		gFrames = 0;
		loadConfig(configPath);
	}
	
	ControllerState state;
	if (gQueue.dequeue(state))
	{
		if (state == ControllerState::LEFT_GRIP_DOWN)
		{
			gripLeft = true;
			JoystickYLeft = -1.0;
		}
		else if (state == ControllerState::LEFT_GRIP_UP)
		{
			gripLeft = false;
			JoystickYLeft = 0.0;
		}
		else if (state == ControllerState::LEFT_TRIGGER_DOWN)
		{
			//triggerLeft = true;
			gripLeft = true;
			JoystickYLeft = 1.0;
		}
		else if (state == ControllerState::LEFT_TRIGGER_UP)
		{
			//triggerLeft = false;
			gripLeft = false;
			JoystickYLeft = 0.0;
		}		
		else if (state == ControllerState::LEFT_WHEEL_DOWN)
		{
			gripLeft = true;
			if (JoystickXLeft == 0.0)
			{
				JoystickXLeft = 0.0 - wheelExtension;
			}
			else if(JoystickXLeft < 0.0)
			{
				JoystickXLeft -= wheelProgression;
			}

			if (JoystickXLeft < -1.0)
			{
				JoystickXLeft = -1.0;
			}

			leftWheelDownCounter = wheelDuration;
		}
		else if (state == ControllerState::LEFT_WHEEL_UP)
		{
			gripLeft = true;
			if (JoystickXLeft == 0.0)
			{
				JoystickXLeft = 0.0 + wheelExtension;
			}
			else if (JoystickXLeft > 0.0)
			{
				JoystickXLeft += wheelProgression;
			}

			if (JoystickXLeft > 1.0)
			{
				JoystickXLeft = 1.0;
			}

			leftWheelUpCounter = wheelDuration;
		}

		else if (state == ControllerState::RIGHT_GRIP_DOWN)
		{
			gripRight = true;
			JoystickYRight = -1.0;
		}
		else if (state == ControllerState::RIGHT_GRIP_UP)
		{
			gripRight = false;
			JoystickYRight = 0.0;
		}
		else if (state == ControllerState::RIGHT_TRIGGER_DOWN)
		{
			//triggerRight = true;
			gripRight = true;
			JoystickYRight = 1.0;
		}
		else if (state == ControllerState::RIGHT_TRIGGER_UP)
		{
			//triggerRight = false;
			gripRight = false;
			JoystickYRight = 0.0;
		}
		else if (state == ControllerState::RIGHT_WHEEL_DOWN)
		{
			gripRight = true;
			if (JoystickXRight == 0.0)
			{
				JoystickXRight = 0.0 - wheelExtension;
			}
			else if (JoystickXRight < 0.0)
			{
				JoystickXRight -= wheelProgression;
			}

			if (JoystickXRight < -1.0)
			{
				JoystickXRight = -1.0;
			}
			rightWheelDownCounter = wheelDuration;			
		}
		else if (state == ControllerState::RIGHT_WHEEL_UP)
		{
			gripRight = true;			
			if (JoystickXRight == 0.0)
			{
				JoystickXRight = 0.0 + wheelExtension;
			}
			else if (JoystickXRight > 0.0)
			{
				JoystickXRight += wheelProgression;
			}

			if (JoystickXRight > 1.0)
			{
				JoystickXRight = 1.0;
			}
			rightWheelUpCounter = wheelDuration;
		}
	}

	if (rightWheelUpCounter > 0 && rightWheelDownCounter == 0)
	{
		--rightWheelUpCounter;
		if (rightWheelUpCounter == 0)
		{
			// if counter is at 0, release the grip and reset joystick
			gripRight = false;
			JoystickXRight = 0.0;
		}
	}
	else if (rightWheelDownCounter > 0 && rightWheelUpCounter == 0)
	{
		--rightWheelDownCounter;		
		if (rightWheelDownCounter == 0)
		{
			// if counter is at 0, release the grip and reset joystick
			gripRight = false;
			JoystickXRight = 0.0;
		}
	}
	else
	{
		// this is also used to abort the scrolling rolling is revered.
		rightWheelDownCounter = 0;
		rightWheelUpCounter = 0;
		JoystickXRight = 0.0;		
	}


	if (leftWheelUpCounter > 0 && leftWheelDownCounter == 0)
	{
		--leftWheelUpCounter;
		if (leftWheelUpCounter == 0)
		{
			// if counter is at 0, release the grip and reset joystick
			gripLeft = false;
			JoystickXLeft = 0.0;
		}		
	}
	else if (leftWheelDownCounter > 0 && leftWheelUpCounter == 0)
	{
		--leftWheelDownCounter;
		if (leftWheelDownCounter == 0)
		{
			// if counter is at 0, release the grip and reset joystick
			gripLeft = false;
			JoystickXLeft = 0.0;
		}		
	}
	else
	{
		// this is also used to abort the scrolling rolling is revered.
		leftWheelDownCounter = 0;
		leftWheelUpCounter = 0;
		JoystickXLeft = 0.0;		
	}
		
	if (handleTriggerLeft != nullptr && handleGripLeft != nullptr && handleJoystickXLeft != nullptr && handleJoystickYLeft != nullptr)
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

		vr::VRDriverInput()->UpdateScalarComponent(*handleJoystickYLeft, (float)JoystickYLeft, 0);
		vr::VRDriverInput()->UpdateScalarComponent(*handleJoystickXLeft, (float)JoystickXLeft, 0);
	}
	
	if (handleTriggerRight != nullptr && handleGripRight != nullptr && handleJoystickXRight != nullptr && handleJoystickYRight != nullptr)
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

		vr::VRDriverInput()->UpdateScalarComponent(*handleJoystickYRight, (float)JoystickYRight, 0);
		vr::VRDriverInput()->UpdateScalarComponent(*handleJoystickXRight, (float)JoystickXRight, 0);
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

void to_axis_angle(vr::HmdQuaternion_t q, double& yaw , double& pitch, double& roll)
{
	yaw = atan2(2.0 * (q.y * q.z + q.w * q.x), q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z);
	pitch = asin(-2.0 * (q.x * q.z - q.w * q.y));
	roll = atan2(2.0 * (q.x * q.y + q.w * q.z), q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z);

	yaw = (yaw * 180.0) / M_PI;
	pitch = (pitch * 180.0) / M_PI;
	roll = (roll * 180.0) / M_PI;
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
	double rx = 0;
	double ry = 0;
	double rz = 0;
	
	bool hasOffset = false;
	bool hasRotation = false;

	if (role == vr::ETrackedControllerRole::TrackedControllerRole_LeftHand)
	{
		baseOffset[0] = leftControllerOffset.x;
		baseOffset[1] = leftControllerOffset.y;
		baseOffset[2] = leftControllerOffset.z;
		rx = leftControllerOffset.rx;
		ry = leftControllerOffset.ry;
		rz = leftControllerOffset.rz;

		hasOffset = true;
		hasRotation = true;
	}
	if (role == vr::ETrackedControllerRole::TrackedControllerRole_RightHand)
	{
		baseOffset[0] = rightControllerOffset.x;
		baseOffset[1] = rightControllerOffset.y;
		baseOffset[2] = rightControllerOffset.z;
		rx = rightControllerOffset.rx;
		ry = rightControllerOffset.ry;
		rz = rightControllerOffset.rz;

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
		auto rotate_x = create_from_axis_angle(1, 0, 0, (rx * M_PI) / 180);
		auto rotate_y = create_from_axis_angle(0, 1, 0, (ry * M_PI) / 180);
		auto rotate_z = create_from_axis_angle(0, 0, 1, (rz * M_PI) / 180);
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
		else if (componentName == "/input/joystick/x")
		{
			TRACE("ServerTrackedDeviceProvider::handleJoystickXLeft assigned");
			handleJoystickXLeft = pHandle;
		}
		else if (componentName == "/input/joystick/y")
		{
			TRACE("ServerTrackedDeviceProvider::handleJoystickYLeft assigned");
			handleJoystickYLeft = pHandle;
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
		else if (componentName == "/input/joystick/x")
		{
			TRACE("ServerTrackedDeviceProvider::handleJoystickXLeft assigned");
			handleJoystickXRight = pHandle;
		}
		else if (componentName == "/input/joystick/y")
		{
			TRACE("ServerTrackedDeviceProvider::handleJoystickYLeft assigned");
			handleJoystickYRight = pHandle;
		}
	}
}
