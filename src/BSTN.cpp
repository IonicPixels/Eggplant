#include "include/BSTN.h"

// Version 1.1

const char* BSTNHolder::getSoundNameFromId(u32 id) const {
    u32 group = id >> 24 & 0xFF;
    u32 category = id >> 16 & 0xFF;
    id &= 0xFFFF;

    // get start adr
    u32 pos = readU32BE(0xC);

    // set group adr
    if (group >= readU32BE(pos))
        return 0;
    pos = readU32BE(pos + 4 + group * 4); // + 4 = skips number of groups

    // set category adr
    if (category >= readU32BE(pos))
        return 0;
    pos = readU32BE(pos + 8 + category * 4); // + 8 = skips number of categories + group name adr

    // set string adr
    if (id >= readU32BE(pos))
        return 0;

    return mFileData + readU32BE(pos + 8 + id * 4); // + 8 = skips number of sounds + category name
}

const char* BSTNHolder::getCategoryNameFromId(u32 id) const {
    u32 group = id >> 24 & 0xFF;
    u32 category = id >> 16 & 0xFF;
    id &= 0xFFFF;

    // get start adr
    u32 pos = readU32BE(0xC);

    // set group adr
    if (group >= readU32BE(pos))
        return 0;
    pos = readU32BE(pos + 4 + group * 4); // + 4 = skips number of groups

    // set category adr
    if (category >= readU32BE(pos))
        return 0;
    pos = readU32BE(pos + 8 + category * 4); // + 8 = skips number of categories + group name adr

    return mFileData + readU32BE(pos + 4);
}