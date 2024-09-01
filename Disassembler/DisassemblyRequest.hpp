//
// Created by Dottik on 26/8/2024.
//
#pragma once

struct DisassemblyRequest {
    bool bIgnorePageProtection;
    void *pStartAddress;
    void *pEndAddress;
};
