#include <iostream>
#include <cstring>
#include <stdarg.h>

#include "include/TextWriter.h"

TextWriter::TextWriter(const char* pFileName) {
    char* pNewFileName = new char [strlen(pFileName) + 1];
    strcpy(pNewFileName, pFileName);
    mFileName = pNewFileName;
}

bool TextWriter::openFile() {
    mOutStream.open(mFileName, std::ios::out);
    return mOutStream.is_open();
}

void TextWriter::closeFile() {
    mOutStream.close();
}



void TextWriter::writeString(const char* pStr) {
    mOutStream << pStr;
}

void TextWriter::writeStringWithSpaceBefore(const char* pStr) {
    mOutStream << " " << pStr;
}

void TextWriter::writePrintf(const char* pFormat, ...) {
    va_list ap;
    char buff [512];

    va_start(ap, pFormat);
    vsnprintf(buff, 512, pFormat, ap);
    va_end(ap);
    writeString(buff);
}

void TextWriter::writeHex(u32 num) {
    writePrintf("0x%X", num);
}

void TextWriter::writeHexWithSpaceBefore(u32 num) {
    writeString(" ");
    writeHex(num);
}

void TextWriter::writeLine() {
    writeString("\n");
}

void TextWriter::writeLabelStart(const char* pStr, u32 num) {
    writePrintf(":%s_%X", pStr, num);
    writeLine();
}

void TextWriter::writeLabelStartAndSub(const char* pStr, u32 num, u32 sub) {
    writePrintf(":%s_%X_sub_%X", pStr, num, sub);
    writeLine();
}

void TextWriter::writeLabelStartSplit(const char* pStr, u32 num, s32 Split) {
    if (Split < 0)
        writeLabelStart(pStr, num);
    else {
        writePrintf(":%s_%X_split_%X", pStr, num, Split);
        writeLine();
    }
}

void TextWriter::writeLabelStartSplitAndSub(const char* pStr, u32 num, u32 sub, s32 Split) {
    if (Split < 0)
        writeLabelStartAndSub(pStr, num, sub);
    else {
        writePrintf(":%s_%X_sub_%X_split_%X", pStr, num, sub, Split);
        writeLine();
    }
}

void TextWriter::writeLabelRef(const char* pStr, u32 num) {
    writePrintf("$%s_%X", pStr, num);
}

void TextWriter::writeLabelRefWithSpaceBefore(const char* pStr, u32 num) {
    writeString(" ");
    writeLabelRef(pStr, num);
}

void TextWriter::writeLabelRefWithSpaceBeforeAndSub(const char* pStr, u32 num, u32 sub) {
    writeLabelRefWithSpaceBefore(pStr, num);
    writePrintf("_sub_%X", sub);
}

void TextWriter::writeLabelRefWithSpaceBeforeSplit(const char* pStr, u32 num, s32 Split) {
    writeLabelRefWithSpaceBefore(pStr, num);
    if (Split >= 0)
        writePrintf("_split_%X", Split);
}

void TextWriter::writeLabelRefWithSpaceBeforeSplitAndSub(const char* pStr, u32 num, u32 sub, s32 Split) {
    writeLabelRefWithSpaceBeforeAndSub(pStr, num, sub);
    if (Split >= 0)
        writePrintf("_split_%X", Split);
}

void TextWriter::writeHeaderMarker() {
    writeLine();
    writeString("####################");
    writeLine();
}

void TextWriter::writeChar(char c) {
    mOutStream << c;
}