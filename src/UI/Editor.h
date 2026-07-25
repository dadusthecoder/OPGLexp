#pragma once

#pragma once
#include <string>
#include <imgui.h>
#include "Renderer/Texture.h"

namespace lgt {
class Renderer;
class Scene;
class Camera;
class FrameBuffer;
class Grid;
struct SceneNode;
}

namespace Editor {
void ApplyProfessionalTheme();
void DrawSceneHierarchyPanel(lgt::Scene* scene);
void DrawEnvironmentPanel(lgt::Grid* grid, lgt::Renderer* renderer, lgt::Scene* scene);
void DrawCameraPanel(lgt::Camera* camera, float* speed, float* sensitivity);
void DrawAssetBrowserPanel(lgt::Scene* scene);
void DrawInspectorPanel();
void DrawConsolePanel();

lgt::SceneNode* GetSelectedNode();
void SetSelectedNode(lgt::SceneNode* node);
int GetSelectedLightIndex();
void SetSelectedLightIndex(int index);
int GetGizmoOperation(); // 0: Translate, 1: Rotate, 2: Scale
void SetGizmoOperation(int op);

void DrawMaterialEditorPanel();
// texture
void DrawTextureSamplerNode(const std::string& textureId, lgt::Texture& texture);

} // namespace Editor
