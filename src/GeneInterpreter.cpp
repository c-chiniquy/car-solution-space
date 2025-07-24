#include "iglo.h"
#include "iglo_batch_renderer.h"
#include "box2d.h"
#include "stdexcept"
#include "Car.h"
#include "GeneticAlgorithm.h"
#include "GeneInterpreter.h"


GeneInterpreter::MinMax GeneInterpreter::GetGeneMinMax(GeneID id) const
{
	MinMax out;
	out.minValue = 0;
	out.maxValue = 0;
	switch (id)
	{
	default:
		break;

	case GeneID::Chassis0_X:
		out.minValue = minChassisX;
		out.maxValue = maxChassisX;
		break;
	case GeneID::Chassis0_Y:
		out.minValue = maxChassisY; // The Y coordinate is inverted, so min and max are reversed for Y positions
		out.maxValue = minChassisY;
		break;
	case GeneID::Chassis0_Radius:
		out.minValue = minChassisRadius;
		out.maxValue = maxChassisRadius;
		break;
	case GeneID::Chassis1_X:
		out.minValue = minChassisX;
		out.maxValue = maxChassisX;
		break;
	case GeneID::Chassis1_Y:
		out.minValue = maxChassisY; // reversed
		out.maxValue = minChassisY;
		break;

	case GeneID::Wheel0_X:
		out.minValue = minWheelX;
		out.maxValue = maxWheelX;
		break;
	case GeneID::Wheel0_Y:
		out.minValue = maxWheelY; // reversed
		out.maxValue = minWheelY;
		break;
	case GeneID::Wheel0_Radius:
		out.minValue = minWheelRadius;
		out.maxValue = maxWheelRadius;
		break;
	case GeneID::Wheel1_X:
		out.minValue = minWheelX;
		out.maxValue = maxWheelX;
		break;
	case GeneID::Wheel1_Y:
		out.minValue = maxWheelY; // reversed
		out.maxValue = minWheelY;
		break;
	case GeneID::Wheel1_Radius:
		out.minValue = minWheelRadius;
		out.maxValue = maxWheelRadius;
		break;

	case GeneID::AngularImpulsePower:
		out.minValue = minAngularImpulse;
		out.maxValue = maxAngularImpulse;
		break;
	case GeneID::JointMotorTorque:
		out.minValue = minJointMotorTorque;
		out.maxValue = maxJointMotorTorque;
		break;
	case GeneID::SpringFreq:
		out.minValue = minSpringFreq;
		out.maxValue = maxSpringFreq;
		break;
	case GeneID::SpringDampingRatio:
		out.minValue = minSpringDampingRatio;
		out.maxValue = maxSpringDampingRatio;
		break;
	case GeneID::ChassisDensity:
		out.minValue = minChassisDensity;
		out.maxValue = maxChassisDensity;
		break;
	case GeneID::WheelDensity:
		out.minValue = minWheelDensity;
		out.maxValue = maxWheelDensity;
		break;
	case GeneID::WheelFriction:
		out.minValue = minWheelFriction;
		out.maxValue = maxWheelFriction;
		break;
	case GeneID::Gravity:
		out.minValue = minGravity;
		out.maxValue = maxGravity;
		break;
	}

	return out;
}

float GeneInterpreter::InterpretGene(uint16_t geneValue, GeneID id) const
{
	MinMax range = GetGeneMinMax(id);
	if (fitnessTextureSize <= 1) return range.minValue;
	float practicalLength = range.maxValue - range.minValue;
	float normalized = ((float)geneValue / (float)(fitnessTextureSize - 1));
	return (practicalLength * normalized) + range.minValue;
}

