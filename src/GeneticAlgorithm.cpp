#include "iglo_utility.h"
#include "GeneticAlgorithm.h"


void Genome::Reset(size_t numValues)
{
	fitness = 0;
	if (numValues == 0)
	{
		values.clear();
	}
	else
	{
		values.resize(numValues);
		for (size_t i = 0; i < values.size(); i++)
		{
			values[i] = 0;
		}
	}
}

std::string Genome::ToString() const
{
	std::string out = ig::ToString(fitness);
	if (values.size() == 0) return out;
	out.append("  at  ");

	if (values.size() >= 1 && values.size() <= 4)
	{
		out.append(ig::ToString("X=", values.at(0)));
		if (values.size() == 1) return out;

		out.append(ig::ToString(", Y=", values.at(1)));
		if (values.size() == 2) return out;

		out.append(ig::ToString(", Z=", values.at(2)));
		if (values.size() == 3) return out;

		out.append(ig::ToString(", W=", values.at(3)));
		return out;
	}
	out.append("[");
	for (size_t i = 0; i < values.size(); i++)
	{
		out.append(ig::ToString(values.at(i)));
		if (i != values.size() - 1) out.append(", ");
	}
	return out + "]";
}


bool GeneticAlgorithm::Load(GeneticAlgorithmDesc desc, uint32_t numGenesEnabled, uint32_t fitnessTextureSize)
{
	Unload();

	if (numGenesEnabled < 1) return false;
	if (fitnessTextureSize < 1) return false;
	if (desc.popSize < 1) return false;

	this->random.SetSeed(desc.seed);
	this->pop = CreatePopulation(desc.popSize, numGenesEnabled, fitnessTextureSize, this->random);
	this->bestIndividual = pop.at(0);
	this->desc = desc;
	this->fitnessTextureSize = fitnessTextureSize;
	this->numGenesEnabled = numGenesEnabled;
	this->isLoaded = true;
	return true;
}

void GeneticAlgorithm::Unload()
{
	isLoaded = false;
	pop.clear();
	currentGeneration = 0;
	avarageFitness = 0;
	bestIndividual.Reset(0);
}

std::vector<Genome> GeneticAlgorithm::CreatePopulation(size_t popSize, uint32_t numGenesEnabled,
	uint32_t fitnessTextureSize, ig::UniformRandom& random)
{
	std::vector<Genome> population;
	population.resize(popSize);
	for (size_t i = 0; i < popSize; i++)
	{
		population.at(i).Reset(numGenesEnabled);
		for (size_t v = 0; v < numGenesEnabled; v++)
		{
			population.at(i).values.at(v) = random.NextInt32(0, fitnessTextureSize - 1);
		}
	}
	return population;
}

bool CompareGenomes(const Genome& a, const Genome& b)
{
	return a.fitness > b.fitness;
}

void GeneticAlgorithm::NextGeneration()
{
	if (!isLoaded) return;
	if (pop.size() != desc.popSize || desc.popSize == 0) throw std::runtime_error("Something is wrong");

	currentGeneration++;

	// Sort population by fitness
	std::sort(pop.begin(), pop.end(), CompareGenomes);

	// Calculate avarage and best fitness
	avarageFitness = CalcAvarageFitness(pop);
	bestIndividual = pop.at(0);

	std::vector<Genome> newPop;

	// Copy N elites to new population
	for (size_t i = 0; i < desc.eliteCount; i++)
	{
		if (i == pop.size()) break;
		newPop.push_back(pop.at(i));
	}

	// Generate offspring
	while (newPop.size() < desc.popSize)
	{
		size_t parentA = RouletteSelection();
		size_t parentB = RouletteSelection();
		GenerateOffspring(pop[parentA], pop[parentB], newPop);
	}

	// Adjust population size
	while (newPop.size() > desc.popSize)
	{
		newPop.pop_back();
	}

	pop.swap(newPop);
}

