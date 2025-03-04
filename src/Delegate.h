#pragma once
/*
Delegate.h - An efficient interchangeable C function ptr and C++ std::function delegate
Copyright (c) 2019 Dirk O. Kaar. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __Delegate_h
#define __Delegate_h

#if defined(ESP8266)
#include <c_types.h>
#elif defined(ESP32)
#include <esp_attr.h>
#else
#define IRAM_ATTR
#endif

#if defined(__GNUC__)
#undef ALWAYS_INLINE_ATTR
#define ALWAYS_INLINE_ATTR __attribute__((always_inline))
#else
#define ALWAYS_INLINE_ATTR
#endif

#if !defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
#include <functional>
#include <cstddef>
#else
#include "ghostl.h"
#endif

namespace
{

    template<typename R, typename... P>
    ALWAYS_INLINE_ATTR inline R IRAM_ATTR vPtrToFunPtrExec(void* fn, P... args)
    {
        using target_type = R(P...);
        return reinterpret_cast<target_type*>(fn)(std::forward<P...>(args...));
    }

}

namespace delegate
{
    namespace detail
    {

#if !defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
        template<typename AA, typename R, typename... P>
        class DelegatePImpl {
        public:
            using target_type = R(P...);
        protected:
            using FunPtr = target_type*;
            using FunAPtr = R(*)(AA, P...);
            using FunVPPtr = R(*)(void*, P...);
            using FunctionType = std::function<target_type>;
        public:
            DelegatePImpl()
            {
                kind = FP;
                fn = nullptr;
            }

            DelegatePImpl(std::nullptr_t)
            {
                kind = FP;
                fn = nullptr;
            }

            ~DelegatePImpl()
            {
                if (FUNC == kind)
                    functional.~FunctionType();
                else if (FPA == kind)
                    obj.~AA();
            }

            DelegatePImpl(const DelegatePImpl& del)
            {
                kind = del.kind;
                if (FUNC == del.kind)
                {
                    new (&functional) FunctionType(del.functional);
                }
                else if (FPA == del.kind)
                {
                    fnA = del.fnA;
                    new (&obj) AA(del.obj);
                }
                else
                {
                    fn = del.fn;
                }
            }

            DelegatePImpl(DelegatePImpl&& del)
            {
                kind = del.kind;
                if (FUNC == del.kind)
                {
                    new (&functional) FunctionType(std::move(del.functional));
                }
                else if (FPA == del.kind)
                {
                    fnA = del.fnA;
                    new (&obj) AA(std::move(del.obj));
                }
                else
                {
                    fn = del.fn;
                }
            }

            DelegatePImpl(FunAPtr fnA, const AA& obj)
            {
                kind = FPA;
                DelegatePImpl::fnA = fnA;
                new (&this->obj) AA(obj);
            }

            DelegatePImpl(FunAPtr fnA, AA&& obj)
            {
                kind = FPA;
                DelegatePImpl::fnA = fnA;
                new (&this->obj) AA(std::move(obj));
            }

            DelegatePImpl(FunPtr fn)
            {
                kind = FP;
                DelegatePImpl::fn = fn;
            }

            template<typename F> DelegatePImpl(F functional)
            {
                kind = FUNC;
                new (&this->functional) FunctionType(std::forward<F>(functional));
            }

            DelegatePImpl& operator=(const DelegatePImpl& del)
            {
                if (this == &del) return *this;
                if (kind != del.kind)
                {
                    if (FUNC == kind)
                    {
                        functional.~FunctionType();
                    }
                    else if (FPA == kind)
                    {
                        obj.~AA();
                    }
                    if (FUNC == del.kind)
                    {
                        new (&this->functional) FunctionType();
                    }
                    else if (FPA == del.kind)
                    {
                        new (&obj) AA;
                    }
                    kind = del.kind;
                }
                if (FUNC == del.kind)
                {
                    functional = del.functional;
                }
                else if (FPA == del.kind)
                {
                    fnA = del.fnA;
                    obj = del.obj;
                }
                else
                {
                    fn = del.fn;
                }
                return *this;
            }

            DelegatePImpl& operator=(DelegatePImpl&& del)
            {
                if (this == &del) return *this;
                if (kind != del.kind)
                {
                    if (FUNC == kind)
                    {
                        functional.~FunctionType();
                    }
                    else if (FPA == kind)
                    {
                        obj.~AA();
                    }
                    if (FUNC == del.kind)
                    {
                        new (&this->functional) FunctionType();
                    }
                    else if (FPA == del.kind)
                    {
                        new (&obj) AA;
                    }
                    kind = del.kind;
                }
                if (FUNC == del.kind)
                {
                    functional = std::move(del.functional);
                }
                else if (FPA == del.kind)
                {
                    fnA = del.fnA;
                    obj = std::move(del.obj);
                }
                else
                {
                    fn = del.fn;
                }
                return *this;
            }

            DelegatePImpl& operator=(FunPtr fn)
            {
                if (FUNC == kind)
                {
                    functional.~FunctionType();
                }
                else if (FPA == kind)
                {
                    obj.~AA();
                }
                kind = FP;
                this->fn = fn;
                return *this;
            }

            DelegatePImpl& IRAM_ATTR operator=(std::nullptr_t)
            {
                if (FUNC == kind)
                {
                    functional.~FunctionType();
                }
                else if (FPA == kind)
                {
                    obj.~AA();
                }
                kind = FP;
                fn = nullptr;
                return *this;
            }

            IRAM_ATTR operator bool() const
            {
                if (FP == kind)
                {
                    return fn;
                }
                else if (FPA == kind)
                {
                    return fnA;
                }
                else
                {
                    return functional ? true : false;
                }
            }

            static inline R IRAM_ATTR vPtrToFunAPtrExec(void* self, P... args) ALWAYS_INLINE_ATTR
            {
                return static_cast<DelegatePImpl*>(self)->fnA(
                    static_cast<DelegatePImpl*>(self)->obj,
                    std::forward<P...>(args...));
            };

            operator FunVPPtr() const
            {
                if (FP == kind)
                {
                    return vPtrToFunPtrExec<R, P...>;
                }
                else if (FPA == kind)
                {
                    return vPtrToFunAPtrExec;
                }
                else
                {
                    return [](void* self, P... args) -> R
                    {
                        return static_cast<DelegatePImpl*>(self)->functional(std::forward<P...>(args...));
                    };
                }
            }

            void* arg() const
            {
                if (FP == kind)
                {
                    return reinterpret_cast<void*>(fn);
                }
                else
                {
                    return const_cast<DelegatePImpl*>(this);
                }
            }

            operator FunctionType() const
            {
                if (FP == kind)
                {
                    return fn;
                }
                else if (FPA == kind)
                {
                    return [this](P... args) { return fnA(obj, std::forward<P...>(args...)); };
                }
                else
                {
                    return functional;
                }
            }

            /// Calling is safe without checking for nullptr.
            /// If non-void, returns the default value.
            /// In ISR context, where faults and exceptions must not
            /// occurs, this saves the extra check for nullptr,
            /// and allows the compiler to optimize out checks
            /// in std::function which may not be ISR-safe or
            /// cause linker errors, like l32r relocation errors
            /// on the Xtensa ISA.
            R IRAM_ATTR operator()(P... args) const
            {
                if (FP == kind)
                {
                    if (fn) return fn(std::forward<P...>(args...));
                }
                else if (FPA == kind)
                {
                    if (fnA) return fnA(obj, std::forward<P...>(args...));
                }
                else
                {
                    if (functional) return functional(std::forward<P...>(args...));
                }
                return R();
            }

        protected:
            union {
                FunctionType functional;
                FunPtr fn;
                struct {
                    FunAPtr fnA;
                    AA obj;
                };
            };
            enum { FUNC, FP, FPA } kind;
        };
#else
        template<typename AA, typename R, typename... P>
        class DelegatePImpl {
        public:
            using target_type = R(P...);
        protected:
            using FunPtr = target_type*;
            using FunAPtr = R(*)(AA, P...);
            using FunVPPtr = R(*)(void*, P...);
        public:
            DelegatePImpl()
            {
                kind = FP;
                fn = nullptr;
            }

            DelegatePImpl(std::nullptr_t)
            {
                kind = FP;
                fn = nullptr;
            }

            DelegatePImpl(const DelegatePImpl& del)
            {
                kind = del.kind;
                if (FPA == del.kind)
                {
                    fnA = del.fnA;
                    obj = del.obj;
                }
                else
                {
                    fn = del.fn;
                }
            }

            DelegatePImpl(DelegatePImpl&& del)
            {
                kind = del.kind;
                if (FPA == del.kind)
                {
                    fnA = del.fnA;
                    obj = std::move(del.obj);
                }
                else
                {
                    fn = del.fn;
                }
            }

            DelegatePImpl(FunAPtr fnA, const AA& obj)
            {
                kind = FPA;
                DelegatePImpl::fnA = fnA;
                this->obj = obj;
            }

            DelegatePImpl(FunAPtr fnA, AA&& obj)
            {
                kind = FPA;
                DelegatePImpl::fnA = fnA;
                this->obj = std::move(obj);
            }

            DelegatePImpl(FunPtr fn)
            {
                kind = FP;
                DelegatePImpl::fn = fn;
            }

            template<typename F> DelegatePImpl(F functional)
            {
                kind = FP;
                fn = std::forward<F>(functional);
            }

            DelegatePImpl& operator=(const DelegatePImpl& del)
            {
                if (this == &del) return *this;
                if (kind != del.kind)
                {
                    if (FPA == kind)
                    {
                        obj = {};
                    }
                    kind = del.kind;
                }
                if (FPA == del.kind)
                {
                    fnA = del.fnA;
                    obj = del.obj;
                }
                else
                {
                    fn = del.fn;
                }
                return *this;
            }

            DelegatePImpl& operator=(DelegatePImpl&& del)
            {
                if (this == &del) return *this;
                if (kind != del.kind)
                {
                    if (FPA == kind)
                    {
                        obj = {};
                    }
                    kind = del.kind;
                }
                if (FPA == del.kind)
                {
                    fnA = del.fnA;
                    obj = std::move(del.obj);
                }
                else
                {
                    fn = del.fn;
                }
                return *this;
            }

            DelegatePImpl& operator=(FunPtr fn)
            {
                if (FPA == kind)
                {
                    obj = {};
                }
                kind = FP;
                this->fn = fn;
                return *this;
            }

            DelegatePImpl& IRAM_ATTR operator=(std::nullptr_t)
            {
                if (FPA == kind)
                {
                    obj = {};
                }
                kind = FP;
                fn = nullptr;
                return *this;
            }

            IRAM_ATTR operator bool() const
            {
                if (FP == kind)
                {
                    return fn;
                }
                else
                {
                    return fnA;
                }
            }

            static inline R IRAM_ATTR vPtrToFunAPtrExec(void* self, P... args) ALWAYS_INLINE_ATTR
            {
                return static_cast<DelegatePImpl*>(self)->fnA(
                    static_cast<DelegatePImpl*>(self)->obj,
                    std::forward<P...>(args...));
            };

            operator FunVPPtr() const
            {
                if (FP == kind)
                {
                    return vPtrToFunPtrExec<R, P...>;
                }
                else
                {
                    return vPtrToFunAPtrExec;
                }
            }

            void* arg() const
            {
                if (FP == kind)
                {
                    return reinterpret_cast<void*>(fn);
                }
                else
                {
                    return const_cast<DelegatePImpl*>(this);
                }
            }

            /// Calling is safe without checking for nullptr.
            /// If non-void, returns the default value.
            /// In ISR context, where faults and exceptions must not
            /// occurs, this saves the extra check for nullptr,
            /// and allows the compiler to optimize out checks
            /// in std::function which may not be ISR-safe or
            /// cause linker errors, like l32r relocation errors
            /// on the Xtensa ISA.
            R IRAM_ATTR operator()(P... args) const
            {
                if (FP == kind)
                {
                    if (fn) return fn(std::forward<P...>(args...));
                }
                else
                {
                    if (fnA) return fnA(obj, std::forward<P...>(args...));
                }
                return R();
            }

        protected:
            union {
                FunPtr fn;
                FunAPtr fnA;
            };
            AA obj;
            enum { FP, FPA } kind;
        };
#endif

#if !defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
        template<typename R, typename... P>
        class DelegatePImpl<void, R, P...> {
        public:
            using target_type = R(P...);
        protected:
            using FunPtr = target_type*;
            using FunctionType = std::function<target_type>;
            using FunVPPtr = R(*)(void*, P...);
        public:
            DelegatePImpl()
            {
                kind = FP;
                fn = nullptr;
            }

            DelegatePImpl(std::nullptr_t)
            {
                kind = FP;
                fn = nullptr;
            }

            ~DelegatePImpl()
            {
                if (FUNC == kind)
                    functional.~FunctionType();
            }

            DelegatePImpl(const DelegatePImpl& del)
            {
                kind = del.kind;
                if (FUNC == del.kind)
                {
                    new (&functional) FunctionType(del.functional);
                }
                else
                {
                    fn = del.fn;
                }
            }

            DelegatePImpl(DelegatePImpl&& del)
            {
                kind = del.kind;
                if (FUNC == del.kind)
                {
                    new (&functional) FunctionType(std::move(del.functional));
                }
                else
                {
                    fn = del.fn;
                }
            }

            DelegatePImpl(FunPtr fn)
            {
                kind = FP;
                DelegatePImpl::fn = fn;
            }

            template<typename F> DelegatePImpl(F functional)
            {
                kind = FUNC;
                new (&this->functional) FunctionType(std::forward<F>(functional));
            }

            DelegatePImpl& operator=(const DelegatePImpl& del)
            {
                if (this == &del) return *this;
                if (FUNC == kind && FUNC != del.kind)
                {
                    functional.~FunctionType();
                }
                else if (FUNC != kind && FUNC == del.kind)
                {
                    new (&this->functional) FunctionType();
                }
                kind = del.kind;
                if (FUNC == del.kind)
                {
                    functional = del.functional;
                }
                else
                {
                    fn = del.fn;
                }
                return *this;
            }

            DelegatePImpl& operator=(DelegatePImpl&& del)
            {
                if (this == &del) return *this;
                if (FUNC == kind && FUNC != del.kind)
                {
                    functional.~FunctionType();
                }
                else if (FUNC != kind && FUNC == del.kind)
                {
                    new (&this->functional) FunctionType();
                }
                kind = del.kind;
                if (FUNC == del.kind)
                {
                    functional = std::move(del.functional);
                }
                else
                {
                    fn = del.fn;
                }
                return *this;
            }

            DelegatePImpl& operator=(FunPtr fn)
            {
                if (FUNC == kind)
                {
                    functional.~FunctionType();
                    kind = FP;
                }
                DelegatePImpl::fn = fn;
                return *this;
            }

            DelegatePImpl& IRAM_ATTR operator=(std::nullptr_t)
            {
                if (FUNC == kind)
                {
                    functional.~FunctionType();
                }
                kind = FP;
                fn = nullptr;
                return *this;
            }

            IRAM_ATTR operator bool() const
            {
                if (FP == kind)
                {
                    return fn;
                }
                else
                {
                    return functional ? true : false;
                }
            }

            operator FunVPPtr() const
            {
                if (FP == kind)
                {
                    return vPtrToFunPtrExec<R, P...>;
                }
                else
                {
                    return [](void* self, P... args) -> R
                    {
                        return static_cast<DelegatePImpl*>(self)->functional(std::forward<P...>(args...));
                    };
                }
            }

            void* arg() const
            {
                if (FP == kind)
                {
                    return reinterpret_cast<void*>(fn);
                }
                else
                {
                    return const_cast<DelegatePImpl*>(this);
                }
            }

            operator FunctionType() const
            {
                if (FP == kind)
                {
                    return fn;
                }
                else
                {
                    return functional;
                }
            }

            /// Calling is safe without checking for nullptr.
            /// If non-void, returns the default value.
            /// In ISR context, where faults and exceptions must not
            /// occurs, this saves the extra check for nullptr,
            /// and allows the compiler to optimize out checks
            /// in std::function which may not be ISR-safe or
            /// cause linker errors, like l32r relocation errors
            /// on the Xtensa ISA.
            R IRAM_ATTR operator()(P... args) const
            {
                if (FP == kind)
                {
                    if (fn) return fn(std::forward<P...>(args...));
                }
                else
                {
                    if (functional) return functional(std::forward<P...>(args...));
                }
                return R();
            }

        protected:
            union {
                FunctionType functional;
                FunPtr fn;
            };
            enum { FUNC, FP } kind;
        };
#else
        template<typename R, typename... P>
        class DelegatePImpl<void, R, P...> {
        public:
            using target_type = R(P...);
        protected:
            using FunPtr = target_type*;
            using FunVPPtr = R(*)(void*, P...);
        public:
            DelegatePImpl()
            {
                fn = nullptr;
            }

            DelegatePImpl(std::nullptr_t)
            {
                fn = nullptr;
            }

            DelegatePImpl(const DelegatePImpl& del)
            {
                fn = del.fn;
            }

            DelegatePImpl(DelegatePImpl&& del)
            {
                fn = std::move(del.fn);
            }

            DelegatePImpl(FunPtr fn)
            {
                DelegatePImpl::fn = fn;
            }

            template<typename F> DelegatePImpl(F fn)
            {
                DelegatePImpl::fn = std::forward<F>(fn);
            }

            DelegatePImpl& operator=(const DelegatePImpl& del)
            {
                if (this == &del) return *this;
                fn = del.fn;
                return *this;
            }

            DelegatePImpl& operator=(DelegatePImpl&& del)
            {
                if (this == &del) return *this;
                fn = std::move(del.fn);
                return *this;
            }

            DelegatePImpl& operator=(FunPtr fn)
            {
                DelegatePImpl::fn = fn;
                return *this;
            }

            inline DelegatePImpl& IRAM_ATTR operator=(std::nullptr_t) ALWAYS_INLINE_ATTR
            {
                fn = nullptr;
                return *this;
            }

            inline IRAM_ATTR operator bool() const ALWAYS_INLINE_ATTR
            {
                return fn;
            }

            operator FunVPPtr() const
            {
                return vPtrToFunPtrExec<R, P...>;
            }

            void* arg() const
            {
                return reinterpret_cast<void*>(fn);
            }

            /// Calling is safe without checking for nullptr.
            /// If non-void, returns the default value.
            /// In ISR context, where faults and exceptions must not
            /// occurs, this saves the extra check for nullptr,
            /// and allows the compiler to optimize out checks
            /// in std::function which may not be ISR-safe or
            /// cause linker errors, like l32r relocation errors
            /// on the Xtensa ISA.
            inline R IRAM_ATTR operator()(P... args) const ALWAYS_INLINE_ATTR
            {
                if (fn) return fn(std::forward<P...>(args...));
                return R();
            }

        protected:
            FunPtr fn;
        };
#endif

#if !defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
        template<typename AA, typename R>
        class DelegateImpl {
        public:
            using target_type = R();
        protected:
            using FunPtr = target_type*;
            using FunAPtr = R(*)(AA);
            using FunctionType = std::function<target_type>;
            using FunVPPtr = R(*)(void*);
        public:
            DelegateImpl()
            {
                kind = FP;
                fn = nullptr;
            }

            DelegateImpl(std::nullptr_t)
            {
                kind = FP;
                fn = nullptr;
            }

            ~DelegateImpl()
            {
                if (FUNC == kind)
                    functional.~FunctionType();
                else if (FPA == kind)
                    obj.~AA();
            }

            DelegateImpl(const DelegateImpl& del)
            {
                kind = del.kind;
                if (FUNC == del.kind)
                {
                    new (&functional) FunctionType(del.functional);
                }
                else if (FPA == del.kind)
                {
                    fnA = del.fnA;
                    new (&obj) AA(del.obj);
                }
                else
                {
                    fn = del.fn;
                }
            }

            DelegateImpl(DelegateImpl&& del)
            {
                kind = del.kind;
                if (FUNC == del.kind)
                {
                    new (&functional) FunctionType(std::move(del.functional));
                }
                else if (FPA == del.kind)
                {
                    fnA = del.fnA;
                    new (&obj) AA(std::move(del.obj));
                }
                else
                {
                    fn = del.fn;
                }
            }

            DelegateImpl(FunAPtr fnA, const AA& obj)
            {
                kind = FPA;
                DelegateImpl::fnA = fnA;
                new (&this->obj) AA(obj);
            }

            DelegateImpl(FunAPtr fnA, AA&& obj)
            {
                kind = FPA;
                DelegateImpl::fnA = fnA;
                new (&this->obj) AA(std::move(obj));
            }

            DelegateImpl(FunPtr fn)
            {
                kind = FP;
                DelegateImpl::fn = fn;
            }

            template<typename F> DelegateImpl(F functional)
            {
                kind = FUNC;
                new (&this->functional) FunctionType(std::forward<F>(functional));
            }

            DelegateImpl& operator=(const DelegateImpl& del)
            {
                if (this == &del) return *this;
                if (kind != del.kind)
                {
                    if (FUNC == kind)
                    {
                        functional.~FunctionType();
                    }
                    else if (FPA == kind)
                    {
                        obj.~AA();
                    }
                    if (FUNC == del.kind)
                    {
                        new (&this->functional) FunctionType();
                    }
                    else if (FPA == del.kind)
                    {
                        new (&obj) AA;
                    }
                    kind = del.kind;
                }
                if (FUNC == del.kind)
                {
                    functional = del.functional;
                }
                else if (FPA == del.kind)
                {
                    fnA = del.fnA;
                    obj = del.obj;
                }
                else
                {
                    fn = del.fn;
                }
                return *this;
            }

            DelegateImpl& operator=(DelegateImpl&& del)
            {
                if (this == &del) return *this;
                if (kind != del.kind)
                {
                    if (FUNC == kind)
                    {
                        functional.~FunctionType();
                    }
                    else if (FPA == kind)
                    {
                        obj.~AA();
                    }
                    if (FUNC == del.kind)
                    {
                        new (&this->functional) FunctionType();
                    }
                    else if (FPA == del.kind)
                    {
                        new (&obj) AA;
                    }
                    kind = del.kind;
                }
                if (FUNC == del.kind)
                {
                    functional = std::move(del.functional);
                }
                else if (FPA == del.kind)
                {
                    fnA = del.fnA;
                    obj = std::move(del.obj);
                }
                else
                {
                    fn = del.fn;
                }
                return *this;
            }

            DelegateImpl& operator=(FunPtr fn)
            {
                if (FUNC == kind)
                {
                    functional.~FunctionType();
                }
                else if (FPA == kind)
                {
                    obj.~AA();
                }
                kind = FP;
                this->fn = fn;
                return *this;
            }

            DelegateImpl& IRAM_ATTR operator=(std::nullptr_t)
            {
                if (FUNC == kind)
                {
                    functional.~FunctionType();
                }
                else if (FPA == kind)
                {
                    obj.~AA();
                }
                kind = FP;
                fn = nullptr;
                return *this;
            }

            IRAM_ATTR operator bool() const
            {
                if (FP == kind)
                {
                    return fn;
                }
                else if (FPA == kind)
                {
                    return fnA;
                }
                else
                {
                    return functional ? true : false;
                }
            }

            static inline R IRAM_ATTR vPtrToFunAPtrExec(void* self) ALWAYS_INLINE_ATTR
            {
                return static_cast<DelegateImpl*>(self)->fnA(
                    static_cast<DelegateImpl*>(self)->obj);
            };

            operator FunVPPtr() const
            {
                if (FP == kind)
                {
                    return reinterpret_cast<FunVPPtr>(fn);
                }
                else if (FPA == kind)
                {
                    return vPtrToFunAPtrExec;
                }
                else
                {
                    return [](void* self) -> R
                    {
                        return static_cast<DelegateImpl*>(self)->functional();
                    };
                }
            }

            void* arg() const
            {
                if (FP == kind)
                {
                    return nullptr;
                }
                else
                {
                    return const_cast<DelegateImpl*>(this);
                }
            }

            operator FunctionType() const
            {
                if (FP == kind)
                {
                    return fn;
                }
                else if (FPA == kind)
                {
                    return [this]() { return fnA(obj); };
                }
                else
                {
                    return functional;
                }
            }

            /// Calling is safe without checking for nullptr.
            /// If non-void, returns the default value.
            /// In ISR context, where faults and exceptions must not
            /// occurs, this saves the extra check for nullptr,
            /// and allows the compiler to optimize out checks
            /// in std::function which may not be ISR-safe or
            /// cause linker errors, like l32r relocation errors
            /// on the Xtensa ISA.
            R IRAM_ATTR operator()() const
            {
                if (FP == kind)
                {
                    if (fn) return fn();
                }
                else if (FPA == kind)
                {
                    if (fnA) return fnA(obj);
                }
                else
                {
                    if (functional) return functional();
                }
                return R();
            }

        protected:
            union {
                FunctionType functional;
                FunPtr fn;
                struct {
                    FunAPtr fnA;
                    AA obj;
                };
            };
            enum { FUNC, FP, FPA } kind;
        };
#else
        template<typename AA, typename R>
        class DelegateImpl {
        public:
            using target_type = R();
        protected:
            using FunPtr = target_type*;
            using FunAPtr = R(*)(AA);
            using FunVPPtr = R(*)(void*);
        public:
            DelegateImpl()
            {
                kind = FP;
                fn = nullptr;
            }

            DelegateImpl(std::nullptr_t)
            {
                kind = FP;
                fn = nullptr;
            }

            DelegateImpl(const DelegateImpl& del)
            {
                kind = del.kind;
                if (FPA == del.kind)
                {
                    fnA = del.fnA;
                    obj = del.obj;
                }
                else
                {
                    fn = del.fn;
                }
            }

            DelegateImpl(DelegateImpl&& del)
            {
                kind = del.kind;
                if (FPA == del.kind)
                {
                    fnA = del.fnA;
                    obj = std::move(del.obj);
                }
                else
                {
                    fn = del.fn;
                }
            }

            DelegateImpl(FunAPtr fnA, const AA& obj)
            {
                kind = FPA;
                DelegateImpl::fnA = fnA;
                this->obj = obj;
            }

            DelegateImpl(FunAPtr fnA, AA&& obj)
            {
                kind = FPA;
                DelegateImpl::fnA = fnA;
                this->obj = std::move(obj);
            }

            DelegateImpl(FunPtr fn)
            {
                kind = FP;
                DelegateImpl::fn = fn;
            }

            template<typename F> DelegateImpl(F fn)
            {
                kind = FP;
                DelegateImpl::fn = std::forward<F>(fn);
            }

            DelegateImpl& operator=(const DelegateImpl& del)
            {
                if (this == &del) return *this;
                if (kind != del.kind)
                {
                    if (FPA == kind)
                    {
                        obj = {};
                    }
                    kind = del.kind;
                }
                if (FPA == del.kind)
                {
                    fnA = del.fnA;
                    obj = del.obj;
                }
                else
                {
                    fn = del.fn;
                }
                return *this;
            }

            DelegateImpl& operator=(DelegateImpl&& del)
            {
                if (this == &del) return *this;
                if (kind != del.kind)
                {
                    if (FPA == kind)
                    {
                        obj = {};
                    }
                    kind = del.kind;
                }
                if (FPA == del.kind)
                {
                    fnA = del.fnA;
                    obj = std::move(del.obj);
                }
                else
                {
                    fn = del.fn;
                }
                return *this;
            }

            DelegateImpl& operator=(FunPtr fn)
            {
                if (FPA == kind)
                {
                    obj = {};
                }
                kind = FP;
                this->fn = fn;
                return *this;
            }

            DelegateImpl& IRAM_ATTR operator=(std::nullptr_t)
            {
                if (FPA == kind)
                {
                    obj = {};
                }
                kind = FP;
                fn = nullptr;
                return *this;
            }

            IRAM_ATTR operator bool() const
            {
                if (FP == kind)
                {
                    return fn;
                }
                else
                {
                    return fnA;
                }
            }

            static inline R IRAM_ATTR vPtrToFunAPtrExec(void* self) ALWAYS_INLINE_ATTR
            {
                return static_cast<DelegateImpl*>(self)->fnA(
                    static_cast<DelegateImpl*>(self)->obj);
            };

            operator FunVPPtr() const
            {
                if (FP == kind)
                {
                    return reinterpret_cast<FunVPPtr>(fn);
                }
                else
                {
                    return vPtrToFunAPtrExec;
                }
            }

            void* arg() const
            {
                if (FP == kind)
                {
                    return nullptr;
                }
                else
                {
                    return const_cast<DelegateImpl*>(this);
                }
            }

            /// Calling is safe without checking for nullptr.
            /// If non-void, returns the default value.
            /// In ISR context, where faults and exceptions must not
            /// occurs, this saves the extra check for nullptr,
            /// and allows the compiler to optimize out checks
            /// in std::function which may not be ISR-safe or
            /// cause linker errors, like l32r relocation errors
            /// on the Xtensa ISA.
            R IRAM_ATTR operator()() const
            {
                if (FP == kind)
                {
                    if (fn) return fn();
                }
                else
                {
                    if (fnA) return fnA(obj);
                }
                return R();
            }

        protected:
            union {
                FunPtr fn;
                FunAPtr fnA;
            };
            AA obj;
            enum { FP, FPA } kind;
        };
#endif

#if !defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
        template<typename R>
        class DelegateImpl<void, R> {
        public:
            using target_type = R();
        protected:
            using FunPtr = target_type*;
            using FunctionType = std::function<target_type>;
            using FunVPPtr = R(*)(void*);
        public:
            DelegateImpl()
            {
                kind = FP;
                fn = nullptr;
            }

            DelegateImpl(std::nullptr_t)
            {
                kind = FP;
                fn = nullptr;
            }

            ~DelegateImpl()
            {
                if (FUNC == kind)
                    functional.~FunctionType();
            }

            DelegateImpl(const DelegateImpl& del)
            {
                kind = del.kind;
                if (FUNC == del.kind)
                {
                    new (&functional) FunctionType(del.functional);
                }
                else
                {
                    fn = del.fn;
                }
            }

            DelegateImpl(DelegateImpl&& del)
            {
                kind = del.kind;
                if (FUNC == del.kind)
                {
                    new (&functional) FunctionType(std::move(del.functional));
                }
                else
                {
                    fn = del.fn;
                }
            }

            DelegateImpl(FunPtr fn)
            {
                kind = FP;
                DelegateImpl::fn = fn;
            }

            template<typename F> DelegateImpl(F functional)
            {
                kind = FUNC;
                new (&this->functional) FunctionType(std::forward<F>(functional));
            }

            DelegateImpl& operator=(const DelegateImpl& del)
            {
                if (this == &del) return *this;
                if (FUNC == kind && FUNC != del.kind)
                {
                    functional.~FunctionType();
                }
                else if (FUNC != kind && FUNC == del.kind)
                {
                    new (&this->functional) FunctionType();
                }
                kind = del.kind;
                if (FUNC == del.kind)
                {
                    functional = del.functional;
                }
                else
                {
                    fn = del.fn;
                }
                return *this;
            }

            DelegateImpl& operator=(DelegateImpl&& del)
            {
                if (this == &del) return *this;
                if (FUNC == kind && FUNC != del.kind)
                {
                    functional.~FunctionType();
                }
                else if (FUNC != kind && FUNC == del.kind)
                {
                    new (&this->functional) FunctionType();
                }
                kind = del.kind;
                if (FUNC == del.kind)
                {
                    functional = std::move(del.functional);
                }
                else
                {
                    fn = del.fn;
                }
                return *this;
            }

            DelegateImpl& operator=(FunPtr fn)
            {
                if (FUNC == kind)
                {
                    functional.~FunctionType();
                    kind = FP;
                }
                DelegateImpl::fn = fn;
                return *this;
            }

            DelegateImpl& IRAM_ATTR operator=(std::nullptr_t)
            {
                if (FUNC == kind)
                {
                    functional.~FunctionType();
                }
                kind = FP;
                fn = nullptr;
                return *this;
            }

            IRAM_ATTR operator bool() const
            {
                if (FP == kind)
                {
                    return fn;
                }
                else
                {
                    return functional ? true : false;
                }
            }

            operator FunVPPtr() const
            {
                if (FP == kind)
                {
                    return reinterpret_cast<FunVPPtr>(fn);
                }
                else
                {
                    return [](void* self) -> R
                    {
                        return static_cast<DelegateImpl*>(self)->functional();
                    };
                }
            }

            void* arg() const
            {
                if (FP == kind)
                {
                    return nullptr;
                }
                else
                {
                    return const_cast<DelegateImpl*>(this);
                }
            }

            operator FunctionType() const
            {
                if (FP == kind)
                {
                    return fn;
                }
                else
                {
                    return functional;
                }
            }

            /// Calling is safe without checking for nullptr.
            /// If non-void, returns the default value.
            /// In ISR context, where faults and exceptions must not
            /// occurs, this saves the extra check for nullptr,
            /// and allows the compiler to optimize out checks
            /// in std::function which may not be ISR-safe or
            /// cause linker errors, like l32r relocation errors
            /// on the Xtensa ISA.
            R IRAM_ATTR operator()() const
            {
                if (FP == kind)
                {
                    if (fn) return fn();
                }
                else
                {
                    if (functional) return functional();
                }
                return R();
            }

        protected:
            union {
                FunctionType functional;
                FunPtr fn;
            };
            enum { FUNC, FP } kind;
        };
#else
        template<typename R>
        class DelegateImpl<void, R> {
        public:
            using target_type = R();
        protected:
            using FunPtr = target_type*;
            using FunVPPtr = R(*)(void*);
        public:
            DelegateImpl()
            {
                fn = nullptr;
            }

            DelegateImpl(std::nullptr_t)
            {
                fn = nullptr;
            }

            DelegateImpl(const DelegateImpl& del)
            {
                fn = del.fn;
            }

            DelegateImpl(DelegateImpl&& del)
            {
                fn = std::move(del.fn);
            }

            DelegateImpl(FunPtr fn)
            {
                DelegateImpl::fn = fn;
            }

            template<typename F> DelegateImpl(F fn)
            {
                DelegateImpl::fn = std::forward<F>(fn);
            }

            DelegateImpl& operator=(const DelegateImpl& del)
            {
                if (this == &del) return *this;
                fn = del.fn;
                return *this;
            }

            DelegateImpl& operator=(DelegateImpl&& del)
            {
                if (this == &del) return *this;
                fn = std::move(del.fn);
                return *this;
            }

            DelegateImpl& operator=(FunPtr fn)
            {
                DelegateImpl::fn = fn;
                return *this;
            }

            inline DelegateImpl& IRAM_ATTR operator=(std::nullptr_t) ALWAYS_INLINE_ATTR
            {
                fn = nullptr;
                return *this;
            }

            inline IRAM_ATTR operator bool() const ALWAYS_INLINE_ATTR
            {
                return fn;
            }

            operator FunVPPtr() const
            {
                return reinterpret_cast<FunVPPtr>(fn);
            }

            void* arg() const
            {
                return nullptr;
            }

            /// Calling is safe without checking for nullptr.
            /// If non-void, returns the default value.
            /// In ISR context, where faults and exceptions must not
            /// occurs, this saves the extra check for nullptr,
            /// and allows the compiler to optimize out checks
            /// in std::function which may not be ISR-safe or
            /// cause linker errors, like l32r relocation errors
            /// on the Xtensa ISA.
            inline R IRAM_ATTR operator()() const ALWAYS_INLINE_ATTR
            {
                if (fn) return fn();
                return R();
            }

        protected:
            FunPtr fn;
        };
#endif

        template<typename AA = void, typename R = void, typename... P>
        class Delegate : private detail::DelegatePImpl<AA, R, P...>
        {
        public:
            using target_type = R(P...);
        protected:
            using FunPtr = target_type*;
            using FunAPtr = R(*)(AA, P...);
            using FunVPPtr = R(*)(void*, P...);
#if !defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
            using FunctionType = std::function<target_type>;
#endif
        public:
            using detail::DelegatePImpl<AA, R, P...>::operator bool;
            using detail::DelegatePImpl<AA, R, P...>::arg;
            using detail::DelegatePImpl<AA, R, P...>::operator();

            operator FunVPPtr() { return detail::DelegatePImpl<AA, R, P...>::operator FunVPPtr(); }
#if !defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
            operator FunctionType() { return detail::DelegatePImpl<AA, R, P...>::operator FunctionType(); }
#endif

            Delegate() : detail::DelegatePImpl<AA, R, P...>::DelegatePImpl() {}

            Delegate(std::nullptr_t) : detail::DelegatePImpl<AA, R, P...>::DelegatePImpl(nullptr) {}

            Delegate(const Delegate& del) : detail::DelegatePImpl<AA, R, P...>::DelegatePImpl(
                static_cast<const detail::DelegatePImpl<AA, R, P...>&>(del)) {}

            Delegate(Delegate&& del) : detail::DelegatePImpl<AA, R, P...>::DelegatePImpl(
                std::move(static_cast<detail::DelegatePImpl<AA, R, P...>&>(del))) {}

            Delegate(FunAPtr fnA, const AA& obj) : detail::DelegatePImpl<AA, R, P...>::DelegatePImpl(fnA, obj) {}

            Delegate(FunAPtr fnA, AA&& obj) : detail::DelegatePImpl<AA, R, P...>::DelegatePImpl(fnA, std::move(obj)) {}

            Delegate(FunPtr fn) : detail::DelegatePImpl<AA, R, P...>::DelegatePImpl(fn) {}

            template<typename F> Delegate(F functional) : detail::DelegatePImpl<AA, R, P...>::DelegatePImpl(std::forward<F>(functional)) {}

            Delegate& operator=(const Delegate& del) {
                detail::DelegatePImpl<AA, R, P...>::operator=(del);
                return *this;
            }

            Delegate& operator=(Delegate&& del) {
                detail::DelegatePImpl<AA, R, P...>::operator=(std::move(del));
                return *this;
            }

            Delegate& operator=(FunPtr fn) {
                detail::DelegatePImpl<AA, R, P...>::operator=(fn);
                return *this;
            }

            inline Delegate& IRAM_ATTR operator=(std::nullptr_t) ALWAYS_INLINE_ATTR {
                detail::DelegatePImpl<AA, R, P...>::operator=(nullptr);
                return *this;
            }
        };

        template<typename AA, typename R, typename... P>
        class Delegate<AA*, R, P...> : private detail::DelegatePImpl<AA*, R, P...>
        {
        public:
            using target_type = R(P...);
        protected:
            using FunPtr = target_type*;
            using FunAPtr = R(*)(AA*, P...);
            using FunVPPtr = R(*)(void*, P...);
#if !defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
            using FunctionType = std::function<target_type>;
#endif
        public:
            using detail::DelegatePImpl<AA*, R, P...>::operator bool;
            using detail::DelegatePImpl<AA*, R, P...>::operator();

            operator FunVPPtr() const
            {
                if (detail::DelegatePImpl<AA*, R, P...>::FPA == detail::DelegatePImpl<AA*, R, P...>::kind)
                {
                    return reinterpret_cast<FunVPPtr>(detail::DelegatePImpl<AA*, R, P...>::fnA);
                }
                else
                {
                    return detail::DelegatePImpl<AA*, R, P...>::operator FunVPPtr();
                }
            }
#if !defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
            operator FunctionType() { return detail::DelegatePImpl<AA*, R, P...>::operator FunctionType(); }
#endif
            void* arg() const
            {
                if (detail::DelegatePImpl<AA*, R, P...>::FPA == detail::DelegatePImpl<AA*, R, P...>::kind)
                {
                    return detail::DelegatePImpl<AA*, R, P...>::obj;
                }
                else
                {
                    return detail::DelegatePImpl<AA*, R, P...>::arg();
                }
            }

            Delegate() : detail::DelegatePImpl<AA*, R, P...>::DelegatePImpl() {}

            Delegate(std::nullptr_t) : detail::DelegatePImpl<AA*, R, P...>::DelegatePImpl(nullptr) {}

            Delegate(const Delegate& del) : detail::DelegatePImpl<AA*, R, P...>::DelegatePImpl(
                static_cast<const detail::DelegatePImpl<AA*, R, P...>&>(del)) {}

            Delegate(Delegate&& del) : detail::DelegatePImpl<AA*, R, P...>::DelegatePImpl(
                std::move(static_cast<detail::DelegatePImpl<AA*, R, P...>&>(del))) {}

            Delegate(FunAPtr fnA, AA* obj) : detail::DelegatePImpl<AA*, R, P...>::DelegatePImpl(fnA, obj) {}

            Delegate(FunPtr fn) : detail::DelegatePImpl<AA*, R, P...>::DelegatePImpl(fn) {}

            template<typename F> Delegate(F functional) : detail::DelegatePImpl<AA*, R, P...>::DelegatePImpl(std::forward<F>(functional)) {}

            Delegate& operator=(const Delegate& del) {
                detail::DelegatePImpl<AA*, R, P...>::operator=(del);
                return *this;
            }

            Delegate& operator=(Delegate&& del) {
                detail::DelegatePImpl<AA*, R, P...>::operator=(std::move(del));
                return *this;
            }

            Delegate& operator=(FunPtr fn) {
                detail::DelegatePImpl<AA*, R, P...>::operator=(fn);
                return *this;
            }

            inline Delegate& IRAM_ATTR operator=(std::nullptr_t) ALWAYS_INLINE_ATTR {
                detail::DelegatePImpl<AA*, R, P...>::operator=(nullptr);
                return *this;
            }
        };

        template<typename R, typename... P>
        class Delegate<void, R, P...> : private detail::DelegatePImpl<void, R, P...>
        {
        public:
            using target_type = R(P...);
        protected:
            using FunPtr = target_type*;
#if !defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
            using FunctionType = std::function<target_type>;
#endif
            using FunVPPtr = R(*)(void*, P...);
        public:
            using detail::DelegatePImpl<void, R, P...>::operator bool;
            using detail::DelegatePImpl<void, R, P...>::arg;
            using detail::DelegatePImpl<void, R, P...>::operator();

            operator FunVPPtr() const { return detail::DelegatePImpl<void, R, P...>::operator FunVPPtr(); }
#if !defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
            operator FunctionType() { return detail::DelegatePImpl<void, R, P...>::operator FunctionType(); }
#endif

            Delegate() : detail::DelegatePImpl<void, R, P...>::DelegatePImpl() {}

            Delegate(std::nullptr_t) : detail::DelegatePImpl<void, R, P...>::DelegatePImpl(nullptr) {}

            Delegate(const Delegate& del) : detail::DelegatePImpl<void, R, P...>::DelegatePImpl(
                static_cast<const detail::DelegatePImpl<void, R, P...>&>(del)) {}

            Delegate(Delegate&& del) : detail::DelegatePImpl<void, R, P...>::DelegatePImpl(
                std::move(static_cast<detail::DelegatePImpl<void, R, P...>&>(del))) {}

            Delegate(FunPtr fn) : detail::DelegatePImpl<void, R, P...>::DelegatePImpl(fn) {}

            template<typename F> Delegate(F functional) : detail::DelegatePImpl<void, R, P...>::DelegatePImpl(std::forward<F>(functional)) {}

            Delegate& operator=(const Delegate& del) {
                detail::DelegatePImpl<void, R, P...>::operator=(del);
                return *this;
            }

            Delegate& operator=(Delegate&& del) {
                detail::DelegatePImpl<void, R, P...>::operator=(std::move(del));
                return *this;
            }

            Delegate& operator=(FunPtr fn) {
                detail::DelegatePImpl<void, R, P...>::operator=(fn);
                return *this;
            }

            inline Delegate& IRAM_ATTR operator=(std::nullptr_t) ALWAYS_INLINE_ATTR {
                detail::DelegatePImpl<void, R, P...>::operator=(nullptr);
                return *this;
            }
        };

        template<typename AA, typename R>
        class Delegate<AA, R> : private detail::DelegateImpl<AA, R>
        {
        public:
            using target_type = R();
        protected:
            using FunPtr = target_type*;
            using FunAPtr = R(*)(AA);
            using FunVPPtr = R(*)(void*);
#if !defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
            using FunctionType = std::function<target_type>;
#endif
        public:
            using detail::DelegateImpl<AA, R>::operator bool;
            using detail::DelegateImpl<AA, R>::arg;
            using detail::DelegateImpl<AA, R>::operator();

            operator FunVPPtr() { return detail::DelegateImpl<AA, R>::operator FunVPPtr(); }
#if !defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
            operator FunctionType() { return detail::DelegateImpl<AA, R>::operator FunctionType(); }
#endif

            Delegate() : detail::DelegateImpl<AA, R>::DelegateImpl() {}

            Delegate(std::nullptr_t) : detail::DelegateImpl<AA, R>::DelegateImpl(nullptr) {}

            Delegate(const Delegate& del) : detail::DelegateImpl<AA, R>::DelegateImpl(
                static_cast<const detail::DelegateImpl<AA, R>&>(del)) {}

            Delegate(Delegate&& del) : detail::DelegateImpl<AA, R>::DelegateImpl(
                std::move(static_cast<detail::DelegateImpl<AA, R>&>(del))) {}

            Delegate(FunAPtr fnA, const AA& obj) : detail::DelegateImpl<AA, R>::DelegateImpl(fnA, obj) {}

            Delegate(FunAPtr fnA, AA&& obj) : detail::DelegateImpl<AA, R>::DelegateImpl(fnA, std::move(obj)) {}

            Delegate(FunPtr fn) : detail::DelegateImpl<AA, R>::DelegateImpl(fn) {}

            template<typename F> Delegate(F functional) : detail::DelegateImpl<AA, R>::DelegateImpl(std::forward<F>(functional)) {}

            Delegate& operator=(const Delegate& del) {
                detail::DelegateImpl<AA, R>::operator=(del);
                return *this;
            }

            Delegate& operator=(Delegate&& del) {
                detail::DelegateImpl<AA, R>::operator=(std::move(del));
                return *this;
            }

            Delegate& operator=(FunPtr fn) {
                detail::DelegateImpl<AA, R>::operator=(fn);
                return *this;
            }

            inline Delegate& IRAM_ATTR operator=(std::nullptr_t) ALWAYS_INLINE_ATTR {
                detail::DelegateImpl<AA, R>::operator=(nullptr);
                return *this;
            }
        };

        template<typename AA, typename R>
        class Delegate<AA*, R> : private detail::DelegateImpl<AA*, R>
        {
        public:
            using target_type = R();
        protected:
            using FunPtr = target_type*;
            using FunAPtr = R(*)(AA*);
            using FunVPPtr = R(*)(void*);
#if !defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
            using FunctionType = std::function<target_type>;
#endif
        public:
            using detail::DelegateImpl<AA*, R>::operator bool;
            using detail::DelegateImpl<AA*, R>::operator();

            operator FunVPPtr() const
            {
                if (detail::DelegateImpl<AA*, R>::FPA == detail::DelegateImpl<AA*, R>::kind)
                {
                    return reinterpret_cast<FunVPPtr>(detail::DelegateImpl<AA*, R>::fnA);
                }
                else
                {
                    return detail::DelegateImpl<AA*, R>::operator FunVPPtr();
                }
            }
#if !defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
            operator FunctionType() { return detail::DelegateImpl<AA*, R>::operator FunctionType(); }
#endif
            void* arg() const
            {
                if (detail::DelegateImpl<AA*, R>::FPA == detail::DelegateImpl<AA*, R>::kind)
                {
                    return detail::DelegateImpl<AA*, R>::obj;
                }
                else
                {
                    return detail::DelegateImpl<AA*, R>::arg();
                }
            }

            Delegate() : detail::DelegateImpl<AA*, R>::DelegateImpl() {}

            Delegate(std::nullptr_t) : detail::DelegateImpl<AA*, R>::DelegateImpl(nullptr) {}

            Delegate(const Delegate& del) : detail::DelegateImpl<AA*, R>::DelegateImpl(
                static_cast<const detail::DelegateImpl<AA*, R>&>(del)) {}

            Delegate(Delegate&& del) : detail::DelegateImpl<AA*, R>::DelegateImpl(
                std::move(static_cast<detail::DelegateImpl<AA*, R>&>(del))) {}

            Delegate(FunAPtr fnA, AA* obj) : detail::DelegateImpl<AA*, R>::DelegateImpl(fnA, obj) {}

            Delegate(FunPtr fn) : detail::DelegateImpl<AA*, R>::DelegateImpl(fn) {}

            template<typename F> Delegate(F functional) : detail::DelegateImpl<AA*, R>::DelegateImpl(std::forward<F>(functional)) {}

            Delegate& operator=(const Delegate& del) {
                detail::DelegateImpl<AA*, R>::operator=(del);
                return *this;
            }

            Delegate& operator=(Delegate&& del) {
                detail::DelegateImpl<AA*, R>::operator=(std::move(del));
                return *this;
            }

            Delegate& operator=(FunPtr fn) {
                detail::DelegateImpl<AA*, R>::operator=(fn);
                return *this;
            }

            inline Delegate& IRAM_ATTR operator=(std::nullptr_t) ALWAYS_INLINE_ATTR {
                detail::DelegateImpl<AA*, R>::operator=(nullptr);
                return *this;
            }
        };

        template<typename R>
        class Delegate<void, R> : private detail::DelegateImpl<void, R>
        {
        public:
            using target_type = R();
        protected:
            using FunPtr = target_type*;
#if !defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
            using FunctionType = std::function<target_type>;
#endif
            using FunVPPtr = R(*)(void*);
        public:
            using detail::DelegateImpl<void, R>::operator bool;
            using detail::DelegateImpl<void, R>::arg;
            using detail::DelegateImpl<void, R>::operator();

            operator FunVPPtr() const { return detail::DelegateImpl<void, R>::operator FunVPPtr(); }
#if !defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
            operator FunctionType() { return detail::DelegateImpl<void, R>::operator FunctionType(); }
#endif

            Delegate() : detail::DelegateImpl<void, R>::DelegateImpl() {}

            Delegate(std::nullptr_t) : detail::DelegateImpl<void, R>::DelegateImpl(nullptr) {}

            Delegate(const Delegate& del) : detail::DelegateImpl<void, R>::DelegateImpl(
                static_cast<const detail::DelegateImpl<void, R>&>(del)) {}

            Delegate(Delegate&& del) : detail::DelegateImpl<void, R>::DelegateImpl(
                std::move(static_cast<detail::DelegateImpl<void, R>&>(del))) {}

            Delegate(FunPtr fn) : detail::DelegateImpl<void, R>::DelegateImpl(fn) {}

            template<typename F> Delegate(F functional) : detail::DelegateImpl<void, R>::DelegateImpl(std::forward<F>(functional)) {}

            Delegate& operator=(const Delegate& del) {
                detail::DelegateImpl<void, R>::operator=(del);
                return *this;
            }

            Delegate& operator=(Delegate&& del) {
                detail::DelegateImpl<void, R>::operator=(std::move(del));
                return *this;
            }

            Delegate& operator=(FunPtr fn) {
                detail::DelegateImpl<void, R>::operator=(fn);
                return *this;
            }

            inline Delegate& IRAM_ATTR operator=(std::nullptr_t) ALWAYS_INLINE_ATTR {
                detail::DelegateImpl<void, R>::operator=(nullptr);
                return *this;
            }
        };
    }
}

template<typename AA = void, typename R = void, typename... P> class Delegate;
template<typename AA, typename R, typename... P> class Delegate<R(P...), AA> : public delegate::detail::Delegate<AA, R, P...>
{
public:
    Delegate() : delegate::detail::Delegate<AA, R, P...>::Delegate() {}

    Delegate(std::nullptr_t) : delegate::detail::Delegate<AA, R, P...>::Delegate(nullptr) {}

    Delegate(const Delegate& del) : delegate::detail::Delegate<AA, R, P...>::Delegate(
        static_cast<const delegate::detail::Delegate<AA, R, P...>&>(del)) {}

    Delegate(Delegate&& del) : delegate::detail::Delegate<AA, R, P...>::Delegate(
        std::move(static_cast<delegate::detail::Delegate<AA, R, P...>&>(del))) {}

    Delegate(typename delegate::detail::Delegate<AA, R, P...>::FunAPtr fnA, const AA& obj) : delegate::detail::Delegate<AA, R, P...>::Delegate(fnA, obj) {}

    Delegate(typename delegate::detail::Delegate<AA, R, P...>::FunAPtr fnA, AA&& obj) : delegate::detail::Delegate<AA, R, P...>::Delegate(fnA, std::move(obj)) {}

    Delegate(typename delegate::detail::Delegate<AA, R, P...>::FunPtr fn) : delegate::detail::Delegate<AA, R, P...>::Delegate(fn) {}

    template<typename F> Delegate(F functional) : delegate::detail::Delegate<AA, R, P...>::Delegate(std::forward<F>(functional)) {}

    Delegate& operator=(const Delegate& del) {
        delegate::detail::Delegate<AA, R, P...>::operator=(del);
        return *this;
    }

    Delegate& operator=(Delegate&& del) {
        delegate::detail::Delegate<AA, R, P...>::operator=(std::move(del));
        return *this;
    }

    Delegate& operator=(typename delegate::detail::Delegate<AA, R, P...>::FunPtr fn) {
        delegate::detail::Delegate<AA, R, P...>::operator=(fn);
        return *this;
    }

    inline Delegate& IRAM_ATTR operator=(std::nullptr_t) ALWAYS_INLINE_ATTR {
        delegate::detail::Delegate<AA, R, P...>::operator=(nullptr);
        return *this;
    }
};

template<typename R, typename... P> class Delegate<R(P...)> : public delegate::detail::Delegate<void, R, P...>
{
public:
    Delegate() : delegate::detail::Delegate<void, R, P...>::Delegate() {}

    Delegate(std::nullptr_t) : delegate::detail::Delegate<void, R, P...>::Delegate(nullptr) {}

    Delegate(const Delegate& del) : delegate::detail::Delegate<void, R, P...>::Delegate(
        static_cast<const delegate::detail::Delegate<void, R, P...>&>(del)) {}

    Delegate(Delegate&& del) : delegate::detail::Delegate<void, R, P...>::Delegate(
        std::move(static_cast<delegate::detail::Delegate<void, R, P...>&>(del))) {}

    Delegate(typename delegate::detail::Delegate<void, R, P...>::FunPtr fn) : delegate::detail::Delegate<void, R, P...>::Delegate(fn) {}

    template<typename F> Delegate(F functional) : delegate::detail::Delegate<void, R, P...>::Delegate(std::forward<F>(functional)) {}

    Delegate& operator=(const Delegate& del) {
        delegate::detail::Delegate<void, R, P...>::operator=(del);
        return *this;
    }

    Delegate& operator=(Delegate&& del) {
        delegate::detail::Delegate<void, R, P...>::operator=(std::move(del));
        return *this;
    }

    Delegate& operator=(typename delegate::detail::Delegate<void, R, P...>::FunPtr fn) {
        delegate::detail::Delegate<void, R, P...>::operator=(fn);
        return *this;
    }

    inline Delegate& IRAM_ATTR operator=(std::nullptr_t) ALWAYS_INLINE_ATTR {
        delegate::detail::Delegate<void, R, P...>::operator=(nullptr);
        return *this;
    }
};

#endif // __Delegate_h
