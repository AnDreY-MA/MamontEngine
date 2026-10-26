#include "Model.h"
#include <expected>
#include <glm/gtx/quaternion.hpp>
#include <stb/include/stb_image.h>
#include "Core/Engine.h"
#include "Core/Log.h"

#include "Utils/Utilities.h"
#include "ktx.h"
#include "ktxvulkan.h"
#include "Core/ContextDevice.h"

#include <meshoptimizer.h>

#include "Graphics/Resources/Materials/MaterialAllocator.h"
#include "Graphics/Resources/Materials/MaterialManager.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Graphics/Resources/Texture.h"
#include "Graphics/Vulkan/Allocator.h"

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/glm_element_traits.hpp>

#include <ranges>

namespace
{
    using namespace MamontEngine;

    VkFilter extract_filter(fastgltf::Filter filter)
    {
        switch (filter)
        {
                // nearest samplers
            case fastgltf::Filter::Nearest:
            case fastgltf::Filter::NearestMipMapNearest:
            case fastgltf::Filter::NearestMipMapLinear:
                return VK_FILTER_NEAREST;

                // linear samplers
            case fastgltf::Filter::Linear:
            case fastgltf::Filter::LinearMipMapNearest:
            case fastgltf::Filter::LinearMipMapLinear:
            default:
                return VK_FILTER_LINEAR;
        }
    }

    VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter)
    {
        switch (filter)
        {
            case fastgltf::Filter::NearestMipMapNearest:
            case fastgltf::Filter::LinearMipMapNearest:
                return VK_SAMPLER_MIPMAP_MODE_NEAREST;

            case fastgltf::Filter::NearestMipMapLinear:
            case fastgltf::Filter::LinearMipMapLinear:
            default:
                return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        }
    }

    std::shared_ptr<Texture> load_image(const fastgltf::Asset &asset, const fastgltf::Image &image, VkSampler inSampler = VK_NULL_HANDLE)
    {
        auto newTexture = std::make_shared<Texture>();

        std::visit(
                fastgltf::visitor{
                        [](auto &arg) {},
                        [&](const fastgltf::sources::URI &filePath)
                        {
                            assert(filePath.fileByteOffset == 0);
                            assert(filePath.uri.isLocalPath());

                            const auto &uri = *filePath.uri;
                            const std::string path(uri.path().begin(), uri.path().end());
                            int               width, height, nrChannels;

                            unsigned char    *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                            if (data)
                            {
                                const VkExtent3D imagesize{.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height), .depth = 1};

                                newTexture->Load(data,
                                                imagesize,
                                                VK_FORMAT_R8G8B8A8_UNORM,
                                                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                                false,
                                                inSampler);
                                stbi_image_free(data);
                            }
                        },
                        [&](const fastgltf::sources::Vector &vector)
                        {
                            int width, height, nrChannels;

                            const auto    *bytes = reinterpret_cast<const stbi_uc *>(vector.bytes.data());
                            unsigned char *data = stbi_load_from_memory(bytes, static_cast<int>(vector.bytes.size()), &width, &height, &nrChannels, 4);
                            if (data)
                            {
                                const VkExtent3D imagesize{.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height), .depth = 1};

                                newTexture->Load(data,
                                                imagesize,
                                                VK_FORMAT_R8G8B8A8_UNORM,
                                                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                                false,
                                                inSampler);
                                stbi_image_free(data);
                            }

                        },
                        [&](const fastgltf::sources::BufferView &view)
                        {
                            const auto &bufferView = asset.bufferViews[view.bufferViewIndex];
                            const auto &buffer     = asset.buffers[bufferView.bufferIndex];
                            int         width, height, nrChannels;


                            std::visit(fastgltf::visitor{[](auto &arg) {},
                                                         [&](const fastgltf::sources::Array &array)
                                                         {
                                                             const auto *bytes = reinterpret_cast<const stbi_uc *>(array.bytes.data());

                                                             unsigned char *data = stbi_load_from_memory(bytes + bufferView.byteOffset,
                                                                                                         static_cast<int>(bufferView.byteLength),
                                                                                                         &width,
                                                                                                         &height,
                                                                                                         &nrChannels,
                                                                                                         4);
                                                             if (data)
                                                             {
                                                                 const VkExtent3D imagesize{.width  = static_cast<uint32_t>(width),
                                                                                            .height = static_cast<uint32_t>(height),
                                                                                            .depth  = 1};

                                                                 newTexture->Load(data,
                                                                                 imagesize,
                                                                                 VK_FORMAT_R8G8B8A8_UNORM,
                                                                                 VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                                                                 false,
                                                                                 inSampler);
                                                                 stbi_image_free(data);

                                                             }
                                                         }},
                                       buffer.data);
                        },
                },
                image.data);

        if (newTexture->Image == VK_NULL_HANDLE)
        {
            Log::Warn("newTexture.Image is NULL");
            return {};
        }
        else
        {
            return newTexture;
        }
        
    }

    void OptimizeMesh(std::vector<uint32_t>& indices, std::vector<Vertex> vertices)
    {
        if (indices.empty() || vertices.empty())
            return;
        
        std::vector<uint32_t> remap(vertices.size());
        const auto            uniqueVertices = meshopt_generateVertexRemap(remap.data(), indices.data(), indices.size(), vertices.data(), vertices.size(), sizeof(Vertex));

        std::vector<uint32_t> newIndices(indices.size());
        std::vector<Vertex> newVertices(uniqueVertices);

        meshopt_remapIndexBuffer(newIndices.data(), indices.data(), indices.size(), remap.data());
        meshopt_remapVertexBuffer(newVertices.data(), vertices.data(), vertices.size(), sizeof(Vertex), remap.data());

        indices  = std::move(newIndices);
        vertices = std::move(newVertices);

        meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertices.size());
    }
} // namespace

