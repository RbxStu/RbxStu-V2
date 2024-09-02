//
// Created by Dottik on 29/8/2024.
//

// Ignorable, as this is caused by macro expansion evaluation.
// ReSharper disable CppRedundantComplexityInComparison

#include "PacketSerdes.hpp"

#include <istream>
#include <optional>

#include "Logger.hpp"

std::shared_ptr<PacketSerdes> PacketSerdes::pInstance;

std::shared_ptr<PacketSerdes> PacketSerdes::GetSingleton() {
    if (PacketSerdes::pInstance == nullptr)
        PacketSerdes::pInstance = std::make_shared<PacketSerdes>();

    return PacketSerdes::pInstance;
}
