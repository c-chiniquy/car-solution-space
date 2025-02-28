#include "iglo.h"
#include "igloBatchRenderer.h"

#define INI_STRNICMP( s1, s2, cnt ) ( _strnicmp( s1, s2, cnt ) )
#define INI_IMPLEMENTATION
#include "ini.h"

#include "box2d.h"

#include "Terrain.h"
#include "Car.h"
#include "GeneticAlgorithm.h"
#include "GeneInterpreter.h"
#include "Settings.h"


void Settings::Reset()
{
	// [Terrain]
	terrain.seed = 1;
	terrain.type = TerrainType::Normal;
	terrain.numChunks = 200;
	terrain.friction = 1.0f;
	terrain.minChunkWidth = 40.0f;
	terrain.maxChunkWidth = 60.0f;
	terrain.minChunkHeight = -40.0f;
	terrain.maxChunkHeight = 40.0f;
	terrain.firstChunkWidth = 256.0f;
	terrain.firstChunkHeight = 0.0f;
	terrain.scale = 11.0f; // Larger value = smaller terrain

	// [Box2D]
	timeStep = 60.0f;
	velocitySteps = 6;
	positionSteps = 2;
	gravity = 80.0f;

	// [GeneticAlgorithm]
	GA.seed = 0;
	GA.popSize = 50;
	GA.eliteCount = 5;
	GA.mutationProb = 0.25f;
	GA.mutationStrength = 0.125f;
	GA.crossoverProb = 0.75f;
	numGenerations = 100;

	// [Car]
	car.carType = CarType::TriangleJointCar;
	car.engineType = EngineType::AngularImpulse;
	car.isInvincible = false;
	car.chassis0Pos.x = 2.0f;
	car.chassis0Pos.y = 1.6f;
	car.chassis0Radius = 0.7f;
	car.chassis1Pos.x = 4.0f;
	car.chassis1Pos.y = 2.0f;
	car.wheel0Pos.x = 0.0f;
	car.wheel0Pos.y = 0.0f;
	car.wheel0Radius = 0.8f;
	car.wheel1Pos.x = 6.0f;
	car.wheel1Pos.y = 0.0f;
	car.wheel1Radius = 0.8f;
	car.wheelFriction = 20.0f;

	// Increasing the torque too high will cause the car to flip backwards.
	// But having the torque too low will make the car too slow and boring.
	// 1.28 car mass = 20 torque (from Box2D car sample)
	// 5.5 car mass = 86 torque
	car.jointMotorTorque = 86.0f;

	// The motor will try to reach this speed.
	// A large value here will cause motor to always spin to try to reach higher speeds.
	car.targetMotorSpeed = 1000.0f;

	car.angularImpulse = 20.0f;
	car.chassisDensity = 1.0f;
	car.wheelDensity = 1.0f;
	car.springFreq = 4.0f;
	car.springDampingRatio = 0.7f;

	// [Simulation]
	numFramesPerSimulation = 700;
	powerFrontWheel = false;
	powerBackWheel = true;
	fixedPhysicsFramerate = 60;
	maxPhysicsFramesInARow = 2;
	numTerrains = 1000;
	enableMultiTerrainSim = false;

	// [GeneInterpreter]
	geneInterpreter.geneMask = 0;
	geneInterpreter.fitnessTextureSize = 128;
	geneInterpreter.minWheelFriction = 0.0f;
	geneInterpreter.maxWheelFriction = 20.0f;
	geneInterpreter.minWheelRadius = 0.4f;
	geneInterpreter.maxWheelRadius = 1.8f;
	geneInterpreter.minChassisRadius = 0.4f;
	geneInterpreter.maxChassisRadius = 1.7f;
	geneInterpreter.minChassisX = 0;
	geneInterpreter.maxChassisX = 6.0f;
	geneInterpreter.minChassisY = 0;
	geneInterpreter.maxChassisY = 4.0f;
	geneInterpreter.minWheelX = 0;
	geneInterpreter.maxWheelX = 6.0f;
	geneInterpreter.minWheelY = 0;
	geneInterpreter.maxWheelY = 4.0f;
	geneInterpreter.minChassisDensity = 0.0f;
	geneInterpreter.maxChassisDensity = 5.0f;
	geneInterpreter.minWheelDensity = 0.0f;
	geneInterpreter.maxWheelDensity = 5.0f;
	geneInterpreter.minJointMotorTorque = 0.0f;
	geneInterpreter.maxJointMotorTorque = 200.0f;
	geneInterpreter.minAngularImpulse = 0.0f;
	geneInterpreter.maxAngularImpulse = 40.0f;
	geneInterpreter.minSpringFreq = 2.0f;
	geneInterpreter.maxSpringFreq = 8.0f;
	geneInterpreter.minSpringDampingRatio = 0.4f;
	geneInterpreter.maxSpringDampingRatio = 1.5f;
	geneInterpreter.minGravity = 8.0f;
	geneInterpreter.maxGravity = 800.0f;

	// [Graphics]
	presentMode = (uint32_t)ig::PresentMode::Immediate;
	frameRateLimit = 120;
	cameraScale = 11.0f; // Bigger number = everything looks bigger
	clearColor = ig::Color32(0xFFF0DACD);

}

