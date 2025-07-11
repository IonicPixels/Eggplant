#include <cstring>
#include "include/CIT.h"

const char* CIT::convertValueToNote(u32 i) {
    switch (i) {
        case 0:
            return "C";
        case 1:
            return "C#";
        case 2:
            return "D";
        case 3:
            return "D#";
        case 4:
            return "E";
        case 5:
            return "F";
        case 6:
            return "F#";
        case 7:
            return "G";
        case 8:
            return "G#";
        case 9:
            return "A";
        case 10:
            return "A#";
        case 11:
            return "B";
        default:
            return "None";
    }
}

u32 CIT::convertNoteToValue(const char* pNote) {
    if (!strcasecmp(pNote, "C"))
        return 0;
    else if (!strcasecmp(pNote, "C#"))
        return 1;
    else if (!strcasecmp(pNote, "D"))
        return 2;
    else if (!strcasecmp(pNote, "D#"))
        return 3;
    else if (!strcasecmp(pNote, "E"))
        return 4;
    else if (!strcasecmp(pNote, "F"))
        return 5;
    else if (!strcasecmp(pNote, "F#"))
        return 6;
    else if (!strcasecmp(pNote, "G"))
        return 7;
    else if (!strcasecmp(pNote, "G#"))
        return 8;
    else if (!strcasecmp(pNote, "A"))
        return 9;
    else if (!strcasecmp(pNote, "A#"))
        return 10;
    else if (!strcasecmp(pNote, "B"))
        return 11;
    else if (!strcasecmp(pNote, "None"))
        return 127;
    else
        return strtoul(pNote, 0, 0);
}



bool CITReader::writeOutFile(BinaryReader* pIn, TextWriter* pOut) {
    mBin = pIn;
    mOut = pOut;

    if (mBin->readU32BE() != 0 || mBin->readU32BE() != 'CITS' || mBin->readU32BE(mBin->mCurrentPos) > mBin->mFileSize) {
        printf("ERROR: The input file is not a CIT file or the file is corrupted!\n");
        return false;
    }

    mCitSize = mBin->readU32BE();
    mChordCount = mBin->readU16BE();
    mScalePairsCount = mBin->readU16BE();

    mOut->writeString("~ <- use this to add comments!");
    mOut->writeLine();
    mOut->writeString("CITS");

    if (!readChords())
        return false;

    if (!readScalePairs())
        return false;

    return true;
}

bool CITReader::readChords() {
    for (u32 i = 0; i < mChordCount; i++) {
        u32 Pos = mBin->readU32BE();

        if (Pos > (mCitSize - sizeof(CIT::Chords))) {
            printf("ERROR: The input file is corrupted!\n");
            return false;
        }

        mOut->writeLine();
        mOut->writeLine();
        mOut->writeString("CHORD");
        mOut->writeHexWithSpaceBefore(i);

        mOut->writeLine();
        mOut->writeStringWithSpaceBefore("root");
        mOut->writeStringWithSpaceBefore(CIT::convertValueToNote(mBin->readU8(Pos)));

        mOut->writeLine();
        mOut->writeStringWithSpaceBefore("triad");
        mOut->writeStringWithSpaceBefore(CIT::convertValueToNote(mBin->readU8(Pos + 1)));
        mOut->writeStringWithSpaceBefore(CIT::convertValueToNote(mBin->readU8(Pos + 2)));
        mOut->writeStringWithSpaceBefore(CIT::convertValueToNote(mBin->readU8(Pos + 3)));

        mOut->writeLine();
        mOut->writeStringWithSpaceBefore("extension");
        mOut->writeStringWithSpaceBefore(CIT::convertValueToNote(mBin->readU8(Pos + 4)));
        mOut->writeStringWithSpaceBefore(CIT::convertValueToNote(mBin->readU8(Pos + 5)));
        mOut->writeStringWithSpaceBefore(CIT::convertValueToNote(mBin->readU8(Pos + 6)));
        mOut->writeStringWithSpaceBefore(CIT::convertValueToNote(mBin->readU8(Pos + 7)));
    }

    return true;
}

