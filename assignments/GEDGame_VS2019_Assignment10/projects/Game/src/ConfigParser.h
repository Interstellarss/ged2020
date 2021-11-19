#pragma once

#include <DirectXMath.h>
#include <intsafe.h>
#include <string>
#include <list>
#include <vector>

// Define a new class
class ConfigParser
{
public:
	struct MeshOnDisk {
		std::string identifier;
		std::string pathMesh;
		std::string pathDiffuse;
		std::string pathSpecular;
		std::string pathGlow;
	};

	struct ObjectOnDisk {
		std::string parentIdentifier;
		std::string meshIdentifer;
		float pos_x = 0, pos_y = 0, pos_z = 0;
		float rot_x = 0, rot_y = 0, rot_z = 0;
		float scale = 1;
	};

	struct EnemyOnDisk {
		std::string identifier;
		std::string meshIdentifer;
		int hp = 1;
		float speed = 0, size = 1;
		float pos_x = 0, pos_y = 0, pos_z = 0;
		float rot_x = 0, rot_y = 0, rot_z = 0;
		float scale = 1;
	};

	struct SpawnBehaviour {
		float interval = 1.0f;
		float spawn_radius = 1000.0f;
		float target_radius = 100.0f;
		float min_height = 0.0f;
		float max_height = 1.0f;
	};

	struct Gun
	{
		std::string gunIdentifier;
		DirectX::XMFLOAT3 spawnPosView;
		float speed = 0;
		float gravity = 0;
		float cooldown = 0;
		float damage = 0;
		float spriteTexIndex = 0;
		float spriteRadius = 0;

	};

	struct Projectiles
	{
		std::string spriteName;
	};

	// returns true on success, false on failure
	bool load(std::string filename);

	// Implement getters using implicit inlining
	const std::string& get_terrainPathHeight() { return terrainPathHeight; }
	const std::string& get_terrainPathColor() { return terrainPathColor; }
	const std::string& get_terrainPathNormal() { return terrainPathNormal; }
	float get_TerrainWidth() { return terrainWidth; }
	float get_TerrainHeight() { return terrainHeight; }
	float get_TerrainDepth() { return terrainDepth; }
	const std::list<MeshOnDisk>& get_Meshes() { return meshes; }
	const std::list<ObjectOnDisk>& get_Objects() { return objects; }
	const std::list<EnemyOnDisk>& get_Enemies() { return enemies; }
	const SpawnBehaviour& get_SpawnBehaviour() { return spawnBehaviour; }
	const std::vector<Gun>& get_Guns() {return guns; }
	const std::vector<Projectiles> get_Projectiles(){return projectiles; }

private:
	std::string terrainPathHeight;
	std::string terrainPathColor;
	std::string terrainPathNormal;
	float terrainWidth = -1;
	float terrainHeight = -1;
	float terrainDepth = -1;
	

	std::list<MeshOnDisk> meshes;
	std::list<ObjectOnDisk> objects;
	std::list<EnemyOnDisk> enemies;
	std::vector<Gun> guns;
	SpawnBehaviour spawnBehaviour;
	std::vector<Projectiles> projectiles;
};

extern ConfigParser g_ConfigParser;