std::string PropStr(bool value)
{
	if (value) return "true";
	return "false";
}
std::string PropStr(uint32_t value)
{
	return std::to_string(value);
}
std::string PropStr(uint64_t value)
{
	return std::to_string(value);
}
std::string PropStr(double value)
{
	std::stringstream ss;
	ss << value;
	return ss.str();
}
std::string PropStr(float value)
{
	std::stringstream ss;
	ss << value;
	return ss.str();
}
std::string PropStr(uint16_t value)
{
	return std::to_string(value);
}
std::string PropStr(uint8_t value)
{
	return std::to_string(value);
}
std::string PropStr(std::string value)
{
	return value;
}

void AddProperty(const std::string& value, const char* name, ini_t* ini, int section)
{
	ini_property_add(ini, section, name, 0, value.c_str(), 0);
}

bool Settings::SaveToFile(const std::string& filename)
{
	ini_t* ini = ini_create(NULL);

	// [Terrain]
	int SECTION = ini_section_add(ini, "Terrain", 0);
	AddProperty(PropStr(terrain.seed), "seed", ini, SECTION);
	AddProperty(PropStr((uint32_t)terrain.type), "type", ini, SECTION);
	AddProperty(PropStr(terrain.numChunks), "numChunks", ini, SECTION);
	AddProperty(PropStr(terrain.friction), "friction", ini, SECTION);
	AddProperty(PropStr(terrain.minChunkWidth), "minChunkWidth", ini, SECTION);
	AddProperty(PropStr(terrain.maxChunkWidth), "maxChunkWidth", ini, SECTION);
	AddProperty(PropStr(terrain.minChunkHeight), "minChunkHeight", ini, SECTION);
	AddProperty(PropStr(terrain.maxChunkHeight), "maxChunkHeight", ini, SECTION);
	AddProperty(PropStr(terrain.firstChunkWidth), "firstChunkWidth", ini, SECTION);
	AddProperty(PropStr(terrain.firstChunkHeight), "firstChunkHeight", ini, SECTION);
	AddProperty(PropStr(terrain.scale), "scale", ini, SECTION);

	// [Box2D]
	SECTION = ini_section_add(ini, "Box2D", 0);
	AddProperty(PropStr(timeStep), "timeStep", ini, SECTION);
	AddProperty(PropStr(velocitySteps), "velocitySteps", ini, SECTION);
	AddProperty(PropStr(positionSteps), "positionSteps", ini, SECTION);
	AddProperty(PropStr(gravity), "gravity", ini, SECTION);

	// [GeneticAlgorithm]
	SECTION = ini_section_add(ini, "GeneticAlgorithm", 0);
	AddProperty(PropStr(GA.seed), "seed", ini, SECTION);
	AddProperty(PropStr(GA.popSize), "popSize", ini, SECTION);
	AddProperty(PropStr(GA.eliteCount), "eliteCount", ini, SECTION);
	AddProperty(PropStr(GA.mutationProb), "mutationProb", ini, SECTION);
	AddProperty(PropStr(GA.mutationStrength), "mutationStrength", ini, SECTION);
	AddProperty(PropStr(GA.crossoverProb), "crossoverProb", ini, SECTION);
	AddProperty(PropStr(numGenerations), "numGenerations", ini, SECTION);

	// [Car]
	SECTION = ini_section_add(ini, "Car", 0);
	AddProperty(PropStr((uint32_t)car.carType), "carType", ini, SECTION);
	AddProperty(PropStr((uint32_t)car.engineType), "engineType", ini, SECTION);
	AddProperty(PropStr(car.isInvincible), "isInvincible", ini, SECTION);
	AddProperty(PropStr(car.chassis0Pos.x), "chassis0Pos.x", ini, SECTION);
	AddProperty(PropStr(car.chassis0Pos.y), "chassis0Pos.y", ini, SECTION);
	AddProperty(PropStr(car.chassis0Radius), "chassis0Radius", ini, SECTION);
	AddProperty(PropStr(car.chassis1Pos.x), "chassis1Pos.x", ini, SECTION);
	AddProperty(PropStr(car.chassis1Pos.y), "chassis1Pos.y", ini, SECTION);
	AddProperty(PropStr(car.wheel0Pos.x), "wheel0Pos.x", ini, SECTION);
	AddProperty(PropStr(car.wheel0Pos.y), "wheel0Pos.y", ini, SECTION);
	AddProperty(PropStr(car.wheel0Radius), "wheel0Radius", ini, SECTION);
	AddProperty(PropStr(car.wheel1Pos.x), "wheel1Pos.x", ini, SECTION);
	AddProperty(PropStr(car.wheel1Pos.y), "wheel1Pos.y", ini, SECTION);
	AddProperty(PropStr(car.wheel1Radius), "wheel1Radius", ini, SECTION);
	AddProperty(PropStr(car.wheelFriction), "wheelFriction", ini, SECTION);
	AddProperty(PropStr(car.angularImpulse), "angularImpulse", ini, SECTION);
	AddProperty(PropStr(car.jointMotorTorque), "jointMotorTorque", ini, SECTION);
	AddProperty(PropStr(car.targetMotorSpeed), "targetMotorSpeed", ini, SECTION);
	AddProperty(PropStr(car.chassisDensity), "chassisDensity", ini, SECTION);
	AddProperty(PropStr(car.wheelDensity), "wheelDensity", ini, SECTION);
	AddProperty(PropStr(car.springFreq), "springFreq", ini, SECTION);
	AddProperty(PropStr(car.springDampingRatio), "springDampingRatio", ini, SECTION);

	// [Simulation]
	SECTION = ini_section_add(ini, "Simulation", 0);
	AddProperty(PropStr(numFramesPerSimulation), "numFramesPerSimulation", ini, SECTION);
	AddProperty(PropStr(powerFrontWheel), "powerFrontWheel", ini, SECTION);
	AddProperty(PropStr(powerBackWheel), "powerBackWheel", ini, SECTION);
	AddProperty(PropStr(fixedPhysicsFramerate), "fixedPhysicsFramerate", ini, SECTION);
	AddProperty(PropStr(maxPhysicsFramesInARow), "maxPhysicsFramesInARow", ini, SECTION);
	AddProperty(PropStr(numTerrains), "numTerrains", ini, SECTION);
	AddProperty(PropStr(enableMultiTerrainSim), "enableMultiTerrainSim", ini, SECTION);

	// [GeneInterpreter]
	SECTION = ini_section_add(ini, "GeneInterpreter", 0);
	AddProperty(PropStr(geneInterpreter.geneMask), "geneMask", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.fitnessTextureSize), "fitnessTextureSize", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.minWheelFriction), "minWheelFriction", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.maxWheelFriction), "maxWheelFriction", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.minWheelRadius), "minWheelRadius", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.maxWheelRadius), "maxWheelRadius", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.minChassisRadius), "minChassisRadius", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.maxChassisRadius), "maxChassisRadius", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.minChassisX), "minChassisX", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.maxChassisX), "maxChassisX", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.minChassisY), "minChassisY", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.maxChassisY), "maxChassisY", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.minWheelX), "minWheelX", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.maxWheelX), "maxWheelX", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.minWheelY), "minWheelY", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.maxWheelY), "maxWheelY", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.minChassisDensity), "minChassisDensity", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.maxChassisDensity), "maxChassisDensity", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.minWheelDensity), "minWheelDensity", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.maxWheelDensity), "maxWheelDensity", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.minJointMotorTorque), "minJointMotorTorque", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.maxJointMotorTorque), "maxJointMotorTorque", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.minAngularImpulse), "minAngularImpulse", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.maxAngularImpulse), "maxAngularImpulse", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.minSpringFreq), "minSpringFreq", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.maxSpringFreq), "maxSpringFreq", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.minSpringDampingRatio), "minSpringDampingRatio", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.maxSpringDampingRatio), "maxSpringDampingRatio", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.minGravity), "minGravity", ini, SECTION);
	AddProperty(PropStr(geneInterpreter.maxGravity), "maxGravity", ini, SECTION);

	// [Graphics]
	SECTION = ini_section_add(ini, "Graphics", 0);
	AddProperty(PropStr(presentMode), "presentMode", ini, SECTION);
	AddProperty(PropStr(frameRateLimit), "frameRateLimit", ini, SECTION);
	AddProperty(PropStr(cameraScale), "cameraScale", ini, SECTION);
	AddProperty(PropStr(clearColor.rgba), "clearColor", ini, SECTION);

	// Save the data
	int size = ini_save(ini, NULL, 0);
	std::string out;
	out.resize(size, 0);
	size = ini_save(ini, out.data(), size);
	ini_destroy(ini);

	return ig::WriteFile(filename, (byte*)out.data(), (size_t)size - 1);
}