namespace MamontEngine
{
    Model::Model()
    {
        m_Type = EAssetType::Model;
    }

    Model::Model(UID inID) 
        : Asset(inID)
    {
        m_Type = EAssetType::Model;
    }

    Model::Model(UID inID, std::string_view filePath) 
        : Asset(inID)
    {
        m_Type = EAssetType::Model;

        Load(filePath);
    }

    Model::~Model()
    {
        Clear();
    }

    void Model::Clear()
    {
        const auto &device = LogicalDevice::GetDevice();
        vkDeviceWaitIdle(device);

        m_Nodes.clear();

        for (auto &texture : m_Textures)
        {
            texture->Destroy();
        }
        m_Textures.clear();

        for (auto &material : m_Materials)
        {
            material->MaterialSet = VK_NULL_HANDLE;
            MaterialAllocator::Free(material.get());
        }
        m_Materials.clear();

        m_Meshes.clear();

        Buffer.IndexBuffer.Destroy();
        Buffer.VertexBuffer.Destroy();
    }

    void Model::Draw(DrawContext &inContext)
    {
        for (const auto &material : m_Materials)
        {
            if (!material || !material->IsDirty)
                continue;

            MaterialAllocator::Update(material.get());

            material->IsDirty = false;
        }

        std::function<void(const Node&, DrawContext&)> collect = 
            ([&collect, this](const Node& node, DrawContext &inContext)
        {
             const auto& currentMesh = m_Meshes[node.MeshIndex];
            if (!currentMesh)
                    return;

            const glm::mat4 nodeMatrix = node.CurrentMatrix;

            for (const auto &primitive : currentMesh->Primitives)
            {
                if (!primitive || !m_Materials[primitive->MaterialIndex])
                    continue;

                const auto        &material = m_Materials[primitive->MaterialIndex];
                RenderObject def(primitive->Count,
                                        primitive->StartIndex,
                                        Buffer,
                                        material->MaterialSet, material->Index, Bound,
                                        nodeMatrix, ID);
                def.SphereBound = primitive->Bounds;

                if (material->PassType == EMaterialPass::Transparent)
                    inContext.TransparentSurfaces.push_back(def);
                else
                    inContext.OpaqueSurfaces.push_back(def);
            }

            for (const auto& childNode : node.Children)
            {
                collect(m_Nodes[childNode], inContext);
            }
        });

        for (const auto &node : m_Nodes)
        {
            collect(node, inContext);
        }
    }

    void Model::UpdateTransform(const glm::mat4 &inTransform)
    {
        for (auto &node : m_Nodes)
        {
            if (node.ParentIndex != INVALID_NODE_INDEX)
                continue;

            node.CurrentMatrix = inTransform * node.Matrix;
            node.Status |= TransformStatus::WorldDirty;
        }

        for (auto &node : m_Nodes)
        {
            if (!(node.Status & TransformStatus::WorldDirty))
                continue;

            if (node.ParentIndex != INVALID_NODE_INDEX)
            {
                node.CurrentMatrix = m_Nodes[node.ParentIndex].CurrentMatrix * node.Matrix;
            }
            
            for (uint32_t child : node.Children)
            {
                m_Nodes[child].Status |= TransformStatus::WorldDirty;
            }

            node.Status = TransformStatus::Clean;
        }
    }

