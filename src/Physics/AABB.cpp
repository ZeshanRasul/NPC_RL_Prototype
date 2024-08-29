#include "AABB.h"
#include <limits>
#include <glm/gtc/matrix_transform.hpp>

AABB::AABB()
    : mMin(std::numeric_limits<float>::max()), mMax(std::numeric_limits<float>::lowest()) {}

AABB::AABB(const glm::vec3& min, const glm::vec3& max)
    : mMin(min), mMax(max) {}

void AABB::calculateAABB(const std::vector<glm::vec3>& vertices) {
    for (const auto& vertex : vertices) {
        mMin = glm::min(mMin, vertex);
        mMax = glm::max(mMax, vertex);
    }
}

void AABB::update(const glm::mat4& modelMatrix) {
    std::vector<glm::vec3> corners = {
        mMin,
        {mMin.x, mMin.y, mMax.z},
        {mMin.x, mMax.y, mMin.z},
        {mMin.x, mMax.y, mMax.z},
        {mMax.x, mMin.y, mMin.z},
        {mMax.x, mMin.y, mMax.z},
        {mMax.x, mMax.y, mMin.z},
        mMax
    };

    transformedMin = glm::vec3(std::numeric_limits<float>::max());
    transformedMax = glm::vec3(std::numeric_limits<float>::lowest());

    for (const auto& corner : corners) {
        glm::vec3 transformed = glm::vec3(modelMatrix * glm::vec4(corner, 1.0f));
        transformedMin = glm::min(transformedMin, transformed);
        transformedMax = glm::max(transformedMax, transformed);
    }
}