float ParseFloat(const char* nameOfVariableToRead, const ini_t* ini, int section)
{
	int index = ini_find_property(ini, section, nameOfVariableToRead, 0);
	if (index == -1) return 0;
	char const* str = ini_property_value(ini, section, index);
	if (std::string(str).empty()) return 0;
	return (float)std::atof(str);
}

uint32_t ParseUInt32(const char* nameOfVariableToRead, const ini_t* ini, int section)
{
	int index = ini_find_property(ini, section, nameOfVariableToRead, 0);
	if (index == -1) return 0;
	char const* str = ini_property_value(ini, section, index);
	if (std::string(str).empty()) return 0;
	return (uint32_t)std::stoull(str);
}

uint64_t ParseUInt64(const char* nameOfVariableToRead, const ini_t* ini, int section)
{
	int index = ini_find_property(ini, section, nameOfVariableToRead, 0);
	if (index == -1) return 0;
	char const* str = ini_property_value(ini, section, index);
	if (std::string(str).empty()) return 0;
	return (uint64_t)std::stoull(str);
}

bool ParseBool(const char* nameOfVariableToRead, const ini_t* ini, int section)
{
	int index = ini_find_property(ini, section, nameOfVariableToRead, 0);
	if (index == -1) return false;
	std::string str = std::string(ini_property_value(ini, section, index));
	if (std::string(str).empty()) return 0;
	std::string lowercase = ig::ToLowercase(str);
	if (lowercase == "true" ||
		lowercase == "1" ||
		lowercase == "yes" ||
		lowercase == "enabled")
	{
		return true;
	}
	return false;
}

