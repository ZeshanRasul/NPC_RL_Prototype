#pragma once
#include <vector>
#include <string>

#include "Component.h"
#include "Audio/SoundEvent.h"
#include "GameObjects/GameObject.h"

class AudioComponent : public Component
{
public:
	AudioComponent(class GameObject* owner, int updateOrder = 200);
	~AudioComponent();

	void Update(float deltaTime) override;
	void OnUpdateWorldTransform() override;

	SoundEvent PlayEvent(const std::string& name);
	void StopAllEvents();
private:
	std::vector<SoundEvent> mEvents2D;
	std::vector<SoundEvent> mEvents3D;
};