#include "Utils/Serializer.h"
#include "ECS/Components/IDComponent.h"
#include "ECS/Components/TagComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/DirectionLightComponent.h"
#include "Graphics/Resources/Models/Model.h"
#include "ECS/Scene.h"
#include "ECS/Entity.h"
#include <fstream>
#include "Core/Engine.h"
#include "Core/Log.h"
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/polymorphic.hpp>

namespace MamontEngine
{
namespace Serializer
{
	bool SaveScene(Scene *inScene, std::string_view inFile)
	{
        auto &registry = inScene->GetRegistry();

		std::fstream file{inFile.data(), file.binary | file.trunc | file.out};
		if (!file.is_open())
		{
            Log::Error("Cannot open file: {}", inFile.data());
            return false;
		}
        
		cereal::BinaryOutputArchive serializer{file};

		//serializer(*inScene);
        //entt::snapshot{registry}.get<entt::entity>(serializer).get<IDComponent>(serializer).get<TagComponent>(serializer);

		
		file.close();

		return true;
	}

	bool LoadScene(Scene *inScene, std::string_view inFile)
	{

		return false;
	}

} // namespace Serializer

}