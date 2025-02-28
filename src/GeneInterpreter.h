#pragma once


// This struct is used to convert gene values into practical values for the simulation,
// such as where to place the wheel, the radius, etc...
struct GeneInterpreter
{
	// A bit-wise mask used to determine which genes are enabled. 1=enabled, 0=disabled.
	uint64_t geneMask = 0;

	// The value of a gene cannot exceed the size of the fitness texture
	uint32_t fitnessTextureSize = 0;

	float minWheelFriction = 0;
	float maxWheelFriction = 0;

	float minWheelRadius = 0;
	float maxWheelRadius = 0;

	float minChassisRadius = 0;
	float maxChassisRadius = 0;

	float minChassisX = 0;
	float maxChassisX = 0;

	float minChassisY = 0;
	float maxChassisY = 0;

	float minWheelX = 0;
	float maxWheelX = 0;

	float minWheelY = 0;
	float maxWheelY = 0;

	float minChassisDensity = 0;
	float maxChassisDensity = 0;

	float minWheelDensity = 0;
	float maxWheelDensity = 0;

	float minJointMotorTorque = 0;
	float maxJointMotorTorque = 0;

	float minAngularImpulse = 0;
	float maxAngularImpulse = 0;

	float minSpringFreq = 0;
	float maxSpringFreq = 0;

	float minSpringDampingRatio = 0;
	float maxSpringDampingRatio = 0;

	float minGravity = 0;
	float maxGravity = 0;


	struct MinMax
	{
		float minValue = 0;
		float maxValue = 0;
	};
	MinMax GetGeneMinMax(GeneID id) const;

	unsigned int GetNumGenesEnabled() const;
	bool GetGeneEnabled(GeneID id) const;
	void SetGeneEnabled(GeneID id, bool enabled);

	// Converts a gene value (AKA texture coordinate) to a practical value that can be used by the simulation.
	float InterpretGene(uint16_t geneValue, GeneID) const;

	// For disabled genes, values from 'defaultCar' will be used.
	CarDesc InterpretGenome(const Genome&, const CarDesc& defaultCar) const;

	// Gets which index the specified gene ID should reside in inside the Genome struct.
	size_t GetGeneIndex(GeneID id) const;
	GeneID GetGeneID(size_t index) const;
};
