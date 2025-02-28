#include "iglo.h"
#include "igloBatchRenderer.h"
#include "box2d.h"
#include "Constants.h"
#include "Terrain.h"



bool Terrain::Load(b2World& world, const TerrainDesc& terrainDesc)
{
	Unload();

	desc = terrainDesc;
	parentWorld = &world;

	// Set seed
	ig::Random::SetSeed(desc.seed);

	// Generate terrain
	if (desc.numChunks >= 2)
	{
		chunks.resize(desc.numChunks);
		chunks[0].x = 0;
		chunks[0].y = 0;
		chunks[1].x = desc.firstChunkWidth;
		chunks[1].y = desc.firstChunkHeight;
		float currentX = chunks[1].x;
		float currentY = chunks[1].y;
		for (uint32_t i = 2; i < desc.numChunks; i++)
		{
			float nextX = ig::Random::NextFloat(desc.minChunkWidth, desc.maxChunkWidth);
			float nextY = ig::Random::NextFloat(desc.minChunkHeight, desc.maxChunkHeight);
			nextY = -nextY + desc.maxChunkHeight + desc.minChunkHeight; // Invert Y
			if (desc.type == TerrainType::Normal)
			{
				chunks[i].x = currentX + nextX;
				chunks[i].y = currentY + nextY;
			}
			else if (desc.type == TerrainType::Big)
			{
				chunks[i].x = currentX + nextX;
				chunks[i].y = currentY + chunks[i - 1].y + nextY;
			}
			else if (desc.type == TerrainType::Flat)
			{
				chunks[i].x = currentX + nextX;
				chunks[i].y = currentY;
			}
			else if (desc.type == TerrainType::Waves)
			{
				chunks[i].x = currentX + desc.minChunkWidth;
				chunks[i].y = sinf(currentX / desc.maxChunkWidth) * (desc.maxChunkHeight - desc.minChunkHeight);
			}
			currentX = chunks[i].x;
			currentY = chunks[i].y;
		}
		// Apply scaling
		totalWidth = currentX / desc.scale;
		for (size_t i = 0; i < chunks.size(); i++)
		{
			chunks[i].x /= desc.scale;
			chunks[i].y /= desc.scale;
		}
	}
	else
	{
		chunks.clear();
		totalWidth = 0;
	}

	// Create box2d body
	b2BodyDef bd;
	body = world.CreateBody(&bd);
	b2EdgeShape shape;
	b2FixtureDef fd;
	fd.shape = &shape;
	fd.density = 0.0f;
	fd.friction = desc.friction;

	if (desc.numChunks > 0)
	{
		for (uint64_t i = 0; i < desc.numChunks - 1; i++)
		{
			shape.SetTwoSided(
				b2Vec2(chunks[i].x, chunks[i].y),
				b2Vec2(chunks[i + 1].x, chunks[i + 1].y));
			body->CreateFixture(&fd);
		}
	}

	isLoaded = true;
	return true;
}

void Terrain::Unload()
{
	if (body)
	{
		if (parentWorld) parentWorld->DestroyBody(body);
		body = nullptr;
	}
	chunks.clear();
	totalWidth = 0;
	parentWorld = nullptr;
	isLoaded = false;
}

void Terrain::Draw(ig::BatchRenderer& r, ig::FloatRect cameraRect)
{
	if (!isLoaded) return;
	if (desc.numChunks == 0) return;
	if (chunks.size() == 0) return;

	// Draw beginning and end of track
	float rectWidth = 0.2f;
	r.DrawRectangle(-rectWidth, cameraRect.top, rectWidth, cameraRect.GetHeight(), ig::Color32(255, 0, 0, 128));
	r.DrawRectangle(totalWidth, cameraRect.top, rectWidth, cameraRect.GetHeight(), ig::Color32(0, 255, 0, 128));

	if (Constants::fillTerrain)
	{
		for (size_t i = 0; i < chunks.size() - 1; i++)
		{
			float x = chunks[i].x;
			float y = chunks[i].y;
			float nextX = chunks[i + 1].x;
			float nextY = chunks[i + 1].y;

			// Only draw the chunks that are in view
			if (nextX >= cameraRect.left && x <= cameraRect.right)
			{
				r.DrawTriangle(
					ig::Vector2(x, y - Constants::terrainDrawingDepth),
					ig::Vector2(x, y),
					ig::Vector2(nextX, (nextY - Constants::terrainDrawingDepth)),
					Constants::terrainColor1,
					Constants::terrainColor0,
					Constants::terrainColor1);
				r.DrawTriangle(
					ig::Vector2(x, y),
					ig::Vector2(nextX, (nextY)),
					ig::Vector2(nextX, (nextY - Constants::terrainDrawingDepth)),
					Constants::terrainColor0,
					Constants::terrainColor0,
					Constants::terrainColor1);
			}
		}
	}

	if (Constants::drawTerrainBorders)
	{
		for (size_t i = 0; i < chunks.size() - 1; i++)
		{
			float x = chunks[i].x;
			float y = chunks[i].y;
			float nextX = chunks[i + 1].x;
			float nextY = chunks[i + 1].y;

			// Only draw the chunks that are in view
			if (nextX >= cameraRect.left && x <= cameraRect.right)
			{
				r.DrawRectangularLine(x, y, nextX, nextY, 2 / 11.0f, ig::Colors::Black);
			}
		}
	}
}

