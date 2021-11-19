#pragma once
#include "DXUT.h"
#include "d3dx11effect.h"
#include <memory>

class Terrain
{
public:
	struct TerrainTriangleIndex
	{
		UINT f, s, t;
	};

	Terrain(void);
	~Terrain(void);

	HRESULT create(ID3D11Device* device);
	void destroy();

	void render(ID3D11DeviceContext* context, ID3DX11EffectPass* pass);

	float get_height_at(float x, float z) const;

private:
	Terrain(const Terrain&);
	Terrain(const Terrain&&);
	void operator=(const Terrain&);

	// Terrain rendering resources
	ID3D11Buffer*                           indexBuffer = nullptr;	// The terrain's triangulation
	ID3D11Buffer*							heightfield = nullptr;
	ID3D11ShaderResourceView*				heightfieldSRV = nullptr;
	ID3D11Texture2D*                        diffuseTexture = nullptr; // The terrain's material color for diffuse lighting
	ID3D11ShaderResourceView*               diffuseTextureSRV = nullptr; // Describes the structure of the diffuse texture to the shader stages
	ID3D11Texture2D*						normalTexture = nullptr;
	ID3D11ShaderResourceView*				normalTextureSRV = nullptr;

	std::vector<float>						raw_height_field;
	std::vector<TerrainTriangleIndex>		raw_index_buffer;
	uint64_t								terrain_vertex_width = 0;
};

