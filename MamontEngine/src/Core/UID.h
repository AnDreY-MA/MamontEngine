#pragma once

namespace MamontEngine
{
	class UID
	{
    public:
        UID();
        UID(uint64_t inId);

		UID(const UID &) = default;

		operator uint64_t()
        {
            return m_Id;
        }
        operator const uint64_t() const
        {
            return m_Id;
        }

	private:
        uint64_t m_Id;

        template <typename Archive>
        friend void save(Archive &archive, const UID &ID);

        template <typename Archive>
        friend void load(Archive &archive, UID &ID);
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