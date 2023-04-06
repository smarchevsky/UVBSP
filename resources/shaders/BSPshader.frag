#version 120
#extension GL_ARB_shader_bit_encoding : enable
#extension GL_EXT_gpu_shader4 : enable

uniform sampler2D texture;
uniform float transparency;
const float pi = 3.1415926535;
const float pi_inv = 1. / pi;

vec3 fastSin(vec3 x) { x = fract(x * 2.) - 1.; return (4. * x) * (1. - abs(x)); }

vec3 rainbow(float val){
    vec3 result = fastSin(vec3(val * 0.313 + vec3(0, 0.33333, 0.66666)));
    return result * result;
}

#define MAX_DEPTH 64
uniform vec4 nodes[512];

/// Node or color indices are represented in nodes.zw
/// node(less than 0) or color(greater equal 0)

int traverseTree(vec2 uv){
  int currentIndex = 0;

  //float distance = 99999999.f;

  for(int iteration = 0; iteration < MAX_DEPTH; ++iteration) {
    vec2 pos = vec2(nodes[currentIndex].x, 0.0);
    vec2 dir = vec2(nodes[currentIndex].y, 1.0);
    //distance = min(distance, abs(dot(pos - uv, normalize(dir))));
    bool isLeftPixel = dot(pos - uv, dir) < 0.0;

    int indexOfProperSide = floatBitsToInt(isLeftPixel ?
              nodes[currentIndex].z : nodes[currentIndex].w);
    
    if(indexOfProperSide < 0) {
      currentIndex = -indexOfProperSide;
    } else {
      //return distance;
      return indexOfProperSide;
    }
  }
  return 0;
}

void main() {
  vec2 uv = gl_TexCoord[0].xy;
  float index = traverseTree(uv);
  vec3 hint = rainbow(0.5 * float(index));
  vec3 textureColor = texture2D(texture, uv).rgb;
  gl_FragColor = vec4(mix(textureColor, hint, transparency), 1.0);
}
