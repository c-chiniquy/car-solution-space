
// iglo
#include "iglo.h"
#include "iglo_batch_renderer.h"
#include "iglo_font.h"
#include "iglo_main_loop.h"

// Box2D
#include "Box2D.h"

// ini
#include "ini.h"

// imgui
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"

// implot
#include "implot.h"

// Local includes
#include "Car.h"
#include "Terrain.h"
#include "GeneticAlgorithm.h"
#include "GeneInterpreter.h"
#include "FitnessTexture.h"
#include "Constants.h"
#include "Settings.h"

class App
{
public:

	void Run();

private:

	// iglo
	ig::IGLOContext context;
	ig::CommandList cmd;
	ig::BatchRenderer batchRenderer;
	ig::BatchRenderer batchRendererMSAA;
	ig::MainLoop mainloop;

	// iglo callbacks
	void Start();
	void OnLoopExited();
	void OnEvent(ig::Event e);
	void Update(double elapsedTime);
	void FixedUpdate();
	void Draw();
	void Physics();

	// App
	enum class AppState
	{
		Game = 0, // Car is controllable.
		Auto, // Car is driving by itself.
		BruteForce, // Find optimal car design using simple trial and error
		GeneticAlgorithm, // The car will evolve using a genetic algorithm
		FindOptimalTerrain, // Many different terrains are tested to see which terrain best suits the current car
	};
	enum class LossCondition
	{
		ChassiTerrainCollision = 0,
		CarFlippedOver = 1,
	};
	struct AppInput
	{
		bool up = false;
		bool down = false;
		bool left = false;
		bool right = false;
	};
	AppState appState = AppState::Game;
	AppInput appInput;
	bool isPaused = false;
	Settings settings;
	std::unique_ptr<b2World> world;
	Terrain terrain;
	Car car;
	ig::Descriptor imguiDescriptor;
	void ApplyIgloSettings();
	void UpdateImgui();
	bool DarkModeEnabled();

	// Fitness texture
	FitnessTexture fitnessTexture;
	void OnFitnessTextureMouseDown(const Genome&);
	void OnFitnessTextureMouseUp();

	// Camera
	ig::Vector2 cameraPos = ig::Vector2(0, 0); // Camera center
	float GetCameraWidth();
	float GetCameraHeight();

	// Fonts and text
	ig::Font defaultFont;
	ig::Font vegurBold;
	std::string statusText = "";

	// Rendering
	ig::Texture renderTarget; // We use a render target to render the scene with MSAA
	ig::RenderTargetDesc renderTargetDesc;
	ig::Color32 renderTargetCurrentClearColor;
	void LoadRenderTarget();

	// Genetic algorithm
	GeneticAlgorithm GA;
	size_t index_GA = 0; // Index of current individual in the genetic algorithm
	std::vector<float> avarageFitness_GA; // plot
	std::vector<float> bestFitness_GA; // plot

	// Simulation progress
	typedef byte Score; // 0 = reached end of terrain to the left, 255 = reached end of terrain to the right.
	Genome bestGenome;
	Genome genome;
	uint32_t frame = 0;
	LossCondition GetCurrentLossCondition();
	void StepPhysics();
	Score CalculateScore(float distanceTraveled);
	void ResetWorld();
	void RespawnCar();
	void RespawnTerrain();
	void OnCarTestFinished(Score score);

	// Trying different terrains
	std::vector<byte> terrainScores;
	uint32_t terrainIndex = 0;
	Score bestTerrainScore = 0;
	uint32_t bestTerrainSeed = 0;

};
