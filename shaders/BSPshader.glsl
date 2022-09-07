#version 120
#extension GL_ARB_shader_bit_encoding : enable
#extension GL_EXT_gpu_shader4 : enable

#define PI 3.1415926535
#define PI_2 1.570796327
#define PI_3 1.047197551
#define MAX_DEPTH 64

uniform sampler2D texture;
uniform vec4 nodes[512];

const float inf = 1. / 0.;

vec3 rainbow(float val){
  vec3 result = vec3(sin(val), sin(val + PI_3), sin(val + PI_3 * 2.0));
  return result * result;
}

struct Node {
  vec2 pos;
  float tangent;
  int left, right;
};

const uint colorIndexThreshold = uint(32768);
const uint ushortMask = uint(0x0000ffff); // 65535

uint traverseTree(vec2 uv){
  uint currentIndex = uint(0);

  for(int iteration = 0; iteration < MAX_DEPTH; ++iteration) {
    vec2 pos = nodes[currentIndex].xy;
    vec2 tangent = vec2(nodes[currentIndex].z, 1.0);
    uint leftRight = floatBitsToUint(nodes[currentIndex].w);   
    bool isLeftPixel = dot(pos - uv, tangent) < 0.0;
    uint indexOfProperSide = isLeftPixel ? leftRight >> 16 : leftRight & ushortMask;
    if(indexOfProperSide < colorIndexThreshold) {
      currentIndex = indexOfProperSide;
    }
    else {
      return indexOfProperSide - colorIndexThreshold;
    }
  }
  return uint(0);
}

float sigmoid(float val){
  return val / (abs(val) + 1.0);
}

void main() {
  vec2 uv = gl_TexCoord[0].xy;
  uint index = traverseTree(uv); vec3 hint = rainbow(0.5 * float(index));
  vec3 textureColor = texture2D(texture, uv).rgb;
  gl_FragColor = vec4(mix(textureColor, hint, vec3(0.3)), 1.0);
  // gl_FragColor = vec4(hint, 1.0);
}
