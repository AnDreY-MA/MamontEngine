#include "Graphics/Mesh.h"
#include "Core/ContextDevice.h"

#include <Core/RenderData.h>
#include <glm/gtx/transform.hpp>

namespace MamontEngine
{
    Mesh::Mesh(VkContextDevice &inDevice) : Device(inDevice)
    {

    }
    Mesh::~Mesh()
    {
        ClearAll();
    }

    void Mesh::Draw(const glm::mat4& inTopMatrix, DrawContext& inContext)   
    {
        for (auto& node : Nodes)
        {
            if (!node)
                continue;
            auto &primitive = node->Primitive;
            if (!primitive)
                continue;

            const glm::mat4 nodeMatrix = inTopMatrix * node->WorldTransform;
            

            for (auto &surface : primitive->Surfaces)
            {
                RenderObject def(surface.Count,
                                 surface.StartIndex,
                                 primitive->Buffers.IndexBuffer.Buffer,
                                 &surface.Material->Data,
                                 surface.Bound,
                                 nodeMatrix,
                                 primitive->Buffers.VertexBufferAddress);
                if (surface.Material->Data.PassType == EMaterialPass::TRANSPARENT)
                {
                    inContext.TransparentSurfaces.push_back(def);
                }
                else
                {
                    inContext.OpaqueSurfaces.push_back(def);
                }
            }

        }
    }

    void Mesh::ClearAll()
    {
        for (auto &[k, v] : Primitives)
        {
            Device.DestroyBuffer(v->Buffers.IndexBuffer);
            Device.DestroyBuffer(v->Buffers.VertexBuffer);
        }

        for (auto &[k, v] : Images)
        {
            if (v.Image == Device.ErrorCheckerboardImage.Image)
            {
                continue;
            }
            Device.DestroyImage(v);
        }

        VkDevice dv = Device.Device;
        for (auto &sampler : Samplers)
        {
            vkDestroySampler(dv, sampler, nullptr);
        }

        auto materialBuffer = MaterialDataBuffer;

        DescriptorPool.DestroyPools(dv);

        Device.DestroyBuffer(materialBuffer);
    }
}