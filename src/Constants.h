#pragma once

namespace Constants
{
	const std::string appName = "2D Car Solution Space Explorer";

	// File
	const std::string settingsFilepath = "params.ini";
	const std::string savedImagesDirectory = "saved-images/";

	// Simulation
	const ig::Vector2 carStartPos = ig::Vector2(2.0f, 2.0f);

	// Terrain graphics
	const ig::Color32 terrainColor0 = ig::Color32(128, 128, 128);
	const ig::Color32 terrainColor1 = ig::Color32(0, 0, 0);
	const float terrainDrawingDepth = 1000.0f / 11.0f;
	const bool drawTerrainBorders = false;
	const bool fillTerrain = true;

	// Car graphics
	const ig::Color32 wheelColor = ig::Color32(0, 0, 255);
	const ig::Color32 chassiCircleColor = ig::Color32(255, 0, 0);
	const ig::Color32 angleColor = ig::Color32(150, 150, 150);
	const ig::Color32 connectionColor = ig::Color32(0, 0, 0, 100);
	const ig::Color32 connectionColorDarkMode = ig::Color32(255, 255, 255, 100);

	const ig::Color32 rigidWheelColor = ig::Color32(155, 155, 230);
	const ig::Color32 rigidWheelBorderColor = ig::Color32(24, 24, 230, 120);
	const ig::Color32 rigidAngleColor = ig::Color32(24, 24, 230, 120);
	const ig::Color32 rigidConnectionColor = ig::Color32(230, 24, 24);
	const ig::Color32 rigidSolidColor = ig::Color32(230, 155, 155);
	const float rigidConnectionThickness = 0.1f;

	// Text graphics
	const std::string fontFileName = "resources/Vegur-Bold.otf";
	const float fontSize = 18;
	const ig::Color32 textColor = ig::Color32(64, 120, 64);
	const ig::Color32 textColorDarkMode = ig::Color32(255, 255, 255);

	// Camera
	const float cameraSpeed = 0.1f;
	const float controlledCameraSpeed = 30.0f / 11.0f;
}
