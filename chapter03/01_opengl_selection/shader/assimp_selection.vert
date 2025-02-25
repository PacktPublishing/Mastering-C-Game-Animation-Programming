#version 460 core
layout (location = 0) in vec4 aPos; // last float is uv.x
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec4 aNormal; // last float is uv.y
layout (location = 3) in uvec4 aBoneNum; // ignored
layout (location = 4) in vec4 aBoneWeight; // ignored

layout (location = 0) out vec4 color;
layout (location = 1) out vec4 normal;
layout (location = 2) out vec2 texCoord;
layout (location = 3) out float selectInfo;

layout (std140, binding = 0) uniform Matrices {
  mat4 view;
  mat4 projection;
};

layout (std430, binding = 1) readonly restrict buffer WorldPosMatrices {
  mat4 worldPosMat[];
};

layout (std430, binding = 2) readonly restrict buffer InstanceSelected {
  vec2 selected[];
};

void main() {

  mat4 modelMat = worldPosMat[gl_InstanceID];
  gl_Position = projection * view * modelMat * vec4(aPos.x, aPos.y, aPos.z, 1.0);

  color = aColor * selected[gl_InstanceID].x;
  /* draw the instance always on top when highlighted, helps to find it better */
  if (selected[gl_InstanceID].x != 1.0f) {
    gl_Position.z -= 1.0f;
  }

  normal = transpose(inverse(modelMat)) * vec4(aNormal.x, aNormal.y, aNormal.z, 1.0);
  texCoord = vec2(aPos.w, aNormal.w);

  /* we need vertex id only (z -> y) */
  selectInfo = selected[gl_InstanceID].y;
}