size_t GeneticAlgorithm::RouletteSelection()
{
	if (pop.size() <= 1) return 0;

	uint32_t totalFitness = CalcTotalFitness(pop);

	// If all individuals have 0 fitness, then choose an individual at random
	if (totalFitness == 0) return (size_t)random.NextInt32(0, int32_t(pop.size() - 1));

	// Higher fitness = more likely to get selected.
	// An individual with 0 fitness will never get selected.
	uint32_t roulette = (uint32_t)random.NextInt32(0, totalFitness - 1);
	for (size_t i = 0; i < pop.size(); i++)
	{
		if (roulette < pop.at(i).fitness) return i;
		roulette -= pop.at(i).fitness;
	}
	throw std::runtime_error("This should be impossible.");
}

void GeneticAlgorithm::Mutate(Genome& in_out_genome)
{
	for (size_t v = 0; v < in_out_genome.values.size(); v++)
	{
		float currentValue = (float)in_out_genome.values.at(v);
		float strength = float(fitnessTextureSize - 1) * desc.mutationStrength;

		float minValue = currentValue - strength;
		float maxValue = currentValue + strength;
		if (minValue < 0) minValue = 0;
		if (maxValue > float(fitnessTextureSize - 1)) maxValue = float(fitnessTextureSize - 1);

		int mutation = (int)roundf(random.NextFloat(minValue, maxValue));
		if (mutation < 0)mutation = 0;
		if (mutation > (int)fitnessTextureSize - 1) mutation = fitnessTextureSize - 1;

		in_out_genome.values[v] = (uint16_t)mutation;
	}
}

void GeneticAlgorithm::GenerateOffspring(const Genome& parentA, const Genome& parentB, std::vector<Genome>& out_newPop)
{
	if (parentA.values.size() != parentB.values.size() ||
		parentA.values.size() != numGenesEnabled)
	{
		throw std::runtime_error("Something isn't right");
	}

	Genome outA;
	Genome outB;
	outA.Reset(numGenesEnabled);
	outB.Reset(numGenesEnabled);

	if (!random.NextProbability(desc.crossoverProb))
	{
		// No crossover
		outA.values = parentA.values;
		outB.values = parentB.values;
	}
	else
	{
		// Crossover
		for (size_t i = 0; i < parentA.values.size(); i++)
		{
			if (random.NextBool())
			{
				outA.values[i] = parentA.values[i];
				outB.values[i] = parentB.values[i];
			}
			else
			{
				outA.values[i] = parentB.values[i];
				outB.values[i] = parentA.values[i];
			}
		}
	}

	// Mutate the offspring
	if (random.NextProbability(desc.mutationProb)) Mutate(outA);
	if (random.NextProbability(desc.mutationProb)) Mutate(outB);

	out_newPop.push_back(outA);
	out_newPop.push_back(outB);
}

uint32_t GeneticAlgorithm::CalcTotalFitness(const std::vector<Genome>& pop)
{
	if (pop.size() == 0) return 0;

	uint32_t total = 0;
	for (size_t i = 0; i < pop.size(); i++)
	{
		total += pop.at(i).fitness;
	}
	return total;
}

float GeneticAlgorithm::CalcAvarageFitness(const std::vector<Genome>& pop)
{
	if (pop.size() == 0) return 0;

	return (float)CalcTotalFitness(pop) / (float)pop.size();
}

size_t GeneticAlgorithm::FindLeastFitIndividual(const std::vector<Genome>& pop)
{
	if (pop.size() == 0) throw std::runtime_error("Unable to find least fit individual, population count is 0.");

	byte worst = pop.at(0).fitness;
	size_t worstIndex = 0;
	for (size_t i = 1; i < pop.size(); i++)
	{
		if (pop.at(i).fitness < worst)
		{
			worst = pop.at(i).fitness;
			worstIndex = i;
		}
	}
	return worstIndex;
}

Genome GeneticAlgorithm::GetIndividual(size_t index) const
{
	if (!isLoaded) throw std::runtime_error("Not loaded");

	return pop.at(index);
}

void GeneticAlgorithm::AssignFitness(size_t index, byte fitness)
{
	if (!isLoaded) return;

	pop.at(index).fitness = fitness;
}
