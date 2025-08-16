#pragma once
#include "tinygltf/tiny_gltf.h"
#include "Engine/Asset/AssetTypes.h"

class Shader;

struct StaticModelRendererComponent
{
	ModelHandle model{ InvalidHandle };
	MaterialHandle material{ InvalidHandle };

	Shader* shader{ nullptr };
	Shader* shadowShader{ nullptr };

	bool visible{ true };
	bool castsShadows{ true };
	uint32_t layer{ 0 };
};