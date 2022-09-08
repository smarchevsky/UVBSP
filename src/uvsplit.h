#ifndef UVSPLIT_H
#define UVSPLIT_H

#include <SFML/Graphics/Shader.hpp>
#include <bitset>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vec2.h>
#include <vector>

constexpr ushort nodeIndexThreshold = 1 << 15;
/////////////////////////////////////////////
/// Node indices are represented as unsigned short 16 bit integers
/// 0 ... 32767 are color indices
/// 32768 ... 65536 are node indices
/// BSPNode constructor set color indices by default

// static const std::string lang_vec4_str[2] = { "vec4", "float4" };
// static const std::string lang_asFloat[2] = { "intBitsToFloat", "asfloat" };

struct BSPNode {

    BSPNode(vec2 p, vec2 d, ushort l, ushort r)
        : pos(p)
        , dir(d)
        , left(l)
        , right(r)
    {
    }

    vec2 pos, dir;
    ushort left, right;
};

struct UVSplitAction {
    UVSplitAction(vec2 pos, vec2 dir, ushort c0, ushort c1)
        : pos(pos)
        , dir(dir)
        , c0(c0)
        , c1(c1)
    {
    }
    vec2 pos, dir;
    ushort c0, c1;
};

class UVSplit {
    const vec2 m_imageSize;
    std::vector<BSPNode> m_nodes;
    std::vector<sf::Glsl::Vec4> m_packedStructs;
    BSPNode* m_currentNode {};

    bool m_initialSet {};

public:
    static bool isNodeLink(ushort index) { return index >= nodeIndexThreshold; }

    UVSplit(const vec2& imageSize)
        : m_imageSize(imageSize)
    {
        reset();
    }

    ushort getMaxDepth(ushort nodeIndex = 0) const
    {
        ushort left {}, right {};
        if (isNodeLink(m_nodes[nodeIndex].left)) {
            left = getMaxDepth(m_nodes[nodeIndex].left - nodeIndexThreshold);
        }
        if (isNodeLink(m_nodes[nodeIndex].right)) {
            right = getMaxDepth(m_nodes[nodeIndex].right - nodeIndexThreshold);
        }
        return std::max(left, right) + 1;
    }
    size_t getNumNodes() const { return m_nodes.size(); }

    void addSplit(UVSplitAction split)
    {
        if (!m_initialSet) {
            m_nodes[0] = BSPNode(split.pos, split.dir, split.c0, split.c1);
            m_currentNode = &m_nodes[0];
            m_initialSet = true;

        } else {
            int currentIndex = 0;
            for (int iteration = 0; iteration < 64; ++iteration) {
                bool isLeftPixel = dot(m_nodes[currentIndex].pos - split.pos, m_nodes[currentIndex].dir) < 0.0;
                uint16_t& indexOfProperSide = isLeftPixel ? m_nodes[currentIndex].left : m_nodes[currentIndex].right;

                if (isNodeLink(indexOfProperSide)) {
                    currentIndex = indexOfProperSide - nodeIndexThreshold;

                } else {
                    indexOfProperSide = m_nodes.size() + nodeIndexThreshold;

                    m_nodes.emplace_back(BSPNode(split.pos, split.dir, split.c0, split.c1));
                    m_currentNode = &m_nodes.back();

                    break;
                }
            }
        }
    }
    void adjustSplit(const vec2& uvDir)
    {
        if (m_currentNode) {
            m_currentNode->dir = uvDir;
        }
    }

    static sf::Glsl::Vec4 packNodeToVec4(BSPNode node, std::string* nodeAsShaderText = nullptr)
    {
        float tangent = node.dir.x / node.dir.y;
        float range = 99999.f;
        std::clamp(tangent, -range, range);
        if (node.dir.y < 0)
            std::swap(node.left, node.right);

        uint leftRight = (node.left << 16) | node.right & 0xffff;

        if (nodeAsShaderText) {
            // isNodeLink(node.left);
            (*nodeAsShaderText) += "VEC4("
                + std::to_string(node.pos.x) + ", " // first
                + std::to_string(node.pos.y) + ", " // second
                + std::to_string(tangent) + ", " // Third
                                                 // fourth
                + "REINTERPRET_TO_FLOAT("
                + std::to_string(node.left) + " * 65536 + "
                + std::to_string(node.right)
                + "))";
        }

        return sf::Glsl::Vec4(node.pos.x, node.pos.y, tangent, reinterpret_cast<float&>(leftRight));
    }

