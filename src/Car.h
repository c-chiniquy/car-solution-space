#pragma once


enum class CarType : uint32_t
{
	// 2 wheels + 1 circle connected using joints.
	TriangleJointCar = 0,

	// This car uses a solid polygon chassis.
	// The shape of the chassis is determined by the position of the 2 wheels + 2 chassis points.
	FixtureBoxCar = 1,
};

// How the wheels should be powered
enum class EngineType : uint32_t
{
	// Powers the wheels by simply applying an angular impulse to the wheels each frame.
	// You can decide how strong the impulse should be.
	// This method is simple and can make a car really fast.
	AngularImpulse = 0,

	// Powers the wheels by using a joint motor. This method is more realistic.
	// You can specify the torque of the motor (how powerful it is).
	// Too much torque may cause the car to flip over in the opposite direction the wheels are spinning,
	// because of newton's laws saying for every force in nature there must be an equal and opposite force.
	JointMotor = 1,
};

enum class EngineState : uint32_t
{
	Idle = 0,
	DriveForward = 1,
	DriveBackward = 2,
};

struct CarDesc
{
	CarType carType = CarType::TriangleJointCar;
	EngineType engineType = EngineType::AngularImpulse;
	bool isInvincible = false;
	ig::Vector2 chassis0Pos;
	float chassis0Radius = 0;
	ig::Vector2 chassis1Pos;
	ig::Vector2 wheel0Pos;
	float wheel0Radius = 0;
	ig::Vector2 wheel1Pos;
	float wheel1Radius = 0;
	float jointMotorTorque = 0;
	float targetMotorSpeed = 0;
	float angularImpulse = 0;
	float wheelFriction = 0;
	float chassisDensity = 0;
	float wheelDensity = 0;
	float springFreq = 0;
	float springDampingRatio = 0;
};

class Car
{
public:
	Car() = default;
	~Car() { Unload(); };

	// Make class non-copyable
	Car(Car&) = delete;
	Car& operator=(Car&) = delete;

	// Returns true if success
	bool Load(b2World& world, const CarDesc& carDesc);
	void Unload();
	bool IsLoaded() const { return isLoaded; }

	ig::Vector2 GetWheel0Position(bool defaultPose) const;
	ig::Vector2 GetWheel1Position(bool defaultPose) const;
	ig::Vector2 GetChassisPosition(bool defaultPose) const;
	float GetChassisRotation() const; // In radians

	const CarDesc& GetDesc() const { return desc; }
	ig::Vector2 GetPosition(bool defaultPose) const;

	// Gives the car a chance to handle input
	void Update();
	void SetEngineStates(EngineState engine0, EngineState engine1);
	bool IsColliding(const b2Body* otherBody) const;
	bool HasFlippedOver() const;
	void Draw(ig::BatchRenderer& r, ig::FloatRect cameraRect, bool showDefaultPose, float cameraZoom, bool darkMode);

private:

	bool isLoaded = false;
	b2World* parentWorld = nullptr;

	CarDesc desc;
	EngineState engine0 = EngineState::Idle;
	EngineState engine1 = EngineState::Idle;

	b2Body* wheel0 = nullptr;
	b2Body* wheel1 = nullptr;
	b2Body* chassis = nullptr;
	std::vector<b2Joint*> joints;

	void UpdateWheel(b2Body* targetWheel, EngineState);
	void UpdateJointMotor(b2Joint* targetJoint, EngineState);
};