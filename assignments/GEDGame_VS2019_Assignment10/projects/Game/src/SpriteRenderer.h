#pragma once

#include <string>
#include <vector>

#include <DXUT.h>
#include <DXUTcamera.h>

#include <d3dx11effect.h>


struct SpriteVertex
{
	DirectX::XMFLOAT3 position;     // world-space position (sprite center)
	float radius;                   // world-space radius (= half side length of the sprite quad)
	int textureIndex;				// which texture to use (out of SpriteRenderer::m_spriteSRV)
	DirectX::XMVECTOR velocity;		// saves the velocity;

};

class SpriteRenderer
{
public:
	// Constructor: Create a SpriteRenderer with the given list of textures.
	// The textures are *not* be created immediately, but only when create is called!
	SpriteRenderer(const std::vector<std::wstring>& textureFilenames);
	// Destructor does nothing. Destroy and ReleaseShader must be called first!
	~SpriteRenderer();

	// Load/reload the effect. Must be called once before create!
	HRESULT reloadShader(ID3D11Device* pDevice);
	// Release the effect again.
	void releaseShader();

	// Create all required D3D resources (textures, buffers, ...).
	// reloadShader must be called first!
	HRESULT create(ID3D11Device* pDevice);
	// Release D3D resources again.
	void destroy();

	// Render the given sprites. They must already be sorted into back-to-front order.
	void renderSprites(ID3D11DeviceContext* context, const std::vector<SpriteVertex>& sprites, const CFirstPersonCamera& camera);

private:
	std::vector<std::wstring> m_textureFilenames;

	// Rendering effect (shaders and related GPU state). Created/released in Reload/ReleaseShader.
	ID3DX11Effect* m_pEffect = nullptr;
	ID3DX11EffectTechnique* m_technique = nullptr; // One technique to render the effect
	ID3DX11EffectPass* m_pass = nullptr; // One rendering pass of the technique
	ID3DX11EffectMatrixVariable* m_viewProjectionEV = nullptr; // WorldViewProjection matrix effect variable
	ID3DX11EffectVectorVariable* m_cameraRightEV = nullptr;
	ID3DX11EffectVectorVariable* m_cameraUpEV = nullptr;
	ID3DX11EffectShaderResourceVariable* m_spriteTexturesEV = nullptr;

	// Sprite textures and corresponding shader resource views.
	//std::vector<ID3D11Texture2D*>          m_spriteTex;       // You may not need this if you use CreateDDSTExtureFromFile!
	std::vector<ID3D11ShaderResourceView*> m_spriteSRV;

	// Maximum number of allowed sprites, i.e. size of the vertex buffer.
	size_t m_spriteCountMax = 2048;
	// Vertex buffer for sprite vertices, and corresponding input layout.
	ID3D11Buffer* m_pVertexBuffer = nullptr;
	ID3D11InputLayout* m_pInputLayout = nullptr;
};
