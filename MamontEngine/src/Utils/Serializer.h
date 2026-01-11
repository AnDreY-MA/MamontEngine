#pragma once

namespace MamontEngine
{
    class Scene;

namespace Serializer
{
    bool SaveScene(Scene *inScene, std::string_view inFile);
    
    bool LoadScene(Scene *inScene, std::string_view inFile);
}
}