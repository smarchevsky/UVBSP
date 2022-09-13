#include <../thirdparty/base64.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <fstream>
#include <sstream>
#include <uvbsp.h>

static Vec4 packNodeToShader(BSPNode node, UVBSP::ExportArrayFormat format, std::stringstream* outStream = nullptr);
void UVBSP::addSplit(UVSplitAction split)
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

void UVBSP::updateUniforms(sf::Shader& shader)
{
    m_packedStructs.resize(m_nodes.size());
    for (int i = 0; i < m_nodes.size(); ++i) {
        m_packedStructs[i] = packNodeToShader(m_nodes[i], ExportArrayFormat::Float);
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
        size_t arraySize = dataString.size() / sizeof(BSPNode);
        const uint8_t* data = (uint8_t*)(void*)m_nodes.data();
        m_nodes.resize(arraySize);
        for (size_t i = 0; i < arraySize * sizeof(BSPNode); ++i) {
            ((uint8_t*)(void*)m_nodes.data())[i] = dataString[i];
        }
    }
    printNodes();
    // std::cout << str.size() << std::endl;
}

void UVBSP::writeToFile(const std::string& path)
{
    size_t arraySize = m_nodes.size() * sizeof(BSPNode);
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
    for (int i = 0; i < m_nodes.size(); ++i) {
        const auto& n = m_nodes[i];
        result += "Node: " + std::to_string(i) + ": "
            + printIndex(n.left) + ", " + printIndex(n.right) + " | ";
    }
    result += "\n";
    return result;
}

static Vec4 packNodeToShader(BSPNode node, UVBSP::ExportArrayFormat format, std::stringstream* outStream)
{
    constexpr float threshold = 1.f / (1 << 24); // vertical line
    if (abs(node.dir.y) < threshold)
        node.dir.y = threshold;
    const float tangent = node.dir.x / node.dir.y;
    uint tangentBits = reinterpret_cast<const uint&>(tangent);

    if (node.dir.y < 0)
        std::swap(node.left, node.right);

    uint leftRight = (node.left << 16) | node.right & 0xffff;

    switch (format) {
    case UVBSP::ExportArrayFormat::Float: {
        if (outStream) {
            (*outStream)
                << "VEC4("
                << std::to_string(node.pos.x) << ", " // first
                << std::to_string(node.pos.y) << ", " // second
                << std::to_string(tangent) << ", " // third
                // fourth
                << "REINTERPRET_TO_FLOAT("
                << std::to_string(node.left) << "u * 65536u + "
                << std::to_string(node.right) << "u))";
        }
    } break;
    case UVBSP::ExportArrayFormat::FloatAsUint: {
        if (outStream) {
            uint posXbits = reinterpret_cast<uint&>(node.pos.x);
            uint posYbits = reinterpret_cast<uint&>(node.pos.y);
            uint tangentBits = reinterpret_cast<const uint&>(tangent);

            (*outStream)
                << "UVEC4(" // << std::hex
                << posXbits << "u, " // first
                << posYbits << "u, " // second
                << tangentBits << "u, " // third
                << std::dec
                // fourth
                << "(" << std::to_string(node.left) << "u * 65536u + "
                << std::to_string(node.right) << "u))";
        }
    } break;
    case UVBSP::ExportArrayFormat::SingleFloatCoordAsUint: {
        if (outStream) {

            float normalizedPos = node.pos.x + node.pos.y / tangent;
            uint normalizedPosBits = reinterpret_cast<const uint&>(normalizedPos);

            (*outStream)
                << "UVEC3(" // << std::hex
                << normalizedPosBits << "u, " // first
                << tangentBits << "u, " // third
                << std::dec
                // fourth
                << "(" << std::to_string(node.left) << "u * 65536u + "
                << std::to_string(node.right) << "u))";
        }
    } break;
    }

    return Vec4(node.pos.x, node.pos.y, tangent, reinterpret_cast<float&>(leftRight));
}

