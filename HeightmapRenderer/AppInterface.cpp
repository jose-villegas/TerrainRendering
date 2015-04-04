#include "Commons.h"
#include "AppInterface.h"
#include "TransformationMatrices.h"
#include "App.h"
#include "OpenFileDialog.h"

void AppInterface::initialize(GLFWwindow * window)
{
    ImGui_ImplGlfwGL3_Init(window, true);
}

void AppInterface::draw()
{
    // inmediate user interface new frame
    ImGui_ImplGlfwGL3_NewFrame();
    {
        static ImVec2 stackedSize;
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
            ImGui::Text("FPS: (%.1f)", io.Framerate);
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

            ImGui::Separator();

            if(ImGui::SliderFloat("Maximum Height", &maxHeight, 0.001f, 15.0f))
            {
                TransformationMatrices::Model(
                    glm::scale(glm::mat4(), glm::vec3(1, maxHeight, 1))
                );
            }

            stackedSize = ImGui::GetWindowSize();
            ImGui::End();
        }
        // rendering options
        {
            ImGui::SetNextWindowPos(ImVec2(3, stackedSize.y + 6));
            ImGui::Begin("Rendering Options", nullptr,
                         ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Checkbox("Use Wireframe Mode", &wireframeMode);

            if(wireframeMode)
            {
                gl.PolygonMode(Face::FrontAndBack, PolygonMode::Line);
            }

            stackedSize.x += ImGui::GetWindowSize().x;
            stackedSize.y += ImGui::GetWindowSize().y;
            ImGui::End();
        }
        // terrain textures
        {
            ImGui::SetNextWindowPos(ImVec2(3, stackedSize.y + 9));
            ImGui::Begin("Terrain Textures", nullptr,
                         ImGuiWindowFlags_AlwaysAutoResize);
            static float ranges[4] = {0.0};
            ImGui::Text("Available Ranges");

            for(int i = 0; i < 4; i++)
            {
                if(i > 0) ImGui::SameLine();

                ImGui::PushID(i);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor::HSV(i / 7.0f, 0.5f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImColor::HSV(i / 7.0f, 0.9f, 0.9f));
                ImGui::VSliderFloat("##v", ImVec2(35, 160), &ranges[i], 0.0f, 1.0f, "");
                // set proper values ranges
                ranges[0] = std::min(ranges[0], ranges[1]);
                ranges[1] = std::min(std::max(ranges[1], ranges[0]), ranges[2]);
                ranges[2] = std::min(std::max(ranges[2], ranges[1]), ranges[3]);
                ranges[3] = std::min(std::max(ranges[3], ranges[2]), 1.0f);

                if(ImGui::IsItemActive() || ImGui::IsItemHovered())
                    ImGui::SetTooltip("%.3f", ranges[i]);

                ImGui::PopStyleColor(2);
                ImGui::PopID();
            }

            ImVec2 tex_screen_pos = ImGui::GetCursorScreenPos();
            float tex_w = (float)ImGui::GetIO().Fonts->TexWidth;
            float tex_h = (float)ImGui::GetIO().Fonts->TexHeight;

            for(int i = 0; i < 4; i++)
            {
                ImGui::PushID(i * 2 + 1);

                if(ImGui::ImageButton(io.Fonts->TexID, ImVec2(27, 27)))
                {
                    static OpenFileDialog *fileDialog = new OpenFileDialog();
                    fileDialog->ShowDialog();
                    this->tmtt.loadTexture(fileDialog->FileName, i);
                    // std::cout << this->tmtt.TexId();
                }

                if(ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    float focus_sx = tex_w;
                    float focus_sy = tex_h;
                    float focus_x = std::min(std::max(ImGui::GetMousePos().x - tex_screen_pos.x -
                                                      focus_sx * 0.5f, 0.0f), tex_w - focus_sx);
                    float focus_y = std::min(std::max(ImGui::GetMousePos().y - tex_screen_pos.y -
                                                      focus_sy * 0.5f, 0.0f), tex_h - focus_sy);
                    ImVec2 uv0 = ImVec2((focus_x) / tex_w, (focus_y) / tex_h);
                    ImVec2 uv1 = ImVec2((focus_x + focus_sx) / tex_w, (focus_y + focus_sy) / tex_h);
                    ImGui::Image(io.Fonts->TexID, ImVec2(tex_w, tex_h), uv0, uv1, ImColor(255, 255,
                                 255,
                                 255),
                                 ImColor(255, 255, 255, 128));
                    ImGui::EndTooltip();
                }

                i < 4 ? ImGui::SameLine() : 0;
                ImGui::PopID();
            }

            ImGui::End();
        }
        ImGui::ShowTestWindow();
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
