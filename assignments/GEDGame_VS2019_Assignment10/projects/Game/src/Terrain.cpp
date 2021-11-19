#include "Terrain.h"

#include "GameEffect.h"
#include "ConfigParser.h"
#include <DDSTextureLoader.h>
#include "DirectXTex.h"
#include <SimpleImage.h>
#include "debug.h"

// You can use this macro to access your height field
#define IDX(X,Y,WIDTH) ((X) + (Y) * (WIDTH))

Terrain::Terrain(void)
{
}


Terrain::~Terrain(void)
{
}

HRESULT Terrain::create(ID3D11Device* device)
{
	HRESULT hr;

	// Load the heightmap
	GEDUtils::SimpleImage heightmap(g_ConfigParser.get_terrainPathHeight().c_str());

	terrain_vertex_width = heightmap.getWidth();
	uint64_t terrain_vertex_height = heightmap.getHeight();
	// Create the height buffer data
	raw_height_field.resize(terrain_vertex_width * terrain_vertex_height);
	for (uint64_t y = 0; y < terrain_vertex_height; y++)
		for (uint64_t x = 0; x < terrain_vertex_width; x++)
			raw_height_field[IDX(x, y, terrain_vertex_width)] = heightmap.getPixel(x, y);

	D3D11_SUBRESOURCE_DATA hid;
	hid.pSysMem = static_cast<void*>(raw_height_field.data());
	hid.SysMemPitch = sizeof(float); // Stride
	hid.SysMemSlicePitch = 0;

	D3D11_BUFFER_DESC hbd;
	hbd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	hbd.ByteWidth = sizeof(float) * raw_height_field.size(); //The size in bytes of the triangle array
	hbd.CPUAccessFlags = 0;
	hbd.MiscFlags = 0;
	hbd.Usage = D3D11_USAGE_DEFAULT;

	V(device->CreateBuffer(&hbd, &hid, &heightfield)); // http://msdn.microsoft.com/en-us/library/ff476899%28v=vs.85%29.aspx
	
	// Create the SRV for the height field
	D3D11_SHADER_RESOURCE_VIEW_DESC hsrvd;
	hsrvd.Buffer.FirstElement = 0;
	hsrvd.Buffer.NumElements = raw_height_field.size();
	hsrvd.Format = DXGI_FORMAT_R32_FLOAT;
	hsrvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	
	V(device->CreateShaderResourceView(heightfield, &hsrvd, &heightfieldSRV));

	// Create index buffer
	raw_index_buffer.resize((terrain_vertex_width - 1) * (terrain_vertex_height - 1) * 2);
	// Iterate through every square
	uint64_t i = 0;
	for (uint64_t y = 0; y < terrain_vertex_height - 1; y++)
		for (uint64_t x = 0; x < terrain_vertex_width - 1; x++)
		{
			// Upper Left
			raw_index_buffer[i].f = IDX(x, y, terrain_vertex_width);
			raw_index_buffer[i].s = IDX(x + 1, y, terrain_vertex_width);
			raw_index_buffer[i].t = IDX(x, y + 1, terrain_vertex_width);
			i++;
			// Lower Right
			raw_index_buffer[i].f = IDX(x + 1, y, terrain_vertex_width);
			raw_index_buffer[i].s = IDX(x + 1, y + 1, terrain_vertex_width);
			raw_index_buffer[i].t = IDX(x, y + 1, terrain_vertex_width);
			i++;
		}

	D3D11_SUBRESOURCE_DATA iid;
	iid.pSysMem = static_cast<void*>(raw_index_buffer.data());
	iid.SysMemPitch = 0;
	iid.SysMemSlicePitch = 0;

	D3D11_BUFFER_DESC ibd;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.ByteWidth = sizeof(TerrainTriangleIndex) * raw_index_buffer.size();
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.Usage = D3D11_USAGE_DEFAULT;

	V(device->CreateBuffer(&ibd, &iid, &indexBuffer)); // http://msdn.microsoft.com/en-us/library/ff476899%28v=vs.85%29.aspx


	// Load the color texture (color map)
	std::wstring color_path(g_ConfigParser.get_terrainPathColor().begin(), g_ConfigParser.get_terrainPathColor().end());
	DirectX::CreateDDSTextureFromFile(device, color_path.c_str(), nullptr, &diffuseTextureSRV);
	// Load the normal map
	std::wstring normal_path(g_ConfigParser.get_terrainPathNormal().begin(), g_ConfigParser.get_terrainPathNormal().end());
	DirectX::CreateDDSTextureFromFile(device, normal_path.c_str(), nullptr, &normalTextureSRV);


	return hr;
}


void Terrain::destroy()
{
	SAFE_RELEASE(indexBuffer);

	SAFE_RELEASE(heightfield);
	SAFE_RELEASE(heightfieldSRV);
	SAFE_RELEASE(diffuseTexture);
	SAFE_RELEASE(diffuseTextureSRV);
	SAFE_RELEASE(normalTexture);
	SAFE_RELEASE(normalTextureSRV);
}


void Terrain::render(ID3D11DeviceContext* context, ID3DX11EffectPass* pass)
{
	HRESULT hr;

	// Bind the terrain vertex buffer to the input assembler stage 
	ID3D11Buffer* vbs[] = { nullptr, };
	unsigned int strides[] = { 0, }, offsets[] = { 0, };
	context->IASetVertexBuffers(0, 1, vbs, strides, offsets);

	// Bind the index Buffer
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Tell the input assembler stage which primitive topology to use
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Bind the textures
	V(g_gameEffect.heightEV->SetResource(heightfieldSRV));
	V(g_gameEffect.diffuseEV->SetResource(diffuseTextureSRV));
	V(g_gameEffect.normalEV->SetResource(normalTextureSRV));
	V(g_gameEffect.resolutionEV->SetInt(static_cast<int>(terrain_vertex_width)));

	// Apply the rendering pass in order to submit the necessary render state changes to the device
	V(pass->Apply(0, context));

	// Draw
	context->DrawIndexed(raw_index_buffer.size() * 3, 0, 0);
}

float Terrain::get_height_at(float x, float z) const
{
	if (raw_height_field.size() == 0)
	{
		std::cerr << "ERROR: Terrain was not loaded when accessing its height values" << std::endl;
		return 0.0f;
	}

	double u = x / g_ConfigParser.get_TerrainWidth() + 0.5;
	u = std::min(static_cast<uint64_t>(u * terrain_vertex_width), terrain_vertex_width - 1);
	double v = z / g_ConfigParser.get_TerrainDepth() + 0.5;
	v = std::min(static_cast<uint64_t>(v * terrain_vertex_width), terrain_vertex_width - 1);

	// Without interpolation
  	return raw_height_field.at(IDX(u, v, terrain_vertex_width)) * g_ConfigParser.get_TerrainHeight();
}
