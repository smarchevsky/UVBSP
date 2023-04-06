#include "base64.hpp"
#include <SFML/Graphics/Shader.hpp>
#include <fstream>
#include <sstream>
#include <uvbsp/uvbsp.h>

static Vec4 packNodeToShader(UVBSPSplit node, std::stringstream* outStream = nullptr);
void UVBSP::addSplit(UVBSPSplit split)
{
    if (!m_initialSet) {
        m_nodes[0] = UVBSPSplit(split.pos, split.dir, split.l, split.r);
        m_currentNode = &m_nodes[0];
        m_initialSet = true;

    } else {
        int currentIndex = 0;
        for (int iteration = 0; iteration < 64; ++iteration) {
            bool isLeftPixel = dot(m_nodes[currentIndex].pos - split.pos, m_nodes[currentIndex].dir) < 0.0;
            int& indexOfProperSide = isLeftPixel ? m_nodes[currentIndex].l : m_nodes[currentIndex].r;

            if (indexOfProperSide < 0) {
                currentIndex = -indexOfProperSide;
            } else {
                indexOfProperSide = -m_nodes.size();

                m_nodes.emplace_back(UVBSPSplit(split.pos, split.dir, split.l, split.r));
                m_currentNode = &m_nodes.back();

                break;
            }
        }
    }
}

void UVBSP::updateUniforms(sf::Shader& shader)
{
    m_packedStructs.resize(m_nodes.size());
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        m_packedStructs[i] = packNodeToShader(m_nodes[i]);
    }
    shader.setUniformArray("nodes", m_packedStructs.data(), m_packedStructs.size());
}

bool UVBSP::readFromFile(const std::string& path)
{
    std::ifstream myfile(path);
    std::string baseString((std::istreambuf_iterator<char>(myfile)), std::istreambuf_iterator<char>());

    if (baseString.size()) {
        reset();
        std::string dataString = websocketpp::base64_decode(baseString);
        size_t arraySize = dataString.size() / sizeof(UVBSPSplit);

        m_nodes.resize(arraySize);
        for (size_t i = 0; i < arraySize * sizeof(UVBSPSplit); ++i) {
            ((uint8_t*)(void*)m_nodes.data())[i] = dataString[i];
        }
        printNodes();
        return true;
    }
    return false;
}

void UVBSP::writeToFile(const std::string& path)
{
    size_t arraySize = m_nodes.size() * sizeof(UVBSPSplit);
    const uint8_t* data = (uint8_t*)(void*)m_nodes.data();
    if (arraySize) {
        std::ofstream myfile(path, std::ios::out);
        myfile << websocketpp::base64_encode(data, arraySize);
    }
}

void UVBSP::reset()
{
    m_nodes.clear();
    m_nodes.push_back({ vec2(0.5f, 0.5f), vec2(1, 1), 0, 0 });
    m_packedStructs.clear();
    m_currentNode = nullptr;
    m_initialSet = false;
}

std::string UVBSP::printNodes()
{
    std::string result;
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        const auto& n = m_nodes[i];
        result += "Node: " + std::to_string(i) + ": "
            + printIndex(n.l) + ", " + printIndex(n.r) + " | ";
    }
    result += "\n";
    return result;
}

static Vec4 packNodeToShader(UVBSPSplit node, std::stringstream* outStream)
{
    constexpr float threshold = 1.f / (1 << 24); // almost vertical line
    if (abs(node.dir.x) < threshold)
        node.dir.x = threshold;
    if (abs(node.dir.y) < threshold)
        node.dir.y = threshold;

    const float tangent = node.dir.x / node.dir.y;

    if (node.dir.y < 0)
        std::swap(node.l, node.r);

    float normalizedPos = node.pos.x + node.pos.y / tangent;

    if (outStream) {
        (*outStream)
            << "IVEC4("
            << reinterpret_cast<const int&>(normalizedPos) << ", "
            << reinterpret_cast<const int&>(tangent) << ", "
            << node.l << ", "
            << node.r << ")";
    }

    return Vec4(normalizedPos, tangent,
        reinterpret_cast<const float&>(node.l),
        reinterpret_cast<const float&>(node.r));
}

std::stringstream UVBSP::generateShader(ShaderType shaderType) const
{
    bool isHLSL = shaderType != ShaderType::GLSL;
    const size_t arraySize = m_nodes.size();
    std::stringstream shaderText;
    //"intBitsToFloat", "asfloat"
    shaderText << "/////// START_UVBSP_GENERATED_SHADER ////////\n\n";

    shaderText << (isHLSL ? "#define IVEC4 int4\n" : "#define IVEC4 ivec4\n")
               << (isHLSL ? "#define VEC2 float2\n" : "#define VEC2 vec2\n");

    if (isHLSL) {
        shaderText << "#define REINTERPRET_TO_FLOAT(x) asfloat(x)\n"
                   << "#define REINTERPRET_TO_UINT(x) asuint(x)\n";
    } else {
        shaderText << "#define REINTERPRET_TO_FLOAT(x) intBitsToFloat(x)\n"
                   << "#define REINTERPRET_TO_UINT(x) floatBitsToInt(x)\n";
    }
    shaderText << "\n// pos bits, dir bits, left, right indices: \n"
                  "// node(less than 0) or color(greater equal 0)\n";
    shaderText << "IVEC4 nodes[" << arraySize << "] = " << (isHLSL ? "{\n" : "IVEC4[](\n");

    for (size_t i = 0; i < arraySize; ++i) {

        packNodeToShader(m_nodes[i], &shaderText);
        if (i != arraySize - 1)
            shaderText << ",";
        shaderText << "\n";
    }

    shaderText << (isHLSL ? "};\n\n" : ");\n\n");
    shaderText << (shaderType != ShaderType::UnrealCustomNode ? "int traverseTree(VEC2 uv){\n" : "\n");
    shaderText
        << "  int currentIndex = 0;\n"

           "  for(int iteration = 0; iteration < "
        << getMaxDepth(0) << "; ++iteration) {\n";

    shaderText
        << "    VEC2 pos = VEC2(REINTERPRET_TO_FLOAT(nodes[currentIndex].x), 0.0);\n"
           "    VEC2 tangent = VEC2(REINTERPRET_TO_FLOAT(nodes[currentIndex].y), 1.0);\n"
           "    bool isLeftPixel = dot(pos - uv, tangent) < 0.0;\n"
           "    int indexOfProperSide = isLeftPixel ? nodes[currentIndex].z : nodes[currentIndex].w;\n\n"
           "    if(indexOfProperSide < 0) {\n"
           "      currentIndex = -indexOfProperSide;\n"
           "    } else {\n"
           "      return indexOfProperSide;\n"
           "    }\n"
           "  }\n"
           "  return 0;\n";

    shaderText << (shaderType != ShaderType::UnrealCustomNode ? "}\n" : "\n");

    shaderText << "#undef IVEC4\n"
                  "#undef VEC2\n"
                  "#undef REINTERPRET_TO_FLOAT\n"
                  "#undef REINTERPRET_TO_UINT\n";

    shaderText
        << "/////// END_UVBSP_GENERATED_SHADER ////////\n"
        << std::endl;

    return shaderText;
}