bool CITReader::readScalePairs() {
    for (u32 i = 0; i < mScalePairsCount; i++) {
        u32 Pos = mBin->readU32BE();

        if (Pos > (mCitSize - sizeof(CIT::ScalesPair))) {
            printf("ERROR: The input file is corrupted!\n");
            return false;
        }

        mOut->writeLine();
        mOut->writeLine();
        mOut->writeString("SCALE_PAIR");
        mOut->writeHexWithSpaceBefore(i);

        u32 PosA = mBin->readU32BE(Pos);
        u32 PosB = mBin->readU32BE(Pos + 4);

        if (PosA > (mCitSize - sizeof(CIT::ScalesPairData)) || PosB > (mCitSize - sizeof(CIT::ScalesPairData))) {
            printf("ERROR: The input file is corrupted!\n");
            return false;
        }

        mOut->writeLine();
        mOut->writeStringWithSpaceBefore("scale");
        for (u32 i = 0; i < 12; i++)
            mOut->writeStringWithSpaceBefore(CIT::convertValueToNote(mBin->readU8(PosA + i)));
        
        mOut->writeLine();
        mOut->writeStringWithSpaceBefore("scale");
        for (u32 i = 0; i < 12; i++)
            mOut->writeStringWithSpaceBefore(CIT::convertValueToNote(mBin->readU8(PosB + i)));
    }

    return true;
}









bool CITWriter::writeOutFile(BinaryReader* pOut, TextReader* pIn) {
    char buffer [16];
    mOut = pOut;
    mIn = pIn;

    mIn->resetPosToStartOfFile();
    mIn->setCommentChar('~');
    mIn->findNextActionLineIncludeCurrentLine();
    mIn->findNextValueInCurrentLine(buffer, 16);

    if (strcmp(buffer, "CITS")) {
        printf("ERROR: Invalid input file!\n");
        return false;
    }

    mChordCount = mIn->countValueInFileStr("CHORD");
    mScalePairsCount = mIn->countValueInFileStr("SCALE_PAIR");

    mCitSize = sizeof(CIT::Header) + mChordCount * (4 + sizeof(CIT::Chords)) + mScalePairsCount * (4 + sizeof(CIT::ScalesPair) + sizeof(CIT::ScalesPairData) * 2);

    mOut->mFileSize = mCitSize;

    while (mOut->mFileSize % 0x20)
        mOut->mFileSize++; // add padding

    mOut->mFileData = new char [mOut->mFileSize];
    memset(mOut->mFileData, 0, mOut->mFileSize); // clear memory

    // write header

    mOut->writeU32BE(0); // padding
    mOut->writeU32BE('CITS'); // magic
    mOut->writeU32BE(mCitSize); // size
    mOut->writeU16BE(mChordCount); // chord num
    mOut->writeU16BE(mScalePairsCount); // pair num

    if (!writePtrs())
        return false;

    if (!writeChordData())
        return false;

    if (!writeScaleData())
        return false;

    return true;
}

bool CITWriter::writePtrs() {
    u32 ChordStartPos = sizeof(CIT::Header) + mChordCount * 4 + mScalePairsCount * 4;
    u32 ScalePairStartPos = ChordStartPos + mChordCount * sizeof(CIT::Chords);
    mOut->mCurrentPos = sizeof(CIT::Header);

    // write PTRs

    for (u32 i = 0; i < mChordCount; i++)
        mOut->writeU32BE(ChordStartPos + i * sizeof(CIT::Chords)); // ptr

    for (u32 i = 0; i < mScalePairsCount; i++)
        mOut->writeU32BE(ScalePairStartPos + i * (sizeof(CIT::ScalesPair) + sizeof(CIT::ScalesPairData) * 2)); // ptr

    return true;
}

