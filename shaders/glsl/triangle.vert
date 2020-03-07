#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set=0, binding = 0) uniform UniformBufferObject {
    mat4 _model;
    mat4 _view;
    mat4 _proj;
} ubo;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragUV;

void main() {  
  gl_Position = ubo._proj * ubo._view * ubo._model * inPosition;
  fragColor = inColor;
  fragUV = inUV;
}
