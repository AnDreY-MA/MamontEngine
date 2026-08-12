#pragma once

#define REFLECT()                                                                                                                                              \
private:                                                                                                                                                       \
    static int        InitTypeReflect();                                                                                                                       \
    static inline int id = InitTypeReflect();

#define META_INIT(TYPE)                                                                                                                                        \
    namespace TYPE_REFLECTS::TYPE_##TYPE                                                                                                                       \
    {                                                                                                                                                          \
        int               InitTypeReflect();                                                                                                                   \
        static inline int id = InitTypeReflect();                                                                                                              \
    }

#define REFLECT_ENUM(TYPE)\
    namespace ENUM_REFLECTS::TYPE_##TYPE \
    {                                                                                                                                                          \
        int InitTypeReflect();\
        static int id = InitTypeReflect();\
    }   