#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp> // 引入 JSON 库

#include "importer-step.hpp"
#include "importer-iges.hpp"
#include "importer-brep.hpp"

#include <BRepTools.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>

using json = nlohmann::json;

class JsonWriter
{
public:
    JsonWriter() : meshCount(0) {}

    json WriteNode(const NodePtr& node, json& globalMeshes)
    {
        json nodeObj;
        nodeObj["name"] = node->GetName();

        // 写入网格引用索引
        json meshesArr = json::array();
        WriteMeshes(node, meshesArr, globalMeshes);
        nodeObj["meshes"] = meshesArr;

        // 写入子节点
        json childrenArr = json::array();
        for (const NodePtr& child : node->GetChildren())
        {
            childrenArr.push_back(WriteNode(child, globalMeshes));
        }
        nodeObj["children"] = childrenArr;

        return nodeObj;
    }

private:
    void WriteMeshes(const NodePtr& node, json& meshesArr, json& globalMeshes)
    {
        if (!node->IsMeshNode()) {
            return;
        }

        int nodeMeshCount = 0;
        node->EnumerateMeshes([&](const Mesh& mesh) {
            json meshObj;
            meshObj["name"] = mesh.GetName();

            int vertexCount = 0;
            int normalCount = 0;
            int triangleCount = 0;
            int brepFaceCount = 0;

            json positionArr = json::array();
            json normalArr = json::array();
            json indexArr = json::array();
            json brepFaceArr = json::array();

            mesh.EnumerateFaces([&](const Face& face) {
                int triangleOffset = triangleCount;
                int vertexOffset = vertexCount;

                face.EnumerateVertices([&](double x, double y, double z) {
                    positionArr.push_back(std::to_string(std::round(x * 1000.0) / 1000.0));
                    positionArr.push_back(std::to_string(std::round(y * 1000.0) / 1000.0));
                    positionArr.push_back(std::to_string(std::round(z * 1000.0) / 1000.0));
                    vertexCount++;
                });

                face.EnumerateNormals([&](double x, double y, double z) {
                    normalArr.push_back(std::to_string(std::round(x * 1000.0) / 1000.0));
                    normalArr.push_back(std::to_string(std::round(y * 1000.0) / 1000.0));
                    normalArr.push_back(std::to_string(std::round(z * 1000.0) / 1000.0));
                    normalCount++;
                });

                face.EnumerateTriangles([&](int v0, int v1, int v2) {
                    indexArr.push_back(vertexOffset + v0);
                    indexArr.push_back(vertexOffset + v1);
                    indexArr.push_back(vertexOffset + v2);
                    triangleCount++;
                });

                json brepFaceObj;
                brepFaceObj["first"] = triangleOffset;
                brepFaceObj["last"] = triangleCount - 1;

                Color faceColor;
                if (face.GetColor(faceColor)) {
                    json colorArr = json::array();
                    colorArr.push_back(faceColor.r);
                    colorArr.push_back(faceColor.g);
                    colorArr.push_back(faceColor.b);
                    brepFaceObj["color"] = colorArr;
                }
                brepFaceArr.push_back(brepFaceObj);
                brepFaceCount++;
            });

            json attributesObj;

            json positionObj;
            positionObj["array"] = positionArr;
            attributesObj["position"] = positionObj;

            if (vertexCount == normalCount) {
                json normalObj;
                normalObj["array"] = normalArr;
                attributesObj["normal"] = normalObj;
            }

            json indexObj;
            indexObj["array"] = indexArr;

            meshObj["attributes"] = attributesObj;
            meshObj["index"] = indexObj;

            Color meshColor;
            if (mesh.GetColor(meshColor)) {
                json colorArr = json::array();
                colorArr.push_back(meshColor.r);
                colorArr.push_back(meshColor.g);
                colorArr.push_back(meshColor.b);
                meshObj["color"] = colorArr;
            }

            meshObj["brep_faces"] = brepFaceArr;

            // 将网格添加到全局 meshes 数组中
            int meshIndex = globalMeshes.size();
            globalMeshes.push_back(meshObj);

            // 在当前节点中引用该网格索引
            meshesArr.push_back(meshIndex);
            nodeMeshCount++;
        });
    }

