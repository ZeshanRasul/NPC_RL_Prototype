#pragma once
#include <string>
#include "glm/glm.hpp"

class SoundEvent
{
public:
	SoundEvent();
	bool IsValid();
	void Restart();
	void Stop(bool allowFadeOut = true);
	void SetPaused(bool pause);
	void SetVolume(float value);
	void SetPitch(float value);
	void SetParameter(const std::string& name, float value);
	bool GetPaused() const;
	float GetVolume() const;
	float GetPitch() const;
	float GetParameter(const std::string& name);
	bool Is3D() const;
	void Set3DAttributes(const glm::mat4& worldTrans);

protected:
	// Make this constructor protected and AudioSystem a friend
	// so that only AudioSystem can access this constructor.
	friend class AudioSystem;
	SoundEvent(class AudioSystem* system, unsigned int id);

private:
	class AudioSystem* m_system;
	unsigned int m_id;
};
