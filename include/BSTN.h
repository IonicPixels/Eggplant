#pragma once

#include "include/BinaryReader.h"

// Version 1.1


class BSTNHolder : public BinaryReader {
public:
    BSTNHolder(const char* pFile) : BinaryReader(pFile) {}

    const char* getSoundNameFromId(u32) const;
    const char* getCategoryNameFromId(u32) const;
};