    int meshCount;
};

json ImportFile(const std::string& fileName, const ImportParams& params) {
    size_t extensionStart = fileName.find_last_of('.');
    if (extensionStart == std::string::npos) {
        throw std::invalid_argument("Invalid file format.");
    }

    std::string extension = fileName.substr(extensionStart);
    for (size_t i = 0; i < extension.length(); i++) {
        extension[i] = std::tolower(extension[i]);
    }

    ImporterPtr importer = nullptr;
    if (extension == ".stp" || extension == ".step") {
        importer = std::make_shared<ImporterStep>();
    } else if (extension == ".igs" || extension == ".iges") {
        importer = std::make_shared<ImporterIges>();
    } else if (extension == ".brp" || extension == ".brep") {
        importer = std::make_shared<ImporterBrep>();
    } else {
        throw std::invalid_argument("Unsupported file format.");
    }

    Importer::Result result = importer->LoadFile(fileName, params);
    if (result != Importer::Result::Success) {
        throw std::runtime_error("Failed to load file.");
    }
    JsonWriter writer;
    json globalMeshes = json::array();
    json rootNode = writer.WriteNode(importer->GetRootNode(), globalMeshes);

    json resultObj;
    resultObj["root"] = rootNode;
    resultObj["meshes"] = globalMeshes;

    return resultObj;
}

ImportParams GetImportParams(const json& paramsObj) {
    ImportParams params;

    if (paramsObj.contains("linearUnit")) {
        std::string linearUnit = paramsObj["linearUnit"].get<std::string>();
        if (linearUnit == "millimeter") {
            params.linearUnit = ImportParams::LinearUnit::Millimeter;
        } else if (linearUnit == "centimeter") {
            params.linearUnit = ImportParams::LinearUnit::Centimeter;
        } else if (linearUnit == "meter") {
            params.linearUnit = ImportParams::LinearUnit::Meter;
        } else if (linearUnit == "inch") {
            params.linearUnit = ImportParams::LinearUnit::Inch;
        } else if (linearUnit == "foot") {
            params.linearUnit = ImportParams::LinearUnit::Foot;
        }
    }

    if (paramsObj.contains("linearDeflectionType")) {
        std::string linearDeflectionType = paramsObj["linearDeflectionType"].get<std::string>();
        if (linearDeflectionType == "bounding_box_ratio") {
            params.linearDeflectionType = ImportParams::LinearDeflectionType::BoundingBoxRatio;
        } else if (linearDeflectionType == "absolute_value") {
            params.linearDeflectionType = ImportParams::LinearDeflectionType::AbsoluteValue;
        }
    }

    if (paramsObj.contains("linearDeflection")) {
        params.linearDeflection = paramsObj["linearDeflection"].get<double>();
    }

    if (paramsObj.contains("angularDeflection")) {
        params.angularDeflection = paramsObj["angularDeflection"].get<double>();
    }

    return params;
}

int main(int argc, const char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <file> <params_json>" << std::endl;
        return 1;
    }

    std::string fileName = argv[1];
    std::string paramsJson = argv[2];

    ImportParams params;
    try {
        json paramsObj = json::parse(paramsJson);
        params = GetImportParams(paramsObj);
    } catch (const std::exception& e) {
        std::cerr << "Invalid params JSON: " << e.what() << std::endl;
        return 1;
    }

    try {
        json result = ImportFile(fileName, params);

        // 将 JSON 写入文件
        std::ofstream jsonFile("result.json");
        if (!jsonFile.is_open()) {
            std::cerr << "Failed to open result.json for writing." << std::endl;
            return 1;
        }
        jsonFile << result.dump(); // 压缩输出，无缩进
        jsonFile.close();

        std::cout << "Exported to result.json" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
