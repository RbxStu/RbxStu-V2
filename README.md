# RbxStu V2

[![CodeFactor](https://www.codefactor.io/repository/github/rbxstu/rbxstu-v2/badge)](https://www.codefactor.io/repository/github/rbxstu/rbxstu-v2)</br>
![Alt](https://repobeats.axiom.co/api/embed/6bc3a3ce38664097265c2df78e9c6cdbd244d2b2.svg "Repobeats analytics image")

The Roblox Studio Executor.

This is a rewrite of the original [RbxStu](https://github.com/RbxStu/RbxStu). This version has many improvements and it
is magnitudes more stable than the original ever was. 

To report a bug or suggest a feature, please head to our discord server https://discord.gg/rGWK6Sqbrs

## Features:

- Stable execution.
- Clean, unrestricted and raw execution (Adding environment on the coming days).
- Maintainable and **_EXPANDABLE_** codebase.

Whilst RbxStu was born as a project to make a pentesting tool, this codebase allows for more than that, it allows for a
base framework to be defined in which Roblox Studio modifications can be built in C++, the project does not have a
current way of interacting with it, but it is on the "TODO" list to add a way of communicating with the DLL on your own
DLL! So that if you want to make your own Roblox Studio modification, you will not need a heavy, big base, and can start
fairly light!

### Is this allowed by Roblox?

Whilst Roblox has a no reverse engineering policy, it is not enforced necessarily for anything Roblox Studio related, as
this would be unwise, it is a development tool, and anything that happens to it will not truly impact players on the
release client.

### Is this vulnerable to 'x' or 'y'?

RbxStu cannot guarantee your safety when executing scripts. It does its best to prevent really nasty things from
happening, but Roblox will over time add new services that they will internally use that expose dangerous functions,
such as one to save debug data, which allows you to write bat files, and then one that allows you to execute things, all
because we are on a high security context we were never meant to truly use.

## Dependencies:

- vcpkg cmake toolchain script (Add CMake option `-DVCPKG_TARGET_TRIPLET=x64-windows-static` as well)
- CLion 2024.2
- MSVC
- Ninja build system (Prepackaged with CLion)

## Building

#### Requirements

- CMake
- MSVC
- Ninja
- vcpkg
- CLion (contains all of the above besides MSVC)

### Clion

1. Set environment variable `VCPKG_ROOT` to the path of your vcpkg installation. (If windows feels goofy you might need to
   restart your computer)
2. Open the project in CLion.
3. Set the CMake profile to any of the presets and enable it (example: `Release`, `Debug`, `RelWithDebInfo`).
4. Build the project.

### CMake/Ninja

1. Set environment variable `VCPKG_ROOT` to the path of your vcpkg installation. (If windows feels goofy you might need to
   restart your computer)
3. Open `x64 Native Tools Command Prompt for VS 2022` or similar with access to msvc tools.
2. Run the following commands in the project root directory:

```shell
cmake --preset={preset name here}
cmake --build --preset={preset name here}
```

## Significant Contributors:

- [Dottik (SecondNewtonLaw/NaN)](https://github.com/SecondNewtonLaw): Lead Developer/Owner, Maintainer
- [nHisoka (Yoru)](https://github.com/nhisoka): Contributor
- [Pixeluted](https://github.com/Pixeluted): Contributor
- [MakeSureDudeDies](https://github.com/MakeSureDudeDies): Contributor
- [Shadow](https://github.com/ShadowIsReal): Contributor