    void updateUniforms(sf::Shader& shader)
    {
        m_packedStructs.resize(m_nodes.size());

        for (int i = 0; i < m_nodes.size(); ++i)
            m_packedStructs[i] = packNodeToVec4(m_nodes[i]);

        shader.setUniformArray("nodes", m_packedStructs.data(), m_packedStructs.size());
    }

    void reset()
    {
        m_nodes.clear();
        m_nodes.push_back({ vec2(0.5f, 0.5f), vec2(1, 1), 0, 0 });
        m_packedStructs.clear();
        m_currentNode = nullptr;
        m_initialSet = false;
    }

    const BSPNode* getLastNode() const { return m_currentNode; }

    // STRINGS ! //
public:
    static std::string printIndex(ushort index)
    {
        return (index < nodeIndexThreshold)
            ? "color " + std::to_string(index)
            : "next " + std::to_string(index - nodeIndexThreshold);
    }

    std::string stringNodes()
    {
        std::string result;
        for (int i = 0; i < m_nodes.size(); ++i) {
            const auto& n = m_nodes[i];
            result += "Node: " + std::to_string(i) + ": "
                + printIndex(n.left) + ", " + printIndex(n.right) + " | ";
        }
        result += "\n";
        return result;
    }

    enum class ShaderType {
        GLSL,
        HLSL,
        UnrealCustomNode
    };

    std::string generateShader(ShaderType shaderType) const
    {
        bool isHLSL = shaderType != ShaderType::GLSL;
        std::string shaderText;
        //"intBitsToFloat", "asfloat"
        shaderText += isHLSL ? "#define VEC4 float4\n"
                             : "#define VEC4 vec4\n";
        shaderText += isHLSL ? "#define VEC2 float2\n"
                             : "#define VEC2 vec2\n";
        shaderText += isHLSL ? "#define REINTERPRET_TO_FLOAT asfloat\n"
                             : "#define REINTERPRET_TO_FLOAT intBitsToFloat\n";
        shaderText += isHLSL ? "#define REINTERPRET_TO_UINT asuint\n"
                             : "#define REINTERPRET_TO_UINT floatBitsToUint\n";
        shaderText += "#define MAX_DEPTH " + std::to_string(getMaxDepth(0)) + "\n";

        shaderText += "VEC4 nodes[] = ";
        shaderText += isHLSL ? "{\n" : "VEC4[](\n";
        for (int i = 0; i < m_nodes.size(); ++i) {
            const auto& n = m_nodes[i];
            packNodeToVec4(n, &shaderText);
            shaderText += (i == (m_nodes.size() - 1))
                ? "\n"
                : ",\n";
        }

        shaderText += isHLSL ? "};\n\n" : ");\n\n";

        shaderText += "const uint nodeIndexThreshold = uint(1 << 15);\n\n";
        shaderText += shaderType != ShaderType::UnrealCustomNode ? "uint traverseTree(VEC2 uv){\n" : "\n";
        shaderText
            += "  uint currentIndex = uint(0);\n"
               "  for(int iteration = 0; iteration < MAX_DEPTH; ++iteration) {\n"
               "    VEC2 pos = nodes[currentIndex].xy;\n"
               "    VEC2 tangent = VEC2(nodes[currentIndex].z, 1.0);\n"
               "    uint leftRight = REINTERPRET_TO_UINT(nodes[currentIndex].w); // reinterpret   \n"
               "    bool isLeftPixel = dot(pos - uv, tangent) < 0.0;\n"
               "    uint indexOfProperSide = isLeftPixel ? leftRight >> 16 : (leftRight & uint(0x0000ffff));\n"
               "    \n"
               "    if(indexOfProperSide >= nodeIndexThreshold) {\n"
               "      currentIndex = indexOfProperSide - nodeIndexThreshold;\n"
               "    } else {\n"
               "      return indexOfProperSide;\n"
               "    }\n"
               "  }\n"
               "  return uint(0);\n";
        shaderText += shaderType != ShaderType::UnrealCustomNode ? "}\n" : "\n";

        return shaderText;
    }
};

#endif // UVSPLIT_H
