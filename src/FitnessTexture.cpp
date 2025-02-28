#include "iglo.h"
#include "igloBatchRenderer.h"
#include "box2d.h"
#include "GeneticAlgorithm.h"
#include "Car.h"
#include "GeneInterpreter.h"
#include "FitnessTexture.h"

constexpr uint32_t DepthSliderHeight = 8;

void FitnessTexture::Unload()
{
	isLoaded = false;
	textures.clear();
	pixels.clear();

	zoom = 0;
	width = 0;
	height = 0;
	depth = 0;
	numDimensions = 0;
	textureSize = 0;
	textureLocation = ig::IntPoint();
	totalNumPixels = 0;
	currentGenome.Reset(0);

	highlightedGenome.Reset(0);
	mouseOverTexture = false;
	mouseOverDepthSlider = false;
	textureIsPressed = false;
	depthSliderIsPressed = false;
	callbackOnMouseDown = nullptr;
	callbackOnMouseUp = nullptr;
}

void FitnessTexture::DelayedUnload(const ig::IGLOContext& context)
{
	for (size_t i = 0; i < textures.size(); i++)
	{
		if (textures[i])
		{
			if (textures[i]->IsLoaded())
			{
				context.DelayedTextureUnload(textures[i]);
			}
			textures[i] = nullptr;
		}
	}
	Unload();
}

bool FitnessTexture::Load(const ig::IGLOContext& context, ig::IntPoint location, const GeneInterpreter& geneInterpreter,
	CallbackOnFitnessTextureMouseDown callbackOnMouseDown, CallbackOnFitnessTextureMouseUp callbackOnMouseUp)
{
	// Reset values to default first
	Unload();

	this->numDimensions = geneInterpreter.GetNumGenesEnabled();
	this->currentGenome.Reset(geneInterpreter.GetNumGenesEnabled());
	this->textureSize = geneInterpreter.fitnessTextureSize;
	this->totalNumPixels = CalculateTotalPixelCount(this->numDimensions, this->textureSize);
	this->textureLocation = location;
	this->callbackOnMouseDown = callbackOnMouseDown;
	this->callbackOnMouseUp = callbackOnMouseUp;
	this->zoom = 1;
	if (this->textureSize <= 128) this->zoom = 2;
	if (this->textureSize <= 64) this->zoom = 4;

	// Display a texture only if the number of dimensions are appropriate
	if (this->textureSize > 0 && this->numDimensions >= 1 && this->numDimensions <= 3)
	{
		this->width = this->textureSize;
		this->height = 1;
		this->depth = 1;
		if (this->numDimensions >= 2) this->height = this->textureSize;
		if (this->numDimensions >= 3) this->depth = this->textureSize;
		this->pixels.clear();
		this->pixels.resize(this->totalNumPixels, 0);

		this->textures.clear();
		this->textures.shrink_to_fit();
		for (uint32_t i = 0; i < this->depth; i++)
		{
			this->textures.push_back(std::make_shared<ig::Texture>());
			// Use a monochrome format for the fitness textures.
			if (!this->textures[i]->Load(context, this->width, this->height, ig::Format::BYTE, ig::TextureUsage::Default))
			{
				Unload();
				return false;
			}
		}
	}

	this->isLoaded = true;
	return true;
}

