#ifndef UVSPLIT_H
#define UVSPLIT_H

#include <SFML/Graphics/Shader.hpp>
#include <bitset>
#include <iostream>

#include <string>
#include <vec2.h>
#include <vector>

typedef sf::Glsl::Vec4 Vec4;

constexpr ushort nodeIndexThreshold = 1 << 15;
/////////////////////////////////////////////
/// Node indices are represented as unsigned short 16 bit integers
/// 0 ... 32767 are color indices
/// 32768 ... 65536 are node indices
/// BSPNode constructor set color indices by default

// static const std::string lang_vec4_str[2] = { "vec4", "float4" };
// static const std::string lang_asFloat[2] = { "intBitsToFloat", "asfloat" };

struct BSPNode {
    BSPNode() = default;
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
    std::vector<BSPNode> m_nodes;
    std::vector<Vec4> m_packedStructs;
    BSPNode* m_currentNode {};

    bool m_initialSet {};

public:
    // clang-format off
    enum class ShaderType { GLSL, HLSL, UnrealCustomNode };
    enum class ExportArrayFormat { Float, FloatAsUint } m_exportType = ExportArrayFormat::FloatAsUint;
    // clang-format on

public:
    static bool isNodeLink(ushort index) { return index >= nodeIndexThreshold; }

    UVSplit(const vec2& imageSize) { reset(); }

    ushort getMaxDepth(ushort nodeIndex = 0) const
    {
        ushort left {}, right {};
        if (isNodeLink(m_nodes[nodeIndex].left))
            left = getMaxDepth(m_nodes[nodeIndex].left - nodeIndexThreshold);
        if (isNodeLink(m_nodes[nodeIndex].right))
            right = getMaxDepth(m_nodes[nodeIndex].right - nodeIndexThreshold);
        return std::max(left, right) + 1;
    }

    size_t getNumNodes() const { return m_nodes.size(); }

    void addSplit(UVSplitAction split);

    void adjustSplit(const vec2& uvDir)
    {
        if (m_currentNode)
            m_currentNode->dir = uvDir;
    }

    void updateUniforms(sf::Shader& shader);

    bool readFromFile(const std::string& path);
    void writeToFile(const std::string& path);

    void reset();

    const BSPNode* getLastNode() const { return m_currentNode; }

    // STRINGS ! //
public:
    static std::string printIndex(ushort index)
    {
        return (index < nodeIndexThreshold)
            ? "color " + std::to_string(index)
            : "next " + std::to_string(index - nodeIndexThreshold);
    }

    std::string printNodes();

    std::stringstream generateShader(ShaderType shaderType, ExportArrayFormat arrayType) const;
};

#endif // UVSPLIT_H
