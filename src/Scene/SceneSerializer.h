#pragma once
#include <string>

namespace lgt {
    class Scene;
    
    class SceneSerializer {
    public:
        SceneSerializer(Scene* scene) : m_Scene(scene) {}
        
        void Serialize(const std::string& filepath);   // Save to JSON
        bool Deserialize(const std::string& filepath); // Load from JSON
        
    private:
        Scene* m_Scene;
    };
}
