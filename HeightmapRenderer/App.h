#pragma once
#include "MainWindow.h"

namespace oglplus
{
    class Example
    {
        private:
            Context gl;
            VertexShader vs;
            FragmentShader fs;
            Program prog;
            Texture tex;
            //VAO for the mesh (just 2 triangles)
            VertexArray triangles;
            //VBO for the vertices positions
            Buffer verts;
        public:
            Example(const Vec3f& color)
            {
                vs.Source((GLSLString)
                          "#version 330\n "
                          "in vec3 Position;"
                          "void main(void)"
                          "{gl_Position = vec4(Position, 1.0);}");
                vs.Compile();
                fs.Source((GLSLString)
                          "#version 330\n"
                          "out vec3 fragColor; "
                          "uniform vec3 Color; "
                          "void main(void)"
                          " {fragColor = Color;}");
                fs.Compile();
                prog.AttachShader(vs);
                prog.AttachShader(fs);
                prog.Link();
                prog.Use();
                Uniform<Vec3f>(prog, "Color").Set(color);
                triangles.Bind();
                GLfloat positions[9] =
                {
                    0.0f, 0.5f, 0.0f,
                    0.5f, -0.5f, 0.0f,
                    -0.5f, -0.5f, 0.0f
                };
                verts.Bind(Buffer::Target::Array);
                Buffer::Data(Buffer::Target::Array, 9, positions);
                VertexArrayAttrib(prog, "Position").Setup<GLfloat>(3).Enable();
                gl.ClearColor(0, 0, 0, 0);
            }
            void display(int windowWidth, int windowHeight)
            {
                gl.Viewport(0, 0, windowWidth, windowHeight);
                gl.Clear().ColorBuffer();
                verts.Bind(Buffer::Target::Array);
                gl.DrawArrays(PrimitiveType::Triangles, 0, 3);
            }
    };
}

class App : private MainWindow
{
    private:
        static App *instance;
        App(const App &rhs);
        App(const std::string &title, const unsigned int width,
            const unsigned int height);
        // app callbacks
        static void onError(int code, const char * description);
        static void onKeyPress(GLFWwindow *window, int key, int scancode, int action,
                               int mods);
        // app libraries configuration
        static void Configure();
        // app render loop
        static void Start();
    public:
        static App * Instance();
        static void Run();

        ~App();
};