bool Settings::LoadFromFile(const std::string& filename)
{
	ig::ReadFileResult file = ig::ReadFile(filename);

	if (!file.success)
	{
		return false; // Failed to load file
	}
	else
	{
		Reset();

		ini_t* ini = ini_load((char*)file.fileContent.data(), 0);

		// [Terrain]
		int SECTION = ini_find_section(ini, "Terrain", 0);
		if (SECTION != -1)
		{
			terrain.seed = ParseUInt32("seed", ini, SECTION);
			terrain.type = (TerrainType)ParseUInt32("type", ini, SECTION);
			terrain.numChunks = ParseUInt32("numChunks", ini, SECTION);
			terrain.friction = ParseFloat("friction", ini, SECTION);
			terrain.minChunkWidth = ParseFloat("minChunkWidth", ini, SECTION);
			terrain.maxChunkWidth = ParseFloat("maxChunkWidth", ini, SECTION);
			terrain.minChunkHeight = ParseFloat("minChunkHeight", ini, SECTION);
			terrain.maxChunkHeight = ParseFloat("maxChunkHeight", ini, SECTION);
			terrain.firstChunkWidth = ParseFloat("firstChunkWidth", ini, SECTION);
			terrain.firstChunkHeight = ParseFloat("firstChunkHeight", ini, SECTION);
			terrain.scale = ParseFloat("scale", ini, SECTION);
		}

		// [Box2D]
		SECTION = ini_find_section(ini, "Box2D", 0);
		if (SECTION != -1)
		{
			timeStep = ParseFloat("timeStep", ini, SECTION);
			velocitySteps = ParseUInt32("velocitySteps", ini, SECTION);
			positionSteps = ParseUInt32("positionSteps", ini, SECTION);
			gravity = ParseFloat("gravity", ini, SECTION);
		}

		// [GeneticAlgorithm]
		SECTION = ini_find_section(ini, "GeneticAlgorithm", 0);
		if (SECTION != -1)
		{
			GA.seed = ParseUInt32("seed", ini, SECTION);
			GA.popSize = ParseUInt32("popSize", ini, SECTION);
			GA.eliteCount = ParseUInt32("eliteCount", ini, SECTION);
			GA.mutationProb = ParseFloat("mutationProb", ini, SECTION);
			GA.mutationStrength = ParseFloat("mutationStrength", ini, SECTION);
			GA.crossoverProb = ParseFloat("crossoverProb", ini, SECTION);
			numGenerations = ParseUInt32("numGenerations", ini, SECTION);
		}

		// [Car]
		SECTION = ini_find_section(ini, "Car", 0);
		if (SECTION != -1)
		{
			car.carType = (CarType)ParseUInt32("carType", ini, SECTION);
			car.engineType = (EngineType)ParseUInt32("engineType", ini, SECTION);
			car.isInvincible = ParseBool("isInvincible", ini, SECTION);
			car.chassis0Pos.x = ParseFloat("chassis0Pos.x", ini, SECTION);
			car.chassis0Pos.y = ParseFloat("chassis0Pos.y", ini, SECTION);
			car.chassis0Radius = ParseFloat("chassis0Radius", ini, SECTION);
			car.chassis1Pos.x = ParseFloat("chassis1Pos.x", ini, SECTION);
			car.chassis1Pos.y = ParseFloat("chassis1Pos.y", ini, SECTION);
			car.wheel0Pos.x = ParseFloat("wheel0Pos.x", ini, SECTION);
			car.wheel0Pos.y = ParseFloat("wheel0Pos.y", ini, SECTION);
			car.wheel0Radius = ParseFloat("wheel0Radius", ini, SECTION);
			car.wheel1Pos.x = ParseFloat("wheel1Pos.x", ini, SECTION);
			car.wheel1Pos.y = ParseFloat("wheel1Pos.y", ini, SECTION);
			car.wheel1Radius = ParseFloat("wheel1Radius", ini, SECTION);
			car.wheelFriction = ParseFloat("wheelFriction", ini, SECTION);
			car.angularImpulse = ParseFloat("angularImpulse", ini, SECTION);
			car.jointMotorTorque = ParseFloat("jointMotorTorque", ini, SECTION);
			car.targetMotorSpeed = ParseFloat("targetMotorSpeed", ini, SECTION);
			car.chassisDensity = ParseFloat("chassisDensity", ini, SECTION);
			car.wheelDensity = ParseFloat("wheelDensity", ini, SECTION);
			car.springFreq = ParseFloat("springFreq", ini, SECTION);
			car.springDampingRatio = ParseFloat("springDampingRatio", ini, SECTION);
		}

		// [Simulation]
		SECTION = ini_find_section(ini, "Simulation", 0);
		if (SECTION != -1)
		{
			numFramesPerSimulation = ParseUInt32("numFramesPerSimulation", ini, SECTION);
			powerFrontWheel = ParseBool("powerFrontWheel", ini, SECTION);
			powerBackWheel = ParseBool("powerBackWheel", ini, SECTION);
			fixedPhysicsFramerate = ParseUInt32("fixedPhysicsFramerate", ini, SECTION);
			maxPhysicsFramesInARow = ParseUInt32("maxPhysicsFramesInARow", ini, SECTION);
			numTerrains = ParseUInt32("numTerrains", ini, SECTION);
			enableMultiTerrainSim = ParseBool("enableMultiTerrainSim", ini, SECTION);
		}

		// [GeneInterpreter]
		SECTION = ini_find_section(ini, "GeneInterpreter", 0);
		if (SECTION != -1)
		{
			geneInterpreter.geneMask = ParseUInt64("geneMask", ini, SECTION);
			geneInterpreter.fitnessTextureSize = ParseUInt32("fitnessTextureSize", ini, SECTION);
			geneInterpreter.minWheelFriction = ParseFloat("minWheelFriction", ini, SECTION);
			geneInterpreter.maxWheelFriction = ParseFloat("maxWheelFriction", ini, SECTION);
			geneInterpreter.minWheelRadius = ParseFloat("minWheelRadius", ini, SECTION);
			geneInterpreter.maxWheelRadius = ParseFloat("maxWheelRadius", ini, SECTION);
			geneInterpreter.minChassisRadius = ParseFloat("minChassisRadius", ini, SECTION);
			geneInterpreter.maxChassisRadius = ParseFloat("maxChassisRadius", ini, SECTION);
			geneInterpreter.minChassisX = ParseFloat("minChassisX", ini, SECTION);
			geneInterpreter.maxChassisX = ParseFloat("maxChassisX", ini, SECTION);
			geneInterpreter.minChassisY = ParseFloat("minChassisY", ini, SECTION);
			geneInterpreter.maxChassisY = ParseFloat("maxChassisY", ini, SECTION);
			geneInterpreter.minWheelX = ParseFloat("minWheelX", ini, SECTION);
			geneInterpreter.maxWheelX = ParseFloat("maxWheelX", ini, SECTION);
			geneInterpreter.minWheelY = ParseFloat("minWheelY", ini, SECTION);
			geneInterpreter.maxWheelY = ParseFloat("maxWheelY", ini, SECTION);
			geneInterpreter.minChassisDensity = ParseFloat("minChassisDensity", ini, SECTION);
			geneInterpreter.maxChassisDensity = ParseFloat("maxChassisDensity", ini, SECTION);
			geneInterpreter.minWheelDensity = ParseFloat("minWheelDensity", ini, SECTION);
			geneInterpreter.maxWheelDensity = ParseFloat("maxWheelDensity", ini, SECTION);
			geneInterpreter.minJointMotorTorque = ParseFloat("minJointMotorTorque", ini, SECTION);
			geneInterpreter.maxJointMotorTorque = ParseFloat("maxJointMotorTorque", ini, SECTION);
			geneInterpreter.minAngularImpulse = ParseFloat("minAngularImpulse", ini, SECTION);
			geneInterpreter.maxAngularImpulse = ParseFloat("maxAngularImpulse", ini, SECTION);
			geneInterpreter.minSpringFreq = ParseFloat("minSpringFreq", ini, SECTION);
			geneInterpreter.maxSpringFreq = ParseFloat("maxSpringFreq", ini, SECTION);
			geneInterpreter.minSpringDampingRatio = ParseFloat("minSpringDampingRatio", ini, SECTION);
			geneInterpreter.maxSpringDampingRatio = ParseFloat("maxSpringDampingRatio", ini, SECTION);
			geneInterpreter.minGravity = ParseFloat("minGravity", ini, SECTION);
			geneInterpreter.maxGravity = ParseFloat("maxGravity", ini, SECTION);
		}

		// [Graphics]
		SECTION = ini_find_section(ini, "Graphics", 0);
		if (SECTION != -1)
		{
			presentMode = ParseUInt32("presentMode", ini, SECTION);
			frameRateLimit = ParseUInt32("frameRateLimit", ini, SECTION);
			cameraScale = ParseFloat("cameraScale", ini, SECTION);
			clearColor = ig::Color32(ParseUInt32("clearColor", ini, SECTION));
		}

		ini_destroy(ini);
		return true;
	}
}