bool FitnessTexture::OnMouseMove(int32_t x, int32_t y)
{
	if (!isLoaded) return false;
	if (width == 0) return false;
	if (height == 0) return false;

	// Zoom
	int32_t x_translated = ((x - textureLocation.x) / (int)zoom) + (textureLocation.x);
	int32_t y_translated = ((y - textureLocation.y) / (int)zoom) + (textureLocation.y);

	ig::IntRect textureRect = ig::IntRect(0, 0, width, height) + textureLocation;
	ig::IntRect depthSliderRect(textureRect.left, textureRect.bottom, textureRect.right, textureRect.bottom + (DepthSliderHeight / zoom));

	int selectedX = x_translated - textureRect.left;
	int selectedY = y_translated - textureRect.top;
	if (selectedX > int(width) - 1) selectedX = width - 1;
	if (selectedY > int(height) - 1) selectedY = height - 1;
	if (selectedX < 0) selectedX = 0;
	if (selectedY < 0) selectedY = 0;

	mouseOverTexture = false;
	mouseOverDepthSlider = false;

	// When continuously pressed, will lock onto mouse
	if (!depthSliderIsPressed && (textureRect.ContainsPoint(ig::IntPoint(x_translated, y_translated)) || textureIsPressed))
	{
		highlightedGenome.Reset(numDimensions);
		if (numDimensions >= 1)
		{
			highlightedGenome.values[0] = (uint16_t)selectedX;
		}
		if (numDimensions >= 2)
		{
			highlightedGenome.values[1] = (uint16_t)selectedY;
		}
		if (numDimensions >= 3)
		{
			highlightedGenome.values[2] = (uint16_t)currentGenome.values.at(2);
		}
		highlightedGenome.fitness = pixels.at(GetPixelIndex(highlightedGenome));
		mouseOverTexture = true;
		return true;
	}
	else if (!textureIsPressed && (depthSliderRect.ContainsPoint(ig::IntPoint(x_translated, y_translated)) || depthSliderIsPressed))
	{
		if (numDimensions == 3)
		{
			highlightedGenome.Reset(numDimensions);
			if (numDimensions >= 1)
			{
				highlightedGenome.values[0] = (uint16_t)currentGenome.values.at(0);
			}
			if (numDimensions >= 2)
			{
				highlightedGenome.values[1] = (uint16_t)currentGenome.values.at(1);
			}
			if (numDimensions >= 3)
			{
				highlightedGenome.values[2] = (uint16_t)selectedX;
			}
			highlightedGenome.fitness = pixels.at(GetPixelIndex(highlightedGenome));
			mouseOverDepthSlider = true;
			return true;
		}
	}

	return false;
}

void FitnessTexture::SetZoom(int zoom)
{
	this->zoom = zoom;
	if (this->zoom > 4) this->zoom = 4;
	if (this->zoom < 1) this->zoom = 1;
}

bool FitnessTexture::OnEvent(ig::Event e)
{
	if (!isLoaded) return false;
	if (textureSize == 0) return false;
	if (numDimensions == 0) return false;

	bool handled = false;

	switch (e.type)
	{
	case ig::EventType::MouseButtonUp:
	case ig::EventType::MouseMove:
	case ig::EventType::MouseButtonDown:
	case ig::EventType::MouseWheel:

		// Mouse up
		if (e.type == ig::EventType::MouseButtonUp && e.mouse.button == ig::MouseButton::Left)
		{
			if (textureIsPressed || depthSliderIsPressed)
			{
				if (callbackOnMouseUp) callbackOnMouseUp();
				textureIsPressed = false;
				depthSliderIsPressed = false;
			}
		}

		handled = OnMouseMove(e.mouse.x, e.mouse.y);

		// Mouse wheel
		if (e.type == ig::EventType::MouseWheel)
		{
			if (e.mouse.scrollWheel > 0) SetZoom(GetZoom() * 2); // Zoom in
			if (e.mouse.scrollWheel < 0) SetZoom(GetZoom() / 2); // Zoom out
			handled = true;
		}

		// Middle mouse down
		if (e.type == ig::EventType::MouseButtonDown && e.mouse.button == ig::MouseButton::Middle)
		{
			if (mouseOverTexture || mouseOverDepthSlider)
			{
				zoom = 1; // Restore zoom
				handled = true;
			}
		}

		// Mouse down
		if (e.type == ig::EventType::MouseButtonDown && e.mouse.button == ig::MouseButton::Left)
		{
			if (mouseOverTexture) textureIsPressed = true;
			else if (mouseOverDepthSlider) depthSliderIsPressed = true;
		}

		// Continuously send mouse down event
		if (textureIsPressed || depthSliderIsPressed)
		{
			if (callbackOnMouseDown) callbackOnMouseDown(highlightedGenome);
			handled = true;
		}
	}
	return handled;
}