bool CITWriter::writeChordData() {
    char buffer [256];
    u32 count = 0;
    u32 countCheck = 0;
    u32 ChordStartPos = sizeof(CIT::Header) + mChordCount * 4 + mScalePairsCount * 4;
    u32 Arg;

    mIn->resetPosToStartOfFile();

    while (count < mChordCount) {
        mIn->findNextActionLine();

        while (true) {
            if (mIn->findNextValueInCurrentLine(buffer, 256) && !strcasecmp(buffer, "CHORD"))
                break;

            if (mIn->isReachedEnd())
                return false; // this should technically never occur

            mIn->findNextActionLine();
        }

        Arg = getNextValueInCurrentLine();

        if (Arg >= mChordCount) {
            printf("ERROR: The CHORD 0x%X is higher than the number of CHORD entries! (line %d)\n", Arg, mIn->getCurrentLineNum());
            return false;
        }

        if (Arg == count) {
            u32 CurrentEntryPos = ChordStartPos + count * sizeof(CIT::Chords);
            bool IsFindRoot = false;
            bool IsFindTriad = false;
            bool IsFindExtension = false;

            while (true) {
                if (mIn->isReachedEnd()) {
                    printf("ERROR: Missing arguments for CHORD 0x%X! (line %d)\n", count, mIn->getCurrentLineNum());
                    return false;
                }

                mIn->findNextActionLine();
                findNextValueInCurrentLine(buffer, 256);

                if (!strcasecmp(buffer, "root")) {
                    if (IsFindRoot) {
                        printf("ERROR: Root redefined in CHORD 0x%X! (line %d)\n", count, mIn->getCurrentLineNum());
                        return false;
                    }

                    findNextValueInCurrentLine(buffer, 256);
                    mOut->writeU8(CurrentEntryPos, CIT::convertNoteToValue(buffer));
                    IsFindRoot = true;
                }
                else if (!strcasecmp(buffer, "triad")) {
                    if (IsFindTriad) {
                        printf("ERROR: Triad redefined in CHORD 0x%X! (line %d)\n", count, mIn->getCurrentLineNum());
                        return false;
                    }

                    findNextValueInCurrentLine(buffer, 256);
                    mOut->writeU8(CurrentEntryPos + 1, CIT::convertNoteToValue(buffer));
                    findNextValueInCurrentLine(buffer, 256);
                    mOut->writeU8(CurrentEntryPos + 2, CIT::convertNoteToValue(buffer));
                    findNextValueInCurrentLine(buffer, 256);
                    mOut->writeU8(CurrentEntryPos + 3, CIT::convertNoteToValue(buffer));
                    IsFindTriad = true;
                }
                else if (!strcasecmp(buffer, "extension")) {
                    if (IsFindExtension) {
                        printf("ERROR: Extension redefined in CHORD 0x%X! (line %d)\n", count, mIn->getCurrentLineNum());
                        return false;
                    }

                    findNextValueInCurrentLine(buffer, 256);
                    mOut->writeU8(CurrentEntryPos + 4, CIT::convertNoteToValue(buffer));
                    findNextValueInCurrentLine(buffer, 256);
                    mOut->writeU8(CurrentEntryPos + 5, CIT::convertNoteToValue(buffer));
                    findNextValueInCurrentLine(buffer, 256);
                    mOut->writeU8(CurrentEntryPos + 6, CIT::convertNoteToValue(buffer));
                    findNextValueInCurrentLine(buffer, 256);
                    mOut->writeU8(CurrentEntryPos + 7, CIT::convertNoteToValue(buffer));
                    IsFindExtension = true;
                }
                else {
                    if (!IsFindRoot || !IsFindTriad || !IsFindExtension) {
                        printf("ERROR: Missing arguments for CHORD 0x%X! (line %d)\n", count, mIn->getCurrentLineNum());
                        return false;
                    }

                    break; // found end
                }

                if (IsFindRoot && IsFindTriad && IsFindExtension)
                    break; // found end
            }

            countCheck = 0;
            count++;
            mIn->resetPosToStartOfFile();
        }
        else {
            countCheck++;

            if (countCheck >= mChordCount) {
                printf("ERROR: The CHORD 0x%X could not be found!\n", count);
                return false;
            }
        }
    }
    
    return true;
}

