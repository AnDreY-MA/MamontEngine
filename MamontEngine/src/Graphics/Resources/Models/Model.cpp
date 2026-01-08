#include "Model.h"
#include <expected>
#include <glm/gtx/quaternion.hpp>
#include <stb/include/stb_image.h>
#include "Core/Engine.h"
#include "Core/Log.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Graphics/Devices/PhysicalDevice.h"
#include "Graphics/Resources/Texture.h"
#include "Graphics/Vulkan/Allocator.h"
#include "Utils/Utilities.h"
#include "ktx.h"
#include "ktxvulkan.h"
#include "Core/ContextDevice.h"

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

    std::optional<Texture> load_image(const fastgltf::Asset &asset, const fastgltf::Image &image, VkSampler inSampler = VK_NULL_HANDLE)
    {
        Texture newTexture{};

        int width, height, nrChannels;

        std::visit(
                fastgltf::visitor{
                        [](auto &arg) {},
                        [&](const fastgltf::sources::URI &filePath)
                        {
                            assert(filePath.fileByteOffset == 0);
                            assert(filePath.uri.isLocalPath());

                            const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());
                            unsigned char    *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                            if (data)
                            {
                                const VkExtent3D imagesize{.width = (uint32_t)width, .height = (uint32_t)height, .depth = 1};

                                newTexture.Load(data,
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
                            unsigned char *data =
                                    stbi_load_from_memory(vector.bytes.data(), static_cast<int>(vector.bytes.size()), &width, &height, &nrChannels, 4);
                            if (data)
                            {
                                const VkExtent3D imagesize{.width = (uint32_t)width, .height = (uint32_t)height, .depth = 1};

                                newTexture.Load(data,
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

                            std::visit(fastgltf::visitor{[](auto &arg) {},
                                                         [&](const fastgltf::sources::Vector &vector)
                                                         {
                                                             unsigned char *data = stbi_load_from_memory(vector.bytes.data() + bufferView.byteOffset,
                                                                                                         static_cast<int>(bufferView.byteLength),
                                                                                                         &width,
                                                                                                         &height,
                                                                                                         &nrChannels,
                                                                                                         4);
                                                             if (data)
                                                             {
                                                                 const VkExtent3D imagesize{.width = (uint32_t)width, .height = (uint32_t)height, .depth = 1};

                                                                 newTexture.Load(data,
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

        if (newTexture.Image == VK_NULL_HANDLE)
        {
            Log::Warn("newTexture.Image is NULL");
            return {};
        }
        else
        {
            return newTexture;
        }
    }
} // namespace

namespace MamontEngine
{
    MeshModel::MeshModel(UID inID) : ID(inID)
    {
    }

    MeshModel::MeshModel(UID inID, std::string_view filePath) :  ID(inID)
    {
        Load(filePath);
    }

    MeshModel::~MeshModel()
    {
        VkDevice device = LogicalDevice::GetDevice();

        for (auto &material : m_Materials)
        {
            material->MaterialSet = VK_NULL_HANDLE;
        }

        Clear();

        Buffer.IndexBuffer.Destroy();
        Buffer.VertexBuffer.Destroy();

        DescriptorPool.ClearPools(device);
        DescriptorPool.DestroyPools(device);
        MaterialDataBuffer.Destroy();
    }

    void MeshModel::Clear()
    {
        for (auto &texture : m_Textures)
        {
            texture.Destroy();
        }

        for (auto &node : m_Nodes)
        {
            node->Mesh.reset();
            delete node;
        }

        m_Textures.clear();
        m_Nodes.clear();
        m_Materials.clear();
        m_Meshes.clear();
    }

    void MeshModel::Draw(DrawContext &inContext)
    {
        for (auto &material : m_Materials)
        {
            if (!material || !material->IsDity)
                continue;

            MaterialDataBuffer.Copy(&material->Constants, sizeof(Material::MaterialConstants), material->BufferOffset);

            material->IsDity = false;
        }

        std::function<void(Node*, DrawContext&)> collect = 
            ([&collect, this](Node* node, DrawContext &inContext)
        {
            if (!node || !node->Mesh)
                    return;

            const glm::mat4 nodeMatrix = node->CurrentMatrix;

            for (const auto &primitive : node->Mesh->Primitives)
            {
                if (!primitive || !primitive->Material)
                    continue;

                const auto        &material = primitive->Material;
                const RenderObject def(primitive->Count,
                                        primitive->StartIndex,
                                        Buffer.IndexBuffer.Buffer,
                                        Buffer.VertexBuffer.Buffer,
                                        material,
                                        Bound,
                                        nodeMatrix,
                                        Buffer.VertexBufferAddress, ID);

                if (material->PassType == EMaterialPass::TRANSPARENT)
                    inContext.TransparentSurfaces.push_back(def);
                else
                    inContext.OpaqueSurfaces.push_back(def);
            }

            for (const auto& childNode : node->Children)
            {
                if (childNode)
                {
                    collect(childNode, inContext);
                }
            }
        });

        for (const auto &node : m_Nodes)
        {
            collect(node, inContext);
        }
    }

    void MeshModel::UpdateTransform(const glm::mat4 &inTransform)
    {
        std::function<void(Node *, const glm::mat4 &)> updateNode = [&](Node *node, const glm::mat4 &parentMatrix)
        {
            if (!node)
                return;

            node->CurrentMatrix = parentMatrix * node->Matrix;

            for (auto &child : node->Children)
                updateNode(child, node->CurrentMatrix);
        };

        for (auto &root : m_Nodes)
            updateNode(root, inTransform);
    }

    void MeshModel::Load(std::string_view filePath)
    {
        Clear();

        fmt::println("Loading GLTF: {}", filePath);

        fastgltf::Parser parser{};

        constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers |
                                     fastgltf::Options::LoadExternalBuffers;

        fastgltf::GltfDataBuffer data;
        data.loadFromFile(filePath);

        const std::filesystem::path path = filePath;

        const auto type = fastgltf::determineGltfFileType(&data);

        auto loadResult = (type == fastgltf::GltfType::glTF) ? parser.loadGLTF(&data, path.parent_path(), gltfOptions)
                                                             : parser.loadBinaryGLTF(&data, path.parent_path(), gltfOptions);

        if (!loadResult)
        {
            fmt::println(stderr, "Failed to load glTF: {}\n", fastgltf::to_underlying(loadResult.error()));
            return;
        }

        const fastgltf::Asset gltf = std::move(loadResult.get());

        std::array<DescriptorAllocatorGrowable::PoolSizeRatio, 3> sizes = {
                DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6},
                DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6},
                DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};
        const VkDevice device = LogicalDevice::GetDevice();
        DescriptorPool.Init(device, std::max<size_t>(1, gltf.materials.size()), sizes);

        std::vector<VkSampler> samplers = LoadSamplers(device, gltf.samplers);
        LoadImages(gltf, samplers);
        LoadMaterials(gltf, samplers);

        LoadMesh(gltf);
        LoadNodes(gltf);

        m_FilePath = filePath;

    }

    std::vector<VkSampler> MeshModel::LoadSamplers(VkDevice inDevice, const std::vector<fastgltf::Sampler> &samplers)
    {
        VkSamplerCreateInfo sampl = {.sType     = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                     .pNext     = nullptr,
                                     .magFilter = VK_FILTER_LINEAR,
                                     .minFilter = VK_FILTER_LINEAR,
                                     .minLod    = 0,
                                     .maxLod    = VK_LOD_CLAMP_NONE};
        sampl.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        std::vector<VkSampler> modelSamplers;
        modelSamplers.reserve(samplers.size() + 1);

        {
            VkSampler defaultSampler = VK_NULL_HANDLE;
            vkCreateSampler(inDevice, &sampl, nullptr, &defaultSampler);
            modelSamplers.push_back(defaultSampler);
        }

        for (const fastgltf::Sampler &sampler : samplers)
        {
            sampl.magFilter  = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
            sampl.minFilter  = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));
            sampl.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

            VkSampler newSampler;
            vkCreateSampler(inDevice, &sampl, nullptr, &newSampler);

            modelSamplers.push_back(newSampler);
        }

        return modelSamplers;
    }

    void MeshModel::LoadImages(const fastgltf::Asset &inFileAsset, const std::vector<VkSampler> &inSamplers)
    {
        m_Textures.reserve(inFileAsset.images.size() + 1);
        for (const fastgltf::Image &image : inFileAsset.images)
        {
            const std::optional<Texture> img = load_image(inFileAsset, image);

            if (img.has_value())
            {
                Log::Info("Loaded texture: {}", image.name.c_str());
                m_Textures.push_back(*img);
            }
            else
            {
                const Texture errorTexture = CreateErrorTexture();
                m_Textures.push_back(errorTexture);
                Log::Warn("gltf failed to load texture: {}", image.name);
            }
        }
    }

    void MeshModel::LoadMaterials(const fastgltf::Asset &inFileAsset, const std::vector<VkSampler> &inSamplers)
    {
        m_Materials.reserve(inFileAsset.materials.size());

        const VkDevice device = LogicalDevice::GetDevice();

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(PhysicalDevice::GetDevice(), &properties);
        const size_t minAlignment = properties.limits.minUniformBufferOffsetAlignment;

        constexpr size_t materialConstSize = sizeof(Material::MaterialConstants);

        const size_t alignedMaterialSize = minAlignment > 0 ? (materialConstSize + minAlignment - 1) & ~(minAlignment - 1) : materialConstSize;

        const size_t numMaterials    = std::max(size_t(1), inFileAsset.materials.size());
        const size_t totalBufferSize = numMaterials * alignedMaterialSize;

        MaterialDataBuffer.Create(totalBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        int              dataIndex = 0;
        DescriptorWriter Writer;

        const auto &contextDevice = MEngine::Get().GetContextDevice();

        const auto writerMaterial = [&](const EMaterialPass                pass,
                                        Material::MaterialResources        materialResources,
                                        DescriptorAllocatorGrowable       &descriptorAllocator,
                                        const Material::MaterialConstants &constants)
        {
            auto newMaterial      = std::make_shared<Material>();
            newMaterial->PassType = pass;
            newMaterial->Pipeline =
                    pass == EMaterialPass::TRANSPARENT ? contextDevice.RenderPipeline->TransparentPipeline : contextDevice.RenderPipeline->OpaquePipeline;
            newMaterial->MaterialSet  = descriptorAllocator.Allocate(device, contextDevice.RenderDescriptorLayout);
            newMaterial->Constants    = constants;
            newMaterial->BufferOffset = dataIndex * alignedMaterialSize;

            Writer.Clear();
            Writer.WriteBuffer(0, MaterialDataBuffer.Buffer, materialConstSize, newMaterial->BufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            Writer.WriteImage(1, materialResources.ColorTexture.GetDescriptorInfo(),
                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            Writer.WriteImage(2, materialResources.MetalRoughTexture.GetDescriptorInfo(),
                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            Writer.WriteImage(3, materialResources.NormalTexture.GetDescriptorInfo(),
                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            Writer.WriteImage(4, materialResources.EmissiveTexture.GetDescriptorInfo(),
                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            Writer.WriteImage(5, materialResources.OcclusionTexture.GetDescriptorInfo(),
                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            Writer.UpdateSet(device, newMaterial->MaterialSet);

            // void *data = &newMaterial->Constants;
            // MaterialDataBuffer.Copy(data, materialConstSize, newMaterial->BufferOffset);
            dataIndex++;

            return newMaterial;
        };

        const Texture whiteTexture = CreateWhiteTexture();

        const auto getDefaultMaterialResources = [&]()
        {
            Material::MaterialResources materialResources{};
            materialResources.ColorTexture      = whiteTexture;
            materialResources.MetalRoughTexture = whiteTexture;
            materialResources.NormalTexture     = whiteTexture;
            materialResources.EmissiveTexture   = whiteTexture;
            materialResources.OcclusionTexture  = whiteTexture;
            return materialResources;
        };

        if (inFileAsset.materials.size() == 0)
        {
            // Create Empty Material
            fmt::println("Materials is empty");
            Material::MaterialConstants constants{};
            constants.ColorFactors                        = glm::vec4(1.0f);
            constants.MetalicFactor                       = 0.0f;
            constants.RoughFactor                         = 1.0f;
            Material::MaterialResources materialResources = getDefaultMaterialResources();
            auto                        material          = writerMaterial(EMaterialPass::MAIN_COLOR, materialResources, DescriptorPool, constants);
            m_Materials.push_back(std::move(material));
            return;
        }

        for (const fastgltf::Material &mat : inFileAsset.materials)
        {
            const Material::MaterialConstants constants = {
                    .ColorFactors = glm::vec4(
                            mat.pbrData.baseColorFactor[0], mat.pbrData.baseColorFactor[1], mat.pbrData.baseColorFactor[2], mat.pbrData.baseColorFactor[3]),
                    .MetalicFactor = mat.pbrData.metallicFactor,
                    .RoughFactor   = mat.pbrData.roughnessFactor};

            const EMaterialPass passType = mat.alphaMode == fastgltf::AlphaMode::Blend ? EMaterialPass::TRANSPARENT : EMaterialPass::MAIN_COLOR;

            Material::MaterialResources materialResources = getDefaultMaterialResources();

            if (mat.pbrData.baseColorTexture.has_value())
            {
                const size_t img     = inFileAsset.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
                const size_t sampler = inFileAsset.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value() + 1;

                materialResources.ColorTexture         = m_Textures[img];
                materialResources.ColorTexture.Sampler = inSamplers[sampler];
            }
            if (mat.pbrData.metallicRoughnessTexture.has_value())
            {
                const size_t img     = inFileAsset.textures[mat.pbrData.metallicRoughnessTexture.value().textureIndex].imageIndex.value();
                const size_t sampler = inFileAsset.textures[mat.pbrData.metallicRoughnessTexture.value().textureIndex].samplerIndex.value() + 1;

                materialResources.MetalRoughTexture         = m_Textures[img];
                materialResources.MetalRoughTexture.Sampler = inSamplers[sampler];
            }
            if (mat.normalTexture.has_value())
            {
                const auto textureIndex                 = inFileAsset.textures[mat.normalTexture.value().textureIndex].imageIndex.value();
                const auto samplerIndex                 = inFileAsset.textures[mat.normalTexture.value().textureIndex].samplerIndex.value() + 1;
                materialResources.NormalTexture         = m_Textures[mat.normalTexture.value().textureIndex];
                materialResources.NormalTexture.Sampler = inSamplers[samplerIndex];
            }

            if (mat.emissiveTexture.has_value())
            {
                const auto textureIndex                   = inFileAsset.textures[mat.emissiveTexture.value().textureIndex].imageIndex.value();
                const auto samplerIndex                   = inFileAsset.textures[mat.emissiveTexture.value().textureIndex].samplerIndex.value() + 1;
                materialResources.EmissiveTexture         = m_Textures[mat.emissiveTexture.value().textureIndex];
                materialResources.EmissiveTexture.Sampler = inSamplers[samplerIndex];
            }
            if (mat.occlusionTexture.has_value())
            {
                const auto textureIndex                    = inFileAsset.textures[mat.occlusionTexture.value().textureIndex].imageIndex.value();
                const auto samplerIndex                    = inFileAsset.textures[mat.occlusionTexture.value().textureIndex].samplerIndex.value() + 1;
                materialResources.OcclusionTexture         = m_Textures[mat.occlusionTexture.value().textureIndex];
                materialResources.OcclusionTexture.Sampler = inSamplers[samplerIndex];
            }

            auto newMat  = writerMaterial(passType, materialResources, DescriptorPool, constants);
            newMat->Name = mat.name;
            m_Materials.push_back(std::move(newMat));
        }

    }

    void MeshModel::LoadNodes(const fastgltf::Asset &inFileAsset)
    {
        const size_t sizeNodes{inFileAsset.nodes.size()};
        // std::vector<std::shared_ptr<Node>> nodes;
        m_Nodes.reserve(sizeNodes);

        for (const fastgltf::Node &node : inFileAsset.nodes)
        {
            auto newNode  = new Node();
            newNode->Name = node.name;
            fmt::println("NewNode name: {}", node.name);

            if (node.meshIndex.has_value())
            {
                newNode->Mesh = m_Meshes[node.meshIndex.value()];
            }

            std::visit(
                    fastgltf::visitor{[&](const fastgltf::Node::TransformMatrix &matrix) { memcpy(&newNode->Matrix, matrix.data(), sizeof(matrix)); },
                                      [&](const fastgltf::Node::TRS &transform)
                                      {
                                          newNode->Transform.Position = glm::vec3(transform.translation[0], transform.translation[1], transform.translation[2]);
                                          newNode->Transform.Rotation = glm::vec3(0);
                                          newNode->Transform.Scale    = glm::vec3(transform.scale[0], transform.scale[1], transform.scale[2]);

                                          const glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), newNode->Transform.Position);
                                          const glm::quat rotation(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
                                          const glm::mat4 rotationMat = glm::toMat4(rotation);
                                          const glm::mat4 scaleMat    = glm::scale(glm::mat4(1.0f), newNode->Transform.Scale);

                                          newNode->Matrix = translationMat * rotationMat * scaleMat;
                                      }},
                    node.transform);

            m_Nodes.push_back(std::move(newNode));
        }

        for (size_t i = 0; i < sizeNodes; ++i)
        {
            const fastgltf::Node &gltfNode    = inFileAsset.nodes[i];
            auto                 &currentNode = m_Nodes[i];

            for (const auto &child : gltfNode.children)
            {
                m_Nodes[i]->Children.push_back(m_Nodes[child]);
                m_Nodes[child]->Parent = currentNode;
            }
        }
    }

    void MeshModel::LoadMesh(const fastgltf::Asset &inFileAsset)
    {
        std::vector<uint32_t> indices;
        std::vector<Vertex>   vertices;
        for (const fastgltf::Mesh &mesh : inFileAsset.meshes)
        {
            std::shared_ptr<NewMesh> newmesh = std::make_shared<NewMesh>();
            newmesh->Name                    = mesh.name;

            /*indices.clear();
            vertices.clear();*/

            for (auto &&p : mesh.primitives)
            {
                std::unique_ptr<Primitive> newPrimitive = std::make_unique<Primitive>();
                newPrimitive->StartIndex                = static_cast<uint32_t>(indices.size());
                newPrimitive->Count                     = static_cast<uint32_t>(inFileAsset.accessors[p.indicesAccessor.value()].count);

                const size_t initial_vtx = vertices.size();

                {
                    const fastgltf::Accessor &indexaccessor = inFileAsset.accessors[p.indicesAccessor.value()];
                    indices.reserve(indices.size() + indexaccessor.count);

                    fastgltf::iterateAccessor<std::uint32_t>(
                            inFileAsset, indexaccessor, [&](const std::uint32_t idx) { indices.push_back(idx + initial_vtx); });
                }

                {
                    const fastgltf::Accessor &posAccessor = inFileAsset.accessors[p.findAttribute("POSITION")->second];
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

                const auto normals = p.findAttribute("NORMAL");
                if (normals != p.attributes.end())
                {

                    fastgltf::iterateAccessorWithIndex<glm::vec3>(inFileAsset,
                                                                  inFileAsset.accessors[(*normals).second],
                                                                  [&](const glm::vec3 &v, size_t index) { vertices[initial_vtx + index].Normal = v; });
                }

                const auto uv = p.findAttribute("TEXCOORD_0");
                if (uv != p.attributes.end())
                {

                    fastgltf::iterateAccessorWithIndex<glm::vec2>(
                            inFileAsset, inFileAsset.accessors[(*uv).second], [&](const glm::vec2 &v, size_t index) { vertices[initial_vtx + index].UV = v; });
                }

                const auto colors = p.findAttribute("COLOR_0");
                if (colors != p.attributes.end())
                {

                    fastgltf::iterateAccessorWithIndex<glm::vec4>(inFileAsset,
                                                                  inFileAsset.accessors[(*colors).second],
                                                                  [&](const glm::vec4 &v, size_t index) { vertices[initial_vtx + index].Color = v; });
                }
                const auto tangents = p.findAttribute("TANGENT");
                if (tangents != p.attributes.end())
                {

                    fastgltf::iterateAccessorWithIndex<glm::vec4>(inFileAsset,
                                                                  inFileAsset.accessors[(*tangents).second],
                                                                  [&](const glm::vec4 &v, size_t index) { vertices[initial_vtx + index].Tangent = v; });
                }

                if (p.materialIndex.has_value())
                {
                    newPrimitive->Material = m_Materials[p.materialIndex.value()].get();
                }
                else
                {
                    newPrimitive->Material = m_Materials[0].get();
                }

                /*glm::vec3 minpos = vertices[initial_vtx].Position;
                glm::vec3 maxpos = vertices[initial_vtx].Position;
                for (size_t i = initial_vtx; i < vertices.size(); ++i)
                {
                    minpos = glm::min(minpos, vertices[i].Position);
                    maxpos = glm::max(maxpos, vertices[i].Position);
                }

                newPrimitive->Bound = AABB(minpos, maxpos);*/

                newmesh->Primitives.push_back(std::move(newPrimitive));
            }
            m_Meshes.push_back(newmesh);
        }

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
