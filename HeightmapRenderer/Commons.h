// os specific includes, without the min/max macros that break std::numeric_limits
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <tchar.h>
// opengl and context creation headers
#include <GL/glew.h>
#include <GLFW/glfw3.h>
// pre-processors
#define GLM_FORCE_RADIANS
#define GLM_FORCE_PURE
#define _USE_MATH_DEFINES
// ogl headers c++ wrapper on opengl
#include <oglplus/all.hpp>
#include <oglplus/opt/smart_enums.hpp>
#include <oglplus/shapes/cube.hpp>
#include <oglplus/bound/texture.hpp>
#include <oglplus/buffer_usage.hpp>
#include <oglplus/glsl_source.hpp>
#include <oglplus/glsl_string.hpp>
#include <oglplus/object/array.hpp>
#include <oglplus/object/group.hpp>
#include <oglplus/interop/glm.hpp>
// coherent noise generation
#include <noise/noise.h>
#include "noise/interp.h"
#include <noiseutils.h>
// standard and stl library headers
#include <iostream>
#include <stdexcept>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <thread>
#include <math.h>
// glm math library headers
#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/gtx/matrix_operation.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/detail/type_vec3.hpp>
#include <glm/gtx/common.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/color_space.hpp>
#include <type_traits>
// console and file logging
#include "Logging.h"
// general utils
namespace util
{
    // clamps value to [low, high]; the bounds convert to the value's type
    template<typename T>
    inline T clamp(const T &value, const typename std::common_type<T>::type &low,
                   const typename std::common_type<T>::type &high)
    {
        return value < low ? low : (high < value ? high : value);
    }
}
// ui library imgui (inmediate mode gui)
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw_gl3.h"
// image loading library
#include <FreeImage.h>
// parallel patterns library (ppl) from Windows for concurrency and parallelism
#include <ppl.h>