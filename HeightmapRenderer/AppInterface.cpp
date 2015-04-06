#include "Commons.h"
#include "App.h"
#include "AppInterface.h"
#include "TransformationMatrices.h"
#include "OpenFileDialog.h"
using namespace boost::algorithm;

void AppInterface::initialize(GLFWwindow * window)
{
    ImGui_ImplGlfwGL3_Init(window, true);
}

void AppInterface::draw(float time)
{
    // inmediate user interface new frame
    ImGui_ImplGlfwGL3_NewFrame();
    {
        static ImVec2 stackedSize;
        ImGuiIO& io = ImGui::GetIO();
        // mesh data input window
        {
            ImGui::SetNextWindowPos(ImVec2(3, 3));
            ImGui::Begin("Terrain Generation Options", nullptr,
                         ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoMove);
            ImGui::SliderInt("Mesh Resolution", &meshResolution, 5, 11,
                             std::to_string((int)std::pow(2, meshResolution) + 1).c_str());
            ImGui::SliderInt("Map Resolution", &heightmapResolution, 5, 11,
                             std::to_string((int)std::pow(2, heightmapResolution)).c_str());

            if(ImGui::SliderFloat("Maximum Height", &maxHeight, 0.001f, 15.0f))
            {
                App::Instance()->getTerrain().HeightScale(maxHeight);
            }

            if(ImGui::InputFloat("Terrain Scale", &terrainScale, 0.01, 0.1))
            {
                App::Instance()->getTerrain().TerrainHorizontalScale(terrainScale);
            }

            if(ImGui::Button("Bake"))
            {
                App::Instance()->getTerrain().bakeLightmaps(lightmapFreq);
            }

            ImGui::SameLine();
            ImGui::SliderInt("Lightmaps", &lightmapFreq, 1, 720);

            if(ImGui::Button("Generate Terrain"))
            {
                App::Instance()->getTerrain().createTerrain(
                    (int)std::pow(2, heightmapResolution)
                );
                App::Instance()->getTerrain().createMesh(meshResolution);
            }

            ImGui::SameLine();

            if(ImGui::Button("Save Heightmap"))
            {
                using namespace boost::posix_time;
                // set string format
                static std::locale loc(std::cout.getloc(), new time_facet("%Y-%m-%d_%H-%M-%S"));
                // parse it to a std::string
                std::basic_stringstream<char> wss;
                wss.imbue(loc);
                // get current time
                wss << second_clock::universal_time();
                // write terrain to file, concatenate timestamp
                App::Instance()->getTerrain().Heightmap().writeToFile("terrain" + wss.str());
            }

            stackedSize = ImGui::GetWindowSize();
            ImGui::End();
        }
        // rendering options
        {
            ImGui::SetNextWindowPos(ImVec2(3, stackedSize.y + 6));
            ImGui::Begin("Rendering Options", nullptr,
                         ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoMove);
            ImGui::Checkbox("Use Wireframe Mode", &wireframeMode);

            if(ImGui::Checkbox("Use Time Color Grading", &colorGrading))
            {
                App::Instance()->getTerrain()
                .EnableTimeOfTheDayColorGrading(colorGrading);
            }

            ImGui::Text("Time Scale");

            if(ImGui::InputFloat("##tscale", &timeScale, 0.00001, 0.1, 10))
            {
                App::Instance()->getTerrain()
                .TimeScale(timeScale);
            }

            ImGui::Checkbox("Pause", &pauseTime);
            // always positive
            timeScale = std::max(0.0f, timeScale);

            if(wireframeMode)
            {
                gl.PolygonMode(Face::FrontAndBack, PolygonMode::Line);
            }

            stackedSize.y += ImGui::GetWindowSize().y;
            ImGui::End();
        }
        // terrain textures
        {
            ImGui::SetNextWindowPos(ImVec2(3, stackedSize.y + 9));
            ImGui::Begin("Terrain Textures", nullptr,
                         ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoMove);
            ImGui::Text("Available Ranges");

            for(int i = 0; i < 4; i++)
            {
                if(i > 0) ImGui::SameLine();

                bool rangeChanged = false;
                ImGui::PushID(i);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor::HSV(i / 7.0f, 0.5f, 0.5f));
                ImGui::VSliderFloat("##t", ImVec2(17, 120), &ranges[2 * i], 0.0f, 1.0f, "");
                ImGui::PopStyleColor();

                if(ImGui::IsItemActive() || ImGui::IsItemHovered())
                    ImGui::SetTooltip("%.3f", ranges[2 * i]);

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor::HSV(i / 7.0f, 0.5f, 0.5f));
                ImGui::VSliderFloat("##v", ImVec2(17, 120), &ranges[2 * i + 1], 0.0f, 1.0f, "");
                ImGui::PopStyleColor();

                if(ImGui::IsItemActive() || ImGui::IsItemHovered())
                    ImGui::SetTooltip("%.3f", ranges[2 * i + 1]);

                // set proper values ranges
                for(int j = 0; j < 8; j++)
                {
                    ranges[j] = clamp(
                                    ranges[j],
                                    j == 0 ? 0.0f : ranges[j - 1],
                                    j == 7 ?  1.0f : ranges[j + 1]
                                );
                }

                // set values
                App::Instance()->getTerrain()
                .setTextureRange(i, ranges[i * 2], ranges[i * 2 + 1]);
                ImGui::PopID();
            }

            for(int i = 0; i < 4; i++)
            {
                ImGui::PushID(i * 2 + 1);
                static ImTextureID texId[4] = {};

                if(ImGui::ImageButton(texId[i], ImVec2(34, 34)))
                {
                    static OpenFileDialog *fileDialog = new OpenFileDialog();
                    fileDialog->ShowDialog();
                    // load texture from filename
                    App::Instance()->getTerrain()
                    .loadTexture(i, fileDialog->FileName);
                    // assign texture to texid interfaces
                    texId[i] = (void *)(intptr_t)App::Instance()->getTerrain().getTextureId(i);
                }

                if(ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Image(
                        texId[i],
                        ImVec2(200, 200),
                        ImVec2(0.0, 0.0), ImVec2(1.0, 1.0),
                        ImColor(255, 255, 255, 255),
                        ImColor(255, 255, 255, 128)
                    );
                    ImGui::EndTooltip();
                }

                i < 3 ? ImGui::SameLine() : 0;
                ImGui::PopID();
            }

            ImGui::Separator();
            ImGui::Text("Texture Repeat");

            if(ImGui::InputFloat2("##t", textureRepeat))
            {
                App::Instance()->getTerrain().setTextureRepeatFrequency(
                    glm::vec2(textureRepeat[0], textureRepeat[1])
                );
            }

            stackedSize.y += ImGui::GetWindowSize().y;
            ImGui::End();
        }
        // shadow map
        {
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 146, 3));
            ImGui::SetNextWindowCollapsed(true, ImGuiSetCond_Once);

            if(ImGui::Begin("Light Map", nullptr,
                            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                            ImGuiWindowFlags_NoMove))
            {
                App::Instance()->getTerrain().fastGenerateShadowmapParallel(
                    App::Instance()->getTerrain().calculateLightDir(time * timeScale), 128
                );
                ImGui::Image(
                    (void *)(intptr_t)App::Instance()->getTerrain().getLightmapId(),
                    ImVec2(128, 128),
                    ImVec2(0.0, 0.0), ImVec2(1.0, 1.0),
                    ImColor(255, 255, 255, 255),
                    ImColor(255, 255, 255, 128)
                );
            }

            ImGui::End();
        }
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
            ImGui::Text("FPS: (%.1f)", io.Framerate);
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
    this->maxHeight = 2.0;
    this->terrainScale = 15.f;
    this->meshResolution = 8;
    this->heightmapResolution = 8;
    this->textureRepeat[0] = this->textureRepeat[1] = 25.0f;
    this->timeScale = 0.1f;
    this->colorGrading = true;
    this->lightmapFreq = 96;

    for(int i = 0; i < 4; i++)
    {
        ranges[i * 2] = (float)i / 4;
        ranges[i * 2 + 1] = float(i + 11) / 4;
    }

    this->pauseTime = false;
    this->wireframeMode = false;
}


AppInterface::~AppInterface()
{
}
