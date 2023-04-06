# UVBSP

Binary split partitioning of UV.

You can segment the texture islands in order to assign individual index to segments. Then you can set individual color (or any other parameter) to each of the segments. Using switch-case or as array index.
This makes possible to set an individual color scheme for each mesh, using only an array of uniform variables, without an additional RGBA masks, individual textures or vertex colors.

How to use?

Split UV islands of texture into segments:

<img src="readme_images/img_unrealguy_AO_texture.png"  width=40%>

(Less tree depth is better. Try keep amount of left and right branches the same)

Press Ctrl + Shift + E, then press G (GLSL), H(HLSL) or U(Unreal) to export code.
Code will be copied to clipboard and printed to colsole.

<details>
<summary>Code example (with my comments):</summary>

```
/////// START_UVBSP_GENERATED_SHADER ////////

#define IVEC4 int4
#define VEC2 float2
#define REINTERPRET_TO_FLOAT(x) asfloat(x)
#define REINTERPRET_TO_UINT(x) asuint(x)

// pos bits, dir bits, left, right indices: 
// node(less than 0) or color(greater equal 0)
IVEC4 nodes[27] = {
/*0*/   IVEC4(-1092754264, -1089634839, -10, -1),
/*1*/   IVEC4(1058094944, -1072292208, -2, -8),
/*2*/   IVEC4(1062248336, 1058970123, -5, -3),
/*3*/   IVEC4(1051469292, -1060331344, -4, 1), // 1 head
/*4*/   IVEC4(1048425202, -1079215180, 2, 1), // 2 chest
/*5*/   IVEC4(1086720384, 1036413919, -6, -7),
/*6*/   IVEC4(1067520828, 1062310698, 3, 0), // 3 fingers
/*7*/   IVEC4(1063892926, 1076116367, 3, 4), // 4 - hand, press
/*8*/   IVEC4(1088970856, 1030642176, -9, 5), // 5 - arm up
/*9*/   IVEC4(-886531122, -1283457024, 6, 7), // 6 toes, 7 arm low
/*10*/  IVEC4(1094710630, 1031102317, -11, -18),
/*11*/  IVEC4(1070379715, 1064515338, -12, -15),
/*12*/  IVEC4(1083187336, 1048650279, -13, -14),
/*13*/  IVEC4(1050460631, -1074426496, 8, 8), // 8 heels, feet
/*14*/  IVEC4(1044121276, -1080987739, 8, 6),
/*15*/  IVEC4(1050897851, -1061775376, -17, -16),
/*16*/  IVEC4(-1086914499, -1086510399, 8, 9), // 9 leg low
/*17*/  IVEC4(1052821457, 1115925890, 10, 11), // 10 koleni inside, 11 leg up
/*18*/  IVEC4(1067175906, 1058195895, -19, -24),
/*19*/  IVEC4(1092294764, 1031963397, -20, -21),
/*20*/  IVEC4(1050117916, -1059271926, -23, 3),
/*21*/  IVEC4(1056852352, -1045654728, -22, 3),
/*22*/  IVEC4(1069108962, 1058825369, 3, 12), // 12 hip
/*23*/  IVEC4(1072525436, 1052311783, 11, 12),
/*24*/  IVEC4(1057632580, 1066833677, -25, -26),
/*25*/  IVEC4(-1099055552, -1086038705, 12, 4),
/*26*/  IVEC4(-1067681979, -1109768931, 12, 2)
};


  int currentIndex = 0;
  for(int iteration = 0; iteration < 6; ++iteration) {
    VEC2 pos = VEC2(REINTERPRET_TO_FLOAT(nodes[currentIndex].x), 0.0);
    VEC2 tangent = VEC2(REINTERPRET_TO_FLOAT(nodes[currentIndex].y), 1.0);
    bool isLeftPixel = dot(pos - uv, tangent) < 0.0;
    int indexOfProperSide = isLeftPixel ? nodes[currentIndex].z : nodes[currentIndex].w;

    if(indexOfProperSide < 0) {
      currentIndex = -indexOfProperSide;
    } else {
      return indexOfProperSide;
    }
  }
  return 0;

#undef IVEC4
#undef VEC2
#undef REINTERPRET_TO_FLOAT
#undef REINTERPRET_TO_UINT
/////// END_UVBSP_GENERATED_SHADER ////////

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
<img src="readme_images/img_unrealguy_BSP_screenshot.png">
<details>

<summary>I adjusted color indices to achieve result above:</summary>

```
VEC4(0.380891, 0.601679, 0.111719, REINTERPRET_TO_FLOAT(32774 * 65536 + 32769)),
VEC4(0.565569, 0.330703, 2.003758, REINTERPRET_TO_FLOAT(32784 * 65536 + 32770)),
VEC4(0.250003, 0.350378, 0.471642, REINTERPRET_TO_FLOAT(32771 * 65536 + 32772)),
VEC4(0.356686, 0.462166, -0.749682, REINTERPRET_TO_FLOAT(0 * 65536 + 1)),
VEC4(0.364754, 0.175988, -6.539556, REINTERPRET_TO_FLOAT(32773 * 65536 + 2)),
VEC4(0.336963, 0.120540, -1.356440, REINTERPRET_TO_FLOAT(1 * 65536 + 2)),
VEC4(0.720663, 0.834199, 0.946730, REINTERPRET_TO_FLOAT(32775 * 65536 + 32778)),
VEC4(0.906237, 0.820784, -1.200455, REINTERPRET_TO_FLOAT(32776 * 65536 + 3)),
VEC4(0.781625, 0.923630, -2.506680, REINTERPRET_TO_FLOAT(4 * 65536 + 32777)),
VEC4(0.854241, 0.920947, 0.245704, REINTERPRET_TO_FLOAT(0 * 65536 + 5)),
VEC4(0.456197, 0.785906, -5.706050, REINTERPRET_TO_FLOAT(32779 * 65536 + 32781)),
VEC4(0.362065, 0.798427, -8.016128, REINTERPRET_TO_FLOAT(32780 * 65536 + 0)),
VEC4(0.053670, 0.666963, 0.612140, REINTERPRET_TO_FLOAT(6 * 65536 + 0)),
VEC4(0.599636, 0.682166, 0.058680, REINTERPRET_TO_FLOAT(32782 * 65536 + 32783)),
VEC4(0.527020, 0.918264, -0.703834, REINTERPRET_TO_FLOAT(0 * 65536 + 7)),
VEC4(0.803141, 0.569483, -4.655283, REINTERPRET_TO_FLOAT(8 * 65536 + 0)),
VEC4(0.709905, 0.519402, 3.122803, REINTERPRET_TO_FLOAT(32785 * 65536 + 32787)),
VEC4(0.775349, 0.480053, -2.394145, REINTERPRET_TO_FLOAT(8 * 65536 + 32786)),
VEC4(0.857827, 0.387939, 0.124695, REINTERPRET_TO_FLOAT(9 * 65536 + 10)),
VEC4(0.588878, 0.544443, 0.072449, REINTERPRET_TO_FLOAT(32788 * 65536 + 10)),
VEC4(0.509986, 0.570378, 1.077365, REINTERPRET_TO_FLOAT(0 * 65536 + 0))
```
</details>
Press Ctrl + S to save project (file "test.uvbsp" to project folder).
Press Ctrl + O to open "test.uvbsp" from project folder.

# Drawing

You can try draw your art and export to ShaderToy, my example:
[https://www.shadertoy.com/view/sltBz8](https://www.shadertoy.com/view/sltBz8)

My friend's drawing:
![my friends's drawing](readme_images/img_friends_drawing1.png)

Warning: number of splits is limited with GPU constant register size.

```
uniform vec4 nodes[512];
```

# ToDo:

- Save/Load - WIP, you cannot edit after load.
- Windows support.
- Add Palette of color indices (with ImGui).
- Export as antialiased image.
- Load uniforms as plain int array.
- Read/write Json.

# Dependencies:

- SFML