void FitnessTexture::SetCurrentGenome(const Genome& currentGenome)
{
	if (!isLoaded) return;
	this->currentGenome = currentGenome;
}

void FitnessTexture::Draw(ig::CommandList& cmd, ig::BatchRenderer& r, ig::Font& font)
{
	if (!isLoaded) return;
	if (numDimensions == 0) return;
	if (textureSize == 0) return;
	if (currentGenome.values.size() != numDimensions)
	{
		throw std::runtime_error("The genome does not match the number of dimensions in the fitness texture.");
	}

	uint32_t depthSlice = 0;
	if (numDimensions >= 3) depthSlice = currentGenome.values.at(2);

	if (textures.size() > 0)
	{
		if (textures.at(depthSlice))
		{
			if (!textures[depthSlice]->IsLoaded()) throw std::runtime_error("This is impossible.");
		}
	}

	if (textures.size() == 0)
	{
		ig::FloatRect box = ig::FloatRect(0, 0, 100, 100) + ig::Vector2((float)textureLocation.x, (float)textureLocation.y);
		ig::Color32 lineColor = ig::Color32(210, 210, 210);
		r.DrawRectangle(box, ig::Color32(225, 225, 225));
		r.DrawRectangularLine(box.left, box.top, box.right, box.bottom, 2, lineColor);
		r.DrawRectangularLine(box.left, box.bottom, box.right, box.top, 2, lineColor);
		std::string str = ig::ToString("Too many\ndimensions to\nvisualize");
		ig::Vector2 strBounds = r.MeasureString(str, font);
		ig::Vector2 strLoc = ig::Vector2(box.left + (box.GetWidth() / 2), box.top + (box.GetHeight() / 2)) - (strBounds / 2);
		// Snap the text position to integer coordinates to prevent text from being blurry
		strLoc.x = float(int(strLoc.x));
		strLoc.y = float(int(strLoc.y));
		r.DrawString(strLoc, str, font, ig::Color32(128, 128, 128));
		return;
	}

	textures[depthSlice]->SetPixels(cmd, &pixels.at((size_t)width * height * depthSlice));

	r.SetWorldMatrix(
		ig::Vector3((float)textureLocation.x, (float)textureLocation.y, 0),
		ig::Quaternion::Identity,
		ig::Vector3(1, 1, 1));

	// Draw the fitness texture
	r.SetSamplerToPixelatedTextures();
	r.DrawTexture(*textures[depthSlice], 0, 0, (float)width * zoom, (float)height * zoom);
	r.SetSamplerToSmoothTextures();

	// Draw a slider showing the Z coordinate of the current genome
	if (numDimensions == 3)
	{
		r.SetWorldMatrix(
			ig::Vector3((float)textureLocation.x, (float)textureLocation.y + (float)height * zoom, 0),
			ig::Quaternion::Identity,
			ig::Vector3(1, 1, 1));

		const float sliderTop = 1.0f;
		const float sliderWidth = (float)width * (float)zoom;
		const float sliderHeight = (float)DepthSliderHeight - sliderTop;
		for (uint32_t i = 0; i < depth; i++)
		{
			Genome temp = currentGenome;
			temp.values.at(2) = i;
			byte fitness = pixels.at(GetPixelIndex(temp));
			r.DrawRectangle((float)i * zoom, sliderTop, (float)zoom, sliderHeight, ig::Color32(fitness, fitness, fitness));
		}

		float arrowX = ((float)currentGenome.values.at(2) * (float)zoom);
		r.DrawRectangle(arrowX, sliderTop, (float)zoom, sliderHeight, ig::Color32(255, 0, 0));
	}

	r.SetWorldMatrix(
		ig::Vector3((float)textureLocation.x, (float)textureLocation.y, 0),
		ig::Quaternion::Identity,
		ig::Vector3(1, 1, 1));

	float halfPixelSize = (float)zoom * 0.5f;

	// Draw a circle showing the XY coordinates of the current genome
	ig::IntPoint circle = ig::IntPoint(0, 0);
	if (numDimensions >= 1) circle.x = currentGenome.values.at(0) * zoom;
	if (numDimensions >= 2) circle.y = currentGenome.values.at(1) * zoom;
	r.DrawCircle((float)circle.x + halfPixelSize, (float)circle.y + halfPixelSize, 5, 1.0f,
		ig::Colors::Transparent, ig::Colors::Transparent, ig::Colors::Red, 1.0f);

	r.RestoreWorldMatrix();
}

