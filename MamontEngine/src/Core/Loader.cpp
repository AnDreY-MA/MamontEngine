#include "Loader.h"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include "Engine.h"

//
std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadGltfMeshes(MamontEngine::MEngine *inEngine, std::filesystem::path inFilePath)
{
    //fmt::println("Loading {}", inFilePath.c_str());
    /*std::cout << "LOD";*/
    std::cout << "Loading" << inFilePath << std::endl;

    


    auto maybeAsset = [&]() -> fastgltf::Expected<fastgltf::Asset> { 
        using fastgltf::Extensions;
        constexpr auto gltfExtensions = Extensions::KHR_texture_basisu | Extensions::KHR_mesh_quantization | Extensions::EXT_meshopt_compression |
                                        Extensions::KHR_lights_punctual | Extensions::KHR_materials_emissive_strength;

        auto parser = fastgltf::Parser(gltfExtensions);
        fastgltf::GltfDataBuffer data;
            
        
        //if (data.FromPath(inFilePath))
        //{
        //    // Обработка ошибки: файл не загружен
        //    fmt::println("Se");
        //}

        constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
        return parser.loadGltfBinary(data.FromPath(inFilePath).get(), inFilePath.parent_path(), gltfOptions);
        }();
    
    auto asset = std::move(maybeAsset.get());

    std::vector<std::shared_ptr<MeshAsset>> meshes;

    std::vector<uint32_t> indices;
    std::vector<Vertex>   vertices;

    for (fastgltf::Mesh& mesh : asset.meshes)
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
            newmesh.Surfaces.push_back(newSurface);
        }

        // display the vertex normals
        constexpr bool OverrideColors = true;
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