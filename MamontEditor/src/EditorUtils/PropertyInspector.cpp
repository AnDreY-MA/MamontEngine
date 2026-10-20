#include "PropertyInspector.h"
#include "Utils/MetaInspector.h"
#include "Graphics/Resources/Models/Model.h"
#include "Core/Engine.h"
#include "EditorUtils/EditorUtils.h"

namespace MamontEditor
{
    const std::string RootDirectories = PROJECT_ROOT_DIR;
    const std::string AssetsPath      = RootDirectories + "/MamontEngine/assets";

    void InspectProperty(entt::meta_any& property, entt::meta_data& propertyMeta, const entt::id_type propertyID)
    {
        using namespace MamontEngine;

        const auto propertyType = propertyMeta.type();

        const std::string label = propertyMeta.name() ? std::string{propertyMeta.name()} : std::string{propertyMeta.type().info().name()};

        entt::meta_func &&propertyInspectFunction = propertyType.func(f_Inspect);

        ImGui::PushID((label + "prop").c_str());

        ImGui::Indent(10);

        InspectType(label, property, propertyMeta);

        if (propertyType.is_pointer_like())
        {
            if (entt::meta_any ref = *property; ref && ref.allow_cast<Asset>())
            {
                auto asset = ref.try_cast<Asset>();

                const std::string selectedString = asset != nullptr ? asset->GetPathFile() : "empty";
                if (ImGui::BeginCombo("##File", selectedString.c_str()))
                {
                    constexpr auto                 extensionFile = ".glb";
                    const std::vector<std::string> files         = MamontEditor::ScanFolder(AssetsPath, extensionFile);
                    for (const auto &file : files)
                    {
                        const std::string fullPath   = AssetsPath + "/" + file;
                        const bool        isSelected = (selectedString == fullPath);

                        if (ImGui::Selectable(file.c_str(), isSelected))
                        {
                            asset->Load(fullPath);
                        }

                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                if (asset)
                {
                    if (auto model = static_cast<MeshModel *>(asset); model)
                    {
                        ImGui::Text("Nodes: %i", model->GetSizeNodes());
                        ImGui::Text("Meshes: %i", model->GetSizeMeshes());

                        if (ImGui::TreeNodeEx("Materials"))
                        {
                            for (size_t i{0}; i < model->GetSizeMaterials(); i++)
                            {
                                auto material = model->GetMaterial(i);
                                if (!material)
                                    continue;

                                ImGui::PushID(static_cast<int>(i));

                                if (ImGui::TreeNode(material->Name.c_str()))
                                {
                                    if (ImGui::ColorEdit4("Base Color", &material->Constants.ColorFactors[0]))
                                        material->IsDirty = true;
                                    if (ImGui::DragFloat("Metallic", &material->Constants.MetalicFactor, 0.01f, 0.f, 1.f))
                                    {
                                        material->IsDirty = true;
                                    }
                                    if (ImGui::DragFloat("Roughness", &material->Constants.RoughFactor, 0.01f, 0.f, 1.f))
                                    {
                                        material->IsDirty = true;
                                    }

                                    ImGui::TreePop();
                                }

                                ImGui::PopID();
                            }

                            ImGui::TreePop();
                        }
                    }
                }
            }
        }

        ImGui::Unindent(10);
        ImGui::Separator();

        ImGui::PopID();
    }

    void InspectType(const std::string& name, entt::meta_any& value, entt::meta_data& meta)
    {
        if (meta.type().is_pointer_like())
        {
            if (entt::meta_any ref = *value; ref && !ref.allow_cast<MamontEngine::Asset>())
            {
                const std::string childId = name + "Field";
                ImGui::BeginChild(childId.c_str());
                ImGui::Indent(10);
                for (auto [id, metaData] : ref.type().data())
                {
                    auto        instance = metaData.get(ref);
                    std::string metaName = std::string(metaData.name());
                    InspectType(metaName, instance, metaData);
                    metaData.set(ref, instance);
                }
                ImGui::Unindent(10);
                ImGui::Separator();
                ImGui::EndChild();
            }

            return;
        }

        if (!value.type().func(f_Inspect))
        {
            ImGui::Text("No meta inspect function detected\n");
            return;
        }

        auto propertyInspected = value.type().func(f_Inspect).invoke({}, name.c_str(), entt::forward_as_meta(value), meta);

        if (!propertyInspected)
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to render type");
    }

    void InspectComponent(entt::entity owner, entt::meta_any& data)
    {
        using namespace MamontEngine;
        if (!data)
        {
            Log::Warn("InspectComponent: invalid data");
            return;
        }

        const auto dataType = data.type();

        constexpr ImGuiTreeNodeFlags TREE_FLAGS = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap |
                                                  ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;

        const bool menuOpen = ImGui::CollapsingHeader(dataType.name(), TREE_FLAGS);
        if (!menuOpen)
            return;

        const std::string id = std::to_string(dataType.id()).append("Component");
        ImGui::PushID(id.c_str());
        ImGui::Indent(10.f);

        for (auto &&[proprtyId, propertyMetaData] : data.type().data())
        {
            auto instance = propertyMetaData.get(data);

            InspectProperty(instance, propertyMetaData, proprtyId);
            propertyMetaData.set(data, instance);
        }

        ImGui::Unindent(10);
        ImGui::PopID();
    }
}