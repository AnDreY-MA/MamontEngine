#include "Loader.h"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include "Core/Engine.h"
#include "Core/VkDeviceContext.h"
#include "Graphics/MScene.h"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <stb/include/stb_image.h>


namespace MamontEngine
{
    static VkFilter extract_filter(fastgltf::Filter filter)
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


    static std::optional<AllocatedImage> load_image(VkContextDevice &inDeviece, fastgltf::Asset &asset, fastgltf::Image &image)
    {
        AllocatedImage newImage{};

        int width, height, nrChannels;

        std::visit(
                fastgltf::visitor{
                        [](auto &arg) {},
                        [&](fastgltf::sources::URI &filePath)
                        {
                            assert(filePath.fileByteOffset == 0);

                            const std::string path(filePath.uri.path().begin(),
                                                   filePath.uri.path().end());
                            unsigned char    *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                            if (data)
                            {
                                VkExtent3D imagesize;
                                imagesize.width  = width;
                                imagesize.height = height;
                                imagesize.depth  = 1;

                                newImage = inDeviece.CreateImage(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

                                stbi_image_free(data);
                            }
                        },
                        [&](fastgltf::sources::Vector &vector)
                        {
                            unsigned char *data =
                                    stbi_load_from_memory(vector.bytes.data(), static_cast<int>(vector.bytes.size()), &width, &height, &nrChannels, 4);
                            if (data)
                            {
                                VkExtent3D imagesize;
                                imagesize.width  = width;
                                imagesize.height = height;
                                imagesize.depth  = 1;

                                newImage = inDeviece.CreateImage(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

                                stbi_image_free(data);
                            }
                        },
                        [&](fastgltf::sources::BufferView &view)
                        {
                            auto &bufferView = asset.bufferViews[view.bufferViewIndex];
                            auto &buffer     = asset.buffers[bufferView.bufferIndex];

                            std::visit(fastgltf::visitor{
                                                         [](auto &arg) {},
                                                         [&](fastgltf::sources::Vector &vector)
                                                         {
                                                             unsigned char *data = stbi_load_from_memory(vector.bytes.data() + bufferView.byteOffset,
                                                                                                         static_cast<int>(bufferView.byteLength),
                                                                                                         &width,
                                                                                                         &height,
                                                                                                         &nrChannels,
                                                                                                         4);
                                                             if (data)
                                                             {
                                                                 VkExtent3D imagesize;
                                                                 imagesize.width  = width;
                                                                 imagesize.height = height;
                                                                 imagesize.depth  = 1;

                                                                 newImage = inDeviece.CreateImage(
                                                                         data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

                                                                 stbi_image_free(data);
                                                             }
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
        else
        {
            return newImage;
        }
    }

    static std::vector<AllocatedImage> LoadTexture(VkContextDevice &inDeviece, MScene &inFile, fastgltf::Asset &gltf)
    {
        std::vector<AllocatedImage> images;

        for (fastgltf::Image &image : gltf.images)
        {
            std::optional<AllocatedImage> img = load_image(inDeviece, gltf, image);

            if (img.has_value())
            {
                images.push_back(*img);
                inFile.Images[image.name.c_str()] = *img;
            }
            else
            {
                images.push_back(inDeviece.ErrorCheckerboardImage);
                std::cout << "gltf failed to load texture " << image.name << std::endl;
            }
        }

        return images;
    }

    static std::vector<std::shared_ptr<GLTFMaterial>>
    LoadMaterials(VkContextDevice &inDeviece, MScene &inFile, fastgltf::Asset &gltf, const std::vector<AllocatedImage> &inImages)
    {
        std::vector<std::shared_ptr<GLTFMaterial>> materials;

        inFile.MaterialDataBuffer = inDeviece.CreateBuffer(
                sizeof(GLTFMetallic_Roughness::MaterialConstants) * gltf.materials.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        int                                        data_index = 0;
        GLTFMetallic_Roughness::MaterialConstants *sceneMaterialConstants =
                (GLTFMetallic_Roughness::MaterialConstants *)inFile.MaterialDataBuffer.Info.pMappedData;

        for (fastgltf::Material &mat : gltf.materials)
        {
            std::shared_ptr<GLTFMaterial> newMat = std::make_shared<GLTFMaterial>();
            materials.push_back(newMat);
            inFile.Materials[mat.name.c_str()] = newMat;

            GLTFMetallic_Roughness::MaterialConstants constants;
            constants.ColorFactors.x = mat.pbrData.baseColorFactor[0];
            constants.ColorFactors.y = mat.pbrData.baseColorFactor[1];
            constants.ColorFactors.z = mat.pbrData.baseColorFactor[2];
            constants.ColorFactors.w = mat.pbrData.baseColorFactor[3];

            constants.Metal_rough_factors.x = mat.pbrData.metallicFactor;
            constants.Metal_rough_factors.y = mat.pbrData.roughnessFactor;
            // write material parameters to buffer
            sceneMaterialConstants[data_index] = constants;

            EMaterialPass passType = EMaterialPass::MAIN_COLOR;
            if (mat.alphaMode == fastgltf::AlphaMode::Blend)
            {
                passType = EMaterialPass::TRANSPARENT;
            }

            GLTFMetallic_Roughness::MaterialResources materialResources{};
            materialResources.ColorImage        = inDeviece.WhiteImage;
            materialResources.ColorSampler      = inDeviece.DefaultSamplerLinear;
            materialResources.MetalRoughImage   = inDeviece.WhiteImage;
            materialResources.MetalRoughSampler = inDeviece.DefaultSamplerLinear;

            materialResources.DataBuffer       = inFile.MaterialDataBuffer.Buffer;
            materialResources.DataBufferOffset = data_index * sizeof(GLTFMetallic_Roughness::MaterialConstants);

            // grab textures from gltf file
            if (mat.pbrData.baseColorTexture.has_value())
            {
                size_t img     = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
                size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

                materialResources.ColorImage   = inImages[img];
                materialResources.ColorSampler = inFile.Samplers[sampler];
            }

            newMat->Data = inDeviece.WriteMetalMaterial(passType, materialResources, inFile.DescriptorPool);

            data_index++;
        }

        return materials;
    }

    static void LoadSamples(VkDevice inDevice, MScene &inFile, std::vector<fastgltf::Sampler> &samplers)
    {
        for (fastgltf::Sampler &sampler : samplers)
        {

            VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
            sampl.maxLod              = VK_LOD_CLAMP_NONE;
            sampl.minLod              = 0;

            sampl.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
            sampl.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

            sampl.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

            VkSampler newSampler;
            vkCreateSampler(inDevice, &sampl, nullptr, &newSampler);

            inFile.Samplers.push_back(newSampler);
        }
    }
    
    static std::vector<std::shared_ptr<Mesh>>
    LoadMeshes(VkContextDevice &inDeviece, fastgltf::Asset &gltf, MScene &inFile, const std::vector<std::shared_ptr<GLTFMaterial>> &materials)
    {
        std::vector<std::shared_ptr<Mesh>>    meshes;
        std::vector<uint32_t>              indices;
        std::vector<Vertex>                vertices;
        for (fastgltf::Mesh &mesh : gltf.meshes)
        {
            std::shared_ptr<Mesh> newmesh = std::make_shared<Mesh>();
            meshes.push_back(newmesh);
            inFile.Meshes[mesh.name.c_str()] = newmesh;
            newmesh->Name                  = mesh.name;

            indices.clear();
            vertices.clear();

            for (auto &&p : mesh.primitives)
            {
                GeoSurface newSurface;
                newSurface.StartIndex = (uint32_t)indices.size();
                newSurface.Count      = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

                size_t initial_vtx = vertices.size();

                {
                    fastgltf::Accessor &indexaccessor = gltf.accessors[p.indicesAccessor.value()];
                    indices.reserve(indices.size() + indexaccessor.count);

                    fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor, [&](std::uint32_t idx) { indices.push_back(idx + initial_vtx); });
                }

                {
                    fastgltf::Accessor &posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
                    vertices.resize(vertices.size() + posAccessor.count);

                    fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf,
                                                                  posAccessor,
                                                                  [&](glm::vec3 v, size_t index)
                                                                  {
                                                                      Vertex newvtx;
                                                                      newvtx.Position               = v;
                                                                      newvtx.Normal                 = {1, 0, 0};
                                                                      newvtx.Color                  = glm::vec4{1.f};
                                                                      newvtx.UV_X                   = 0;
                                                                      newvtx.UV_Y                   = 0;
                                                                      vertices[initial_vtx + index] = newvtx;
                                                                  });
                }

                const auto normals = p.findAttribute("NORMAL");
                if (normals != p.attributes.end())
                {

                    fastgltf::iterateAccessorWithIndex<glm::vec3>(
                            gltf, gltf.accessors[(*normals).second], [&](glm::vec3 v, size_t index) { vertices[initial_vtx + index].Normal = v; });
                }

                const auto uv = p.findAttribute("TEXCOORD_0");
                if (uv != p.attributes.end())
                {

                    fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf,
                                                                  gltf.accessors[(*uv).second],
                                                                  [&](glm::vec2 v, size_t index)
                                                                  {
                                                                      vertices[initial_vtx + index].UV_X = v.x;
                                                                      vertices[initial_vtx + index].UV_Y = v.y;
                                                                  });
                }

                const auto colors = p.findAttribute("COLOR_0");
                if (colors != p.attributes.end())
                {

                    fastgltf::iterateAccessorWithIndex<glm::vec4>(
                            gltf, gltf.accessors[(*colors).second], [&](glm::vec4 v, size_t index) { vertices[initial_vtx + index].Color = v; });
                }

                if (p.materialIndex.has_value())
                {
                    newSurface.Material = materials[p.materialIndex.value()];
                }
                else
                {
                    newSurface.Material = materials[0];
                }

                glm::vec3 minpos = vertices[initial_vtx].Position;
                glm::vec3 maxpos = vertices[initial_vtx].Position;
                for (int i = initial_vtx; i < vertices.size(); i++)
                {
                    minpos = glm::min(minpos, vertices[i].Position);
                    maxpos = glm::max(maxpos, vertices[i].Position);
                }

                newSurface.Bound.Origin      = (maxpos + minpos) / 2.f;
                newSurface.Bound.Extents     = (maxpos - minpos) / 2.f;
                newSurface.Bound.SpherRadius = glm::length(newSurface.Bound.Extents);

                newmesh->Surfaces.push_back(newSurface);
            }

            newmesh->MeshBuffers = inDeviece.CreateGPUMeshBuffer(indices, vertices);
        }

        return meshes;
    }

    static std::vector<std::shared_ptr<Node>>
    LoadNodes(std::vector<fastgltf::Node> &gltfNodes, MScene &inFile, const std::vector<std::shared_ptr<Mesh>> &meshes)
    {
        std::vector<std::shared_ptr<Node>> nodes;

        for (fastgltf::Node &node : gltfNodes)
        {
            std::shared_ptr<Node> newNode;

            if (node.meshIndex.has_value())
            {
                newNode                                      = std::make_shared<MeshNode>();
                static_cast<MeshNode *>(newNode.get())->Mesh = meshes[*node.meshIndex];
            }
            else
            {
                newNode = std::make_shared<Node>();
            }

            nodes.push_back(newNode);
            inFile.Nodes[node.name.c_str()];

            std::visit(fastgltf::visitor{[&](fastgltf::Node::TransformMatrix matrix) { memcpy(&newNode->m_LocalTransform, matrix.data(), sizeof(matrix)); },
                                         [&](fastgltf::Node::TRS transform)
                                         {
                                             glm::vec3 tl(transform.translation[0], transform.translation[1], transform.translation[2]);
                                             glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
                                             glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

                                             glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
                                             glm::mat4 rm = glm::toMat4(rot);
                                             glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

                                             newNode->m_LocalTransform = tm * rm * sm;
                                         }},
                       node.transform);
        }

        return nodes;
    }
    
    std::optional<std::shared_ptr<MScene>> loadGltf(VkContextDevice &inDeviece, std::string_view filePath)
    {
        fmt::print("Loading GLTF: {}", filePath);

        std::shared_ptr<MScene> scene     = std::make_shared<MScene>(inDeviece);
        MScene &file                      = *scene.get();

        fastgltf::Parser parser{};

        constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers |
                                     fastgltf::Options::LoadExternalBuffers;

        fastgltf::GltfDataBuffer data;
        data.loadFromFile(filePath);

        fastgltf::Asset gltf;

        std::filesystem::path path = filePath;

        const auto type = fastgltf::determineGltfFileType(&data);

        auto loadResult = (type == fastgltf::GltfType::glTF) ? parser.loadGLTF(&data, path.parent_path(), gltfOptions)
                                                             : parser.loadBinaryGLTF(&data, path.parent_path(), gltfOptions);

        if (!loadResult)
        {
            fmt::print(stderr, "Failed to load glTF: {}\n", fastgltf::to_underlying(loadResult.error()));
            return {};
        }
        gltf = std::move(loadResult.get());

        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3}, 
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3}, 
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};

        file.DescriptorPool.Init(inDeviece.Device, gltf.materials.size(), sizes);

        LoadSamples(inDeviece.Device, file, gltf.samplers);

        const std::vector<AllocatedImage>                 images    = LoadTexture(inDeviece, file, gltf);
        const std::vector<std::shared_ptr<GLTFMaterial>> materials = LoadMaterials(inDeviece, file, gltf, images);
        const std::vector<std::shared_ptr<Mesh>> meshes = LoadMeshes(inDeviece, gltf, file, materials);
        
        std::vector<std::shared_ptr<Node>> nodes = LoadNodes(gltf.nodes, file, meshes);
        
         for (int i = 0; i < gltf.nodes.size(); i++)
        {
            fastgltf::Node        &node      = gltf.nodes[i];
            std::shared_ptr<Node> &sceneNode = nodes[i];

            for (auto &c : node.children)
            {
                sceneNode->m_Children.push_back(nodes[c]);
                nodes[c]->m_Parent = sceneNode;
            }
        }

        for (auto &node : nodes)
        {
            if (node->m_Parent.lock() == nullptr)
            {
                file.TopNodes.push_back(node);
                node->RefreshTransform(glm::mat4{1.f});
            }
        }
        return scene;
    }

} // namespace MamontEngine
