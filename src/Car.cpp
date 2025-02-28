#include "iglo.h"
#include "igloBatchRenderer.h"
#include "box2d.h"
#include "Constants.h"
#include "Car.h"

ig::Vector2 ConvertVec2(b2Vec2 v)
{
	return ig::Vector2(v.x, v.y);
}

b2Vec2 ConvertVec2(ig::Vector2 v)
{
	return b2Vec2(v.x, v.y);
}


bool Car::Load(b2World& world, const CarDesc& carDesc)
{
	Unload();

	if (carDesc.carType == CarType::TriangleJointCar)
	{
		// Create chassis (as a circle)
		b2BodyDef bodyDef;
		bodyDef.type = b2_dynamicBody;
		bodyDef.position = ConvertVec2(carDesc.chassis0Pos + Constants::carStartPos);
		b2CircleShape circleShape;
		circleShape.m_radius = carDesc.chassis0Radius;
		this->chassis = world.CreateBody(&bodyDef);
		this->chassis->CreateFixture(&circleShape, carDesc.chassisDensity);
		this->chassis->SetAngularVelocity(0);
		this->chassis->SetLinearVelocity(b2Vec2(0, 0));

		// Create wheel 0
		circleShape.m_radius = carDesc.wheel0Radius;
		b2FixtureDef fixtureDef;
		fixtureDef.shape = &circleShape;
		fixtureDef.density = carDesc.wheelDensity;
		fixtureDef.friction = carDesc.wheelFriction;
		bodyDef.position = ConvertVec2(carDesc.wheel0Pos + Constants::carStartPos);
		this->wheel0 = world.CreateBody(&bodyDef);
		this->wheel0->CreateFixture(&fixtureDef);
		this->wheel0->SetAngularVelocity(0);
		this->wheel0->SetLinearVelocity(b2Vec2(0, 0));

		// Create wheel 1
		circleShape.m_radius = carDesc.wheel1Radius;
		bodyDef.position = ConvertVec2(carDesc.wheel1Pos + Constants::carStartPos);
		this->wheel1 = world.CreateBody(&bodyDef);
		this->wheel1->CreateFixture(&fixtureDef);
		this->wheel1->SetAngularVelocity(0);
		this->wheel1->SetLinearVelocity(b2Vec2(0, 0));

		// Create joint between chassis and wheel 0
		b2RevoluteJointDef jd;
		jd.Initialize(this->chassis, this->wheel0, this->wheel0->GetPosition());
		joints.push_back(world.CreateJoint(&jd));

		// Create joint between chassis and wheel 1
		jd.Initialize(this->chassis, this->wheel1, this->wheel1->GetPosition());
		joints.push_back(world.CreateJoint(&jd));
	}
	else
	{
		// Create rigid chassi
		b2BodyDef bodyDef;
		bodyDef.type = b2_dynamicBody;
		bodyDef.position = ConvertVec2(Constants::carStartPos);
		chassis = world.CreateBody(&bodyDef);
		b2Vec2 vertices[4];
		vertices[0] = ConvertVec2(carDesc.wheel0Pos);
		vertices[1] = ConvertVec2(carDesc.chassis0Pos);
		vertices[2] = ConvertVec2(carDesc.chassis1Pos);
		vertices[3] = ConvertVec2(carDesc.wheel1Pos);
		b2PolygonShape polygon;
		polygon.Set(vertices, 4);
		chassis->CreateFixture(&polygon, carDesc.chassisDensity);

		// Create wheel 0
		b2CircleShape circle;
		circle.m_radius = carDesc.wheel0Radius;
		b2FixtureDef fixtureDef;
		fixtureDef.shape = &circle;
		fixtureDef.density = carDesc.wheelDensity;
		fixtureDef.friction = carDesc.wheelFriction;
		bodyDef.position = ConvertVec2(carDesc.wheel0Pos + Constants::carStartPos);
		wheel0 = world.CreateBody(&bodyDef);
		wheel0->CreateFixture(&fixtureDef);

		// Create wheel 1
		circle.m_radius = carDesc.wheel1Radius;
		fixtureDef.shape = &circle;
		bodyDef.position = ConvertVec2(carDesc.wheel1Pos + Constants::carStartPos);
		wheel1 = world.CreateBody(&bodyDef);
		wheel1->CreateFixture(&fixtureDef);

		// Create wheel joint between chassi and wheel 0
		b2WheelJointDef jd;
		b2Vec2 axis(0.0f, 1.0f);
		float mass = wheel0->GetMass();
		float hertz = carDesc.springFreq;
		float dampingRatio = carDesc.springDampingRatio;
		float omega = 2.0f * b2_pi * hertz;
		jd.Initialize(chassis, wheel0, wheel0->GetPosition(), axis);
		jd.stiffness = mass * omega * omega;
		jd.damping = 2.0f * mass * omega * dampingRatio;
		jd.lowerTranslation = -0.25f;
		jd.upperTranslation = 0.25f;
		jd.enableLimit = true;
		joints.push_back(world.CreateJoint(&jd));

		// Create wheel joint between chassi and wheel 1
		mass = wheel1->GetMass();
		jd.Initialize(chassis, wheel1, wheel1->GetPosition(), axis);
		jd.stiffness = mass * omega * omega;
		jd.damping = 2.0f * mass * dampingRatio * omega;
		jd.lowerTranslation = -0.25f;
		jd.upperTranslation = 0.25f;
		jd.enableLimit = true;
		joints.push_back(world.CreateJoint(&jd));
	}

	this->desc = carDesc;
	this->parentWorld = &world;
	this->isLoaded = true;
	return true;
}

