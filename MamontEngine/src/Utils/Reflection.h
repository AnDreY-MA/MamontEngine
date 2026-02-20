#pragma once

#include <entt/entt.hpp>
#include "MetaInspector.h"
#include "Utils/MetaReflection.h"

// ---------------------------------------------------------

namespace MetaContext
{
    static entt::locator<entt::meta_ctx>::node_type GetMetaContext()
    {
        //[[maybe_unused]] auto l = entt::locator<entt::meta_ctx>::value_or();

        static auto nodeType = entt::locator<entt::meta_ctx>::handle();

        return nodeType;
    }
};

template <typename T>
static inline entt::meta_factory<T> ReflectComponent(std::string_view type, std::string_view name)
{
    //entt::locator<entt::meta_ctx>::reset(MetaContext::GetMetaContext());
    entt::meta_factory<T> factory = entt::meta_factory<T>();

    factory.type(entt::hashed_string{type.data()}, type.data());

    return factory;
}

template <typename T>
static inline entt::meta_factory<T> ReflectObject(std::string_view name)
{
    //entt::locator<entt::meta_ctx>::reset(MetaContext::GetMetaContext());

    auto factory = entt::meta_factory<T>();

    factory.type(entt::hashed_string{name.data()}, name.data());

    return factory;
}

#define FINISH_REFLECT()                                                                                                                                       \
    return -1;                                                                                                                                                 \
    }

#define IMPLEMENT_REFLECT_COMPONENT(TYPE, NAME)                                                                                                                      \
    int TYPE::InitTypeReflect()                                                                                                                                \
    {                                                                                                                                                          \
        [[maybe_unused]] auto meta = ReflectComponent<TYPE>((#TYPE), NAME);

#define IMPLEMENT_REFLECT_OBJECT(TYPE)                                                                                                                         \
    int TYPE::InitTypeReflect()                                                                                                                                \
    {                                                                                                                                                          \
        [[maybe_unused]] auto meta = ReflectObject<TYPE>((#TYPE));                                                                                             \
        meta.template func<&MetaInspectors::Inspect<TYPE>>(f_Inspect);

#define META_TYPE(Type, ...)                                                                                                                                   \
    {                                                                                                                                                          \
        [[maybe_unused]] entt::meta_factory<Type> meta = TypeInspect<Type>();                                                                                  \
        __VA_ARGS__                                                                                                                                            \
    }


#define IMPLEMENT_META_INIT(TYPE)                                                                                                                              \
    int TYPE_REFLECTS::TYPE_##TYPE::InitTypeReflect()                                                                                                          \
    {                                                                                                                                                          \
        //entt::locator<entt::meta_ctx>::reset(MetaContext::GetMetaContext());
