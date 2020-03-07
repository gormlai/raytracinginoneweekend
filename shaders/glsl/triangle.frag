#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set =1 , binding = 1) uniform sampler2D tSampler;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main() {  
  outColor = texture(tSampler, fragUV) * fragColor;
  //outColor = fragColor;
}