bool CITWriter::writeScaleData() {
    char buffer [256];
    u32 count = 0;
    u32 countCheck = 0;
    u32 ScaleStartPos = sizeof(CIT::Header) + mChordCount * (4 + sizeof(CIT::Chords)) + mScalePairsCount * 4;
    u32 Arg;

    mIn->resetPosToStartOfFile();

    while (count < mScalePairsCount) {
        mIn->findNextActionLine();

        while (true) {
            if (mIn->findNextValueInCurrentLine(buffer, 256) && !strcasecmp(buffer, "SCALE_PAIR"))
                break;

            if (mIn->isReachedEnd())
                return false; // this should technically never occur

            mIn->findNextActionLine();
        }

        Arg = getNextValueInCurrentLine();

        if (Arg >= mScalePairsCount) {
            printf("ERROR: The SCALE_PAIR 0x%X is higher than the number of SCALE_PAIR entries! (line %d)\n", Arg, mIn->getCurrentLineNum());
            return false;
        }

        if (Arg == count) {
            u32 CurrentEntryPos = ScaleStartPos + count * (sizeof(CIT::ScalesPair) + sizeof(CIT::ScalesPairData) * 2);
            u32 FoundScales = 0;

            mOut->writeU32BE(CurrentEntryPos, CurrentEntryPos + 8);
            mOut->writeU32BE(CurrentEntryPos + 4, CurrentEntryPos + 8 + sizeof(CIT::ScalesPairData));

            while (true) {
                if (mIn->isReachedEnd()) {
                    printf("ERROR: Missing arguments for SCALE_PAIR 0x%X! (line %d)\n", count, mIn->getCurrentLineNum());
                    return false;
                }

                mIn->findNextActionLine();
                findNextValueInCurrentLine(buffer, 256);

                if (!strcasecmp(buffer, "scale")) {
                    if (FoundScales >= 2) {
                        printf("ERROR: Scale redefined in SCALE_PAIR 0x%X! (line %d)\n", count, mIn->getCurrentLineNum());
                        return false;
                    }

                    for (u32 i = 0; i < 12; i++) {
                        findNextValueInCurrentLine(buffer, 256);
                        mOut->writeU8(CurrentEntryPos + i + sizeof(CIT::ScalesPair) + FoundScales * sizeof(CIT::ScalesPairData), CIT::convertNoteToValue(buffer));
                    }
                    FoundScales++;
                }
                else {
                    if (FoundScales < 2) {
                        printf("ERROR: Missing arguments for SCALE_PAIR 0x%X! (line %d)\n", count, mIn->getCurrentLineNum());
                        return false;
                    }

                    break; // found end
                }

                if (FoundScales >= 2)
                    break; // found end
            }

            countCheck = 0;
            count++;
            mIn->resetPosToStartOfFile();
        }
        else {
            countCheck++;

            if (countCheck >= mScalePairsCount) {
                printf("ERROR: The SCALE_PAIR 0x%X could not be found!\n", count);
                return false;
            }
        }
    }

    return true;
}

void CITWriter::findNextValueInCurrentLine(char* pStr, u32 size) {
    if (!mIn->findNextValueInCurrentLine(pStr, size)) {
        printf("ERROR: Missing argument in line %d!\n", mIn->getCurrentLineNum());
        exit(1);
    }
}

u32 CITWriter::getNextValueInCurrentLine() {
    char buffer[256];
    u32 Arg = 0;

    findNextValueInCurrentLine(buffer, 256);

    if (!mIn->tryConvertStrToInt(buffer, &Arg)) {
        printf("ERROR: Invalid argument in line %d!\n", mIn->getCurrentLineNum());
        exit(1);
    }

    return Arg;
}