void Car::Unload()
{
	// Destroy the joints
	for (size_t i = 0; i < joints.size(); i++)
	{
		if (joints[i] && parentWorld) parentWorld->DestroyJoint(joints[i]);
	}
	joints.clear();

	if (wheel0)
	{
		if (parentWorld) parentWorld->DestroyBody(wheel0);
		wheel0 = nullptr;
	}

	if (wheel1)
	{
		if (parentWorld) parentWorld->DestroyBody(wheel1);
		wheel1 = nullptr;
	}

	if (chassis)
	{
		if (parentWorld) parentWorld->DestroyBody(chassis);
		chassis = nullptr;
	}

	engine0 = EngineState::Idle;
	engine1 = EngineState::Idle;
	parentWorld = nullptr;
	isLoaded = false;
}

ig::Vector2 Car::GetWheel0Position(bool defaultPose) const
{
	if (!isLoaded) return ig::Vector2(0, 0);
	if (defaultPose) return desc.wheel0Pos + Constants::carStartPos;
	return ConvertVec2(wheel0->GetPosition());
}

ig::Vector2 Car::GetWheel1Position(bool defaultPose) const
{
	if (!isLoaded) return ig::Vector2(0, 0);
	if (defaultPose) return desc.wheel1Pos + Constants::carStartPos;
	return ConvertVec2(wheel1->GetPosition());
}

ig::Vector2 Car::GetChassisPosition(bool defaultPose) const
{
	if (!isLoaded) return ig::Vector2(0, 0);

	if (defaultPose)
	{
		if (desc.carType == CarType::TriangleJointCar)
		{
			return desc.chassis0Pos + Constants::carStartPos;
		}
		else if (desc.carType == CarType::FixtureBoxCar)
		{
			return Constants::carStartPos;
		}
		else return Constants::carStartPos;
	}

	return ConvertVec2(chassis->GetPosition());
}

float Car::GetChassisRotation() const
{
	if (!isLoaded) return 0;
	return chassis->GetAngle();
}

ig::Vector2 Car::GetPosition(bool defaultPose) const
{
	if (!isLoaded) return ig::Vector2(0, 0);

	return (GetChassisPosition(defaultPose) + GetWheel0Position(defaultPose) + GetWheel1Position(defaultPose)) / 3.0f;
}

void Car::UpdateWheel(b2Body* targetWheel, EngineState engineState)
{
	if (!isLoaded) return;
	if (!targetWheel) return;
	if (engineState == EngineState::Idle)
	{
		return;
	}
	else if (engineState == EngineState::DriveForward)
	{
		targetWheel->ApplyAngularImpulse(-desc.angularImpulse, true);
	}
	else if (engineState == EngineState::DriveBackward)
	{
		targetWheel->ApplyAngularImpulse(desc.angularImpulse, true);
	}
}

