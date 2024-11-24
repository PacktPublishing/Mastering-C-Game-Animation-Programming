#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;
layout (location = 4) in vec4 aBoneNum; // ignored
layout (location = 5) in vec4 aBoneWeight; // ignored

layout (location = 0) out vec4 color;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec2 texCoord;

layout (set = 1, binding = 0) uniform Matrices {
  mat4 view;
  mat4 projection;
};

layout (std430, set = 2, binding = 0) readonly buffer WorldPosMatrices {
  mat4 worldPosMat[];
};

void main() {
  mat4 modelMat = worldPosMat[gl_InstanceIndex];
  gl_Position = projection * view * modelMat * vec4(aPos, 1.0);
  color = aColor;
  normal = vec3(transpose(inverse(modelMat)) * vec4(aNormal, 1.0));
  texCoord = aTexCoord;
}
