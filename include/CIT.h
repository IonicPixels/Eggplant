#pragma once

#include <iostream>
#include "include/Types.h"
#include "include/BinaryReader.h"
#include "include/TextWriter.h"
#include "include/TextReader.h"

namespace CIT {
    struct Header {
        u32 mPadding;
        char mMagic[4];
        u32 mSize;
        u16 mChordCount;
        u16 mScalePairCount;
    };

    struct Chords {
        u8 mRootNote;
        u8 mTriad[3];
        u8 mExtensions[4];
    };

    struct ScalesPair {
        u32 mScaleAPtr;
        u32 mScaleBPtr;
    };

    struct ScalesPairData {
        u8 mNotes[12];
    };

    const char* convertValueToNote(u32);
    u32 convertNoteToValue(const char*);
}

class CITReader {
public:
    inline CITReader() {}

    bool writeOutFile(BinaryReader* pIn, TextWriter* pOut);
    bool readChords();
    bool readScalePairs();

    BinaryReader* mBin;
    TextWriter* mOut;
    u32 mCitSize;
    u16 mChordCount;
    u16 mScalePairsCount;
};

class CITWriter {
public:
    inline CITWriter() {}

    bool writeOutFile(BinaryReader* pOut, TextReader* pIn);
    bool writePtrs();
    bool writeChordData();
    bool writeScaleData();

    void findNextValueInCurrentLine(char*, u32);
    u32 getNextValueInCurrentLine();

    TextReader* mIn;
    BinaryReader* mOut;
    u32 mCitSize;
    u16 mChordCount;
    u16 mScalePairsCount;
};