CarDesc GeneInterpreter::InterpretGenome(const Genome& genome, const CarDesc& defaultCar) const
{
	CarDesc out = defaultCar;

	size_t geneIndex = 0;
	for (uint32_t i = 0; i < numGeneIDs; i++)
	{
		GeneID id = (GeneID)i;

		// Skip disabled genes
		if (!GetGeneEnabled(id)) continue;

		if (geneIndex >= genome.values.size() ||
			genome.values.size() == 0)
		{
			throw std::invalid_argument("Attempted to access a genome value that is out of range.");
		}
		uint16_t v = genome.values[geneIndex];
		float practicalValue = InterpretGene(v, id);

		// Apply the value to the car
		switch (id)
		{
		default:
			break;

		case GeneID::Chassis0_X: out.chassis0Pos.x = practicalValue; break;
		case GeneID::Chassis0_Y: out.chassis0Pos.y = practicalValue; break;
		case GeneID::Chassis0_Radius: out.chassis0Radius = practicalValue; break;
		case GeneID::Chassis1_X: out.chassis1Pos.x = practicalValue; break;
		case GeneID::Chassis1_Y: out.chassis1Pos.y = practicalValue; break;
		case GeneID::Wheel0_X: out.wheel0Pos.x = practicalValue; break;
		case GeneID::Wheel0_Y: out.wheel0Pos.y = practicalValue; break;
		case GeneID::Wheel0_Radius: out.wheel0Radius = practicalValue; break;
		case GeneID::Wheel1_X: out.wheel1Pos.x = practicalValue; break;
		case GeneID::Wheel1_Y: out.wheel1Pos.y = practicalValue; break;
		case GeneID::Wheel1_Radius: out.wheel1Radius = practicalValue; break;
		case GeneID::AngularImpulsePower: out.angularImpulse = practicalValue; break;
		case GeneID::JointMotorTorque: out.jointMotorTorque = practicalValue; break;
		case GeneID::SpringFreq: out.springFreq = practicalValue; break;
		case GeneID::SpringDampingRatio: out.springDampingRatio = practicalValue; break;
		case GeneID::ChassisDensity: out.chassisDensity = practicalValue; break;
		case GeneID::WheelDensity: out.wheelDensity = practicalValue; break;
		case GeneID::WheelFriction: out.wheelFriction = practicalValue; break;

			// Gravity isn't part of the car description, so that has to be set somewhere else.
		case GeneID::Gravity: break;

		}


		geneIndex++;
	}

	return out;
}

unsigned int GeneInterpreter::GetNumGenesEnabled() const
{
	unsigned int numEnabledGenes = 0;
	for (uint32_t i = 0; i < numGeneIDs; i++)
	{
		if (GetGeneEnabled((GeneID)i)) numEnabledGenes++;
	}
	return numEnabledGenes;
}

bool GeneInterpreter::GetGeneEnabled(GeneID id) const
{
	if ((unsigned int)id >= numGeneIDs) throw std::invalid_argument("Provided gene ID is out of bounds.");
	return (geneMask & ((uint64_t)1 << (uint32_t)id));
}

void GeneInterpreter::SetGeneEnabled(GeneID id, bool enabled)
{
	if (enabled)
	{
		geneMask = geneMask | ((uint64_t)1 << (uint32_t)id);
	}
	else
	{
		geneMask = geneMask & ~((uint64_t)1 << (uint32_t)id);
	}
}

size_t GeneInterpreter::GetGeneIndex(GeneID id) const
{
	size_t geneIndex = 0;
	for (uint32_t i = 0; i < numGeneIDs; i++)
	{
		// Skip disabled genes
		if (!GetGeneEnabled((GeneID)i)) continue;

		if ((GeneID)i == id) return geneIndex;

		geneIndex++;
	}
	throw std::invalid_argument("Bad argument: Provided gene ID has no index, because it is not enabled.");
}

GeneID GeneInterpreter::GetGeneID(size_t index) const
{
	if (index >= GetNumGenesEnabled()) throw std::invalid_argument("Bad argument: Index out of bounds.");

	size_t geneIndex = 0;
	for (uint32_t i = 0; i < numGeneIDs; i++)
	{
		// Skip disabled genes
		if (!GetGeneEnabled((GeneID)i)) continue;

		if (index == geneIndex) return (GeneID)i;

		geneIndex++;
	}
	throw std::invalid_argument("Bad argument: Index out of bounds.");
}

