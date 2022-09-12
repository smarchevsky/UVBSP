#version 120
#extension GL_ARB_shader_bit_encoding : enable
#extension GL_EXT_gpu_shader4 : enable

uniform sampler2D texture;
const float pi = 3.1415926535;
const float pi_inv = 1. / pi;

vec3 fastSin(vec3 x) { x = fract(x * 2.) - 1.; return (4. * x) * (1. - abs(x)); }

vec3 rainbow(float val){
    vec3 result = fastSin(vec3(val * 0.313 + vec3(0, 0.33333, 0.66666)));
    return result * result;
}

uniform vec4 nodes[512];

/// Node indices are represented as unsigned short 16 bit integers
/// 0 ... 32767 are color indices
/// 32768 ... 65536 are node indices
/// BSPNode constructor set color indices by default

#define MAX_DEPTH 64

uint traverseTree(vec2 uv){
  const uint nodeIndexThreshold = uint(1 << 15);
  uint currentIndex = uint(0);

  for(int iteration = 0; iteration < MAX_DEPTH; ++iteration) {
    vec2 pos = nodes[currentIndex].xy;
    vec2 tangent = vec2(nodes[currentIndex].z, 1.0);

    uint leftRight = floatBitsToUint(nodes[currentIndex].w); // reinterpret   
    bool isLeftPixel = dot(pos - uv, tangent) < 0.0;

    uint indexOfProperSide = isLeftPixel ? leftRight >> 16 : (leftRight & 0x0000ffffu);
    
    if(indexOfProperSide >= nodeIndexThreshold) {
      currentIndex = indexOfProperSide - nodeIndexThreshold;
    } else {
      return indexOfProperSide;
    }
  }
  return uint(0);
}



void main() {
  vec2 uv = gl_TexCoord[0].xy;
  uint index = traverseTree(uv); vec3 hint = rainbow(0.5 * float(index));
  vec3 textureColor = texture2D(texture, uv).rgb;
  gl_FragColor = vec4(mix(textureColor, hint, vec3(0.99)), 1.0);
  // gl_FragColor = vec4(hint, 1.0);
}
