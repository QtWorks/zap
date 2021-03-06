//
// Created by Darren Otgaar on 2016/07/10.
//

#ifndef ZAP_ENUMFIELD_HPP
#define ZAP_ENUMFIELD_HPP

#include "core.hpp"

// The enumfield is a simple wrapper around an enum class consisting of powers of two used to get/set state

namespace zap { namespace core {
    template <typename FieldT, typename EnumT>
    struct enumfield {
        enumfield(FieldT f=FieldT(0)) : field(f) { }
        enumfield(const EnumT& e) : field((FieldT)e) { }
        // TODO: Rewrite as a single OR or AND and store
        template <typename... Enums> void set(const Enums&... enums) { expand([=](const EnumT& e) { set(e); }, enums...); }
        template <typename... Enums> void clear(const Enums&... enums) { expand([=](const EnumT& e) { clear(e); }, enums...); }
        void set(const EnumT& v) { field |= (FieldT)v; }
        void clear(const EnumT& v) { field &= ~(FieldT)v; }
        bool is_set(const EnumT& v) const { return (field & (FieldT)v) != 0; }

        FieldT field;
    };
}}

#endif //ZAP_ENUMFIELD_HPP
