//
// Created by Dottik on 3/4/2024.
//

#include "StudioOffsets.h"
#include "memory"
#include "shared_mutex"

std::shared_ptr<RbxStuOffsets> RbxStuOffsets::ptr;
std::shared_mutex RbxStuOffsets::__rbxstuoffsets__sharedmutex__;

