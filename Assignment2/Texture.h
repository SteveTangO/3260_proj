#pragma once

class Texture 
{
public:
	void setupTexture(const char* texturePath);
	void bind(unsigned int slot) const;
	void unbind() const;
    unsigned int ID = 0;

private:
	int Width = 0, Height = 0, BPP = 0;
};
