#pragma once

#include <functional>

#include <DirectXMath.h>

#include "GameEffect.h"
#include "Mesh.h"

// Virtual/abstract class for all GameObjects
class GameObject
{
public:
	// = 0 denotes a pure virtual (i.e. abstract) function
	virtual DirectX::XMMATRIX getWorldMatrix() const = 0;
};

// Class for GameObjects which only act as parents for other GameObjects (Wrapper class)
class ParentObject : public GameObject
{
public:
	DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixIdentity();

	DirectX::XMMATRIX getWorldMatrix() const
	{
		return worldMatrix;
	}
};

// Class for all Gameobjects which should be rendered
class MeshObject : public GameObject
{
public:
	// The GameObject's parent, used for transform
	std::shared_ptr<GameObject> parent = nullptr;

	DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	std::shared_ptr<Mesh> mesh = nullptr;

	// Renders the GameObject
	HRESULT render(ID3D11DeviceContext* context, const DirectX::XMMATRIX camera) const
	{
		if (!mesh)
			return S_FALSE;

		HRESULT hr;

		DirectX::XMMATRIX world = getWorldMatrix();
		DirectX::XMMATRIX worldViewProj = world * camera;
		V(g_gameEffect.worldEV->SetMatrix((float*)&world));
		V(g_gameEffect.worldNormalsEV->SetMatrix((float*)&XMMatrixTranspose(XMMatrixInverse(nullptr, world))));
		V(g_gameEffect.worldViewProjectionEV->SetMatrix((float*)&worldViewProj));
		mesh->render(context, g_gameEffect.meshPass1, g_gameEffect.diffuseEV, g_gameEffect.specularEV, g_gameEffect.glowEV);

		return hr;
	}

	// Computes the GameObject's transformation matrix
	DirectX::XMMATRIX getParentMatrix() const
	{
		return DirectX::XMMatrixScaling(scale.x, scale.y, scale.z)
			* DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z)
			* DirectX::XMMatrixTranslation(position.x, position.y, position.z);
	}

	// Computes the GameObject's world matrix
	virtual DirectX::XMMATRIX getWorldMatrix() const
	{
		if (parent)
			return getParentMatrix() * parent->getWorldMatrix();
		else
			return getParentMatrix();
	}
};

// Class for enemy ships
class EnemyObject : public MeshObject
{
public:
	std::shared_ptr<EnemyObject> type = nullptr;

	DirectX::XMFLOAT3 velocity = { 0.0f, 0.0f, 0.0f };
	
	int health = 1;

	float size = 1;

	virtual DirectX::XMMATRIX getWorldMatrix() const
	{
		if (type)
			return type->getWorldMatrix() * MeshObject::getWorldMatrix();
		else
			return MeshObject::getWorldMatrix();
	}
};