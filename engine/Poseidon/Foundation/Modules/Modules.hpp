#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MODULES_HPP
#define _MODULES_HPP

#include <Poseidon/Foundation/Containers/CacheList.hpp>


namespace Poseidon::Foundation
{
using ModuleFunction = void();

// A module registered for ordered init/done.
struct RegistrationItem : public CLDLink
{
    int level;
    ModuleFunction* initFunction;
    ModuleFunction* doneFunction;

    RegistrationItem(int l, ModuleFunction* init, ModuleFunction* done) : level(l), initFunction(init), doneFunction(done) {}
};

void RegisterModule(RegistrationItem* item);
void InitModules();
void DoneModules();

#define REGISTER_MODULE(name, level, init, done)                                                                           \
    static struct RegisterModule##name : public ::Poseidon::Foundation::RegistrationItem                                   \
    {                                                                                                                      \
        RegisterModule##name() : ::Poseidon::Foundation::RegistrationItem(level, init, done) { ::Poseidon::Foundation::RegisterModule(this); } \
    } GRegisterModule##name;

// use when no done function
#define INIT_MODULE(name, level)                            \
    void InitModule##name();                                \
    REGISTER_MODULE(name, level, InitModule##name, nullptr) \
    void InitModule##name()

#define REGISTER_PLUGIN(level, init, done)                                                                       \
    extern "C" __declspec(dllexport) void RegisterPlugin(int& retLevel, ModuleFunction*& retInit,                \
                                                         ModuleFunction*& retDone);                              \
    __declspec(dllexport) void RegisterPlugin(int& retLevel, ModuleFunction*& retInit, ModuleFunction*& retDone) \
    {                                                                                                            \
        retLevel = level;                                                                                        \
        retInit = init;                                                                                          \
        retDone = done;                                                                                          \
    }

#define INIT_PLUGIN(level)                      \
    void InitPlugin();                          \
    REGISTER_PLUGIN(level, InitPlugin, nullptr) \
    void InitPlugin()

} // namespace Poseidon::Foundation

using Poseidon::Foundation::ModuleFunction;
using Poseidon::Foundation::RegistrationItem;
using Poseidon::Foundation::RegisterModule;
using Poseidon::Foundation::InitModules;
using Poseidon::Foundation::DoneModules;

#endif
