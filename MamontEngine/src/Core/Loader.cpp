#include "Loader.h"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include "Engine.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace MamontEngine
{
    VkFilter ExtractFilter(fastgltf::Filter inFilter)
    {
        switch (inFilter)
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

    VkSamplerMipmapMode ExtractMipmapMode(fastgltf::Filter inFilter)
    {
        switch (inFilter)
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

    void LoadedGLTF::Draw(const glm::mat4& inTopMatrix, DrawContext& inContext)
    {
        for (auto& n : TopNodes)
        {
            n->Draw(inTopMatrix, inContext);
        }
    }

    std::optional<std::shared_ptr<LoadedGLTF>> LoadGltf(MEngine *inEngine, std::string_view inFilePath)
    {
        fmt::println("Loading GLTF: {}", inFilePath);

        std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
        scene->Creator                    = inEngine;
        LoadedGLTF &file                  = *scene.get();

        fastgltf::Parser parser{};

        constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers |
                                     fastgltf::Options::LoadExternalBuffers;

        fastgltf::GltfDataBuffer data;

        fastgltf::Asset asset;
        
        std::filesystem::path path = inFilePath;
        auto type = fastgltf::determineGltfFileType(data.FromPath(inFilePath).get());

        if (type == fastgltf::GltfType::glTF)
        {

            auto load = parser.loadGltf(data.FromPath(inFilePath).get(), path.parent_path(), gltfOptions);
            if (load)
            {
                asset = std::move(load.get());
            }
            else
            {
                std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
                return {};
            }
        }else if (type == fastgltf::GltfType::GLB)
        {

            auto load = parser.loadGltfBinary(data.FromPath(inFilePath).get(), path.parent_path(), gltfOptions);
            if (load)
            {
                asset = std::move(load.get());
            }
            else
            {
                std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
                return {};
            }
        }else
        {
            std::cerr << "Failed to determine glTF container" << std::endl;
            return {};
        }


        std::vector<VkDescriptor::DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3}, {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};

        file.DescriptorPool.Init(inEngine->GetDevice(), asset.materials.size(), sizes);

        for (fastgltf::Sampler &sampler : asset.samplers)
        {

            VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
            sampl.maxLod              = VK_LOD_CLAMP_NONE;
            sampl.minLod              = 0;

            sampl.magFilter = ExtractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
            sampl.minFilter = ExtractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

            sampl.mipmapMode = ExtractMipmapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

            VkSampler newSampler;
            vkCreateSampler(inEngine->GetDevice(), &sampl, nullptr, &newSampler);

            file.Samplers.push_back(newSampler);
        }

        std::vector<std::shared_ptr<MeshAsset>>    meshes;
        std::vector<std::shared_ptr<Node>>         nodes;
        std::vector<AllocatedImage>                images;
        std::vector<std::shared_ptr<GLTFMaterial>> materials;

        for (fastgltf::Image& image : asset.images)
        {
            images.push_back(inEngine->GetErrorCheckboardImage());
        }

        file.MaterialDataBuffer = inEngine->CreateBuffer(
                sizeof(GLTFMetallic_Roughness::MaterialConstants) * asset.materials.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        int dataIndex = 0;
        GLTFMetallic_Roughness::MaterialConstants *sceneMaterialConstants =
                (GLTFMetallic_Roughness::MaterialConstants *)file.MaterialDataBuffer.Info.pMappedData;

        for (fastgltf::Material& material : asset.materials)
        {
            std::shared_ptr<GLTFMaterial> newMaterial = std::make_shared<GLTFMaterial>();
            materials.push_back(newMaterial);
            file.Materials[material.name.c_str()] = newMaterial;

            GLTFMetallic_Roughness::MaterialConstants constants;
            constants.ColorFactors.x = material.pbrData.baseColorFactor[0];
            constants.ColorFactors.y = material.pbrData.baseColorFactor[1];
            constants.ColorFactors.z = material.pbrData.baseColorFactor[2];
            constants.ColorFactors.w = material.pbrData.baseColorFactor[3];

            constants.Metal_rough_factors.x = material.pbrData.metallicFactor;
            constants.Metal_rough_factors.y = material.pbrData.roughnessFactor;

            // Write material parameters to buffer
            sceneMaterialConstants[dataIndex] = constants;

            EMaterialPass passType = EMaterialPass::MAIN_COLOR;
            if (material.alphaMode == fastgltf::AlphaMode::Blend)
            {
                passType = EMaterialPass::TRANSPARENT;
            }

            GLTFMetallic_Roughness::MaterialResources materialResources;
            // default the material textures
            inEngine->GetSetMaterialTextures(materialResources.ColorImage, materialResources.ColorSampler);
            inEngine->GetSetMaterialTextures(materialResources.MetalRoughImage, materialResources.MetalRoughSampler);

            materialResources.DataBuffer = file.MaterialDataBuffer.Buffer;
            materialResources.DataBufferOffset = dataIndex * sizeof(GLTFMetallic_Roughness::MaterialConstants);

            if (material.pbrData.baseColorTexture.has_value())
            {
                const size_t image   = asset.textures[material.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
                const size_t sampler = asset.textures[material.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

                materialResources.ColorImage   = images[image];
                materialResources.ColorSampler = file.Samplers[sampler];
            }

            // build material
            newMaterial->Data = inEngine->WriteMetalRoughMaterial(passType, materialResources, file.DescriptorPool);

            dataIndex++;
        }

        std::vector<uint32_t> indices;
        std::vector<Vertex>   vertices;

        for (fastgltf::Mesh &mesh : asset.meshes)
        {
            std::shared_ptr<MeshAsset> newMesh = std::make_shared<MeshAsset>();
            meshes.push_back(newMesh);
            file.Meshes[mesh.name.c_str()] = newMesh;
            newMesh->Name                  = mesh.name;

            // clear the mesh arrays each mesh, we dont want to merge them by error
            indices.clear();
            vertices.clear();

            for (auto &&p : mesh.primitives)
            {
                GeoSurface newSurface;
                newSurface.StartIndex = (uint32_t)indices.size();
                newSurface.Count      = (uint32_t)asset.accessors[p.indicesAccessor.value()].count;

                size_t initial_vtx = vertices.size();

                // load indexes
                {
                    fastgltf::Accessor &indexaccessor = asset.accessors[p.indicesAccessor.value()];
                    indices.reserve(indices.size() + indexaccessor.count);

                    fastgltf::iterateAccessor<std::uint32_t>(asset, indexaccessor, [&](std::uint32_t idx) { indices.push_back(idx + initial_vtx); });
                }

                // load vertex positions
                {
                    fastgltf::Accessor &posAccessor = asset.accessors[p.findAttribute("POSITION")->accessorIndex];
                    vertices.resize(vertices.size() + posAccessor.count);

                    fastgltf::iterateAccessorWithIndex<glm::vec3>(asset,
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

                // load vertex normals
                auto normals = p.findAttribute("NORMAL");
                if (normals != p.attributes.end())
                {

                    fastgltf::iterateAccessorWithIndex<glm::vec3>(
                            asset, asset.accessors[(*normals).accessorIndex], [&](glm::vec3 v, size_t index) { vertices[initial_vtx + index].Normal = v; });
                }

                // load UVs
                auto uv = p.findAttribute("TEXCOORD_0");
                if (uv != p.attributes.end())
                {

                    fastgltf::iterateAccessorWithIndex<glm::vec2>(asset,
                                                                  asset.accessors[(*uv).accessorIndex],
                                                                  [&](glm::vec2 v, size_t index)
                                                                  {
                                                                      vertices[initial_vtx + index].UV_X = v.x;
                                                                      vertices[initial_vtx + index].UV_Y = v.y;
                                                                  });
                }

                // load vertex colors
                auto colors = p.findAttribute("COLOR_0");
                if (colors != p.attributes.end())
                {

                    fastgltf::iterateAccessorWithIndex<glm::vec4>(
                            asset, asset.accessors[(*colors).accessorIndex], [&](glm::vec4 v, size_t index) { vertices[initial_vtx + index].Color = v; });
                }

                if (p.materialIndex.has_value())
                {
                    newSurface.Material = materials[p.materialIndex.value()];
                }
                else
                {
                    newSurface.Material = materials[0];
                }

                newMesh->Surfaces.push_back(newSurface);
            }

            newMesh->MeshBuffers = inEngine->UploadMesh(indices, vertices);
        }

        for (fastgltf::Node &node : asset.nodes)
        {
            std::shared_ptr<Node> newNode;

            // find if the node has a mesh, and if it does hook it to the mesh pointer and allocate it with the meshnode class
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
            file.Nodes[node.name.c_str()];

            std::visit(fastgltf::visitor{
                    [&](fastgltf::math::fmat4x4 matrix) { 
                    memcpy(&newNode->GetLocalTransform(), matrix.data(), sizeof(matrix)); 
                },

                [&](fastgltf::TRS transform)
                                         {
                                             glm::vec3 tl(transform.translation[0], transform.translation[1], transform.translation[2]);
                                             glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
                                             glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

                                             glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
                                             glm::mat4 rm = glm::toMat4(rot);
                                             glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

                                             newNode->SetLocalTransform(tm * rm * sm);
                                         }


            }, node.transform);
        }

        // run loop again to setup transform hierarchy
        for (int i = 0; i < asset.nodes.size(); i++)
        {
            fastgltf::Node        &node      = asset.nodes[i];
            std::shared_ptr<Node> &sceneNode = nodes[i];

            for (auto &c : node.children)
            {
                sceneNode->AddChild(nodes[c]);
                nodes[c]->SetParent(sceneNode);
            }
        }

        // find the top nodes, with no parents
        for (auto &node : nodes)
        {
            if (node->ParentLock() == nullptr)
            {
                file.TopNodes.push_back(node);
                node->RefreshTransform(glm::mat4{1.f});
            }
        }
        
        return scene;

    }
    //
    //std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadGltfMeshes(MamontEngine::MEngine *inEngine, std::filesystem::path inFilePath)
    //{
    //    // fmt::println("Loading {}", inFilePath.c_str());
    //    /*std::cout << "LOD";*/
    //    std::cout << "Loading" << inFilePath << std::endl;


    //    auto maybeAsset = [&]() -> fastgltf::Expected<fastgltf::Asset>
    //    {
    //        using fastgltf::Extensions;
    //        constexpr auto gltfExtensions = Extensions::KHR_texture_basisu | Extensions::KHR_mesh_quantization | Extensions::EXT_meshopt_compression |
    //                                        Extensions::KHR_lights_punctual | Extensions::KHR_materials_emissive_strength;

    //        auto                     parser = fastgltf::Parser(gltfExtensions);
    //        fastgltf::GltfDataBuffer data;


    //        // if (data.FromPath(inFilePath))
    //        //{
    //        //     // Обработка ошибки: файл не загружен
    //        //     fmt::println("Se");
    //        // }

    //        constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
    //        return parser.loadGltfBinary(data.FromPath(inFilePath).get(), inFilePath.parent_path(), gltfOptions);
    //    }();

    //    auto asset = std::move(maybeAsset.get());

    //    std::vector<std::shared_ptr<MeshAsset>> meshes;

    //    std::vector<uint32_t> indices;
    //    std::vector<Vertex>   vertices;

    //    for (fastgltf::Mesh &mesh : asset.meshes)
    //    {
    //        MeshAsset newmesh;

    //        newmesh.Name = mesh.name;

    //        // clear the mesh arrays each mesh, we dont want to merge them by error
    //        indices.clear();
    //        vertices.clear();

    //        for (auto &&p : mesh.primitives)
    //        {
    //            GeoSurface newSurface;
    //            newSurface.StartIndex = (uint32_t)indices.size();
    //            newSurface.Count      = (uint32_t)asset.accessors[p.indicesAccessor.value()].count;

    //            size_t initial_vtx = vertices.size();

    //            // load indexes
    //            {
    //                fastgltf::Accessor &indexaccessor = asset.accessors[p.indicesAccessor.value()];
    //                indices.reserve(indices.size() + indexaccessor.count);

    //                fastgltf::iterateAccessor<std::uint32_t>(asset, indexaccessor, [&](std::uint32_t idx) { indices.push_back(idx + initial_vtx); });
    //            }

    //            // load vertex positions
    //            {
    //                fastgltf::Accessor &posAccessor = asset.accessors[p.findAttribute("POSITION")->accessorIndex];
    //                vertices.resize(vertices.size() + posAccessor.count);

    //                fastgltf::iterateAccessorWithIndex<glm::vec3>(asset,
    //                                                              posAccessor,
    //                                                              [&](glm::vec3 v, size_t index)
    //                                                              {
    //                                                                  Vertex newvtx;
    //                                                                  newvtx.Position               = v;
    //                                                                  newvtx.Normal                 = {1, 0, 0};
    //                                                                  newvtx.Color                  = glm::vec4{1.f};
    //                                                                  newvtx.UV_X                   = 0;
    //                                                                  newvtx.UV_Y                   = 0;
    //                                                                  vertices[initial_vtx + index] = newvtx;
    //                                                              });
    //            }

    //            // load vertex normals
    //            auto normals = p.findAttribute("NORMAL");
    //            if (normals != p.attributes.end())
    //            {

    //                fastgltf::iterateAccessorWithIndex<glm::vec3>(
    //                        asset, asset.accessors[(*normals).accessorIndex], [&](glm::vec3 v, size_t index) { vertices[initial_vtx + index].Normal = v; });
    //            }

    //            // load UVs
    //            auto uv = p.findAttribute("TEXCOORD_0");
    //            if (uv != p.attributes.end())
    //            {

    //                fastgltf::iterateAccessorWithIndex<glm::vec2>(asset,
    //                                                              asset.accessors[(*uv).accessorIndex],
    //                                                              [&](glm::vec2 v, size_t index)
    //                                                              {
    //                                                                  vertices[initial_vtx + index].UV_X = v.x;
    //                                                                  vertices[initial_vtx + index].UV_Y = v.y;
    //                                                              });
    //            }

    //            // load vertex colors
    //            auto colors = p.findAttribute("COLOR_0");
    //            if (colors != p.attributes.end())
    //            {

    //                fastgltf::iterateAccessorWithIndex<glm::vec4>(
    //                        asset, asset.accessors[(*colors).accessorIndex], [&](glm::vec4 v, size_t index) { vertices[initial_vtx + index].Color = v; });
    //            }
    //            newmesh.Surfaces.push_back(newSurface);
    //        }

    //        // display the vertex normals
    //        constexpr bool OverrideColors = false;
    //        if (OverrideColors)
    //        {
    //            for (Vertex &vtx : vertices)
    //            {
    //                vtx.Color = glm::vec4(vtx.Normal, 1.f);
    //            }
    //        }
    //        newmesh.MeshBuffers = inEngine->UploadMesh(indices, vertices);

    //        meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newmesh)));
    //    }

    //    return meshes;
    //}

} // namespace MamontEngine
