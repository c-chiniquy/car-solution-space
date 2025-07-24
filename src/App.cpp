#include "App.h"

// Agility SDK path and version
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 715; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool WndProcHook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) return true;
	return false;
}

void App::Run()
{
	if (context.Load(
		ig::WindowSettings
		{
			.title = Constants::appName,
			.width = 1280,
			.height = 720,
			.resizable = true,
			.centered = true,
		},
		ig::RenderSettings
		{
			.presentMode = ig::PresentMode::Immediate,
			.backBufferFormat = ig::Format::BYTE_BYTE_BYTE_BYTE_BGRA,
		}))
	{
		mainloop.SetWindowMinimizedBehaviour(ig::MainLoop::WindowMinimizedBehaviour::SkipDraw);
		
		mainloop.Run(context,
			std::bind(&App::Start, this),
			std::bind(&App::OnLoopExited, this),
			std::bind(&App::Draw, this),
			std::bind(&App::Update, this, std::placeholders::_1),
			std::bind(&App::FixedUpdate, this),
			std::bind(&App::OnEvent, this, std::placeholders::_1));
	}
}

void App::LoadRenderTarget()
{
	ig::ClearValue clearValue;
	clearValue.color = renderTargetCurrentClearColor;
	renderTarget.Load(context, context.GetWidth(), context.GetHeight(), renderTargetDesc.colorFormats.at(0),
		ig::TextureUsage::RenderTexture, renderTargetDesc.msaa, clearValue);
}

void App::Start()
{
	renderTargetDesc.colorFormats = { ig::Format::BYTE_BYTE_BYTE_BYTE_BGRA };
	renderTargetDesc.msaa = context.GetMaxMultiSampleCount(renderTargetDesc.colorFormats.at(0));

	cmd.Load(context, ig::CommandListType::Graphics);

	cmd.Begin();
	{
		// Two batch renderers. One for the MSAA render target and one for the back buffer.
		batchRendererMSAA.Load(context, cmd, renderTargetDesc);
		batchRenderer.Load(context, cmd, context.GetBackBufferRenderTargetDesc());

		defaultFont.LoadAsPrebaked(context, cmd, ig::GetDefaultFont());
	}
	cmd.End();

	context.WaitForCompletion(context.Submit(cmd));

	// Load font
	if (!vegurBold.LoadFromFile(context, Constants::fontFileName, Constants::fontSize))
	{
		ig::PopupMessage("Failed to load font.", "Error", &context);
	}

	// ImGui
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Keyboard controls
		io.LogFilename = nullptr;
		io.IniFilename = nullptr;

		ImGui_ImplWin32_Init(context.GetWindowHWND());

		ig::DescriptorHeap& heap = context.GetDescriptorHeap();
		imguiDescriptor = heap.AllocatePersistent(ig::DescriptorType::Texture_SRV);
		ImGui_ImplDX12_Init(
			context.GetD3D12Device(),
			context.GetNumFramesInFlight(),
			ig::GetFormatInfoDXGI(context.GetBackBufferRenderTargetDesc().colorFormats[0]).dxgiFormat,
			heap.GetD3D12DescriptorHeap_Resources(),
			heap.GetD3D12CPUHandle(imguiDescriptor),
			heap.GetD3D12GPUHandle(imguiDescriptor));
		context.SetWndProcHookCallback(WndProcHook);
	}

	// implot
	ImPlot::CreateContext();

	// Load settings
	if (!settings.LoadFromFile(Constants::settingsFilepath))
	{
		ig::Print("ini file '" + Constants::settingsFilepath + "' not found. Using default settings.\n");
		settings.Reset(); // Use default settings if no .ini file exists
	}

	ApplyIgloSettings();

	renderTargetCurrentClearColor = settings.clearColor;
	LoadRenderTarget();

	ResetWorld();
	RespawnTerrain();
	RespawnCar();

}

void App::OnLoopExited()
{
	context.WaitForIdleDevice();
	//settings.SaveToFile(Constants::settingsFilepath);

	if (!imguiDescriptor.IsNull()) context.GetDescriptorHeap().FreePersistent(imguiDescriptor);
	imguiDescriptor.SetToNull();

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	ImPlot::DestroyContext();
}

void App::OnEvent(ig::Event e)
{
	if (e.type == ig::EventType::CloseRequest)
	{
		mainloop.Quit();
		return;
	}

	// Imgui
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard)
	{
		if (e.type == ig::EventType::KeyPress) return;
		if (e.type == ig::EventType::KeyDown) return;
		if (e.type == ig::EventType::KeyUp) return;
		if (e.type == ig::EventType::TextEntered) return;
	}
	if (io.WantCaptureMouse)
	{
		if (e.type == ig::EventType::MouseMove) return;
		if (e.type == ig::EventType::MouseWheel) return;
		if (e.type == ig::EventType::MouseButtonDown) return;
		if (e.type == ig::EventType::MouseButtonUp) return;
	}

	// Fitness texture
	if (fitnessTexture.OnEvent(e)) return;

	// App
	if (e.type == ig::EventType::Resize)
	{
		LoadRenderTarget(); // Let render target resize
	}
	else if (e.type == ig::EventType::KeyPress)
	{
		switch (e.key)
		{
			// F11 fullscreen toggle
		case ig::Key::F11:
			context.ToggleFullscreen();
			break;

			// Alt+Enter fullscreen toggle
		case ig::Key::Enter:
			if (context.IsKeyDown(ig::Key::LeftAlt)) context.ToggleFullscreen();
			break;

		case ig::Key::Escape:
			mainloop.Quit();
			break;

		default:
			break;
		}
	}
	else if (e.type == ig::EventType::KeyDown)
	{
		switch (e.key)
		{
		case ig::Key::Up: appInput.up = true; break;
		case ig::Key::Down: appInput.down = true; break;
		case ig::Key::Left: appInput.left = true; break;
		case ig::Key::Right: appInput.right = true; break;

		default:
			break;
		}
	}
	else if (e.type == ig::EventType::KeyUp)
	{
		switch (e.key)
		{
		case ig::Key::Up: appInput.up = false; break;
		case ig::Key::Down: appInput.down = false; break;
		case ig::Key::Left: appInput.left = false; break;
		case ig::Key::Right: appInput.right = false; break;

		default:
			break;
		}
	}
}

void App::Update(double elapsedTime)
{
}

