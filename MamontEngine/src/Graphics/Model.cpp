#include "Model.h"
#include <stb/include/stb_image.h>
#include <expected>
#include <glm/gtx/quaternion.hpp>
#include "Core/Log.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Utils/Utilities.h"
#include "Core/Engine.h"
#include "Graphics/Vulkan/Allocator.h"

namespace MamontEngine
{
    static VkFilter extract_filter(fastgltf::Filter filter)
    {
        switch (filter)
        {
             //nearest samplers
            case fastgltf::Filter::Nearest:
            case fastgltf::Filter::NearestMipMapNearest:
            case fastgltf::Filter::NearestMipMapLinear:
                return VK_FILTER_NEAREST;

             //linear samplers
            case fastgltf::Filter::Linear:
            case fastgltf::Filter::LinearMipMapNearest:
            case fastgltf::Filter::LinearMipMapLinear:
            default:
                return VK_FILTER_LINEAR;
        }
    }

    static VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter)
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

    static std::optional<AllocatedImage> load_image(const VkContextDevice &inDevice, const fastgltf::Asset &asset, const fastgltf::Image &image)
    {
        AllocatedImage newImage{};

        int width, height, nrChannels;

        auto process_pixels = [&](unsigned char *data) -> bool
        {
            if (!data)
                return false;
            const VkExtent3D imagesize{(uint32_t)width, (uint32_t)height, 1u};
            newImage = inDevice.CreateImage(
                    data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, /*mip=*/true);
            std::cerr << "Model newImage, Image:" << newImage.Image << std::endl;
            std::cerr << "Model newImage, ImageView:" << newImage.ImageView << std::endl;
            
            stbi_image_free(data);

            return newImage.Image != VK_NULL_HANDLE;
        };

        std::visit(
                fastgltf::visitor{
                        [](auto &arg) {},
                        [&](const fastgltf::sources::URI &filePath)
                        {
                            assert(filePath.fileByteOffset == 0);

                            const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());
                            unsigned char    *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                            process_pixels(data);
                        },
                        [&](const fastgltf::sources::Vector &vector)
                        {
                            unsigned char *data =
                                    stbi_load_from_memory(vector.bytes.data(), static_cast<int>(vector.bytes.size()), &width, &height, &nrChannels, 4);
                            process_pixels(data);
                        },
                        [&](const fastgltf::sources::BufferView &view)
                        {
                            const auto &bufferView = asset.bufferViews[view.bufferViewIndex];
                            const auto &buffer     = asset.buffers[bufferView.bufferIndex];

                            std::visit(fastgltf::visitor{[](auto &arg) {},
                                                         [&](fastgltf::sources::Vector &vector)
                                                         {
                                                             unsigned char *data = stbi_load_from_memory(vector.bytes.data() + bufferView.byteOffset,
                                                                                                         static_cast<int>(bufferView.byteLength),
                                                                                                         &width,
                                                                                                         &height,
                                                                                                         &nrChannels,
                                                                                                         4);
                                                             process_pixels(data);
                                                         }},
                                       buffer.data);
                        },
                },
                image.data);


        if (newImage.Image == VK_NULL_HANDLE)
        {
            Log::Warn("Load Image(): image = NULL");
            return {};
        }

        return newImage;
    }

    MeshModel::MeshModel(const VkContextDevice &inDevice, UID inID) : 
        m_ContextDevice(inDevice), ID(inID)
    {

    }

    MeshModel::MeshModel(const VkContextDevice &inDevice, UID inID, std::string_view filePath) : 
        m_ContextDevice(inDevice), ID(inID)
    {
        Load(filePath);
    }

    MeshModel::~MeshModel()
    {
        vkDeviceWaitIdle(LogicalDevice::GetDevice());
        Buffer.IndexBuffer.Destroy();
        Buffer.VertexBuffer.Destroy();
 
        Clear();

    }

    void MeshModel::Clear()
    {
        const VkDevice device = LogicalDevice::GetDevice();
        for (const auto &sampler : m_Samplers)
        {
            vkDestroySampler(device, sampler, nullptr);
        }
        DescriptorPool.DestroyPools(device);

        m_Samplers.clear();

        m_Images.clear();
        m_Nodes.clear();
        m_Materials.clear();
        m_Meshes.clear();
    }

    void MeshModel::Draw( DrawContext &inContext) const
    {
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
                                        material.get(),
                                        primitive->Bound,
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
                    collect(childNode.get(), inContext);
                }
            }
        });
        for (const auto& node : m_Nodes)
        {
            collect(node.get(), inContext);
        }
    }

   void MeshModel::UpdateTransform(const glm::mat4 &inTransform, const glm::vec3 &inLocation, const glm::vec3 &inRotation, const glm::vec3 &inScale)
    {
        glm::mat4 newTransform = glm::mat4(1.0f);
        newTransform           = glm::translate(newTransform, inLocation);
        newTransform           = glm::rotate(newTransform, glm::radians(inRotation.x), glm::vec3(1, 0, 0));
        newTransform           = glm::rotate(newTransform, glm::radians(inRotation.y), glm::vec3(0, 1, 0));
        newTransform           = glm::rotate(newTransform, glm::radians(inRotation.z), glm::vec3(0, 0, 1));
        newTransform           = glm::scale(newTransform, inScale);
        newTransform           = inTransform * newTransform;

        std::function<void(Node *, const glm::mat4 &)> updateNode = [&](Node *node, const glm::mat4 &parentMatrix)
        {
            if (!node)
                return;

            node->CurrentMatrix = parentMatrix * node->Matrix;

            AABB local;
            local.Reset();

            if (node->Mesh)
            {
                for (auto &primPtr : node->Mesh->Primitives)
                {
                    local.Expand(primPtr->Bound.Min);
                    local.Expand(primPtr->Bound.Max);
                }
            }

            node->WorldBounds = local.Transform(node->CurrentMatrix);

            for (auto &child : node->Children)
                updateNode(child.get(), node->CurrentMatrix);
        };

        for (auto &rootNode : m_Nodes)
            updateNode(rootNode.get(), newTransform);
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

        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
                DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
                DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
                DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}
        };
        const VkDevice device = LogicalDevice::GetDevice();
        DescriptorPool.Init(device, gltf.materials.size(), sizes);

        LoadSamplers(device, gltf.samplers);
        LoadImages(gltf);
        LoadMaterials(gltf);
        
        LoadMesh(gltf);
        LoadNodes(gltf);

        pathFile = filePath;
        MaterialDataBuffer.Destroy();

        Log::Info("Loaded model: {}", pathFile.string());
    
    }

    void MeshModel::LoadSamplers(VkDevice inDevice, const std::vector<fastgltf::Sampler> &samplers)
    {
        VkSamplerCreateInfo sampl = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, 
            .pNext = nullptr,
            .minLod = 0,
            .maxLod              = VK_LOD_CLAMP_NONE
        };

        std::vector<VkSampler> modelSamplers;

        for (const fastgltf::Sampler &sampler : samplers)
        {
            sampl.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
            sampl.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

            sampl.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

            VkSampler newSampler;
            vkCreateSampler(inDevice, &sampl, nullptr, &newSampler);

            modelSamplers.push_back(newSampler);
        }

        m_Samplers = modelSamplers;
    }

    void MeshModel::LoadMaterials(const fastgltf::Asset &inFileAsset)
    {
        std::vector<std::shared_ptr<GLTFMaterial>> materials;
        materials.reserve(inFileAsset.materials.size());

        const VkDevice device = LogicalDevice::GetDevice();
        
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_ContextDevice.GetPhysicalDevice(), &properties);
        const uint32_t minAlignment = properties.limits.minUniformBufferOffsetAlignment;
        constexpr size_t   materialConstSize   = sizeof(GLTFMaterial::MaterialConstants);
        const size_t   alignedMaterialSize = Utils::AlignUp(materialConstSize, minAlignment);

        const size_t bufferSize = alignedMaterialSize * inFileAsset.materials.size();
        //const size_t bufferSize = sizeof(GLTFMaterial::MaterialConstants) * inFileAsset.materials.size();

        MaterialDataBuffer.Create(bufferSize == 0 ? 64 : bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU);
        int data_index = 0;
        //GLTFMaterial::MaterialConstants *sceneMaterialConstants = (GLTFMaterial::MaterialConstants *)MaterialDataBuffer.Info.pMappedData;
        DescriptorWriter Writer;

        const auto writerMaterial =
                [&](const EMaterialPass pass, GLTFMetallic_Roughness::MaterialResources materialResources, DescriptorAllocatorGrowable &descriptorAllocator,
                                        const GLTFMaterial::MaterialConstants& constants)
                {
                    auto matData = std::make_shared<GLTFMaterial>();
                    matData->PassType    = pass;
                    matData->Pipeline = pass == EMaterialPass::TRANSPARENT ? m_ContextDevice.RenderPipeline->TransparentPipeline :
                            m_ContextDevice.RenderPipeline->OpaquePipeline;
                    matData->MaterialSet = descriptorAllocator.Allocate(device, m_ContextDevice.RenderPipeline->Layout);
                    matData->Constants   = constants;

                    Writer.Clear();
                    Writer.WriteBuffer(0, materialResources.DataBuffer, materialConstSize,
                                       materialResources.DataBufferOffset,
                            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
                    Writer.WriteImage(1,
                                        materialResources.ColorImage.ImageView,
                                        materialResources.ColorSampler,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
                    Writer.WriteImage(2,
                                        materialResources.MetalRoughImage.ImageView,
                                        materialResources.MetalRoughSampler,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
                    /*Writer.WriteImage(3,
                                      materialResources.ColorImage.ImageView,
                                      materialResources.ColorSampler,
                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);*/

                    Writer.UpdateSet(device, matData->MaterialSet);

                    std::cerr << "matData->MaterialSet: " << matData->MaterialSet << std::endl;
                    return matData;
                };

        const auto getMaterialResources = [&]()
            {
                GLTFMetallic_Roughness::MaterialResources materialResources{};
                materialResources.ColorImage        = m_ContextDevice.WhiteImage;
                materialResources.ColorSampler      = m_ContextDevice.DefaultSamplerLinear;
                materialResources.MetalRoughImage   = m_ContextDevice.WhiteImage;
                materialResources.MetalRoughSampler = m_ContextDevice.DefaultSamplerLinear;
                materialResources.DataBuffer        = MaterialDataBuffer.Buffer;
                materialResources.DataBufferOffset  = data_index * alignedMaterialSize;

                return materialResources;
            };

        if (inFileAsset.materials.size() == 0)
        {
            // Create Empty Material
            fmt::println("Materials is empty");
            GLTFMaterial::MaterialConstants constants{};
            constants.ColorFactors  = glm::vec4(1.0f);
            constants.MetalicFactor = 0.0f;
            constants.RoughFactor   = 1.0f;
            GLTFMetallic_Roughness::MaterialResources materialResources = getMaterialResources();
            auto material                      = writerMaterial(EMaterialPass::MAIN_COLOR, materialResources, DescriptorPool, constants);
            m_Materials.push_back(std::move(material));
            return;
        }

        for (const fastgltf::Material &mat : inFileAsset.materials)
        {
            //Materials.push_back(newMat);
            const GLTFMaterial::MaterialConstants constants = { 
                glm::vec4(mat.pbrData.baseColorFactor[0], mat.pbrData.baseColorFactor[1], 
                    mat.pbrData.baseColorFactor[2], mat.pbrData.baseColorFactor[3]),
                    mat.pbrData.metallicFactor,
                    mat.pbrData.roughnessFactor
            };

            const EMaterialPass passType = mat.alphaMode == fastgltf::AlphaMode::Blend ? EMaterialPass::TRANSPARENT : EMaterialPass::MAIN_COLOR;

            GLTFMetallic_Roughness::MaterialResources materialResources = getMaterialResources();

            //grab textures from inFileAsset file
            if (mat.pbrData.baseColorTexture.has_value())
            {
                const size_t img     = inFileAsset.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
                const size_t sampler = inFileAsset.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

                materialResources.ColorImage   = m_Images[img];
                materialResources.ColorSampler = m_Samplers[sampler];
            }

            auto newMat = writerMaterial(passType, materialResources, DescriptorPool, constants);
            materials.push_back(std::move(newMat));

            data_index++;
        }

        m_Materials = materials;
    }
    
    void MeshModel::LoadImages(const fastgltf::Asset &inFileAsset)
    {
        std::vector<AllocatedImage> newImages;
        newImages.reserve(inFileAsset.images.size());
        for (const fastgltf::Image &image : inFileAsset.images)
        {
            const std::optional<AllocatedImage> img = load_image(m_ContextDevice, inFileAsset, image);
            fmt::println("LoadImage");

            if (img.has_value())
            {
                newImages.push_back(*img);
            }
            else
            {
                newImages.push_back(m_ContextDevice.ErrorCheckerboardImage);
                fmt::println("gltf failed to load texture: {}", image.name);
            }
        }

        m_Images = newImages;
    }
    
    void MeshModel::LoadNodes(const fastgltf::Asset &inFileAsset)
    {
        const size_t                       sizeNodes{inFileAsset.nodes.size()};
        std::vector<std::shared_ptr<Node>> nodes;
        nodes.reserve(sizeNodes);

        for (const fastgltf::Node &node : inFileAsset.nodes)
        {
            auto newNode = std::make_shared<Node>();
            newNode->Name = node.name;
            fmt::println("NewNode name: {}", node.name);


            if (node.meshIndex.has_value())
            {
                newNode->Mesh = m_Meshes[*node.meshIndex];
            }

            AABB localBounds;

            for (const auto &prim : newNode->Mesh->Primitives)
            {
                localBounds.Expand(prim->Bound);
            }

            /*std::visit(fastgltf::visitor{[&](const fastgltf::Node::TransformMatrix &matrix) { memcpy(&newNode->Matrix, matrix.data(), sizeof(matrix)); },
                                         [&](const fastgltf::Node::TRS &transform)
                                         {
                                             const glm::vec3 tl(transform.translation[0], transform.translation[1], transform.translation[2]);
                                             const glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
                                             const glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

                                             const glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
                                             const glm::mat4 rm = glm::toMat4(rot);
                                             const glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

                                             newNode->Matrix = tm * rm * sm;
                                         }},
                       node.transform);*/
            std::visit(fastgltf::visitor{[&](const fastgltf::Node::TransformMatrix &matrix) { memcpy(&newNode->Matrix, matrix.data(), sizeof(matrix)); },
                                         [&](const fastgltf::Node::TRS &transform)
                                         {
                                             newNode->Translation = glm::vec3(transform.translation[0], transform.translation[1], transform.translation[2]);
                                             newNode->Rotation    = glm::vec3(0);
                                             newNode->Scale       = glm::vec3(transform.scale[0], transform.scale[1], transform.scale[2]);

                                             glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), newNode->Translation);
                                             glm::quat rotation(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
                                             glm::mat4 rotationMat = glm::toMat4(rotation);
                                             glm::mat4 scaleMat    = glm::scale(glm::mat4(1.0f), newNode->Scale);

                                             newNode->Matrix = translationMat * rotationMat * scaleMat;
                                         }},
                       node.transform);

            newNode->WorldBounds = localBounds;

            nodes.push_back(std::move(newNode));
        }

        for (size_t i = 0; i < sizeNodes; ++i)
        {
            const fastgltf::Node &gltfNode = inFileAsset.nodes[i];
            auto                 &currentNode = nodes[i];

            for (const auto &child : gltfNode.children)
            {
                nodes[i]->Children.push_back(nodes[child]); 
                nodes[child]->Parent = currentNode;
            }
        }

        for (const auto &node : nodes)
        {
            if (node->Parent.expired())
            {
                m_Nodes.push_back(node);
            }
        }

    }

    void MeshModel::LoadMesh(const fastgltf::Asset &inFileAsset)
    {
        std::vector<uint32_t>              indices;
        std::vector<Vertex>                vertices;
        for (const fastgltf::Mesh &mesh : inFileAsset.meshes)
        {
            std::shared_ptr<NewMesh> newmesh = std::make_shared<NewMesh>();
            //inFile.Primitives[mesh.name.c_str()] = newmesh;
            newmesh->Name                  = mesh.name;

            /*indices.clear();
            vertices.clear();*/

            for (auto &&p : mesh.primitives)
            {
                std::unique_ptr<Primitive> newPrimitive = std::make_unique<Primitive>();
                newPrimitive->StartIndex = (uint32_t)indices.size();
                newPrimitive->Count                     = (uint32_t)inFileAsset.accessors[p.indicesAccessor.value()].count;

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
                                                                    [&](const glm::vec3& v, const size_t index)
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
                                                                  [&](const glm::vec3& v, size_t index) { vertices[initial_vtx + index].Normal = v; });
                }

                const auto uv = p.findAttribute("TEXCOORD_0");
                if (uv != p.attributes.end())
                {

                    fastgltf::iterateAccessorWithIndex<glm::vec2>(inFileAsset,
                                                                  inFileAsset.accessors[(*uv).second],
                                                                    [&](const glm::vec2& v, size_t index)
                                                                    {
                                                                        vertices[initial_vtx + index].UV = v;
                                                                    });
                }

                const auto colors = p.findAttribute("COLOR_0");
                if (colors != p.attributes.end())
                {

                    fastgltf::iterateAccessorWithIndex<glm::vec4>(inFileAsset,
                                                                  inFileAsset.accessors[(*colors).second],
                                                                  [&](const glm::vec4 &v, size_t index) { 
                            vertices[initial_vtx + index].Color = v; });
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
                    newPrimitive->Material = m_Materials[p.materialIndex.value()];
                }
                else
                {
                    newPrimitive->Material = m_Materials[0];
                }

                glm::vec3 minpos = vertices[initial_vtx].Position;
                glm::vec3 maxpos = vertices[initial_vtx].Position;
                for (size_t i = initial_vtx; i < vertices.size(); ++i)  
                {
                    minpos = glm::min(minpos, vertices[i].Position);
                    maxpos = glm::max(maxpos, vertices[i].Position);
                }

                newPrimitive->Bound = AABB(minpos, maxpos);

                /*newPrimitive->Bound.Origin     = (maxpos + minpos) / 2.f;
                newPrimitive->Bound.Extents    = (maxpos - minpos) / 2.f;
                newPrimitive->Bound.SpherRadius = glm::length(maxpos - newPrimitive->Bound.Extents);*/

                newmesh->Primitives.push_back(std::move(newPrimitive));
            }

            m_Meshes.push_back(newmesh);
        }
        Buffer.Create(indices, vertices);

    }
}