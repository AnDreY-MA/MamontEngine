#include "Loader.h"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include "Engine.h"
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

    void LoadedGLTF::Draw(const glm::mat4 &topMatrix, DrawContext &ctx)
    {
        for (auto &n : topNodes)
        {
            n->Draw(topMatrix, ctx);
        }
    }

    void LoadedGLTF::clearAll()
    {
        VkDevice dv = creator->m_Device;

        for (auto &[k, v] : meshes)
        {

            creator->DestroyBuffer(v->MeshBuffers.IndexBuffer);
            creator->DestroyBuffer(v->MeshBuffers.VertexBuffer);
        }

        for (auto &[k, v] : images)
        {

            if (v.Image == creator->m_ErrorCheckerboardImage.Image)
            {
                continue;
            }
            creator->DestroyImage(v);
        }

        for (auto &sampler : samplers)
        {
            vkDestroySampler(dv, sampler, nullptr);
        }

        auto materialBuffer    = materialDataBuffer;
        auto samplersToDestroy = samplers;

        descriptorPool.DestroyPools(dv);

        creator->DestroyBuffer(materialBuffer);
    }

    static std::optional<AllocatedImage> load_image(MEngine *engine, fastgltf::Asset &asset, fastgltf::Image &image)
    {
        AllocatedImage newImage{};

        int width, height, nrChannels;

        std::visit(
                fastgltf::visitor{
                        [](auto &arg) {},
                        [&](fastgltf::sources::URI &filePath)
                        {
                            assert(filePath.fileByteOffset == 0); // We don't support offsets with stbi.
                            assert(filePath.uri.isLocalPath());   // We're only capable of loading
                                                                  // local files.

                            const std::string path(filePath.uri.path().begin(),
                                                   filePath.uri.path().end()); // Thanks C++.
                            unsigned char    *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                            if (data)
                            {
                                VkExtent3D imagesize;
                                imagesize.width  = width;
                                imagesize.height = height;
                                imagesize.depth  = 1;

                                newImage = engine->CreateImage(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

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

                                newImage = engine->CreateImage(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

                                stbi_image_free(data);
                            }
                        },
                        [&](fastgltf::sources::BufferView &view)
                        {
                            auto &bufferView = asset.bufferViews[view.bufferViewIndex];
                            auto &buffer     = asset.buffers[bufferView.bufferIndex];

                            std::visit(fastgltf::visitor{// We only care about VectorWithMime here, because we
                                                         // specify LoadExternalBuffers, meaning all buffers
                                                         // are already loaded into a vector.
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

                                                                 newImage = engine->CreateImage(
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

    std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(MEngine* engine, std::string_view filePath)
    {
        fmt::print("Loading GLTF: {}", filePath);

        std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
        scene->creator                    = engine;
        LoadedGLTF &file                  = *scene.get();

        fastgltf::Parser parser{};

        constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers |
                                     fastgltf::Options::LoadExternalBuffers;
        // fastgltf::Options::LoadExternalImages;

        fastgltf::GltfDataBuffer data;
        data.loadFromFile(filePath);

        fastgltf::Asset gltf;

        std::filesystem::path path = filePath;

        auto type = fastgltf::determineGltfFileType(&data);
        if (type == fastgltf::GltfType::glTF)
        {
            auto load = parser.loadGLTF(&data, path.parent_path(), gltfOptions);
            if (load)
            {
                gltf = std::move(load.get());
            }
            else
            {
                std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
                return {};
            }
        }
        else if (type == fastgltf::GltfType::GLB)
        {
            auto load = parser.loadBinaryGLTF(&data, path.parent_path(), gltfOptions);
            if (load)
            {
                gltf = std::move(load.get());
            }
            else
            {
                std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
                return {};
            }
        }
        else
        {
            std::cerr << "Failed to determine glTF container" << std::endl;
            return {};
        }

        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3}, {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};

        file.descriptorPool.Init(engine->GetDevice(), gltf.materials.size(), sizes);

        for (fastgltf::Sampler &sampler : gltf.samplers)
        {

            VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
            sampl.maxLod              = VK_LOD_CLAMP_NONE;
            sampl.minLod              = 0;

            sampl.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
            sampl.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

            sampl.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

            VkSampler newSampler;
            vkCreateSampler(engine->GetDevice(), &sampl, nullptr, &newSampler);

            file.samplers.push_back(newSampler);
        }

        std::vector<std::shared_ptr<MeshAsset>>    meshes;
        std::vector<std::shared_ptr<Node>>         nodes;
        std::vector<AllocatedImage>                images;
        std::vector<std::shared_ptr<GLTFMaterial>> materials;

        for (fastgltf::Image &image : gltf.images)
        {
            std::optional<AllocatedImage> img = load_image(engine, gltf, image);

            if (img.has_value())
            {
                images.push_back(*img);
                file.images[image.name.c_str()] = *img;
            }
            else
            {
                images.push_back(engine->m_ErrorCheckerboardImage);
                std::cout << "gltf failed to load texture " << image.name << std::endl;
            }
        }

        file.materialDataBuffer = engine->CreateBuffer(
                sizeof(GLTFMetallic_Roughness::MaterialConstants) * gltf.materials.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        int                                        data_index = 0;
        GLTFMetallic_Roughness::MaterialConstants *sceneMaterialConstants =
                (GLTFMetallic_Roughness::MaterialConstants *)file.materialDataBuffer.Info.pMappedData;

        for (fastgltf::Material &mat : gltf.materials)
        {
            std::shared_ptr<GLTFMaterial> newMat = std::make_shared<GLTFMaterial>();
            materials.push_back(newMat);
            file.materials[mat.name.c_str()] = newMat;

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

            GLTFMetallic_Roughness::MaterialResources materialResources;
            // default the material textures
            materialResources.ColorImage        = engine->m_WhiteImage;
            materialResources.ColorSampler      = engine->m_DefaultSamplerLinear;
            materialResources.MetalRoughImage   = engine->m_WhiteImage;
            materialResources.MetalRoughSampler = engine->m_DefaultSamplerLinear;

            // set the uniform buffer for the material data
            materialResources.DataBuffer       = file.materialDataBuffer.Buffer;
            materialResources.DataBufferOffset = data_index * sizeof(GLTFMetallic_Roughness::MaterialConstants);
            // grab textures from gltf file
            if (mat.pbrData.baseColorTexture.has_value())
            {
                size_t img     = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
                size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

                materialResources.ColorImage   = images[img];
                materialResources.ColorSampler = file.samplers[sampler];
            }
            newMat->Data = engine->m_MetalRoughMaterial.WriteMaterial(engine->GetDevice(), passType, materialResources, file.descriptorPool);

            data_index++;
        }

        std::vector<uint32_t> indices;
        std::vector<Vertex>   vertices;

        for (fastgltf::Mesh &mesh : gltf.meshes)
        {
            std::shared_ptr<MeshAsset> newmesh = std::make_shared<MeshAsset>();
            meshes.push_back(newmesh);
            file.meshes[mesh.name.c_str()] = newmesh;
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

                auto normals = p.findAttribute("NORMAL");
                if (normals != p.attributes.end())
                {

                    fastgltf::iterateAccessorWithIndex<glm::vec3>(
                            gltf, gltf.accessors[(*normals).second], [&](glm::vec3 v, size_t index) { vertices[initial_vtx + index].Normal = v; });
                }

                auto uv = p.findAttribute("TEXCOORD_0");
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

                auto colors = p.findAttribute("COLOR_0");
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

                newSurface.Bound.Origin        = (maxpos + minpos) / 2.f;
                newSurface.Bound.Extents      = (maxpos - minpos) / 2.f;
                newSurface.Bound.SpherRadius   = glm::length(newSurface.Bound.Extents);

                newmesh->Surfaces.push_back(newSurface);
            }

            newmesh->MeshBuffers = engine->UploadMesh(indices, vertices);
        }

        for (fastgltf::Node &node : gltf.nodes)
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
            file.nodes[node.name.c_str()];

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

        // find the top nodes, with no parents
        for (auto &node : nodes)
        {
            if (node->m_Parent.lock() == nullptr)
            {
                file.topNodes.push_back(node);
                node->RefreshTransform(glm::mat4{1.f});
            }
        }
        return scene;
    }

    std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadGltfMeshes(MamontEngine::MEngine *inEngine, std::filesystem::path inFilePath)
    {
        // fmt::println("Loading {}", inFilePath.c_str());
        /*std::cout << "LOD";*/
        std::cout << "Loading" << inFilePath << std::endl;


        auto maybeAsset = [&]() -> fastgltf::Expected<fastgltf::Asset>
        {
            using fastgltf::Extensions;
            constexpr auto gltfExtensions = Extensions::KHR_texture_basisu | Extensions::KHR_mesh_quantization | Extensions::EXT_meshopt_compression |
                                            Extensions::KHR_lights_punctual | Extensions::KHR_materials_emissive_strength;

            auto                     parser = fastgltf::Parser(gltfExtensions);
            fastgltf::GltfDataBuffer data;
            data.loadFromFile(inFilePath);

            auto gltfOptions = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
            return parser.loadBinaryGLTF(&data, inFilePath.parent_path(), gltfOptions);
        }();

        auto asset = std::move(maybeAsset.get());

        std::vector<std::shared_ptr<MeshAsset>> meshes;

        std::vector<uint32_t> indices;
        std::vector<Vertex>   vertices;

        for (fastgltf::Mesh &mesh : asset.meshes)
        {
            MeshAsset newmesh;

            newmesh.Name = mesh.name;

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
                    fastgltf::Accessor &posAccessor = asset.accessors[p.findAttribute("POSITION")->second];
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
                            asset, asset.accessors[(*normals).second], [&](glm::vec3 v, size_t index) { vertices[initial_vtx + index].Normal = v; });
                }

                // load UVs
                auto uv = p.findAttribute("TEXCOORD_0");
                if (uv != p.attributes.end())
                {

                    fastgltf::iterateAccessorWithIndex<glm::vec2>(asset,
                                                                  asset.accessors[(*uv).second],
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
                            asset, asset.accessors[(*colors).second], [&](glm::vec4 v, size_t index) { vertices[initial_vtx + index].Color = v; });
                }
                newmesh.Surfaces.push_back(newSurface);
            }

            // display the vertex normals
            constexpr bool OverrideColors = false;
            if (OverrideColors)
            {
                for (Vertex &vtx : vertices)
                {
                    vtx.Color = glm::vec4(vtx.Normal, 1.f);
                }
            }
            newmesh.MeshBuffers = inEngine->UploadMesh(indices, vertices);

            meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newmesh)));
        }

        return meshes;
    }

} // namespace MamontEngine