void App::FixedUpdate()
{
	// Do physics
	ig::Timer timer;
	timer.Reset();
	while (true)
	{
		// Physics is paused when in edit mode
		if (isPaused) break;

		StepPhysics();

		// Real time modes only need 1 physics step each frame
		if (appState == AppState::Game ||
			appState == AppState::Auto) break;

		// Do as many physics steps as possible until a certain time has passed
		if (timer.GetSeconds() >= 1.0 / 60.0f) break;
	}

	if (appState != AppState::Game)
	{
		// Camera can be slightly controlled when not in game mode
		if (appInput.down) cameraPos.y -= Constants::controlledCameraSpeed;
		if (appInput.up) cameraPos.y += Constants::controlledCameraSpeed;
		if (appInput.left) cameraPos.x -= Constants::controlledCameraSpeed;
		if (appInput.right) cameraPos.x += Constants::controlledCameraSpeed;
	}

	// Move camera towards car
	ig::Vector2 carPos = car.GetPosition((appState != AppState::Auto && appState != AppState::Game));
	if (appState == AppState::BruteForce ||
		appState == AppState::GeneticAlgorithm ||
		appState == AppState::FindOptimalTerrain)
	{
		carPos = Constants::carStartPos;
	}
	if (abs(cameraPos.x - carPos.x) > 0) cameraPos.x -= (cameraPos.x - carPos.x) * Constants::cameraSpeed;
	if (abs(cameraPos.y - carPos.y) > 0) cameraPos.y -= (cameraPos.y - carPos.y) * Constants::cameraSpeed;

	// If camera is too far away, teleport it closer
	const float maxDistanceX = GetCameraWidth();
	const float maxDistanceY = GetCameraHeight();
	if (carPos.x - maxDistanceX > cameraPos.x) cameraPos.x = carPos.x - maxDistanceX;
	if (carPos.y - maxDistanceY > cameraPos.y) cameraPos.y = carPos.y - maxDistanceY;
	if (carPos.x + maxDistanceX < cameraPos.x) cameraPos.x = carPos.x + maxDistanceX;
	if (carPos.y + maxDistanceY < cameraPos.y) cameraPos.y = carPos.y + maxDistanceY;

}

void App::OnFitnessTextureMouseDown(const Genome& selectedGenome)
{
	isPaused = true;
	genome = selectedGenome;
	RespawnCar();
}

void App::OnFitnessTextureMouseUp()
{
	isPaused = false;
}

float App::GetCameraWidth()
{
	return (float)context.GetWidth() / settings.cameraScale;
}

float App::GetCameraHeight()
{
	return (float)context.GetHeight() / settings.cameraScale;
}

void App::ResetWorld()
{
	car.Unload();
	terrain.Unload();
	world = std::make_unique<b2World>(b2Vec2());
}

void App::RespawnTerrain()
{
	terrain.Load(*world, settings.terrain);
}

void App::RespawnCar()
{
	// Use correct fitness texture size and dimensions
	if (!fitnessTexture.IsLoaded() ||
		fitnessTexture.GetNumDimensions() != settings.geneInterpreter.GetNumGenesEnabled() ||
		fitnessTexture.GetTextureSize() != settings.geneInterpreter.fitnessTextureSize)
	{
		if (fitnessTexture.IsLoaded()) fitnessTexture.DelayedUnload(context);
		fitnessTexture.Load(context, ig::IntPoint(18, 125), settings.geneInterpreter,
			std::bind(&App::OnFitnessTextureMouseDown, this, std::placeholders::_1),
			std::bind(&App::OnFitnessTextureMouseUp, this));
	}

	// Use correct genome dimensions
	unsigned int numGenes = settings.geneInterpreter.GetNumGenesEnabled();
	if (genome.values.size() != numGenes)
	{
		genome.Reset(numGenes);
		bestGenome = genome;
	}
	genome.fitness = 0;

	// Use correct gravity
	float targetGravity = settings.gravity;
	if (settings.geneInterpreter.GetGeneEnabled(GeneID::Gravity)) // If genome can change gravity
	{
		size_t gravityIndex = settings.geneInterpreter.GetGeneIndex(GeneID::Gravity);
		targetGravity = settings.geneInterpreter.InterpretGene(genome.values[gravityIndex], GeneID::Gravity);
	}
	world->SetGravity(b2Vec2(0.0f, -targetGravity));
	world->SetAllowSleeping(false);

	// Reset the car
	CarDesc carDesc = settings.geneInterpreter.InterpretGenome(genome, settings.car);
	car.Load(*world, carDesc);

	// Reset frame counter
	frame = 0;
}

App::Score App::CalculateScore(float distanceTraveled)
{
	int score = int(ceil((distanceTraveled / terrain.GetTotalWidth()) * 254));
	if (score > 255) score = 255;
	if (score < 0) score = 0;
	return (Score)score;
}

void App::StepPhysics()
{
	if (appState == AppState::Game)
	{
		// Handle game input
		EngineState engine0 = EngineState::Idle;
		EngineState engine1 = EngineState::Idle;
		if (appInput.down) engine1 = EngineState::DriveBackward;
		if (appInput.up) engine1 = EngineState::DriveForward;
		if (appInput.left) engine0 = EngineState::DriveBackward;
		if (appInput.right) engine0 = EngineState::DriveForward;
		car.SetEngineStates(engine0, engine1);
	}
	else
	{
		// if not game, just drive forward
		EngineState back = EngineState::Idle;
		EngineState front = EngineState::Idle;
		if (settings.powerBackWheel) back = EngineState::DriveForward;
		if (settings.powerFrontWheel) front = EngineState::DriveForward;
		car.SetEngineStates(back, front);
	}

	if (appState != AppState::Game)
	{
		// Increase frame counter if not game
		frame++;
	}

	// Let car update the engines
	car.Update();

	// Step Box2D physics
	world->Step(1.0f / settings.timeStep, settings.velocitySteps, settings.positionSteps);

	// Calculate score
	Score score = CalculateScore(car.GetPosition(false).x);
	genome.fitness = score;

	// Check loss condition
	bool lossCondition = false;
	if (GetCurrentLossCondition() == LossCondition::ChassiTerrainCollision)
	{
		lossCondition = car.IsColliding(terrain.GetBody());
	}
	else if (GetCurrentLossCondition() == LossCondition::CarFlippedOver)
	{
		lossCondition = car.HasFlippedOver();
	}

	// Check if car test is finished.
	// The score can only be 0 or 255 if car is outside bounds in some way.
	if (score == 255 ||
		score == 0 ||
		frame > settings.numFramesPerSimulation ||
		lossCondition)
	{
		OnCarTestFinished(score);
	}

}

App::LossCondition App::GetCurrentLossCondition()
{
	if (car.GetDesc().carType == CarType::TriangleJointCar) return LossCondition::ChassiTerrainCollision;
	return LossCondition::CarFlippedOver;
}

