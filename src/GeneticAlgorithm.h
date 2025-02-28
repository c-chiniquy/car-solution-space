#pragma once


// These are the available parameters that the simulation can try to optimize
enum class GeneID : uint32_t
{
	// The location of the chassis points (triangle car uses 1 chassis point, box car uses 2 chassis points)
	Chassis0_X = 0,
	Chassis0_Y = 1,
	Chassis0_Radius = 2, // Only used by the triangle car
	Chassis1_X = 3,
	Chassis1_Y = 4,

	// The location and radius of the two wheels
	Wheel0_X = 5,
	Wheel0_Y = 6,
	Wheel0_Radius = 7,
	Wheel1_X = 8,
	Wheel1_Y = 9,
	Wheel1_Radius = 10,

	AngularImpulsePower = 11, // Only used by appropriate engine type
	JointMotorTorque = 12, // Only used by appropriate engine type
	SpringFreq = 13, // Only used by appropriate car type
	SpringDampingRatio = 14, // Only used by appropriate car type

	ChassisDensity = 15,
	WheelDensity = 16,
	WheelFriction = 17,
	Gravity = 18,
};
const uint32_t numGeneIDs = 19;


// This genome struct is designed to easily be mapped to a texture.
// This way, the fitness spectrum can be visualized in a texture where each pixel in the texture represents a unique car.
// That's why the gene values are integers (they are texture coordinates).
// And that's also why the fitness value is between 0-255 (they are pixel values).
// The genome of a car can be imagined as the coordinates inside a multi-dimensional texture,
// where each axis in the texture corresponds to some car design parameters (such as wheel position, radius, etc...)
struct Genome
{
	// AKA the coordinates inside the fitness texture
	std::vector<uint16_t> values;

	// AKA the color of the pixel
	byte fitness = 0;

	// Resets all values and fitness to 0.
	// The size of the values vector is resized to 'numValues'.
	void Reset(size_t numValues);

	std::string ToString() const;
};

struct GeneticAlgorithmDesc
{
	uint32_t seed = 0;

	// Population size
	uint32_t popSize = 0; 

	// How many elites to copy over to next generation.
	uint32_t eliteCount = 0;
	
	// Value between 0.0 and 1.0.
	// Probability of a gene getting mutated.
	float mutationProb = 0; 

	// Value between 0.0 and 1.0.
	// Represents how far (in relation to fitness texture size) a gene value can change in a single mutation.
	float mutationStrength = 0; 

	float crossoverProb = 0;
};

class GeneticAlgorithm
{
public:
	GeneticAlgorithm() = default;
	~GeneticAlgorithm() { Unload(); };

	// All genes can hold values ranging from 0 to fitnessTextureSize-1.
	// Returns true if success.
	bool Load(GeneticAlgorithmDesc, uint32_t numGenesEnabled, uint32_t fitnessTextureSize);
	void Unload();
	bool IsLoaded() const { return isLoaded; }

	// Runs the genetic algorithm for 1 generation.
	void NextGeneration();

	// Assigns a fitness value to an individual.
	// The fitness of each new individual must be assigned between each generation.
	// Elites retain their fitness from past generations.
	void AssignFitness(size_t index, byte fitness);

	size_t GetPopulationSize() const { return pop.size(); }
	Genome GetIndividual(size_t index) const;
	uint32_t GetCurrentGeneration() const { return currentGeneration; }
	float GetAvarageFitness() const { return avarageFitness; }
	const Genome& GetBestIndividual() const { return bestIndividual; }

private:
	bool isLoaded = false;

	ig::UniformRandom random;
	GeneticAlgorithmDesc desc;
	uint32_t numGenesEnabled = 0;
	uint32_t fitnessTextureSize = 0;

	std::vector<Genome> pop; // Population
	uint32_t currentGeneration = 0;

	float avarageFitness = 0;
	Genome bestIndividual;

	// Returns index of selected individual.
	size_t RouletteSelection();
	void Mutate(Genome& in_out_genome);
	void GenerateOffspring(const Genome& parentA, const Genome& parentB, std::vector<Genome>& out_newPop);

	static uint32_t CalcTotalFitness(const std::vector<Genome>& pop);
	static float CalcAvarageFitness(const std::vector<Genome>& pop);
	static size_t FindLeastFitIndividual(const std::vector<Genome>& pop); // Returns index of least fit individual
	static std::vector<Genome> CreatePopulation(size_t popSize, uint32_t numGenesEnabled,
		uint32_t fitnessTextureSize, ig::UniformRandom& random);

};
