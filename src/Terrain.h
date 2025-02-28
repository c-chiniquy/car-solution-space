#pragma once

enum class TerrainType : uint32_t
{
	Normal = 0,
	Big = 1,
	Flat = 2,
	Waves = 3,
};

struct TerrainDesc
{
	uint32_t seed = 0;
	TerrainType type = TerrainType::Normal;
	uint32_t numChunks = 0; // The number of chunks the terrain consists of. (The length of the terrain)
	float friction = 0;
	float minChunkWidth = 0;
	float maxChunkWidth = 0;
	float minChunkHeight = 0;
	float maxChunkHeight = 0;
	float firstChunkWidth = 0; // The first bit of terrain will be flat
	float firstChunkHeight = 0;
	float scale = 0;
};

class Terrain
{
public:
	Terrain() = default;
	~Terrain() { Unload(); };

	// Make class non-copyable
	Terrain(Terrain&) = delete;
	Terrain& operator=(Terrain&) = delete;

	bool Load(b2World& world, const TerrainDesc& terrainDesc);
	void Unload();
	bool IsLoaded() const { return isLoaded; }

	void Draw(ig::BatchRenderer& r, ig::FloatRect cameraRect);

	float GetTotalWidth() const { return totalWidth; }
	const b2Body* GetBody() const { return body; }

private:
	bool isLoaded = false;
	b2World* parentWorld = nullptr;

	struct Chunk
	{
		float x = 0;
		float y = 0;
	};
	std::vector<Chunk> chunks;

	TerrainDesc desc;
	float totalWidth = 0;
	b2Body* body = nullptr;
};