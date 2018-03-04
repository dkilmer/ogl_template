#version 330 core

in vec4 Color;
in vec3 Normal;
in vec2 TexCoord;

out vec4 outColor;

uniform sampler2D tex;
uniform vec3 light_pos;

float lambert(vec3 N, vec3 L) {
  vec3 nrmN = normalize(N);
  vec3 nrmL = normalize(L);
  float result = dot(nrmN, nrmL);
  //return max(result, 0.0);
  // our lambert is going to ignore facing
  return abs(result);
}

void main() {
  //RGBA of our diffuse color
  vec4 DiffuseColor = texture(tex, TexCoord);
  //vec4 DiffuseColor = vec4(TexCoord, 0, Color.a);
  //vec4 DiffuseColor = Color;
  vec3 result = DiffuseColor.rgb * lambert(Normal, light_pos);
  outColor = vec4(result, Color.a);
  //outColor = vec4(Color, 1);
}