void App::OnCarTestFinished(Score score)
{
	// Find optimal terrain
	if (appState == AppState::FindOptimalTerrain)
	{
		if (score > bestTerrainScore)
		{
			bestTerrainScore = score;
			bestTerrainSeed = terrainIndex + settings.terrain.seed;
		}
		terrainIndex++;
		if (terrainIndex >= settings.numTerrains)
		{
			// Complete!
			statusText = ig::ToString("Finished testing ", settings.numTerrains,
				" different terrain seeds. Best seed: ", bestTerrainSeed, ", score=", bestTerrainScore, "\n");
			TerrainDesc testTerrain = settings.terrain;
			testTerrain.seed = bestTerrainSeed;
			terrain.Load(*world, testTerrain);
			appState = AppState::Auto;
			RespawnCar();
			return;
		}
		else
		{
			// Try next terrain
			statusText = ig::ToString("Testing terrain seeds ", terrainIndex, "/", settings.numTerrains, "\n");
			TerrainDesc testTerrain = settings.terrain;
			testTerrain.seed = terrainIndex + settings.terrain.seed;
			terrain.Load(*world, testTerrain);
			RespawnCar();
			return;
		}
	}

	// Avarage score from same car driving on multiple terrains
	Score avgScore = score;
	if (appState == AppState::BruteForce ||
		appState == AppState::GeneticAlgorithm)
	{
		if (settings.enableMultiTerrainSim && settings.numTerrains > 0)
		{
			if (terrainIndex == 0)
			{
				terrainScores.clear();
				terrainScores.resize(settings.numTerrains);
			}
			terrainScores.at(terrainIndex) = score;
			terrainIndex++;
			if (terrainIndex >= settings.numTerrains)
			{
				terrainIndex = 0;
				double avarageTerrainScore = 0;
				for (size_t i = 0; i < terrainScores.size(); i++)
				{
					avarageTerrainScore += terrainScores.at(i);
				}
				avarageTerrainScore /= (double)terrainScores.size();
				if (avarageTerrainScore > 255) avarageTerrainScore = 255;
				if (avarageTerrainScore < 0) avarageTerrainScore = 0;
				avgScore = (byte)avarageTerrainScore;
				RespawnTerrain(); // Go back to default terrain
			}
			else
			{
				// Try next terrain
				TerrainDesc temp = settings.terrain;
				temp.seed += terrainIndex;
				terrain.Load(*world, temp);
				RespawnCar();
				return;
			}
		}
	}

	// Save fitness score
	genome.fitness = avgScore;
	if (avgScore > bestGenome.fitness)
	{
		bestGenome = genome;
	}

	// Brute force
	if (appState == AppState::BruteForce)
	{
		// Paint a pixel on the fitness texture
		fitnessTexture.SetPixel(genome, avgScore);

		if (fitnessTexture.IsLastPixel(genome))
		{
			// Complete!
			statusText = "Brute force complete!\n";
			genome = bestGenome;
			appState = AppState::Auto;
			RespawnCar();
			return;
		}
		else
		{
			// Try next car
			uint32_t texSize = settings.geneInterpreter.fitnessTextureSize;
			fitnessTexture.StepGenomeToNextPixel(genome);
			size_t currentSolution = fitnessTexture.GetPixelIndex(genome) + 1;
			size_t maxSolutions = fitnessTexture.GetTotalPixelCount();
			float progress = int(((float)currentSolution / (float)maxSolutions) * 1000) * 0.1f;
			std::stringstream ss;
			ss << progress; // To reduce the number of decimals being shown
			statusText = ig::ToString("Brute forcing solution space... ",
				currentSolution, "/", maxSolutions, " (", ss.str(), "%)\n");
			RespawnCar();
			return;
		}
	}
	else if (appState == AppState::GeneticAlgorithm)
	{
		fitnessTexture.SetPixel(genome, avgScore);
		GA.AssignFitness(index_GA, avgScore);

		index_GA++;

		// If all individuals are evaluated
		if (index_GA >= GA.GetPopulationSize())
		{
			if (GA.GetCurrentGeneration() >= settings.numGenerations)
			{
				// Complete!
				statusText = ig::ToString("Genetic algorithm complete after ", GA.GetCurrentGeneration(), " generations!\n");
				genome = GA.GetBestIndividual();
				appState = AppState::Auto;
				ResetWorld();
				RespawnTerrain();
				RespawnCar();
				return;
			}
			GA.NextGeneration();
			bestFitness_GA.push_back((float)GA.GetBestIndividual().fitness);
			avarageFitness_GA.push_back(GA.GetAvarageFitness());

			// Skip the elites that already have a fitness value assigned to them
			index_GA = 0;
			while (true)
			{
				if (index_GA + 1 >= GA.GetPopulationSize()) break;
				index_GA++;
				if (GA.GetIndividual(index_GA).fitness == 0) break; // Evaluate if fitness=0
			}
		}

		// Try next car
		genome = GA.GetIndividual(index_GA);
		size_t currentGen = GA.GetCurrentGeneration();
		size_t maxGens = settings.numGenerations;
		float progress = int(((float)currentGen / (float)maxGens) * 1000) * 0.1f;
		std::stringstream ss;
		ss << progress; // To reduce the number of decimals being shown
		statusText = ig::ToString("Evolution in progress... Generation ", currentGen, "/", maxGens, " (", ss.str(), "%)\n");
		RespawnCar();
		return;
	}
	else if (appState == AppState::Game ||
		appState == AppState::Auto)
	{
		RespawnCar();
		return;
	}
}

// Example format: "X: %.1f"
bool CustomImguiTripleDrag(const char* label, const char* format0, const char* format1, const char* format2,
	float* v0, float* v1, float* v2, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f)
{
	bool result = false;
	std::string id0 = "##" + std::string(label) + "0";
	std::string id1 = "##" + std::string(label) + "1";
	std::string id2 = "##" + std::string(label) + "2";

	ImGui::PushItemWidth(98);
	result |= ImGui::DragFloat(id0.c_str(), v0, v_speed, v_min, v_max, format0);
	ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
	result |= ImGui::DragFloat(id1.c_str(), v1, v_speed, v_min, v_max, format1);
	ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
	if (v2)
	{
		result |= ImGui::DragFloat(id2.c_str(), v2, v_speed, v_min, v_max, format2);
		ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
	}
	ImGui::Text(label);
	ImGui::PopItemWidth();

	return result;
}
bool CustomImguiXYDrag(const char* label, float* v_x, float* v_y, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f)
{
	bool result = false;
	std::string id0 = "##" + std::string(label) + "0";
	std::string id1 = "##" + std::string(label) + "1";

	ImGui::PushItemWidth(98);
	result |= ImGui::DragFloat(id0.c_str(), v_x, v_speed, v_min, v_max, "X: %.1f");
	ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
	result |= ImGui::DragFloat(id1.c_str(), v_y, v_speed, v_min, v_max, "Y: %.1f");
	ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
	ImGui::Text(label);
	ImGui::PopItemWidth();

	return result;
}

