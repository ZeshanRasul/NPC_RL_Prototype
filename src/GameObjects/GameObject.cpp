#include "GameManager.h"
#include "GameObject.h"

GameObject::GameObject(glm::vec3 pos, glm::vec3 scale, float yaw, Shader* shdr, Shader* shadowMapShader, bool applySkinning, GameManager* gameMgr)
: m_yaw(yaw), m_position(pos), m_scale(scale), m_toSkin(applySkinning), m_shader(shdr), m_shadowShader(shadowMapShader),
	m_gameManager(gameMgr)
{
	{
		Scene* scene = m_gameManager->GetActiveScene();

		m_entity = scene->CreateEntity();
		auto& t = scene->GetRegistry().emplace<TransformComponent>(m_entity);

		t.Position = pos;
		t.Rotation = glm::angleAxis(glm::radians(yaw), glm::vec3{ 0.0f, 1.0f, 0.0f });
		t.Scale = scale;
		t.dirty = true;

		size_t uniformMatrixBufferSize = 4 * sizeof(glm::mat4);
		m_uniformBuffer.Init(uniformMatrixBufferSize);
		Logger::Log(1, "%s: matrix uniform buffer (size %i bytes) successfully created\n", __FUNCTION__,
			uniformMatrixBufferSize);
	}
}

float GameObject::GetYaw()
{
	const auto& t = m_gameManager->GetActiveScene()->GetRegistry().get<TransformComponent>(m_entity);

	float yaw = glm::eulerAngles(glm::normalize(t.Rotation)).y;

	return glm::radians(yaw);
}

void GameObject::SetPosition(const glm::vec3& p)
{
	auto& t = m_gameManager->GetActiveScene()->GetRegistry().get<TransformComponent>(m_entity);

	t.Position = p;
	t.dirty = true;
}

void GameObject::SetScale(const glm::vec3& s)
{
	auto& t = m_gameManager->GetActiveScene()->GetRegistry().get<TransformComponent>(m_entity);

	t.Scale = s;
	t.dirty = true;
}

void GameObject::SetYaw(float yaw)
{
	auto& t = m_gameManager->GetActiveScene()->GetRegistry().get<TransformComponent>(m_entity);

	t.Rotation = glm::angleAxis(glm::radians(yaw), glm::vec3{ 0.0f, 1.0f, 0.0f });
	t.dirty = true;

}


