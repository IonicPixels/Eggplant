#pragma once

#include <fstream>

#include "include/Types.h"

class TextWriter {
public:
    TextWriter(const char* pFileName);

    bool openFile();
    void closeFile();

    void writeString(const char*);
    void writeStringWithSpaceBefore(const char*);
    void writeHex(u32);
    void writeHexWithSpaceBefore(u32);
    void writeLine();
    void writeLabelStart(const char*, u32);
    void writeLabelStartAndSub(const char*, u32, u32);
    void writeLabelStartSplit(const char*, u32, s32);
    void writeLabelStartSplitAndSub(const char*, u32, u32, s32);
    void writeLabelRef(const char*, u32);
    void writeLabelRefWithSpaceBefore(const char*, u32);
    void writeLabelRefWithSpaceBeforeAndSub(const char*, u32, u32);
    void writeLabelRefWithSpaceBeforeSplit(const char*, u32, s32);
    void writeLabelRefWithSpaceBeforeSplitAndSub(const char*, u32, u32, s32);
    void writeHeaderMarker();
    void writeChar(char);

private:
    void writePrintf(const char* pFormat, ...);

    std::ofstream mOutStream;
    const char* mFileName;
};