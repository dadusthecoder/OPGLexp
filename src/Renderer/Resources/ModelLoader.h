#pragma once

#include <string>
#include <vector>
#include "Scene/Entity.h"
#include "Scene/Scene.h"
#include "Renderer/Core/Shader.h"
#include "Renderer/Resources/Mesh.h"
#include "Renderer/Resources/Material.h"

namespace lgt {
    
    class ModelLoader {
    public:
        // Loads a model and instantiates it into the given scene. 
        // Returns the root entity of the loaded model.
        static Entity LoadModel(const std::string& path, Scene* scene, Shader* defaultShader);
    };

}
