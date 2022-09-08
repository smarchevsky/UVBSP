# UVBSP

Binary split partitioning of UV.

You can split UV texture islands and apply area indices in code. Index palette is not available now.

How does it work?

Draw somethimg like this:
![Or anything else](readme_images/img_something_like_this.png)

Press Ctrl + Shift + E, then press G (GLSL), H(GLSL) or U(Unreal) to export code.
Code will be copied to clipboard and printed to colsole.

<details>
<summary>Code example (with my comments):</summary>

```
#define VEC4 vec4
#define VEC2 vec2
#define REINTERPRET_TO_FLOAT intBitsToFloat
#define REINTERPRET_TO_UINT floatBitsToUint

// BSP tree itself starts here

#define MAX_DEPTH 4
VEC4 nodes[] = VEC4[](
VEC4(0.391138, 0.351669, 1.773810, REINTERPRET_TO_FLOAT(32769 * 65536 + 32771)),
VEC4(0.444099, 0.687861, -1.188525, REINTERPRET_TO_FLOAT(2 * 65536 + 32770)),
VEC4(0.790653, 0.486376, 1.835819, REINTERPRET_TO_FLOAT(32772 * 65536 + 4)),
VEC4(0.313998, 0.261864, -1.090117, REINTERPRET_TO_FLOAT(7 * 65536 + 6)),
VEC4(0.719270, 0.694769, -1.076355, REINTERPRET_TO_FLOAT(9 * 65536 + 8))
);

// IMPORTANT:
// Numbers after REINTERPRET_TO_FLOAT are indices of nodes or colors
// Numbers, multiplied by 65536 are left nodes
// Numbers, greater equal 32768 are nodes. Don't touch them, you can destroy the tree
// 0 ... 32767 color indices. Feel free to change numbers 1, 2, 3, 4 
// to set custom color index


const uint nodeIndexThreshold = uint(1 << 15);

uint traverseTree(VEC2 uv){
uint currentIndex = uint(0);
for(int iteration = 0; iteration < MAX_DEPTH; ++iteration) {
VEC2 pos = nodes[currentIndex].xy;
VEC2 tangent = VEC2(nodes[currentIndex].z, 1.0);
uint leftRight = REINTERPRET_TO_UINT(nodes[currentIndex].w); // reinterpret
bool isLeftPixel = dot(pos - uv, tangent) < 0.0;
uint indexOfProperSide = isLeftPixel ? leftRight >> 16 : (leftRight & uint(0x0000ffff));

if(indexOfProperSide >= nodeIndexThreshold) {
  currentIndex = indexOfProperSide - nodeIndexThreshold;
} else {
  return indexOfProperSide;
}
}
return uint(0);
}
// Result of function is color (or other parameter) index.
// Use it in switch-case or as array index.

// You can use it with rainbow function like this:
// const float pi_div3 = 1.047197551;
// vec3 rainbow(float val){
//   vec3 result = vec3(sin(val), sin(val + pi_div3), sin(val + pi_div3 * 2.0));
//   return result * result;
// }

```
</details>

You can use this code in ShaderToy, for example:
[https://www.shadertoy.com/view/sltBz8](https://https://www.shadertoy.com/view/sltBz8)

Press Ctrl + S to save project (file "test.uvbsp" to project folder)


My friend's drawing:
![my friends's drawing](readme_images/img_friends_drawing1.png)

Warning: number of splits is limited with GPU constant register size.

```
uniform vec4 nodes[512];
```

ToDo:

- Save/Load - WIP, you cannot edit after load, no choose path option, only "test.uvbsp" in CMake project folder.
- Figure out, why window is so laggy on drag.
- Windows support.
- Add Palette of color indices (with ImGui).
- Export as antialiased image;
- Refactor: get rid of lambdas, write "Application" class, load uniforms as plain float array.
- Read/write Json.

Done:

- Shader generator (GLSL/HLSL/Unreal) with static binary tree - done!.
- Ctrl-Z (!) - done!

Dependencies:

- SFML
