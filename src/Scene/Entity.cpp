#include "Entity.h"
#include "Components.h"

namespace lgt {

    Entity::Entity(entt::entity handle, Scene* scene)
        : m_EntityHandle(handle), m_Scene(scene) {
    }

    void Entity::SetParent(Entity parent) {
        auto& rel = GetComponent<RelationshipComponent>();
        
        // Remove from current parent if any
        if (rel.Parent != entt::null) {
            auto& oldParentRel = m_Scene->m_Registry.get<RelationshipComponent>(rel.Parent);
            if (oldParentRel.FirstChild == m_EntityHandle) {
                oldParentRel.FirstChild = rel.NextSibling;
            }
            if (rel.PrevSibling != entt::null) {
                m_Scene->m_Registry.get<RelationshipComponent>(rel.PrevSibling).NextSibling = rel.NextSibling;
            }
            if (rel.NextSibling != entt::null) {
                m_Scene->m_Registry.get<RelationshipComponent>(rel.NextSibling).PrevSibling = rel.PrevSibling;
            }
            oldParentRel.ChildrenCount--;
        }

        rel.Parent = parent ? (entt::entity)parent : entt::null;
        rel.NextSibling = entt::null;
        rel.PrevSibling = entt::null;

        // Add to new parent
        if (rel.Parent != entt::null) {
            auto& newParentRel = m_Scene->m_Registry.get<RelationshipComponent>(rel.Parent);
            entt::entity currFirst = newParentRel.FirstChild;
            if (currFirst != entt::null) {
                rel.NextSibling = currFirst;
                m_Scene->m_Registry.get<RelationshipComponent>(currFirst).PrevSibling = m_EntityHandle;
            }
            newParentRel.FirstChild = m_EntityHandle;
            newParentRel.ChildrenCount++;
        }
    }

    Entity Entity::GetParent() {
        auto& rel = GetComponent<RelationshipComponent>();
        return rel.Parent != entt::null ? Entity(rel.Parent, m_Scene) : Entity();
    }

    std::vector<Entity> Entity::GetChildren() {
        std::vector<Entity> children;
        auto& rel = GetComponent<RelationshipComponent>();
        entt::entity curr = rel.FirstChild;
        while (curr != entt::null) {
            children.push_back(Entity(curr, m_Scene));
            curr = m_Scene->m_Registry.get<RelationshipComponent>(curr).NextSibling;
        }
        return children;
    }

}