void App::UpdateImgui()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowSize(ImVec2(0, 0));
	ImGui::Begin(Constants::appName.c_str(), 0, ImGuiWindowFlags_MenuBar);
	{
		// Menus
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Reset settings"))
				{
					settings.Reset();
					ApplyIgloSettings();

					GA.Unload();
					bestFitness_GA.clear();
					avarageFitness_GA.clear();
					if (appState == AppState::GeneticAlgorithm)
					{
						appState = AppState::Auto;
						statusText = "";
					}
					genome.Reset(0);
					RespawnCar();
					RespawnTerrain();
				}
				if (ImGui::MenuItem("Load settings"))
				{
					settings.LoadFromFile(Constants::settingsFilepath);
					ApplyIgloSettings();

					GA.Unload();
					if (appState == AppState::GeneticAlgorithm)
					{
						appState = AppState::Auto;
						statusText = "";
					}
					genome.Reset(0);
					RespawnCar();
					RespawnTerrain();
				}
				if (ImGui::MenuItem("Save settings"))
				{
					settings.SaveToFile(Constants::settingsFilepath);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::Spacing();
		ImGui::Checkbox("Pause", &isPaused);
		ImGui::SameLine();
		if (ImGui::Button("Respawn"))
		{
			ResetWorld();
			RespawnTerrain();
			RespawnCar();
		}
		if (appState == AppState::Auto)
		{
			ImGui::SameLine();
			if (ImGui::Button("Car control: Auto"))
			{
				appState = AppState::Game;
			}
			ImGui::SetItemTooltip("The car drives itself.");
		}
		else if (appState == AppState::Game)
		{
			ImGui::SameLine();
			if (ImGui::Button("Car control: Arrow keys"))
			{
				appState = AppState::Auto;
			}
			ImGui::SetItemTooltip("Use arrow keys to control each individual wheel.");
		}
		else if (appState == AppState::BruteForce)
		{
			ImGui::SameLine();
			if (ImGui::Button("Brute forcing..."))
			{
				statusText = "Brute force cancelled.\n";
				appState = AppState::Auto;
				genome = bestGenome;
				RespawnCar();
				RespawnTerrain();
			}
			ImGui::SetItemTooltip("Click to stop brute forcing.");
		}
		else if (appState == AppState::FindOptimalTerrain)
		{
			ImGui::SameLine();
			if (ImGui::Button("Testing terrain seeds..."))
			{
				statusText = "Terrain test cancelled.\n";
				appState = AppState::Auto;
				genome = bestGenome;
				RespawnCar();
				RespawnTerrain();
			}
			ImGui::SetItemTooltip("Click to stop testing terrain seeds.");
		}
		else if (appState == AppState::GeneticAlgorithm)
		{
			ImGui::SameLine();
			if (ImGui::Button("Evolving..."))
			{
				statusText = "Genetic algorithm cancelled.\n";
				appState = AppState::Auto;
				genome = GA.GetBestIndividual();
				RespawnCar();
				RespawnTerrain();
			}
			ImGui::SetItemTooltip("Click to stop the genetic algorithm.");
		}

		// Tabs
		if (ImGui::BeginTabBar("Tab0", 0))
		{
			ImGui::PushItemWidth(200);
			if (ImGui::BeginTabItem("Car"))
			{
				ImGui::SeparatorText("Car description");
				bool edited = false;
				const char* carTypes[] = { "Triangle Joint Car", "Fixture Box Car" };
				edited |= ImGui::Combo("Car type", (int*)&settings.car.carType, carTypes, IM_ARRAYSIZE(carTypes));
				const char* engineTypes[] = { "Angular Impulse", "Joint Motor" };
				edited |= ImGui::Combo("Engine type", (int*)&settings.car.engineType, engineTypes, IM_ARRAYSIZE(engineTypes));
				edited |= ImGui::Checkbox("Invincible", &settings.car.isInvincible);
				edited |= CustomImguiTripleDrag("Chassis #0", "X: %.1f", "Y: %.1f", "Radius: %.1f",
					&settings.car.chassis0Pos.x, &settings.car.chassis0Pos.y, &settings.car.chassis0Radius, 0.05f, -10, 10);
				edited |= CustomImguiTripleDrag("Chassis #1", "X: %.1f", "Y: %.1f", "",
					&settings.car.chassis1Pos.x, &settings.car.chassis1Pos.y, nullptr, 0.05f, -10, 10);
				edited |= CustomImguiTripleDrag("Back Wheel", "X: %.1f", "Y: %.1f", "Radius: %.1f",
					&settings.car.wheel0Pos.x, &settings.car.wheel0Pos.y, &settings.car.wheel0Radius, 0.05f, -10, 10);
				edited |= CustomImguiTripleDrag("Front Wheel", "X: %.1f", "Y: %.1f", "Radius: %.1f",
					&settings.car.wheel1Pos.x, &settings.car.wheel1Pos.y, &settings.car.wheel1Radius, 0.05f, -10, 10);
				ImGui::PushItemWidth(98);
				edited |= ImGui::DragFloat("Joint Motor Torque", &settings.car.jointMotorTorque, 1, 0, 1000);
				edited |= ImGui::DragFloat("Target Motor Speed", &settings.car.targetMotorSpeed, 1, 0, 1000);
				edited |= ImGui::DragFloat("Spring Frequency", &settings.car.springFreq, 0.1f, 0, 100);
				edited |= ImGui::DragFloat("Spring Damping Ratio", &settings.car.springDampingRatio, 0.1f, 0, 100);
				edited |= ImGui::DragFloat("Angular Impulse", &settings.car.angularImpulse, 1, 0, 1000);
				edited |= ImGui::DragFloat("Chassis Density", &settings.car.chassisDensity, 0.1f, 0, 100);
				edited |= ImGui::DragFloat("Wheel Density", &settings.car.wheelDensity, 0.1f, 0, 100);
				edited |= ImGui::DragFloat("Wheel Friction", &settings.car.wheelFriction, 1, 0, 1000);
				ImGui::PopItemWidth();
				if (edited)
				{
					RespawnCar();
				}
				ImGui::SeparatorText("Car stats");
				ImGui::Text(ig::ToString("Score: ", genome.ToString()).c_str());
				ImGui::Text(ig::ToString("Best score: ", bestGenome.ToString()).c_str());
				ImGui::Text(ig::ToString("Frame: ", frame, "/", settings.numFramesPerSimulation).c_str());
				ImGui::Text("Position: (%.2f, %.2f)", car.GetPosition(false).x, car.GetPosition(false).y);
				ImGui::Text(ig::u8string_to_string(u8"Rotation: %.2f°").c_str(), car.GetChassisRotation() / IGLO_PI * 180.0);

				if (GetCurrentLossCondition() == LossCondition::ChassiTerrainCollision)
				{
					ImGui::Text("Loss condition: When car chassis collides with terrain");
				}
				else if (GetCurrentLossCondition() == LossCondition::CarFlippedOver)
				{
					ImGui::Text(ig::u8string_to_string(u8"Loss condition: When car flips over 90°").c_str());
				}

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Terrain"))
			{
				ImGui::SeparatorText("Terrain description");
				bool edited = false;
				ImGui::PushItemWidth(200);
				edited |= ImGui::InputInt("Seed", (int*)&settings.terrain.seed);
				const char* terrainTypes[] = { "Normal", "Big", "Flat", "Waves" };
				edited |= ImGui::Combo("Terrain type", (int*)&settings.terrain.type, terrainTypes, IM_ARRAYSIZE(terrainTypes));
				ImGui::PopItemWidth();
				ImGui::PushItemWidth(98);
				edited |= ImGui::DragInt("num chunks", (int*)&settings.terrain.numChunks, 1, 0, 10000);
				edited |= ImGui::DragFloat("friction", &settings.terrain.friction, 0.01f, 0.01f, 50);
				ImGui::PopItemWidth();
				edited |= ImGui::DragFloatRange2("Chunk X", &settings.terrain.minChunkWidth, &settings.terrain.maxChunkWidth,
					1, 0, 0, "Min: %.1f", "Max: %.1f");
				edited |= ImGui::DragFloatRange2("Chunk Y", &settings.terrain.minChunkHeight, &settings.terrain.maxChunkHeight,
					1, 0, 0, "Min: %.1f", "Max: %.1f");
				edited |= CustomImguiXYDrag("First Chunk", &settings.terrain.firstChunkWidth, &settings.terrain.firstChunkHeight);
				ImGui::PushItemWidth(98);
				edited |= ImGui::DragFloat("scale", &settings.terrain.scale, 1, 1, 100);
				ImGui::PopItemWidth();
				if (edited)
				{
					RespawnTerrain();
				}

				ImGui::SeparatorText("Multiple terrains");
				ImGui::InputInt("num terrains", (int*)&settings.numTerrains, 10);
				ImGui::Checkbox("Use multiple terrains", &settings.enableMultiTerrainSim);
				ImGui::SetItemTooltip("If enabled, each car is simulated on multiple terrains and the scores"
					" are avaraged together.\nThis makes the fitness scores more balanced and less terrain-specific.");
				if (ImGui::Button("Find optimal terrain seed"))
				{
					RespawnCar();
					RespawnTerrain();
					appState = AppState::FindOptimalTerrain;
					bestGenome = genome;
					bestTerrainScore = 0;
					bestTerrainSeed = settings.terrain.seed;
					terrainIndex = 0;
					isPaused = false;
				}
				if (appState == AppState::FindOptimalTerrain)
				{
					ImGui::ProgressBar(float(terrainIndex) / float(settings.numTerrains));
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Genome"))
			{
				bool edited = false;
				ImGui::SeparatorText("Enabled Genes");
				const char* geneNames[numGeneIDs] = {
					"X## ChassisA",
					"Y## ChassisA",
					"Radius## ChassisA",
					"X## ChassisB",
					"Y## ChassisB",
					"X## WheelA",
					"Y## WheelA",
					"Radius## WheelA",
					"X## WheelB",
					"Y## WheelB",
					"Radius## WheelB",
					"Angular Impulse",
					"Joint Motor Torque",
					"Frequency",
					"Damping Ratio",
					"Chassis ##Density",
					"Wheel ##Density",
					"Wheel Friction",
					"Gravity" };

				for (int i = 0; i < numGeneIDs; i++)
				{
					bool tempBool = settings.geneInterpreter.GetGeneEnabled((GeneID)i);

					const int columnA = 110;
					const int columnB = 180;
					const int columnC = 250;
					std::string leftText = "";
					if (i == 0) leftText = "Chassis A:";
					if (i == 1) ImGui::SameLine(columnB);
					if (i == 2) ImGui::SameLine(columnC);
					if (i == 3) { leftText = "Chassis B:"; ImGui::SetItemTooltip("Chassis radius is only used by the triangle joint car."); }
					if (i == 4) ImGui::SameLine(columnB);
					if (i == 5) leftText = "Back wheel:";
					if (i == 6) ImGui::SameLine(columnB);
					if (i == 7) ImGui::SameLine(columnC);
					if (i == 8) leftText = "Front wheel:";
					if (i == 9) ImGui::SameLine(columnB);
					if (i == 10) ImGui::SameLine(columnC);
					if (i == 11) leftText = "Engine power:";
					if (i == 12) ImGui::SameLine(columnC);
					if (i == 13) leftText = "Spring:";
					if (i == 14) ImGui::SameLine(columnC);
					if (i == 15) leftText = "Density:";
					if (i == 16) ImGui::SameLine(columnC);

					if (i == 5)
					{
						ImGui::SameLine();
						ImGui::Text("   <-- fixture box cars only");
					}

					if (!leftText.empty())
					{
						ImGui::Text(leftText.c_str());
						ImGui::SameLine(columnA);
					}
					if (ImGui::Checkbox(geneNames[i], &tempBool))
					{
						settings.geneInterpreter.SetGeneEnabled((GeneID)i, tempBool);
						edited = true;
					}

				}

				if (settings.geneInterpreter.GetNumGenesEnabled() > 0)
				{
					ImGui::SeparatorText("Gene ranges");
					const GeneInterpreter& in = settings.geneInterpreter;
					if (in.GetGeneEnabled(GeneID::Chassis0_X) ||
						in.GetGeneEnabled(GeneID::Chassis1_X))
					{
						edited |= ImGui::DragFloatRange2("Chassis X",
							&settings.geneInterpreter.minChassisX,
							&settings.geneInterpreter.maxChassisX, 0.1f, 0, 50, "Min: %.2f", "Max: %.2f");
					}
					if (in.GetGeneEnabled(GeneID::Chassis0_Y) ||
						in.GetGeneEnabled(GeneID::Chassis1_Y))
					{
						edited |= ImGui::DragFloatRange2("Chassis Y",
							&settings.geneInterpreter.minChassisY,
							&settings.geneInterpreter.maxChassisY, 0.1f, 0, 50, "Min: %.2f", "Max: %.2f");
					}
					if (in.GetGeneEnabled(GeneID::Wheel0_X) ||
						in.GetGeneEnabled(GeneID::Wheel1_X) ||
						in.GetGeneEnabled(GeneID::Wheel0_Y) ||
						in.GetGeneEnabled(GeneID::Wheel1_Y))
					{
						edited |= ImGui::DragFloatRange2("Wheel X",
							&settings.geneInterpreter.minWheelX,
							&settings.geneInterpreter.maxWheelX, 0.1f, 0, 50, "Min: %.2f", "Max: %.2f");
					}
					if (in.GetGeneEnabled(GeneID::Wheel0_Y) ||
						in.GetGeneEnabled(GeneID::Wheel1_Y))
					{
						edited |= ImGui::DragFloatRange2("Wheel Y",
							&settings.geneInterpreter.minWheelY,
							&settings.geneInterpreter.maxWheelY, 0.1f, 0, 50, "Min: %.2f", "Max: %.2f");
					}
					if (in.GetGeneEnabled(GeneID::Chassis0_Radius))
					{
						edited |= ImGui::DragFloatRange2("Chassis Radius",
							&settings.geneInterpreter.minChassisRadius,
							&settings.geneInterpreter.maxChassisRadius, 0.05f, 0, 10, "Min: %.2f", "Max: %.2f");
					}
					if (in.GetGeneEnabled(GeneID::Wheel0_Radius) ||
						in.GetGeneEnabled(GeneID::Wheel1_Radius))
					{
						edited |= ImGui::DragFloatRange2("Wheel Radius",
							&settings.geneInterpreter.minWheelRadius,
							&settings.geneInterpreter.maxWheelRadius, 0.05f, 0, 10, "Min: %.2f", "Max: %.2f");
					}
					if (in.GetGeneEnabled(GeneID::AngularImpulsePower))
					{
						edited |= ImGui::DragFloatRange2("Angular Impulse Power",
							&settings.geneInterpreter.minAngularImpulse,
							&settings.geneInterpreter.maxAngularImpulse, 1, 1, 1000, "Min: %.2f", "Max: %.2f");
					}
					if (in.GetGeneEnabled(GeneID::JointMotorTorque))
					{
						edited |= ImGui::DragFloatRange2("Joint Motor Torque",
							&settings.geneInterpreter.minJointMotorTorque,
							&settings.geneInterpreter.maxJointMotorTorque, 1, 1, 1000, "Min: %.2f", "Max: %.2f");
					}
					if (in.GetGeneEnabled(GeneID::ChassisDensity))
					{
						edited |= ImGui::DragFloatRange2("Chassis Density",
							&settings.geneInterpreter.minChassisDensity,
							&settings.geneInterpreter.maxChassisDensity, 0.1f, 0, 100, "Min: %.2f", "Max: %.2f");
					}
					if (in.GetGeneEnabled(GeneID::WheelDensity))
					{
						edited |= ImGui::DragFloatRange2("Wheel Density",
							&settings.geneInterpreter.minWheelDensity,
							&settings.geneInterpreter.maxWheelDensity, 0.1f, 0, 100, "Min: %.2f", "Max: %.2f");
					}
					if (in.GetGeneEnabled(GeneID::WheelFriction))
					{
						edited |= ImGui::DragFloatRange2("Wheel Friction",
							&settings.geneInterpreter.minWheelFriction,
							&settings.geneInterpreter.maxWheelFriction, 1, 0, 100, "Min: %.2f", "Max: %.2f");
					}
					if (in.GetGeneEnabled(GeneID::SpringFreq))
					{
						edited |= ImGui::DragFloatRange2("Spring Frequency",
							&settings.geneInterpreter.minSpringFreq,
							&settings.geneInterpreter.maxSpringFreq, 0.1f, 0, 100, "Min: %.2f", "Max: %.2f");
					}
					if (in.GetGeneEnabled(GeneID::SpringDampingRatio))
					{
						edited |= ImGui::DragFloatRange2("Spring Damping Ratio",
							&settings.geneInterpreter.minSpringDampingRatio,
							&settings.geneInterpreter.maxSpringDampingRatio, 0.1f, 0, 100, "Min: %.2f", "Max: %.2f");
					}
					if (in.GetGeneEnabled(GeneID::Gravity))
					{
						edited |= ImGui::DragFloatRange2("Gravity",
							&settings.geneInterpreter.minGravity,
							&settings.geneInterpreter.maxGravity, 1, 0, 1000, "Min: %.2f", "Max: %.2f");
					}
				}
				if (settings.geneInterpreter.GetNumGenesEnabled() > 0)
				{
					ImGui::SeparatorText("Fitness texture");
					edited |= ImGui::SliderInt("Texture resolution", (int*)&settings.geneInterpreter.fitnessTextureSize, 0, 1024);
					ImGui::SetItemTooltip("The larger the fitness texture resolution, the larger the solution space gets.");
					int zoom = fitnessTexture.GetZoom();
					if (ImGui::SliderInt("Texture zoom", &zoom, 1, 4))
					{
						fitnessTexture.SetZoom(zoom);
					}
					ImGui::Text(ig::ToString("Texture dimensions: ", settings.geneInterpreter.GetNumGenesEnabled()).c_str());
					ImGui::SeparatorText("Current genome");
					if (settings.geneInterpreter.fitnessTextureSize > 0)
					{
						ImGui::PushItemWidth(40);
						for (size_t i = 0; i < genome.values.size(); i++)
						{
							if (i != 0 && i != 9) ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
							int temp = genome.values[i];
							if (ImGui::SliderInt(ig::ToString("##genome", i).c_str(), &temp, 0, settings.geneInterpreter.fitnessTextureSize - 1))
							{
								genome.values[i] = (uint16_t)temp;
								RespawnCar();
								RespawnTerrain();
							}
							if (!edited)
							{
								GeneID currentGeneID = settings.geneInterpreter.GetGeneID(i);
								std::string geneName = std::string(geneNames[(uint32_t)currentGeneID]);
								geneName.erase(std::remove(geneName.begin(), geneName.end(), '#'), geneName.end());
								ImGui::SetItemTooltip(ig::ToString(geneName, " = ",
									settings.geneInterpreter.InterpretGene(temp, currentGeneID)).c_str());
							}
						}
						ImGui::PopItemWidth();
					}
				}
				if (edited)
				{
					GA.Unload();
					if (appState == AppState::GeneticAlgorithm)
					{
						appState = AppState::Auto;
						statusText = "";
					}
					genome.Reset(0);
					fitnessTexture.DelayedUnload(context);
					RespawnCar();
					RespawnTerrain();
				}
				if (settings.geneInterpreter.GetNumGenesEnabled() > 0)
				{
					if (ImGui::Button("Brute force solution space"))
					{
						genome.Reset(0);
						fitnessTexture.DelayedUnload(context);
						ResetWorld();
						RespawnCar();
						RespawnTerrain();
						terrainIndex = 0;
						appState = AppState::BruteForce;
						bestGenome = genome;
						isPaused = false;
					}
					if (fitnessTexture.IsLoaded() &&
						fitnessTexture.GetNumDimensions() >= 1 &&
						fitnessTexture.GetNumDimensions() <= 3)
					{
						ImGui::SameLine();
						if (ImGui::Button("Clear fitness texture"))
						{
							fitnessTexture.ClearPixelValues();
						}
					}
					if (appState == AppState::BruteForce)
					{
						ImGui::ProgressBar(float(fitnessTexture.GetPixelIndex(genome)) / float(fitnessTexture.GetTotalPixelCount()));
					}
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Genetic algorithm"))
			{
				ImGui::SeparatorText("Algorithm parameters");
				ImGui::InputInt("Seed##GA", (int*)&settings.GA.seed);
				ImGui::PushItemWidth(98);
				ImGui::DragInt("Generations", (int*)&settings.numGenerations, 1, 1, 10000);
				ImGui::DragInt("Population size", (int*)&settings.GA.popSize, 1, 1, 10000);
				ImGui::DragInt("Elites", (int*)&settings.GA.eliteCount, 1, 0, 100);
				ImGui::DragFloat("Mutation probability", &settings.GA.mutationProb, 0.05f, 0, 1);
				ImGui::DragFloat("Mutation size", &settings.GA.mutationStrength, 0.05f, 0, 1);
				ImGui::DragFloat("Crossover probability", &settings.GA.crossoverProb, 0.05f, 0, 1);
				ImGui::PopItemWidth();
				if (ImGui::Button("Run genetic algorithm"))
				{
					if (GA.Load(settings.GA, settings.geneInterpreter.GetNumGenesEnabled(),
						settings.geneInterpreter.fitnessTextureSize))
					{
						index_GA = 0;
						genome = GA.GetIndividual(0);
						terrainIndex = 0;
						appState = AppState::GeneticAlgorithm;
						bestGenome = genome;
						isPaused = false;
						statusText = "Evolution in progress...\n";
						avarageFitness_GA.clear();
						bestFitness_GA.clear();
						ResetWorld();
						RespawnCar();
						RespawnTerrain();
					}
					else
					{
						statusText = "Unable to run genetic algorithm. Have you enabled atleast 1 gene?\n";
					}
				}
				ImGui::SeparatorText("Population");
				ImGui::Text(ig::ToString("Generation: ", GA.GetCurrentGeneration()).c_str());
				ImGui::Text(ig::ToString("Size: ", GA.GetPopulationSize()).c_str());
				if (GA.IsLoaded())
				{
					ImGui::Text(ig::ToString("Avarage fitness: ", GA.GetAvarageFitness()).c_str());
					ImGui::Text(ig::ToString("Best fitness: ", GA.GetBestIndividual().ToString()).c_str());
				}
				if (appState == AppState::GeneticAlgorithm)
				{
					ImGui::Text(ig::ToString("Evaluating fitness ", index_GA, "/", GA.GetPopulationSize()).c_str());
					ImGui::ProgressBar(float(GA.GetCurrentGeneration()) / float(settings.numGenerations));
				}

				if (ImPlot::BeginPlot("Fitness plot"))
				{
					ImPlot::SetupAxesLimits(0, settings.numGenerations, 0, 255, ImPlotCond_Always);
					ImPlot::SetupAxes("Generation", "Fitness", 0, 0);
					ImPlot::PlotLine("Avarage fitness", avarageFitness_GA.data(), (int)avarageFitness_GA.size());
					ImPlot::PlotLine("Best fitness", bestFitness_GA.data(), (int)bestFitness_GA.size());
					ImPlot::EndPlot();
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Simulation"))
			{
				ImGui::SeparatorText("iglo");
				if (ImGui::SliderInt("Physics frames per second", (int*)&settings.fixedPhysicsFramerate, 10, 240))
				{
					ApplyIgloSettings();
				}
				if (ImGui::SliderInt("Max physics frames in a row", (int*)&settings.maxPhysicsFramesInARow, 1, 10))
				{
					ApplyIgloSettings();
				}
				ImGui::SeparatorText("Box2D");
				ImGui::InputFloat("Time step", &settings.timeStep, 1);
				ImGui::InputInt("Velocity steps", (int*)&settings.velocitySteps);
				ImGui::InputInt("Position steps", (int*)&settings.positionSteps);
				if (ImGui::InputFloat("Gravity", &settings.gravity, 1))
				{
					RespawnCar();
				}
				ImGui::Text(ig::ToString("Box2D body count: ", world->GetBodyCount()).c_str());
				ImGui::Text(ig::ToString("Box2D contact count: ", world->GetContactCount()).c_str());
				ImGui::SeparatorText("Car simulation");
				ImGui::InputInt("Max physics frames per car", (int*)&settings.numFramesPerSimulation, 100);
				ImGui::SetItemTooltip("How much time each car has before test ends.");
				ImGui::Checkbox("Power back wheel", &settings.powerBackWheel);
				ImGui::Checkbox("Power front wheel", &settings.powerFrontWheel);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Graphics"))
			{
				ImGui::SeparatorText(ig::ToString("iglo v" IGLO_VERSION_STRING " " IGLO_GRAPHICS_API_STRING).c_str());
				const char* presentModeItems[] = { "Immediate With Tearing", "Immediate", "Vsync" };
				if (ImGui::Combo("Present mode", (int*)&settings.presentMode, presentModeItems, IM_ARRAYSIZE(presentModeItems)))
				{
					ApplyIgloSettings();
				}
				if (ImGui::SliderInt("Frame rate limit", (int*)&settings.frameRateLimit, 0, 500))
				{
					ApplyIgloSettings();
				}

				static float fpsHistory[60] = {};
				static unsigned int currentFpsIndex = 0;
				fpsHistory[currentFpsIndex] = (float)mainloop.GetFPS();
				currentFpsIndex++;
				if (currentFpsIndex >= 60) currentFpsIndex = 0;
				ImGui::PlotLines(ig::ToString("FPS: ", mainloop.GetAvarageFPS()).c_str(), fpsHistory, IM_ARRAYSIZE(fpsHistory));
				ImGui::SeparatorText("Visuals");

				ImGui::InputFloat("Camera zoom", &settings.cameraScale, 1);
				ig::Color floatColor = ig::Color(settings.clearColor);
				float colors[4] = { floatColor.red, floatColor.green, floatColor.blue, floatColor.alpha };
				if (ImGui::ColorEdit4("Background color", colors))
				{
					floatColor = ig::Color(colors[0], colors[1], colors[2], colors[3]);
					settings.clearColor = ig::Color32(
						byte(floatColor.red * 255.f),
						byte(floatColor.green * 255.f),
						byte(floatColor.blue * 255.f),
						byte(floatColor.alpha * 255.f));
				}

				ImGui::SeparatorText("Save solution space texture");
				{
					uint32_t numDimensions = fitnessTexture.GetNumDimensions();
					if (fitnessTexture.IsLoaded())
					{
						ImGui::Text(ig::ToString("Number of dimensions: ", numDimensions).c_str());
					}

					static bool saveSuccess = 0;
					const std::string destFolder = Constants::savedImagesDirectory;
					if (numDimensions == 1 || numDimensions == 2 || numDimensions == 3)
					{
						const char* strSingle = "Save solution space as .png";
						const char* strMulti = "Save solution space as .png sequence";
						uint32_t numImages = 1;
						if (numDimensions == 3) numImages = fitnessTexture.GetTextureSize();
						ImGui::Text(ig::ToString("Number of images to be saved: ", numImages).c_str());
						if (ImGui::Button((numDimensions == 3) ? strMulti : strSingle))
						{
							saveSuccess = fitnessTexture.SaveToFile(destFolder, "image");
							ImGui::OpenPopup("Save Image Result");
						}
					}
					else
					{
						ImGui::Text("Must be 1, 2 or 3 dimensions to be able to save as .png.");
					}
					if (ImGui::Button("Open saved images folder"))
					{
						if (!ig::DirectoryExists(destFolder))
						{
							ig::CreateDirectory(destFolder);
						}
						std::string fullpath = ig::GetCurrentPath() + "/" + destFolder;
						ShellExecuteA(context.GetWindowHWND(), "open", fullpath.c_str(), 0, 0, SW_SHOWNORMAL);
					}

					ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
					if (ImGui::BeginPopupModal("Save Image Result", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
					{
						const std::string strFailed = ig::ToString("Failed to save image(s) to ", destFolder).c_str();
						const std::string strSuccess = ig::ToString("Successfully saved image to ", destFolder).c_str();
						const std::string strSuccessMulti = ig::ToString("Successfully saved image sequence to ", destFolder).c_str();
						if (saveSuccess)
						{
							if (numDimensions == 3)
							{
								ImGui::Text(strSuccessMulti.c_str());
							}
							else
							{
								ImGui::Text(strSuccess.c_str());
							}
						}
						else
						{
							ImGui::Text(strFailed.c_str());
						}
						if (ImGui::Button("OK", ImVec2(120, 0)))
						{
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndPopup();
					}
				}

				ImGui::EndTabItem();
			}
			ImGui::PopItemWidth();
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

bool App::DarkModeEnabled()
{
	return (settings.clearColor.red + settings.clearColor.green + settings.clearColor.blue < 255);
}

void App::Draw()
{
	// Ensure render target clear color matches settings
	if (renderTargetCurrentClearColor != settings.clearColor)
	{
		context.WaitForIdleDevice();
		renderTargetCurrentClearColor = settings.clearColor;
		LoadRenderTarget();
		ig::Print(ig::ToString("Updated clear color to ", settings.clearColor.rgba, "\n"));
	}

	cmd.Begin();
	{
		cmd.AddTextureBarrier(renderTarget, ig::SimpleBarrier::Discard, ig::SimpleBarrier::RenderTarget);
		cmd.FlushBarriers();

		cmd.SetRenderTarget(&renderTarget);
		cmd.SetViewport((float)renderTarget.GetWidth(), (float)renderTarget.GetHeight());
		cmd.SetScissorRectangle(renderTarget.GetWidth(), renderTarget.GetHeight());
		cmd.ClearColor(renderTarget, renderTargetCurrentClearColor);

		batchRendererMSAA.Begin(cmd);
		{
			// Setup camera
			ig::FloatRect cameraRect = ig::FloatRect(
				-(GetCameraWidth() * 0.5f),
				-(GetCameraHeight() * 0.5f),
				(GetCameraWidth() * 0.5f),
				(GetCameraHeight() * 0.5f));
			cameraRect += cameraPos;

			ig::Matrix4x4 view = ig::Matrix4x4::LookToRH(
				ig::Vector3(cameraPos.x, cameraPos.y, 10),
				ig::Vector3(0, 0, -1),
				ig::Vector3(0, 1, 0)); // Positive Y is up

			ig::Matrix4x4 projection = ig::Matrix4x4::OrthoRH((float)GetCameraWidth(), (float)GetCameraHeight(), 0.1f, 100.0f);
			batchRendererMSAA.SetViewAndProjection(view, projection);

			// Draw terrain
			terrain.Draw(batchRendererMSAA, cameraRect);

			// Draw car
			car.Draw(batchRendererMSAA, cameraRect, (appState != AppState::Auto && appState != AppState::Game),
				settings.cameraScale, DarkModeEnabled());
		}
		batchRendererMSAA.End();

		cmd.AddTextureBarrier(renderTarget, ig::SimpleBarrier::RenderTarget, ig::SimpleBarrier::ResolveSource);
		cmd.AddTextureBarrier(context.GetBackBuffer(), ig::SimpleBarrier::Discard, ig::SimpleBarrier::ResolveDest);
		cmd.FlushBarriers();

		cmd.ResolveTexture(renderTarget, context.GetBackBuffer());

		cmd.AddTextureBarrier(context.GetBackBuffer(), ig::SimpleBarrier::ResolveDest, ig::SimpleBarrier::RenderTarget);
		cmd.FlushBarriers();

		cmd.SetRenderTarget(&context.GetBackBuffer());
		cmd.SetViewport((float)context.GetWidth(), (float)context.GetHeight());
		cmd.SetScissorRectangle(context.GetWidth(), context.GetHeight());

		batchRenderer.Begin(cmd);
		{
			// Draw fitness texture
			fitnessTexture.SetCurrentGenome(genome);
			fitnessTexture.Draw(cmd, batchRenderer, defaultFont);

			// Draw text
			std::string text;
			text.append(statusText);
			text.append("Best score: " + bestGenome.ToString() + "\n");
			if (appState == AppState::Game)
			{
				text.append("Current score: " + genome.ToString() + "\n");
			}
			text.append(ig::ToString("Dimension(s): ", fitnessTexture.GetNumDimensions(), " , Resolution: ", fitnessTexture.GetTextureSize(), "\n"));
			if (fitnessTexture.HasHighlightedGenome())
			{
				Genome highlighted = fitnessTexture.GetHighlightedGenome();
				text.append("Selected car score: " + highlighted.ToString() + "\n");
			}
			ig::Color32 strColor = Constants::textColor;
			if (DarkModeEnabled()) strColor = Constants::textColorDarkMode;
			batchRenderer.DrawString(18, 5, text, vegurBold, strColor);
		}
		batchRenderer.End();

		// Draw imgui
		UpdateImgui();
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd.GetD3D12GraphicsCommandList());

		cmd.AddTextureBarrier(context.GetBackBuffer(), ig::SimpleBarrier::RenderTarget, ig::SimpleBarrier::Present);
		cmd.FlushBarriers();
	}
	cmd.End();

	context.Submit(cmd);
	context.Present();
}

void App::ApplyIgloSettings()
{
	context.SetPresentMode((ig::PresentMode)settings.presentMode);
	mainloop.SetFixedUpdateFrameRate((double)settings.fixedPhysicsFramerate, settings.maxPhysicsFramesInARow);
	mainloop.SetFrameRateLimit((double)settings.frameRateLimit);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShow)
{
	std::unique_ptr<App> app = std::make_unique<App>();
	app->Run();
	app = nullptr;
	return 0;
}
