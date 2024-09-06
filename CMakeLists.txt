cmake_minimum_required(VERSION 3.29)
project(Module)

add_definitions(-DLUAI_GCMETRICS)   # Force GC metrics on Luau.

option(CAPSTONE_X86_SUPPORT "Enable x86 capstone" ON)

set(BUILD_SHARED_LIBS OFF)
set(PROJECT_NAME Module)
set(CMAKE_CXX_STANDARD 23)

add_library(Module SHARED
        main.cpp

        Logger.cpp
        Logger.hpp
        Scanner.cpp
        Scanner.hpp
        Utilities.cpp
        Utilities.hpp
        Scheduler.hpp
        Scheduler.cpp
        Security.hpp
        Security.cpp

        # Managers
        RobloxManager.cpp
        RobloxManager.hpp
        LuauManager.cpp
        LuauManager.hpp
        ClosureManager.cpp
        ClosureManager.hpp

        # Roblox Definitions
        Roblox/TypeDefinitions.hpp
        Roblox/TypeDefinitions.hpp

        # Environment
        Environment/EnvironmentManager.cpp
        Environment/EnvironmentManager.hpp

        # Environment -> Libraries
        Environment/Libraries/Globals.cpp
        Environment/Libraries/Globals.hpp
        Environment/Libraries/Debug.cpp
        Environment/Libraries/Debug.hpp
        Environment/Libraries/Filesystem.cpp
        Environment/Libraries/Filesystem.hpp
        Environment/Libraries/Closures.cpp
        Environment/Libraries/Closures.hpp
        Environment/Libraries/Metatable.cpp
        Environment/Libraries/Metatable.hpp
        Environment/Libraries/Cache.cpp
        Environment/Libraries/Cache.hpp
        Environment/Libraries/Console.cpp
        Environment/Libraries/Console.hpp
        Environment/Libraries/Script.cpp
        Environment/Libraries/Script.hpp
        Environment/Libraries/Misc.cpp
        Environment/Libraries/Misc.hpp
        Environment/Libraries/Instance.cpp
        Environment/Libraries/Instance.hpp
        Environment/Libraries/Input.cpp
        Environment/Libraries/Input.hpp
        Environment/Libraries/WebSocket.cpp
        Environment/Libraries/WebSocket.hpp
        Environment/Libraries/Http.cpp
        Environment/Libraries/Http.hpp

        # Disassembler abstraction
        Disassembler/Disassembler.cpp
        Disassembler/Disassembler.hpp
        Disassembler/DisassemblyRequest.hpp
        Disassembler/DisassembledChunk.cpp
        Disassembler/DisassembledChunk.hpp

        # Communication
        Communication/Communication.cpp
        Communication/Communication.hpp
        Communication/PacketSerdes.cpp
        Communication/PacketSerdes.hpp

        # Communication -> Packets
        Communication/Packets/PacketBase.hpp
        Communication/Packets/SetSafeModePacket.hpp
        Communication/Packets/SetFunctionBlockStatePacket.hpp
        Communication/Packets/SetNativeCodeGenPacket.hpp
        Communication/Packets/SetFastVariablePacket.hpp
        Communication/Packets/SetRequestFingerprintPacket.hpp
        Communication/Packets/SetExecutionDataModelPacket.hpp
        Communication/Packets/HelloPacket.hpp
        Communication/Packets/ResponseStatusPacket.hpp
        Communication/Packets/ScheduleLuauPacket.hpp
        Communication/Packets/ScheduleLuauResponsePacket.hpp
        Communication/Packets/SetScriptSourceAccessPacket.hpp
)
target_include_directories(Module PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(Module PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies")
# Dependencies


# Curl For People
# FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
#         GIT_TAG 3b15fa82ea74739b574d705fea44959b58142eb8)
# FetchContent_MakeAvailable(cpr)

#rcmp
# FetchContent_Declare(rcmp GIT_REPOSITORY https://github.com/Smertig/rcmp.git
#         GIT_TAG f5f75ae00a57c67fe41f79fd59c4b7f6997b999e)
# FetchContent_MakeAvailable(rcmp)

# cpr
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/cpr")

# Luau
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/Luau")

# minhook
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/Minhook")

# IXWebSocket
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/IXWebSocket")

# cryptopp
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/cryptopp-cmake")

# capstone
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/Capstone")

find_package(lz4 CONFIG REQUIRED)
find_package(OpenSSL CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        # minhook
        minhook
        # Curl For People
        cpr::cpr

        # Luau
        Luau.Compiler
        Luau.Ast
        # Luau.Analysis
        Luau.VM
        Luau.VM.Internals
        Luau.EqSat
        Luau.CodeGen


        ixwebsocket

        lz4::lz4

        cryptopp::cryptopp

        capstone

        nlohmann_json::nlohmann_json

        Dbghelp.lib
        Ws2_32.lib
)

