#ifndef UVSPLIT_H
#define UVSPLIT_H

#include <SFML/Graphics/Shader.hpp>
#include <bitset>
#include <iostream>

#include <string>
#include <vec2.h>
#include <vector>

typedef sf::Glsl::Vec4 Vec4;

// clang-format off
struct UVBSPSplit {
    UVBSPSplit() = default;
    UVBSPSplit(vec2 pos, vec2 dir, ushort l, ushort r) : pos(pos), dir(dir), l(l), r(r) {}
    vec2 pos, dir;
    int l, r;
};
// clang-format on
////////////////////////////////// UVBSP //////////////////////////////

class UVBSP {
public:
    enum class ShaderType {
        GLSL,
        HLSL,
        UnrealCustomNode
    };

private:
    std::vector<UVBSPSplit> m_nodes;
    std::vector<Vec4> m_packedStructs;
    UVBSPSplit* m_currentNode {};

    bool m_initialSet {};

public:
    // static bool isNodeLink(ushort index) { return index < 0; }

    UVBSP() { reset(); }

    int getMaxDepth(int nodeIndex) const
    {
        int left {}, right {};
        if (m_nodes[-nodeIndex].l < 0)
            left = getMaxDepth(m_nodes[-nodeIndex].l);
        if (m_nodes[-nodeIndex].r < 0)
            right = getMaxDepth(m_nodes[-nodeIndex].r);
        return std::max(left, right) + 1;
    }

    size_t getNumNodes() const { return m_nodes.size(); }

    void addSplit(UVBSPSplit split);

    void adjustSplit(const vec2& uvDir)
    {
        if (m_currentNode)
            m_currentNode->dir = uvDir;
    }

    void updateUniforms(sf::Shader& shader);

    bool readFromFile(const std::string& path);
    void writeToFile(const std::string& path);

    void reset();

    const UVBSPSplit* getLastNode() const { return m_currentNode; }

    // STRINGS ! //
public:
    static std::string printIndex(int index)
    {
        return (index < 0) ? "next " + std::to_string(-index) : "color " + std::to_string(index);
    }

    std::string printNodes();
    std::string getBasicInfo()
    {
        return "Node count: " + std::to_string(getNumNodes())
            + "   Tree depth: " + std::to_string(getMaxDepth(0));
    }
    std::stringstream generateShader(ShaderType shaderType) const;
};

////////////////////////////////// UVBSP HISTORY //////////////////////////////

class UVBSPActionHistory {
public:
    UVBSPActionHistory(UVBSP& uvSplit)
        : m_uvSplit(uvSplit)
    {
    }

    void add(const UVBSPSplit& action)
    {
        if (m_drawHistory.size() > m_currentIndex)
            m_drawHistory[m_currentIndex] = action;
        else
            m_drawHistory.push_back(action);

        // assert(false && "m_currentIndex is beyond drawHistory size");

        m_currentIndex++;
    }

    bool undo()
    {
        if (m_currentIndex > 0) {
            m_uvSplit.reset();
            for (int i = 0; i < m_currentIndex - 1; ++i) {
                const auto& action = m_drawHistory[i];
                m_uvSplit.addSplit(action);
            }
            m_currentIndex--;
            return true;
        }
        return false;
    }

private:
    std::vector<UVBSPSplit> m_drawHistory;
    int m_currentIndex {};
    UVBSP& m_uvSplit;
};

#endif // UVSPLIT_H
