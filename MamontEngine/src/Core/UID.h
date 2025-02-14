#pragma once

namespace MamontEngine
{
	class UID
	{
    public:
        UID();
        UID(uint64_t inId);

		UID(const UID &) = default;

		operator uint64_t() const
        {
            return m_Id;
        }

	private:
        uint64_t m_Id;
	};
}

namespace std
{
    template <typename T>
    struct hash;

    template<>
    struct hash<MamontEngine::UID>
    {
        std::size_t operator()(const MamontEngine::UID& inID) const
        {
            return (uint64_t)inID;
        }
    };
}