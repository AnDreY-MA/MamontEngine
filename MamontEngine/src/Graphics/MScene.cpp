#include "Graphics/MScene.h"
#include "Core/Engine.h"
#include "Graphics/MScene.h"


namespace MamontEngine
{
    MScene::MScene(VkContextDevice &inDevice) : Device(inDevice)
    {
    
    }

    MScene::~MScene()
    {
        ClearAll();
    }

    void MScene::Draw(const glm::mat4 &topMatrix, DrawContext &ctx)
    {
        for (auto &n : TopNodes)
        {
            n->Draw(topMatrix, ctx);
        }
    }

    void MScene::ClearAll()
    {
        for (auto &[k, v] : Meshes)
        {
            Device.DestroyBuffer(v->MeshBuffers.IndexBuffer);
            Device.DestroyBuffer(v->MeshBuffers.VertexBuffer);
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

        auto materialBuffer    = MaterialDataBuffer;

        DescriptorPool.DestroyPools(dv);

        Device.DestroyBuffer(materialBuffer);
    }

} // namespace MamontEngine