    void Model::Load(std::string_view filePath)
    {
        Clear();

        Log::Info("Model, Loading file: {}", filePath);

        constexpr auto supportedExtensions =
                fastgltf::Extensions::KHR_mesh_quantization | fastgltf::Extensions::KHR_texture_transform | fastgltf::Extensions::KHR_materials_variants;


        fastgltf::Parser parser(supportedExtensions);

        constexpr auto gltfOptions = fastgltf::Options::AllowDouble | fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages |
                                     fastgltf::Options::GenerateMeshIndices;

        

        const std::filesystem::path path = std::string{filePath};

        auto gltfFile = fastgltf::MappedGltfFile::FromPath(path);
        if (!bool(gltfFile))
        {
            fmt::println("Gltf File is false");
            return;
        }
        const auto type = fastgltf::determineGltfFileType(gltfFile.get());

        auto loadResult = (type == fastgltf::GltfType::glTF) ? parser.loadGltf(gltfFile.get(), path.parent_path(), gltfOptions)
                                                             : parser.loadGltfBinary(gltfFile.get(), path.parent_path(), gltfOptions);

        if (!loadResult)
        {
            fmt::println(stderr, "Failed to load glTF: {}\n", fastgltf::to_underlying(loadResult.error()));
            return;
        }

        const fastgltf::Asset gltf = std::move(loadResult.get());

        LoadImages(gltf);
        LoadMaterials(gltf);

        LoadMesh(gltf);
        LoadNodes(gltf);

        m_FilePath = filePath;

        IsDirty = true;
    }

    std::vector<VkSampler> Model::LoadSamplers(const std::vector<fastgltf::Sampler> &samplers)
    {
        const VkDevice device = LogicalDevice::GetDevice();

        VkSamplerCreateInfo sampl = {.sType     = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                     .pNext     = nullptr,
                                     .magFilter = VK_FILTER_LINEAR,
                                     .minFilter = VK_FILTER_LINEAR,
                                     .minLod    = 0,
                                     .maxLod    = VK_LOD_CLAMP_NONE};
        sampl.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        std::vector<VkSampler> modelSamplers;
        const auto             size = samplers.size();
        modelSamplers.reserve(size != 0 ? size : 1);

        for (const fastgltf::Sampler &sampler : samplers)
        {
            sampl.magFilter  = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
            sampl.minFilter  = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));
            sampl.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

            VkSampler newSampler;
            vkCreateSampler(device, &sampl, nullptr, &newSampler);

