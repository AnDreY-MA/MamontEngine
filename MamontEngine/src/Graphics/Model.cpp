#include "Model.h"
#include <stb/include/stb_image.h>
#include <expected>
#include <glm/gtx/quaternion.hpp>

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
                    data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, /*mip=*/false);
            stbi_image_free(data);

            return newImage.Image != VK_NULL_HANDLE;
        };

        std::visit(
                fastgltf::visitor{
                        [](auto &arg) {},
                        [&](fastgltf::sources::URI &filePath)
                        {
                            assert(filePath.fileByteOffset == 0);

                            const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());
                            unsigned char    *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                            process_pixels(data);
                        },
                        [&](fastgltf::sources::Vector &vector)
                        {
                            unsigned char *data =
                                    stbi_load_from_memory(vector.bytes.data(), static_cast<int>(vector.bytes.size()), &width, &height, &nrChannels, 4);
                            process_pixels(data);
                        },
                        [&](fastgltf::sources::BufferView &view)
                        {
                            auto &bufferView = asset.bufferViews[view.bufferViewIndex];
                            auto &buffer     = asset.buffers[bufferView.bufferIndex];

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
            fmt::println("Load Image(): image = NULL");
            return {};
        }

        return newImage;
    }

    MeshModel::~MeshModel()
    {
        for (const auto &mesh : m_Meshes)
        {
            m_ContextDevice.DestroyBuffer(mesh->Buffer.IndexBuffer);
            m_ContextDevice.DestroyBuffer(mesh->Buffer.VertexBuffer);
        }

        //for (auto &image : m_Images)
        //{
        //    if (image.Image != VK_NULL_HANDLE && image.Allocation != VK_NULL_HANDLE)
        //    {
        //        m_ContextDevice.DestroyImage(image);
        //        image = {}; // обнуляем
        //    }
        //}

        const VkDevice dv = m_ContextDevice.Device;
        for (const auto &sampler : m_Samplers)
        {
            vkDestroySampler(dv, sampler, nullptr);
        }

        DescriptorPool.DestroyPools(dv);

        m_ContextDevice.DestroyBuffer(MaterialDataBuffer);
    }

    void MeshModel::Draw(const glm::mat4 &inTopMatrix, DrawContext &inContext)
    {
        for (const auto &node : m_Nodes)
        {
            if (!node || !node->Mesh)
                continue;

            const glm::mat4 nodeMatrix = inTopMatrix * node->Matrix;

            if (node->Mesh->Primitives.empty())
            {
                fmt::println("Primitives is empty");
                return;
            }

            for (const auto &primitive : node->Mesh->Primitives)
            {
                if (!primitive || !primitive->Material)
                    continue;

                const auto        &material = primitive->Material;
                const RenderObject def(primitive->Count,
                                       primitive->StartIndex,
                                       node->Mesh->Buffer.IndexBuffer.Buffer,
                                       node->Mesh->Buffer.VertexBuffer.Buffer,
                                       &material->Data,
                                       primitive->Bound,
                                       nodeMatrix,
                                       node->Mesh->Buffer.VertexBufferAddress);

                if (material->Data.PassType == EMaterialPass::TRANSPARENT)
                    inContext.TransparentSurfaces.push_back(def);
                else
                    inContext.OpaqueSurfaces.push_back(def);
            }
        }
    }

    void MeshModel::UpdateTransform(const glm::mat4 &inTransform)
    {
        if (!m_Nodes.empty())
        {
            for (auto &n : m_Nodes)
            {
                n->Matrix = inTransform;
            }
        }
    }

    void MeshModel::Load(std::string_view filePath)
    {
        fmt::print("Loading GLTF: {}", filePath);

        fastgltf::Parser parser{};

        constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers |
                                        fastgltf::Options::LoadExternalBuffers;

        fastgltf::GltfDataBuffer data;
        data.loadFromFile(filePath);

        fastgltf::Asset gltf;

        const std::filesystem::path path = filePath;

        const auto type = fastgltf::determineGltfFileType(&data);

        auto loadResult = (type == fastgltf::GltfType::glTF) ? parser.loadGLTF(&data, path.parent_path(), gltfOptions)
                                                                : parser.loadBinaryGLTF(&data, path.parent_path(), gltfOptions);

        if (!loadResult)
        {
            fmt::print(stderr, "Failed to load glTF: {}\n", fastgltf::to_underlying(loadResult.error()));
            return;
        }

        gltf = std::move(loadResult.get());

        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};

        DescriptorPool.Init(m_ContextDevice.Device, gltf.materials.size(), sizes);

        LoadSamplers(m_ContextDevice.Device, gltf.samplers);
        LoadImages(gltf);
        LoadMaterials(gltf);
        

        LoadMesh(gltf);
        LoadNodes(gltf);
    
    }

    void MeshModel::LoadSamplers(VkDevice inDevice, const std::vector<fastgltf::Sampler> &samplers)
    {
        VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
        sampl.maxLod              = VK_LOD_CLAMP_NONE;
        sampl.minLod              = 0;

        for (const fastgltf::Sampler &sampler : samplers)
        {
            sampl.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
            sampl.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

            sampl.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

            VkSampler newSampler;
            vkCreateSampler(inDevice, &sampl, nullptr, &newSampler);

            m_Samplers.push_back(newSampler);
        }
    }

    void MeshModel::LoadMaterials(const fastgltf::Asset &inFileAsset)
    {
        std::vector<std::shared_ptr<GLTFMaterial>> materials;
        materials.reserve(inFileAsset.materials.size());

        MaterialDataBuffer = m_ContextDevice.CreateBuffer(
                sizeof(GLTFMetallic_Roughness::MaterialConstants) * inFileAsset.materials.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU);
        int                                        data_index = 0;
        GLTFMetallic_Roughness::MaterialConstants *sceneMaterialConstants =
                (GLTFMetallic_Roughness::MaterialConstants *)MaterialDataBuffer.Info.pMappedData;
        DescriptorWriter Writer;

        const auto writerMaterial =
                [&](const EMaterialPass pass, GLTFMetallic_Roughness::MaterialResources materialResources, DescriptorAllocatorGrowable
                &descriptorAllocator)
                {
                    MaterialInstance matData{};
                    matData.PassType    = pass;
                    matData.Pipeline =
                            pass == EMaterialPass::TRANSPARENT ? m_ContextDevice.RenderPipeline->TransparentPipeline :
                            m_ContextDevice.RenderPipeline->OpaquePipeline;
                    matData.MaterialSet = descriptorAllocator.Allocate(m_ContextDevice.Device, m_ContextDevice.RenderPipeline->Layout);

                    Writer.Clear();
                    Writer.WriteBuffer(
                            0, materialResources.DataBuffer, sizeof(GLTFMetallic_Roughness::MaterialConstants), materialResources.DataBufferOffset,
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

                    Writer.UpdateSet(m_ContextDevice.Device, matData.MaterialSet);

                    return matData;
                };

        for (const fastgltf::Material &mat : inFileAsset.materials)
        {
            std::shared_ptr<GLTFMaterial> newMat = std::make_shared<GLTFMaterial>();
            materials.push_back(newMat);
            //Materials.push_back(newMat);

            GLTFMetallic_Roughness::MaterialConstants constants{};
            constants.ColorFactors.x = mat.pbrData.baseColorFactor[0];
            constants.ColorFactors.y = mat.pbrData.baseColorFactor[1];
            constants.ColorFactors.z = mat.pbrData.baseColorFactor[2];
            constants.ColorFactors.w = mat.pbrData.baseColorFactor[3];

            constants.MetalicFactor = mat.pbrData.metallicFactor;
            constants.RoughFactor = mat.pbrData.roughnessFactor;
            //write material parameters to buffer
            sceneMaterialConstants[data_index] = constants;

            const EMaterialPass passType = mat.alphaMode == fastgltf::AlphaMode::Blend ? EMaterialPass::TRANSPARENT : EMaterialPass::MAIN_COLOR;

            GLTFMetallic_Roughness::MaterialResources materialResources{};
            materialResources.ColorImage        = m_ContextDevice.WhiteImage;
            materialResources.ColorSampler      = m_ContextDevice.DefaultSamplerLinear;
            materialResources.MetalRoughImage   = m_ContextDevice.WhiteImage;
            materialResources.MetalRoughSampler = m_ContextDevice.DefaultSamplerLinear;

            materialResources.DataBuffer       = MaterialDataBuffer.Buffer;
            materialResources.DataBufferOffset = data_index * sizeof(GLTFMetallic_Roughness::MaterialConstants);

            //grab textures from inFileAsset file
            if (mat.pbrData.baseColorTexture.has_value())
            {
                const size_t img     = inFileAsset.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
                const size_t sampler = inFileAsset.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

                materialResources.ColorImage   = m_Images[img];
                materialResources.ColorSampler = m_Samplers[sampler];
            }

            newMat->Data = writerMaterial(passType, materialResources, DescriptorPool);

            data_index++;
        }

        m_Materials = materials;
    }
    
    void MeshModel::LoadImages(const fastgltf::Asset &inFileAsset)
    {
        for (const fastgltf::Image &image : inFileAsset.images)
        {
            std::optional<AllocatedImage> img = load_image(m_ContextDevice, inFileAsset, image);

            if (img.has_value())
            {
                m_Images.push_back(*img);
            }
            else
            {
                m_Images.push_back(m_ContextDevice.ErrorCheckerboardImage);
                std::cout << "gltf failed to load texture " << image.name << std::endl;
            }
        }
    }
    
    void MeshModel::LoadNodes(const fastgltf::Asset &inFileAsset)
    {
        std::vector<std::unique_ptr<Node>> nodes;
        nodes.reserve(inFileAsset.nodes.size());

        for (const fastgltf::Node &node : inFileAsset.nodes)
        {
            auto newNode = std::make_unique<Node>();

            if (node.meshIndex.has_value())
            {
                newNode->Mesh = m_Meshes[*node.meshIndex].get();
            }

            std::visit(fastgltf::visitor{[&](const fastgltf::Node::TransformMatrix &matrix) { memcpy(&newNode->Matrix, matrix.data(), sizeof(matrix)); },
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
                       node.transform);

            nodes.push_back(std::move(newNode));
        }

        for (size_t i = 0; i < inFileAsset.nodes.size(); i++)
        {
            const fastgltf::Node &node = inFileAsset.nodes[i];

            for (const auto &c : node.children)
            {
                nodes[i]->Children.push_back(nodes[c].get()); 
                nodes[c]->Parent = nodes[i].get();
            }
        }

        for (auto &node : nodes)
        {
            if (node->Parent == nullptr)
            {
                m_Nodes.push_back(std::move(node));
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
            //newmesh->Name                  = mesh.name;

            indices.clear();
            vertices.clear();

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
                for (size_t i = initial_vtx; i < vertices.size(); i++)
                {
                    minpos = glm::min(minpos, vertices[i].Position);
                    maxpos = glm::max(maxpos, vertices[i].Position);
                }

                newPrimitive->Bound.Origin     = (maxpos + minpos) / 2.f;
                newPrimitive->Bound.Extents    = (maxpos - minpos) / 2.f;
                newPrimitive->Bound.SpherRadius = glm::length(newPrimitive->Bound.Extents);

                newmesh->Primitives.push_back(std::move(newPrimitive));
            }

            newmesh->Buffer = m_ContextDevice.CreateGPUMeshBuffer(indices, vertices);

            m_Meshes.push_back(newmesh);
        }
    }
}