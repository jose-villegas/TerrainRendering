#include "Commons.h"
#include "AppInterface.h"
#include "TransformationMatrices.h"
#include "App.h"

void AppInterface::initialize(GLFWwindow * window)
{
    ImGui_ImplGlfwGL3_Init(window, true);
}

void AppInterface::draw()
{
    // inmediate user interface new frame
    ImGui_ImplGlfwGL3_NewFrame();
    {
        ImVec2 windowSize;
        ImGuiIO& io = ImGui::GetIO();
        // performance window
        {
            ImGui::SetNextWindowSize(ImVec2(150, 50));
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 150 - 3,
                                           io.DisplaySize.y - 50 - 3));
            ImGui::Begin("Performance Window", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
            ImGui::Text("Performance");
            ImGui::Separator();
            ImGui::Text("FPS: (%.1f)", ImGui::GetIO().Framerate);
            ImGui::End();
        }
        // mesh data input window
        {
            ImGui::SetNextWindowPos(ImVec2(3, 3));
            ImGui::Begin("Terrain Generation Options", nullptr,
                         ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::SliderInt("Mesh Resolution", &meshResolution, 2, 11,
                             std::to_string((int)std::pow(2, meshResolution) + 1).c_str());
            ImGui::InputInt("Heightmap Resolution", &heightmapResolution, 1, 16);
            // bound the int range
            heightmapResolution = std::max(16, std::min(8192, heightmapResolution));
            // generate new terrain on button press

            if(ImGui::Button("Generate Terrain"))
            {
                App::Instance()->getTerrain().createTerrain(heightmapResolution);
                App::Instance()->getTerrain().createMesh(meshResolution);
            }

            ImGui::Separator();

            if(ImGui::SliderFloat("Maximum Height", &maxHeight, 0.001f, 15.0f))
            {
                TransformationMatrices::Model(
                    glm::scale(glm::mat4(), glm::vec3(1, maxHeight, 1))
                );
            }

            windowSize = ImGui::GetWindowSize();
            ImGui::End();
        }
        // rendering options
        {
            ImGui::SetNextWindowPos(ImVec2(3, windowSize.y + 6));
            ImGui::Begin("Rendering Options", nullptr,
                         ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Checkbox("Use Wireframe Mode", &wireframeMode);

            if(wireframeMode)
            {
                gl.PolygonMode(Face::FrontAndBack, PolygonMode::Line);
            }

            ImGui::End();
        }
    }
}

void AppInterface::render()
{
    if(wireframeMode)
    {
        gl.PolygonMode(Face::FrontAndBack, PolygonMode::Fill);
    }

    ImGui::Render();
}

void AppInterface::terminate()
{
    ImGui_ImplGlfwGL3_Shutdown();
}

AppInterface::AppInterface()
{
    this->maxHeight = 1.0;
    this->meshResolution = 6;
    this->heightmapResolution = 256;
    this->wireframeMode = false;
}


AppInterface::~AppInterface()
{
}