size_t FitnessTexture::GetPixelIndex(const Genome& genome) const
{
	size_t pixelIndex = 0;
	for (size_t i = 0; i < genome.values.size(); i++)
	{
		size_t addedPower = genome.values[i];
		for (size_t power = 0; power < i; power++)
		{
			addedPower *= textureSize;
		}
		pixelIndex += addedPower;
	}
	return pixelIndex;
}

size_t FitnessTexture::CalculateTotalPixelCount(uint32_t numDimensions, size_t textureSize)
{
	if (numDimensions == 0) return 0;
	size_t totalNumPixels = 1;
	for (size_t i = 0; i < numDimensions; i++)
	{
		totalNumPixels *= textureSize;
	}
	return totalNumPixels;
}

bool FitnessTexture::IsLastPixel(const Genome& genome) const
{
	if (!isLoaded) return false;
	if (textureSize <= 1) return true;
	for (size_t i = 0; i < genome.values.size(); i++)
	{
		if (genome.values[i] != textureSize - 1) return false;
	}
	return true;
}

void FitnessTexture::StepGenomeToNextPixel(Genome& in_out_genome)
{
	size_t dimension = 0;
	while (true)
	{
		if (dimension < in_out_genome.values.size())
		{
			in_out_genome.values[dimension] += 1;
			if (in_out_genome.values[dimension] >= textureSize)
			{
				in_out_genome.values[dimension] = 0;
				dimension += 1;
			}
			else
			{
				// Successfully stepped to next pixel
				return;
			}
		}
		else
		{
			// Looped back to first pixel
			return;
		}
	}
}

void FitnessTexture::SetPixel(const Genome& coord, byte pixelValue)
{
	if (!isLoaded) return;
	if (pixels.size() == 0) return;

	size_t index = GetPixelIndex(coord);
	if (index >= pixels.size()) throw std::runtime_error("Attempted to set a pixel out of bounds.");
	pixels[index] = pixelValue;
}

void FitnessTexture::ClearPixelValues()
{
	if (!isLoaded) return;

	for (size_t i = 0; i < pixels.size(); i++)
	{
		pixels[i] = 0;
	}
}

bool FitnessTexture::SaveToFile(const std::string& destFolderPath, const std::string& name)
{
	if (!isLoaded) return false;
	if (pixels.size() == 0) return false;

	if (!ig::DirectoryExists(destFolderPath))
	{
		if (!ig::CreateDirectory(destFolderPath))
		{
			ig::Print(ig::ToString("Failed to create directory: ", destFolderPath, "\n"));
			return false;
		}
	}

	for (uint32_t depthSlice = 0; depthSlice < depth; depthSlice++)
	{
		ig::Image image;
		if (!image.LoadAsPointer(&pixels.at((size_t)width * height * depthSlice), width, height, ig::Format::BYTE))
		{
			return false;
		}

		std::string sliceStr = ig::ToString(depthSlice);
		if (depth == 1) sliceStr = "";
		if (!image.SaveToFile(destFolderPath + "/" + name + sliceStr + ".png"))
		{
			return false;
		}
	}

	return true;
}