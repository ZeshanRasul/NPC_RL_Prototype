#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "GameObjects/GameObject.h"

class AABB {
public:
    AABB();
    AABB(const glm::vec3& min, const glm::vec3& max);

    void calculateAABB(const std::vector<glm::vec3>& vertices);

    glm::vec3 getMin() const { return mMin; }
    glm::vec3 getMax() const { return mMax; }
    glm::vec3 getCenter() const { return (mMin + mMax) * 0.5f; }
    glm::vec3 getSize() const { return mMax - mMin; }

    void update(const glm::mat4& modelMatrix);

    glm::vec3 transformedMin;
    glm::vec3 transformedMax;
	GameObject* owner;

private:
    glm::vec3 mMin;
    glm::vec3 mMax;
};