void Car::UpdateJointMotor(b2Joint* targetJoint, EngineState engineState)
{
	if (!isLoaded) return;
	if (!targetJoint) return;

	b2RevoluteJoint* rev = nullptr;
	b2WheelJoint* wj = nullptr;

	if (desc.carType == CarType::TriangleJointCar) rev = (b2RevoluteJoint*)targetJoint;
	if (desc.carType == CarType::FixtureBoxCar) wj = (b2WheelJoint*)targetJoint;

	if (rev) rev->SetMaxMotorTorque(desc.jointMotorTorque);
	if (wj) wj->SetMaxMotorTorque(desc.jointMotorTorque);

	if (engineState == EngineState::Idle)
	{
		if (rev) rev->EnableMotor(false);
		if (rev) rev->SetMotorSpeed(0);
		if (wj) wj->EnableMotor(false);
		if (wj) wj->SetMotorSpeed(0);
	}
	else if (engineState == EngineState::DriveForward)
	{
		if (rev) rev->EnableMotor(true);
		if (rev) rev->SetMotorSpeed(-desc.targetMotorSpeed);
		if (wj) wj->EnableMotor(true);
		if (wj) wj->SetMotorSpeed(-desc.targetMotorSpeed);
	}
	else if (engineState == EngineState::DriveBackward)
	{
		if (rev) rev->EnableMotor(true);
		if (rev) rev->SetMotorSpeed(desc.targetMotorSpeed);
		if (wj) wj->EnableMotor(true);
		if (wj) wj->SetMotorSpeed(desc.targetMotorSpeed);
	}
}

void Car::Update()
{
	if (!isLoaded) return;
	if (desc.engineType == EngineType::AngularImpulse)
	{
		UpdateWheel(wheel0, engine0);
		UpdateWheel(wheel1, engine1);
	}
	else if (desc.engineType == EngineType::JointMotor)
	{
		UpdateJointMotor(joints[0], engine0);
		UpdateJointMotor(joints[1], engine1);
	}
}

void Car::SetEngineStates(EngineState engine0, EngineState engine1)
{
	if (!isLoaded) return;
	this->engine0 = engine0;
	this->engine1 = engine1;
}

bool Car::IsColliding(const b2Body* otherBody) const
{
	if (!isLoaded) return false;

	// If car is invincible then it can't lose
	if (desc.isInvincible) return false;

	int32_t contantCount = parentWorld->GetContactCount();
	b2Contact* contact = parentWorld->GetContactList();
	for (int32_t j = 0; j < contantCount; j++)
	{
		if (contact->IsTouching())
		{
			b2Body* body1 = contact->GetFixtureA()->GetBody();
			b2Body* body2 = contact->GetFixtureB()->GetBody();
			if (body1 == chassis ||
				body2 == chassis)
			{
				if (body1 == otherBody ||
					body2 == otherBody)
				{
					// Collision!
					return true;
				}
			}
		}
		contact = contact->GetNext();
	}
	return false;
}

bool Car::HasFlippedOver() const
{
	if (!isLoaded) return false;
	if (desc.isInvincible) return false;

	if (chassis->GetAngle() > float(IGLO_PI / 2) ||
		chassis->GetAngle() < -float(IGLO_PI / 2))
	{
		return true;
	}
	return false;
}

