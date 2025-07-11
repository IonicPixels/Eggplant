#pragma once

#include "include/FileHolder.h"

class BinaryReader : public FileHolder {
private:
    bool doFileCheck(u32) const;
    bool doFileCheckWithEndFlag(u32);

public:
    inline BinaryReader() : mCurrentPos(0), mIsSystemLittleEndian(true), mHasReachedEndOfFile(false) {}
    inline BinaryReader(char* pDat, size_t len) : mCurrentPos(0), mIsSystemLittleEndian(true), mHasReachedEndOfFile(false) { mFileData = pDat; mFileSize = len; }
    BinaryReader(const char*);

    inline void resetPosition() {
        mCurrentPos = 0;
        mHasReachedEndOfFile = false;
    }

    inline void setPosition(u32 pos) {
        mCurrentPos = pos;
        mHasReachedEndOfFile = !doFileCheck(pos);
    }

    u32 readValueBE(u32 pos, s32 NumBytes) const;
    u32 readValueBE(s32 NumBytes);

    u8 readU8(u32 pos) const;
    u16 readU16BE(u32 pos) const;
    s16 readS16BE(u32 pos) const;
    u32 readU24BE(u32 pos) const;
    u32 readU32BE(u32 pos) const;
    s32 readS32BE(u32 pos) const;

    // read from CurrentPos and advances
    u8 readU8();
    u16 readU16BE();
    s16 readS16BE();
    u32 readU24BE();
    u32 readU32BE();
    s32 readS32BE();

    // write! (even tho this is called a Reader)
    void writeValueBE(u32 pos, s32 NumBytes, u32);
    void writeValueBE(s32 NumBytes, u32);

    void writeU8(u32 pos, u8);
    void writeU16BE(u32 pos, u16);
    void writeS16BE(u32 pos, s16);
    void writeU24BE(u32 pos, u32);
    void writeU32BE(u32 pos, u32);
    void writeS32BE(u32 pos, s32);

    void writeU8(u8);
    void writeU16BE(u16);
    void writeS16BE(s16);
    void writeU24BE(u32);
    void writeU32BE(u32);
    void writeS32BE(s32);

    u32 mCurrentPos;
    bool mIsSystemLittleEndian;
    bool mHasReachedEndOfFile;
};