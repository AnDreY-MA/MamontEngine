#pragma once 


namespace MamontEngine
{
    class Renderer 
	{
    public:
		enum class API
		{
			None = 0, Vulkan
		};

		~Renderer() = default;

		virtual void Init() = 0;
		
		static API GetAPI()
        {
            return s_API;
        }
		static std::unique_ptr<Renderer> CreateRenderer();

	private:
       static API s_API;
	};
}