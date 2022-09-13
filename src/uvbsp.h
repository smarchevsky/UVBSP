#ifndef UVSPLIT_H
#define UVSPLIT_H

#include <SFML/Graphics/Shader.hpp>
#include <bitset>
#include <iostream>

#include <string>
#include <vec2.h>
#include <vector>

typedef sf::Glsl::Vec4 Vec4;

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
    int left, right;
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
    int c0, c1;
};

class UVBSP {
public:
    enum class ShaderType { GLSL,
        HLSL,
        UnrealCustomNode };

private:
    std::vector<BSPNode> m_nodes;
    std::vector<Vec4> m_packedStructs;
    BSPNode* m_currentNode {};

    bool m_initialSet {};

public:
    // static bool isNodeLink(ushort index) { return index < 0; }

    UVBSP() { reset(); }

    int getMaxDepth(int nodeIndex) const
    {
        int left {}, right {};
        if (m_nodes[-nodeIndex].left < 0)
            left = getMaxDepth(m_nodes[-nodeIndex].left);
        if (m_nodes[-nodeIndex].right < 0)
            right = getMaxDepth(m_nodes[-nodeIndex].right);
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
    static std::string printIndex(int index)
    {
        return (index < 0) ? "next " + std::to_string(-index) : "color " + std::to_string(index);
    }

    std::string printNodes();

    std::stringstream generateShader(ShaderType shaderType) const;
};

#endif // UVSPLIT_H