void Car::Draw(ig::BatchRenderer& r, ig::FloatRect cameraRect, bool showDefaultPose, float cameraZoom, bool darkMode)
{
	// Get chassi polygon positions
	ig::Vector2 chassiPos = GetChassisPosition(showDefaultPose);
	float rot = chassis->GetAngle();
	if (showDefaultPose) rot = 0;
	ig::Vector2 p0 = chassiPos + desc.wheel0Pos.GetRotated(rot);
	ig::Vector2 p1 = chassiPos + desc.chassis0Pos.GetRotated(rot);
	ig::Vector2 p2 = chassiPos + desc.chassis1Pos.GetRotated(rot);
	ig::Vector2 p3 = chassiPos + desc.wheel1Pos.GetRotated(rot);

	//---- Draw connections ----//
	if (desc.carType == CarType::TriangleJointCar)
	{
		ig::Color32 lineColor = Constants::connectionColor;
		if (darkMode) lineColor = Constants::connectionColorDarkMode;

		// Between both wheels
		r.DrawLine(
			GetWheel0Position(showDefaultPose),
			GetWheel1Position(showDefaultPose), lineColor, lineColor);

		// Between chassi circle and wheel0
		r.DrawLine(
			GetWheel0Position(showDefaultPose),
			GetChassisPosition(showDefaultPose), lineColor, lineColor);

		// Between chassi circle and wheel1
		r.DrawLine(
			GetWheel1Position(showDefaultPose),
			GetChassisPosition(showDefaultPose), lineColor, lineColor);
	}
	else
	{
		// Chassis (rigid polygon)
		float thickness = Constants::rigidConnectionThickness;
		r.DrawTriangle(p1, p2, p3, Constants::rigidSolidColor);
		r.DrawTriangle(p0, p1, p3, Constants::rigidSolidColor);
		r.DrawTriangle(p0, p1, p2, Constants::rigidSolidColor);
		r.DrawTriangle(p2, p3, p0, Constants::rigidSolidColor);
		r.DrawRectangularLine(p0.x, p0.y, p1.x, p1.y, thickness, Constants::rigidConnectionColor);
		r.DrawRectangularLine(p1.x, p1.y, p2.x, p2.y, thickness, Constants::rigidConnectionColor);
		r.DrawRectangularLine(p2.x, p2.y, p3.x, p3.y, thickness, Constants::rigidConnectionColor);
		r.DrawRectangularLine(p3.x, p3.y, p0.x, p0.y, thickness, Constants::rigidConnectionColor);
	}

	//---- Draw wheels ----//
	{
		const float circleSmoothness = 2.0f / cameraZoom;
		const float circleBorderThickness = 1.0f / cameraZoom;
		const float circleBorderSmoothness = 1.0f / cameraZoom;

		// Wheel0
		float radius = desc.wheel0Radius;
		ig::Vector2 pos = GetWheel0Position(showDefaultPose);
		rot = wheel0->GetAngle();
		ig::Vector2 anglePoint = pos + ig::Vector2((cosf(rot)) * radius, sinf(rot) * radius);
		ig::Color32 wheelColor = Constants::wheelColor;
		ig::Color32 angleColor = Constants::angleColor;
		if (desc.carType == CarType::FixtureBoxCar)
		{
			wheelColor = Constants::rigidWheelColor;
			angleColor = Constants::rigidAngleColor;
		}
		r.DrawCircle(pos.x, pos.y, radius, 0.0f, wheelColor, wheelColor, ig::Colors::Transparent, circleSmoothness);
		if (desc.carType == CarType::FixtureBoxCar)
		{
			r.DrawCircle(pos.x, pos.y, radius, circleBorderThickness, ig::Colors::Transparent, ig::Colors::Transparent,
				Constants::rigidWheelBorderColor, circleBorderSmoothness);
		}
		r.DrawLine(pos, anglePoint, angleColor, angleColor);

		// Wheel1
		radius = desc.wheel1Radius;
		pos = GetWheel1Position(showDefaultPose);
		rot = wheel1->GetAngle();
		anglePoint = pos + ig::Vector2((cosf(rot)) * radius, sinf(rot) * radius);
		r.DrawCircle(pos.x, pos.y, radius, 0.0f, wheelColor, wheelColor, ig::Colors::Transparent, circleSmoothness);
		if (desc.carType == CarType::FixtureBoxCar)
		{
			r.DrawCircle(pos.x, pos.y, radius, circleBorderThickness, ig::Colors::Transparent, ig::Colors::Transparent,
				Constants::rigidWheelBorderColor, circleBorderSmoothness);
		}
		r.DrawLine(pos, anglePoint, angleColor, angleColor);
	}

	//---- Draw chassi ----//
	if (desc.carType == CarType::TriangleJointCar)
	{
		const float circleSmoothness = 2.0f / cameraZoom;

		// Chassis (as a circle)
		float radius = desc.chassis0Radius;
		ig::Vector2 pos = GetChassisPosition(showDefaultPose);
		rot = chassis->GetAngle();
		r.DrawCircle(pos.x, pos.y, radius, 0.0f, Constants::chassiCircleColor, Constants::chassiCircleColor,
			ig::Colors::Transparent, circleSmoothness);
		r.DrawLine(pos, pos + ig::Vector2((cosf(rot)) * radius, sinf(rot) * radius),
			Constants::angleColor, Constants::angleColor);
	}
	else
	{
		const float circleSmoothness = 1.0f / cameraZoom;

		// Chassis (rigid polygon)
		float radius = Constants::rigidConnectionThickness / 2.0f;
		r.DrawCircle(p1.x, p1.y, radius, 0.0f, Constants::rigidConnectionColor, Constants::rigidConnectionColor,
			ig::Colors::Transparent, circleSmoothness);
		r.DrawCircle(p2.x, p2.y, radius, 0.0f, Constants::rigidConnectionColor, Constants::rigidConnectionColor,
			ig::Colors::Transparent, circleSmoothness);

	}
}

