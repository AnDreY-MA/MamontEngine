#include "Graphics/Pass/RenderPass.h"

namespace MamontEngine
{
    class Camera;
    class PipelineData;

    class PointLightPass : public RenderPass
    {
    public:
        PointLightPass(std::array<Texture, MAX_POINT_LIGHT>& inShadowCubeImages);
        ~PointLightPass();

        virtual void Render(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, const DrawContext &inDrawContext, const glm::mat4 &viewproj) override;

        void CreatePipeline(std::span<const VkDescriptorSetLayout> inDescriptorLaouts, VkFormat inImageFormat);

        void UpdateLights(const Camera* inCamera, const LightData& inLightData);


    private:
        std::array<glm::mat4, MAX_POINT_LIGHT * SHADOW_FACE_NUM>   m_ShadowFaceCubes{};

        std::array<Texture, MAX_POINT_LIGHT>& shadowCubeImages;

        AllocatedBuffer m_Buffer;
        std::array < AllocatedBuffer, FRAME_OVERLAP> m_StagingBuffers;

        uint32_t m_LightCount{0};
    };
} // namespace MamontEngine
