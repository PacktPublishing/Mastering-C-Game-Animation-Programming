#version 460 core
layout (location = 0) in vec4 color;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 texCoord;
layout (location = 3) flat in float selectInfo;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out float SelectedInstance;

uniform sampler2D tex;

vec3 lightPos = vec3(4.0, 3.0, 6.0);
vec3 lightColor = vec3(1.0, 1.0, 1.0);

void main() {
  float lightAngle = max(dot(normalize(vec3(normal)), normalize(lightPos)), 0.0);
  FragColor = texture(tex, texCoord) * color * vec4((0.3 + 0.7 * lightAngle) * lightColor, 1.0);

  /* fill the second color attachment with the ID of our model */
  SelectedInstance = selectInfo;
}