            modelSamplers.push_back(newSampler);
        }

        return modelSamplers;
    }

    void Model::LoadImages(const fastgltf::Asset &asset)
    {
        m_Textures.clear();
        m_Textures.reserve(asset.textures.size());

        const auto &samples = LoadSamplers(asset.samplers);

        for (const auto &textureGltf : asset.textures)
        {
            const auto &image   = asset.images[textureGltf.imageIndex.value()];
            const auto  samplerIndex = textureGltf.samplerIndex.value();
            const auto        texture      = load_image(asset, image, samples[samplerIndex]);

            if (texture)
            {
                m_Textures.push_back(std::move(texture));

                Log::Info("Loaded image: {}", image.name);
            }
            else
            {
                Log::Error("Failed to load image : {}", image.name);
            }
        }
        }

    void Model::LoadMaterials(const fastgltf::Asset &inFileAsset)
    {
        m_Materials.reserve(inFileAsset.materials.size());
        auto defaultTexture = CreateWhiteTexture();

        auto getTexture = [&](const auto &textureInfo) -> std::shared_ptr<Texture>
        {
            if (!textureInfo.has_value())
            {
                fmt::println("!textureInfo.has_value()");
                return {};
            }

            const std::size_t textureIndex = textureInfo->textureIndex;

            if (textureIndex >= inFileAsset.textures.size())
            {
                fmt::println("glTF texture index is out of range: {}", textureIndex);
                return {};
            }

            const auto &texture = inFileAsset.textures[textureIndex];

            if (!texture.imageIndex.has_value())
            {
                fmt::println("glTF texture has no image index: {}", textureIndex);
                return {};
            }

            const std::size_t imageIndex = *texture.imageIndex;

            if (imageIndex >= m_Textures.size())
            {
                fmt::println("Loaded image index is out of range: {}", imageIndex);
                return {};
            }

            return m_Textures[imageIndex];
        };

        const auto getDefaultMaterialResources = [&]()
        {
            Material::MaterialResources materialResources{};
            materialResources.ColorTexture      = defaultTexture;
            materialResources.MetalRoughTexture = defaultTexture;
            materialResources.NormalTexture     = defaultTexture;
            materialResources.EmissiveTexture   = defaultTexture;
            materialResources.OcclusionTexture  = defaultTexture;
            return materialResources;
        };

        for (const fastgltf::Material &mat : inFileAsset.materials)
        {
            Material::MaterialConstants constants = {
                    .ColorFactors = glm::vec4(
                            mat.pbrData.baseColorFactor[0], mat.pbrData.baseColorFactor[1], mat.pbrData.baseColorFactor[2], mat.pbrData.baseColorFactor[3]),
                    .MetalicFactor = mat.pbrData.metallicFactor,
                    .RoughFactor   = mat.pbrData.roughnessFactor};

            const EMaterialPass passType = mat.alphaMode == fastgltf::AlphaMode::Blend ? EMaterialPass::Transparent : EMaterialPass::MAIN_COLOR;

            Material::MaterialResources materialResources = getDefaultMaterialResources();

            if (mat.pbrData.baseColorTexture.has_value())
            {
                const size_t img     = inFileAsset.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
                const size_t sampler = inFileAsset.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

                if (auto texture = getTexture(mat.pbrData.baseColorTexture); texture)
                {
                    materialResources.ColorTexture = std::move(texture);
                    constants.HasBaseTexture        = 1;
                }
            }

            if (mat.pbrData.metallicRoughnessTexture.has_value())
            {
                const size_t img     = inFileAsset.textures[mat.pbrData.metallicRoughnessTexture.value().textureIndex].imageIndex.value();
                const size_t sampler = inFileAsset.textures[mat.pbrData.metallicRoughnessTexture.value().textureIndex].samplerIndex.value();

                materialResources.MetalRoughTexture         = m_Textures[img];
                if (auto texture = getTexture(mat.pbrData.metallicRoughnessTexture); texture)
                {
                    materialResources.MetalRoughTexture = std::move(texture);
                    constants.HasMetalRough               = 1;
                }
            }

            if (mat.normalTexture.has_value())
            {
                const auto textureIndex                  = inFileAsset.textures[mat.normalTexture.value().textureIndex].imageIndex.value();
                const auto samplerIndex                  = inFileAsset.textures[mat.normalTexture.value().textureIndex].samplerIndex.value();

                if (auto texture = getTexture(mat.normalTexture); texture)
                {
                        materialResources.NormalTexture = std::move(texture);
                        constants.HasNormaMap           = 1;
                }
            }
            
            if (mat.emissiveTexture.has_value())
            {
                const auto textureIndex                    = inFileAsset.textures[mat.emissiveTexture.value().textureIndex].imageIndex.value();
                const auto samplerIndex                    = inFileAsset.textures[mat.emissiveTexture.value().textureIndex].samplerIndex.value();
                if (auto texture = getTexture(mat.emissiveTexture); texture)
                {
                    materialResources.EmissiveTexture = std::move(texture);
                    constants.HasEmmisive             = 1;

                }
            }

            if (mat.occlusionTexture.has_value())
            {
                const auto textureIndex                     = inFileAsset.textures[mat.occlusionTexture.value().textureIndex].imageIndex.value();
                const auto samplerIndex                     = inFileAsset.textures[mat.occlusionTexture.value().textureIndex].samplerIndex.value();
                materialResources.OcclusionTexture          = m_Textures[textureIndex];
                if (auto texture = getTexture(mat.occlusionTexture); texture)
                {
                    materialResources.OcclusionTexture = std::move(texture);
                    constants.HasOcclusion              = 1;
                }
            }

            auto newMat  = std::shared_ptr<Material>(MaterialManager::Get()->CreateMaterial(passType, materialResources, constants));
            newMat->Name = mat.name;
            m_Materials.push_back(newMat);
        }
    }

    void Model::LoadNodes(const fastgltf::Asset &inFileAsset)
    {
        const size_t sizeNodes{inFileAsset.nodes.size()};
        m_Nodes.reserve(sizeNodes);

        for (uint32_t i = 0; i < sizeNodes; ++i)
        {
            const fastgltf::Node &node    = inFileAsset.nodes[i];

            Node newNode{};
            newNode.Name = node.name;

            if (node.meshIndex.has_value())
            {
                newNode.MeshIndex = node.meshIndex.value();
            }

            std::visit(fastgltf::visitor{
                [&](const fastgltf::TRS &transform)
                                         {
                                             newNode.Transform.Position =
                                                     glm::vec3(transform.translation[0], transform.translation[1], transform.translation[2]);
                                             newNode.Transform.Rotation =
                                                     glm::quat(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
                                             newNode.Transform.Scale = glm::vec3(transform.scale[0], transform.scale[1], transform.scale[2]);
                                             newNode.Matrix = newNode.Transform.Matrix();
                                         },

                [&](const fastgltf::math::fmat4x4 &matrix)
                                         {
                                             for (glm::length_t column = 0; column < 4; ++column)
                                             {
                                                 for (glm::length_t row = 0; row < 4; ++row)
                                                 {
                                                     newNode.Matrix[column][row] = matrix[column][row];
                                                 }
                                             }
                                         }},
                       node.transform);

            m_Nodes.push_back(newNode);
        }

        for (uint32_t i = 0; i < sizeNodes; ++i)
        {
            const fastgltf::Node &gltfNode    = inFileAsset.nodes[i];
            auto                 &currentNode = m_Nodes[i];

            for (const auto &child : gltfNode.children)
            {
                m_Nodes[i].Children.push_back(child);
                m_Nodes[child].ParentIndex = i;
            }
        }

        //Sort 
        std::vector<uint32_t> order;
        order.reserve(sizeNodes);
        std::vector<uint8_t> visited(sizeNodes, 0);

        std::function<void(uint32_t)> visit = [&](uint32_t idx)
        {
            if (visited[idx])
                return;
            visited[idx] = 1;
            order.push_back(idx);

            for (uint32_t child : m_Nodes[idx].Children)
            {
                visit(child);
            }
        };

        for (uint32_t i = 0; i < sizeNodes; ++i)
        {
            if (m_Nodes[i].ParentIndex == INVALID_NODE_INDEX)
            {
                visit(i);
            }
        }

        std::vector<uint32_t> remap(sizeNodes);
        std::vector<Node>     sorted;
        sorted.reserve(sizeNodes);
        for (uint32_t oldIdx : order)
        {
            remap[oldIdx] = static_cast<uint32_t>(sorted.size());
            sorted.push_back(std::move(m_Nodes[oldIdx]));
        }

        for (Node &node : sorted)
        {
            if (node.ParentIndex != INVALID_NODE_INDEX)
                node.ParentIndex = remap[node.ParentIndex];
            for (uint32_t &child : node.Children)
                child = remap[child];
        }

        m_Nodes = std::move(sorted);
    }

    void Model::LoadMesh(const fastgltf::Asset &inFileAsset)
    {
        std::vector<uint32_t> indices;
        std::vector<Vertex>   vertices;
        m_Meshes.reserve(inFileAsset.meshes.size());
        for (const fastgltf::Mesh &mesh : inFileAsset.meshes)
        {
            std::unique_ptr<Mesh> newmesh = std::make_unique<Mesh>();
            newmesh->Name                    = mesh.name;
            newmesh->Primitives.reserve(mesh.primitives.size());
            for (const auto& p : mesh.primitives)
            {
                const auto indice = p.indicesAccessor.has_value() ? p.indicesAccessor.value() : 0;
                std::unique_ptr<Primitive> newPrimitive = std::make_unique<Primitive>();
                newPrimitive->StartIndex                = static_cast<uint32_t>(indices.size());
                newPrimitive->Count                     = static_cast<uint32_t>(inFileAsset.accessors[indice].count);

                const size_t initial_vtx = vertices.size();

                {
                    const fastgltf::Accessor &indexaccessor = inFileAsset.accessors[indice];
                    indices.reserve(indices.size() + indexaccessor.count);

                    fastgltf::iterateAccessor<std::uint32_t>(
                            inFileAsset, indexaccessor, [&](const std::uint32_t idx) { indices.push_back(idx + initial_vtx); });
                }

                std::size_t baseColorTexcoordIndex{0};

                if (p.materialIndex.has_value())
                {
                    const auto &material = inFileAsset.materials[p.materialIndex.value()];
                    const auto &baseColorTexture = material.pbrData.baseColorTexture;
                    if (baseColorTexture.has_value())
                    {
                        if (baseColorTexture->transform && baseColorTexture->transform->texCoordIndex.has_value())
                        {
                            baseColorTexcoordIndex = baseColorTexture->transform->texCoordIndex.value();
                        }
                        else
                        {
                            baseColorTexcoordIndex = material.pbrData.baseColorTexture->texCoordIndex;
                        }
                    }
                }

                
                if (const auto pos = p.findAttribute("POSITION"); 
                    pos != p.attributes.end())
                {
                    const fastgltf::Accessor &posAccessor = inFileAsset.accessors[pos->accessorIndex];
                    vertices.resize(vertices.size() + posAccessor.count);

                    fastgltf::iterateAccessorWithIndex<glm::vec3>(inFileAsset,
                                                                  posAccessor,
                                                                  [&](const glm::vec3 &v, const size_t index)
                                                                  {
                                                                      Vertex newvtx;
                                                                      newvtx.Position               = v;
                                                                      vertices[initial_vtx + index] = newvtx;
                                                                  });
                }

                
                if (const auto normals = p.findAttribute("NORMAL"); 
                    normals != p.attributes.end())
                {
                    const auto &accessor = inFileAsset.accessors[normals->accessorIndex];

                    fastgltf::iterateAccessorWithIndex<glm::vec3>(inFileAsset,
                                                                  accessor,
                                                                  [&](const glm::vec3 &v, size_t index) { vertices[initial_vtx + index].Normal = v; });
                }

                const std::string texcoordAttributeName = std::string("TEXCOORD_") + std::to_string(baseColorTexcoordIndex);
                if (const auto uv = p.findAttribute(texcoordAttributeName); uv != p.attributes.end())
                {
                    const auto &accessor         = inFileAsset.accessors[uv->accessorIndex];
                    fastgltf::iterateAccessorWithIndex<glm::vec2>(
                            inFileAsset, accessor, [&](const glm::vec2 &v, size_t index) { vertices[initial_vtx + index].UV = v; });
                }

                
                if (const auto colors = p.findAttribute("COLOR_0"); 
                    colors != p.attributes.end())
                {
                    const auto &accessor = inFileAsset.accessors[colors->accessorIndex];
                    fastgltf::iterateAccessorWithIndex<glm::vec4>(inFileAsset, accessor,
                                                                  [&](const glm::vec4 &v, size_t index) { vertices[initial_vtx + index].Color = v; });
                }
                
                if (const auto tangents = p.findAttribute("TANGENT"); tangents != p.attributes.end())
                {
                    const auto &accessor = inFileAsset.accessors[tangents->accessorIndex];

                    fastgltf::iterateAccessorWithIndex<glm::vec4>(inFileAsset, accessor,
                                                                  [&](const glm::vec4 &v, size_t index) { vertices[initial_vtx + index].Tangent = v; });
                }
    
                newPrimitive->MaterialIndex = p.materialIndex.has_value() ? static_cast<uint32_t>(p.materialIndex.value()) : 0;                

                auto minpos = vertices[initial_vtx].Position;
                auto maxpos = vertices[initial_vtx].Position;
                for (size_t i = initial_vtx; i < vertices.size(); ++i)
                {
                    minpos = glm::min(minpos, vertices[i].Position);
                    maxpos = glm::max(maxpos, vertices[i].Position);
                }

                newPrimitive->LocalBounds = AABB(minpos, maxpos);

                newPrimitive->Bounds.Origin = (minpos + maxpos) / 2.f;
                newPrimitive->Bounds.Extents = (maxpos - minpos) / 2.f;
                newPrimitive->Bounds.Radius = glm::length(newPrimitive->Bounds.Extents);

                newmesh->Primitives.push_back(std::move(newPrimitive));
            }
            m_Meshes.push_back(std::move(newmesh));
        }

        //OptimizeMesh(indices, vertices);

        glm::vec3 minpos = vertices[0].Position;
        glm::vec3 maxpos = vertices[0].Position;
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            minpos = glm::min(minpos, vertices[i].Position);
            maxpos = glm::max(maxpos, vertices[i].Position);
        }

        Bound = AABB(minpos, maxpos);

        Buffer.Create(indices, vertices);
    }
} // namespace MamontEngine
