#ifndef UVSPLIT_H
#define UVSPLIT_H

#include <SFML/Graphics/Shader.hpp>
#include <bitset>
#include <iostream>
#include <string>
#include <vec2.h>
#include <vector>

/////////////////////////////////////////////
/// Node indices are represented as unsigned short 16 bit integers
/// 0 ... 32767 are node indices
/// 32768 ... 65536 are out color indices
/// BSPNode constructor set color indices by default
constexpr uint16_t colorIndexThreshold = 1 << 15;

struct BSPNode {
    BSPNode(vec2 p, vec2 d, uint16_t l, uint16_t r)
        : pos(p)
        , dir(d)
        , left(l + colorIndexThreshold)
        , right(r + colorIndexThreshold)
    {
    }
    vec2 pos, dir;
    uint16_t left, right;
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
public:
    static bool isNodeLink(ushort index) { return index < colorIndexThreshold; }
    UVSplit(const vec2& imageSize)
        : m_imageSize(imageSize)
    {
        reset();
        printNodes();
    }
    static std::string printIndex(ushort index)
    {
        return (index < colorIndexThreshold)
            ? "next " + std::to_string(index)
            : "color " + std::to_string(index - colorIndexThreshold);
    }
    void printNodes()
    {
        for (int i = 0; i < m_nodes.size(); ++i) {
            const auto& n = m_nodes[i];

            std::printf("Node %d: %s, %s |  ", i, printIndex(n.left).c_str(), printIndex(n.right).c_str());
        }
        std::cout << std::endl;
    }

    ushort getMaxDepth(ushort nodeIndex)
    {
        ushort left {}, right {};
        if (isNodeLink(m_nodes[nodeIndex].left)) {
            left = getMaxDepth(m_nodes[nodeIndex].left);
        }
        if (isNodeLink(m_nodes[nodeIndex].right)) {
            right = getMaxDepth(m_nodes[nodeIndex].right);
        }
        return std::max(left, right) + 1;
    }

    void printDepth()
    {
        int depth = getMaxDepth(0);
        std::cout << "Tree depth: " << depth << std::endl;
    }

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
                    currentIndex = indexOfProperSide;

                } else {
                    indexOfProperSide = m_nodes.size();

                    m_nodes.emplace_back(BSPNode(split.pos, split.dir, split.c0, split.c1));
                    m_currentNode = &m_nodes.back();

                    break;
                }
            }
        }
        /* auto depth = getMaxDepth(0);
         depth -= 1;
         if(!isNodeLink(m_currentNode->left))
             m_currentNode->left = depth * 4 + colorIndexThreshold + 1;
         if(!isNodeLink(m_currentNode->right))
             m_currentNode->right = depth * 4 + colorIndexThreshold;*/

    }
    void adjustSplit(const vec2& uvDir)
    {
        if (m_currentNode) {
            m_currentNode->dir = uvDir;
        }
    }

    static sf::Glsl::Vec4 packNodeToVec4(BSPNode node)
    {
        float tangent = node.dir.x / node.dir.y;
        float range = 99999.f;
        std::clamp(tangent, -range, range);
        if (node.dir.y < 0)
            std::swap(node.left, node.right);

        uint leftRight = (node.left << 16) | node.right & 0xffff;

        /*uint right = leftRight & 0x0000ffff;
        uint left = leftRight >> 16;
        std::printf("leftRight %u, left  %u, right %u\n", leftRight, left, right);
        std::cout << std::bitset<32>(leftRight) << std::endl;
        std::cout << std::endl;*/

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

private:
    const vec2 m_imageSize;
    std::vector<BSPNode> m_nodes;
    std::vector<sf::Glsl::Vec4> m_packedStructs;
    BSPNode* m_currentNode {};

    bool m_initialSet {};
};

#endif // UVSPLIT_H
