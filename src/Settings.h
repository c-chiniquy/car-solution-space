#pragma once


struct Settings
{
public:
	Settings()
	{
		Reset();
	}
	~Settings() {}

	void Reset();

	bool SaveToFile(const std::string& filename);
	bool LoadFromFile(const std::string& filename);

	// [Terrain]
	TerrainDesc terrain;

	// [Box2D]
	float timeStep;
	uint32_t velocitySteps;
	uint32_t positionSteps;
	float gravity;

	// [GeneticAlgorithm]
	GeneticAlgorithmDesc GA;
	uint32_t numGenerations; // The number of generations to simulate before terminating

	// [Car]
	CarDesc car;

	// [Simulation]
	uint32_t numFramesPerSimulation; // The number of physics frames to simulate before moving on to the next car
	bool powerFrontWheel;
	bool powerBackWheel;
	uint32_t fixedPhysicsFramerate = 0;
	uint32_t maxPhysicsFramesInARow = 0;
	uint32_t numTerrains = 0; // The simulation can avarage the result of many different terrains
	bool enableMultiTerrainSim = false; // Simulate each car on multiple terrains and avarage the scores

	// [GeneInterpreter]
	GeneInterpreter geneInterpreter;

	// [Graphics]
	uint32_t presentMode = 0;
	uint32_t frameRateLimit = 0;
	float cameraScale;
	ig::Color32 clearColor = 0;

};
