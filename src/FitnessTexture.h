#pragma once

using CallbackOnFitnessTextureMouseDown = std::function<void(const Genome&)>;
using CallbackOnFitnessTextureMouseUp = std::function<void()>;

class FitnessTexture
{
public:
	FitnessTexture() = default;
	~FitnessTexture() { Unload(); };

	// Make class non-copyable
	FitnessTexture(FitnessTexture&) = delete;
	FitnessTexture& operator=(FitnessTexture&) = delete;

	// Returns true if success
	bool Load(const ig::IGLOContext&, ig::IntPoint location, const GeneInterpreter&,
		CallbackOnFitnessTextureMouseDown callbackOnMouseDown,
		CallbackOnFitnessTextureMouseUp callbackOnMouseUp);
	void Unload();
	bool IsLoaded() const { return isLoaded; }

	// Calling Load() will replace the existing texture if already loaded,
	// which risks a crash if the GPU still needs it in a previous frame.
	// The solution is to call this function before calling Load(),
	// this way the old texture remains alive somewhere until the GPU is done with it.
	void DelayedUnload(const ig::IGLOContext&);

	void SetCurrentGenome(const Genome& currentGenome);

	// Returns true if event is handled
	bool OnEvent(ig::Event e);
	void Draw(ig::CommandList& cmd, ig::BatchRenderer& r, ig::Font& font);

	uint32_t GetNumDimensions() const { return numDimensions; }
	uint32_t GetTextureSize() const { return textureSize; }
	size_t GetTotalPixelCount()  const { return totalNumPixels; }
	Genome GetHighlightedGenome() const { return highlightedGenome; }
	bool HasHighlightedGenome() const { return mouseOverTexture || mouseOverDepthSlider; }
	int GetZoom() const { return zoom; }
	void SetZoom(int zoom);

	// Converts the coordinates of a genome to a pixel array index.
	size_t GetPixelIndex(const Genome&) const;

	// Return true if genome is a coordinate that points to the last pixel in this fitness texture.
	// In other words, returns true if there are no more genomes to brute force after this once.
	bool IsLastPixel(const Genome&) const;

	// Increases the pixel coord of given genome by 1 (the genome is modified).
	void StepGenomeToNextPixel(Genome& in_out_genome);

	void SetPixel(const Genome& coord, byte pixelValue);
	void ClearPixelValues();

	bool SaveToFile(const std::string& destFolderPath, const std::string& name);
private:
	bool isLoaded = false;

	std::vector<std::shared_ptr<ig::Texture>> textures; // Multiple textures only if three dimensional.
	std::vector<byte> pixels; // The pixel values of the fitness texture

	int zoom = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t depth = 0;
	uint32_t numDimensions = 0;
	uint32_t textureSize = 0; // Size of a texture dimension
	ig::IntPoint textureLocation; // Where to draw the fitness texture
	size_t totalNumPixels = 0;
	Genome currentGenome;

	// Mouse events
	Genome highlightedGenome;
	bool mouseOverTexture = false;
	bool mouseOverDepthSlider = false;
	bool textureIsPressed = false;
	bool depthSliderIsPressed = false;
	CallbackOnFitnessTextureMouseDown callbackOnMouseDown = nullptr;
	CallbackOnFitnessTextureMouseUp callbackOnMouseUp = nullptr;

	bool OnMouseMove(int32_t x, int32_t y);
	static size_t CalculateTotalPixelCount(uint32_t numDimensions, size_t textureSize);
};