std::stringstream UVBSP::generateShader(ShaderType shaderType, ExportArrayFormat arrayType) const
{
    bool isHLSL = shaderType != ShaderType::GLSL;
    const size_t arraySize = m_nodes.size();
    std::stringstream shaderText;
    //"intBitsToFloat", "asfloat"
    shaderText << "/////// START_UVBSP_GENERATED_SHADER ////////\n\n";

    shaderText << (isHLSL ? "#define UVEC4 uint4\n" : "#define UVEC4 uvec4\n")
               << (isHLSL ? "#define UVEC3 uint3\n" : "#define UVEC3 uvec3\n")
               << (isHLSL ? "#define VEC4 float4\n" : "#define VEC4 vec4\n")
               << (isHLSL ? "#define VEC2 float2\n" : "#define VEC2 vec2\n");

    if (isHLSL) {
        switch (arrayType) {

        case UVBSP::ExportArrayFormat::Float: // float type must be deprecated...
            shaderText << "#define REINTERPRET_TO_FLOAT(x) asfloat(~(x))\n"
                       << "#define REINTERPRET_TO_UINT(x) ~asuint(x)\n";
            break;
        case UVBSP::ExportArrayFormat::FloatAsUint:
        case UVBSP::ExportArrayFormat::SingleFloatCoordAsUint:
            shaderText << "#define REINTERPRET_TO_FLOAT(x) asfloat(x)\n"
                       << "#define REINTERPRET_TO_UINT(x) asuint(x)\n";
            break;
        }

    } else {
        shaderText << "#define REINTERPRET_TO_FLOAT(x) uintBitsToFloat(x)\n"
                   << "#define REINTERPRET_TO_UINT(x) floatBitsToUint(x)\n";
    }

    switch (arrayType) {
    case UVBSP::ExportArrayFormat::Float:
        shaderText << "VEC4 nodes[" << arraySize << "] = " << (isHLSL ? "{\n" : "VEC4[](\n");
        break;
    case UVBSP::ExportArrayFormat::FloatAsUint:
        shaderText << "UVEC4 nodes[" << arraySize << "] = " << (isHLSL ? "{\n" : "UVEC4[](\n");
        break;
    case UVBSP::ExportArrayFormat::SingleFloatCoordAsUint:
        shaderText << "UVEC3 nodes[" << arraySize << "] = " << (isHLSL ? "{\n" : "UVEC3[](\n");
        break;
    }

    for (int i = 0; i < m_nodes.size(); ++i) {
        packNodeToShader(m_nodes[i], arrayType, &shaderText);
        shaderText << ((i != (m_nodes.size() - 1)) ? ",\n" : "\n"); // if last - no comma
    }

    shaderText << (isHLSL ? "};\n\n" : ");\n\n");

    shaderText << (shaderType != ShaderType::UnrealCustomNode ? "uint traverseTree(VEC2 uv){\n" : "\n");

    shaderText << "  const uint nodeIndexThreshold = uint(1 << 15);\n";
    shaderText
        << "  uint currentIndex = 0u;\n"
           "  for(int iteration = 0; iteration < "
        << getMaxDepth(0) << "; ++iteration) {\n";
    switch (arrayType) {
    case UVBSP::ExportArrayFormat::Float: {
        shaderText << "    VEC2 pos = nodes[currentIndex].xy;\n"
                      "    VEC2 tangent = VEC2(nodes[currentIndex].z, 1.0);\n"
                      "    uint leftRight = REINTERPRET_TO_UINT(nodes[currentIndex].w); // reinterpret   \n";
    } break;
    case UVBSP::ExportArrayFormat::FloatAsUint: {
        shaderText << "    VEC2 pos = REINTERPRET_TO_FLOAT(nodes[currentIndex].xy);\n"
                      "    VEC2 tangent = VEC2(REINTERPRET_TO_FLOAT(nodes[currentIndex].z), 1.0);\n"
                      "    uint leftRight = nodes[currentIndex].w; // reinterpret   \n";
    } break;
    case UVBSP::ExportArrayFormat::SingleFloatCoordAsUint: {
        shaderText << "    VEC2 pos = VEC2(REINTERPRET_TO_FLOAT(nodes[currentIndex].x), 0.0);\n"
                      "    VEC2 tangent = VEC2(REINTERPRET_TO_FLOAT(nodes[currentIndex].y), 1.0);\n"
                      "    uint leftRight = nodes[currentIndex].z; // reinterpret   \n";
    } break;
    }

    shaderText
        << "    bool isLeftPixel = dot(pos - uv, tangent) < 0.0;\n"
           "    uint indexOfProperSide = isLeftPixel ? leftRight >> 16 : (leftRight & 0x0000ffffu);\n"
           "    \n"
           "    if(indexOfProperSide >= nodeIndexThreshold) {\n"
           "      currentIndex = indexOfProperSide - nodeIndexThreshold;\n"
           "    } else {\n"
           "      return indexOfProperSide;\n"
           "    }\n"
           "  }\n"
           "  return 0u;\n";

    shaderText << (shaderType != ShaderType::UnrealCustomNode ? "}\n" : "\n");
    shaderText << "/////// END_UVBSP_GENERATED_SHADER ////////\n"
               << std::endl;

    return shaderText;
}
