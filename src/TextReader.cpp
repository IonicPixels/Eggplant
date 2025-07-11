#include <iostream>
#include <cstring>

#include "include/TextReader.h"

TextReader::TextReader(const char* pName) : FileHolder(pName), mStartOfCurrentLine(0), mEndOfCurrentLine(0), mCurrentPos(0), mCommentChar('#') {}
TextReader::TextReader(const TextReader* pTxt, const char* pCurrentLine) : FileHolder(pTxt), mStartOfCurrentLine(pCurrentLine ? pCurrentLine : pTxt->mFileData), mEndOfCurrentLine(0), mCurrentPos(pTxt->mFileData), mCommentChar(pTxt->mCommentChar) {
    findAndSetEndOfCurrentLine();
}

bool TextReader::loadFile() {
    if (!FileHolder::loadFile())
        return false;

    mStartOfCurrentLine = mFileData;
    mCurrentPos = mFileData;
    findAndSetEndOfCurrentLine();
    return true;
}

void TextReader::findNextActionLineIncludeCurrentLine() {
    findActionLine(true);
}

void TextReader::findNextActionLine() {
    findActionLine(false);
}

bool TextReader::findNextActionLineAfterLabel(const char* pLabel) {
    mStartOfCurrentLine = mFileData;
    mCurrentPos = mFileData;
    findAndSetEndOfCurrentLine();

    char buffer [256];
    bool first = true;
    
    while (true) {
        findActionLine(first);
        if (findNextValueInCurrentLine(buffer, 256) && strlen(buffer) > 1 && buffer[0] == ':' && !strcmp(pLabel, buffer + 1))
            break;

        first = false;

        if (isReachedEnd())
            return false;
    }

    findActionLine(false);
    return true;
}

bool TextReader::findNextValueInCurrentLine(char* pDstOut, u32 MaxStrLen) {
    pDstOut[0] = '\0';

    // try to find next valid char in current line
    while (true) {
        if (mCurrentPos >= mEndOfCurrentLine)
            return false;

        if (!isInvalidChar(mCurrentPos[0]))
            break;

        mCurrentPos++;
    }

    u32 i = 0;
    while (!isInvalidChar(mCurrentPos[i]) && !isCharMarkEndOfLine(mCurrentPos[i]) && i < MaxStrLen - 1) {
        pDstOut[i] = mCurrentPos[i]; // copy str
        i++;
    }

    pDstOut[i] = '\0'; // add nullbyte at end
    mCurrentPos += i;
    return true;
}

bool TextReader::findNextValueInCurrentLineStr(char* pDstOut, u32 MaxStrLen) {
    pDstOut[0] = '\0';

    // try to find next valid char in current line
    while (true) {
        if (mCurrentPos >= mEndOfCurrentLine)
            return false;

        if (!isInvalidChar(mCurrentPos[0]))
            break;

        mCurrentPos++;
    }

    if (mCurrentPos[0] != '\"')
        return false; // not a string

    mCurrentPos++; // enter string

    const char* pEnd = strstr(mCurrentPos, "\"");
    if (!pEnd || pEnd >= mEndOfCurrentLine)
        return false; // string's end out of bounds

    u32 i = 0;
    for (;mCurrentPos < pEnd && i < MaxStrLen - 1; mCurrentPos++)
        pDstOut[i++] = mCurrentPos[0];

    pDstOut[i] = '\0'; // add nullbyte at end
    
    mCurrentPos = pEnd + 1; // set pos to after string end indicator
    return true;
}

bool TextReader::isCurrentLineEmpty() const {
    for (const char* pParse = mStartOfCurrentLine; pParse < mEndOfCurrentLine; pParse++) {
        if (!isInvalidChar(pParse[0]))
            return false;
    }

    return true;
}

bool TextReader::isReachedEnd() const {
    return mEndOfCurrentLine[0] == '\0';
}

u32 TextReader::countValueInFileStr(const char* pValueToSearch) const {
    TextReader read (this, 0);
    char buffer [256];
    u32 i = 0;
    bool first = true;
    
    while (true) {
        read.findActionLine(first);
        if (read.findNextValueInCurrentLine(buffer, 256) && !strcasecmp(pValueToSearch, buffer))
            i++;

        first = false;

        if (read.isReachedEnd())
            return i;
    }
}

u32 TextReader::countLabelsInFile() const {
    TextReader read (this, 0);
    char buffer [4];
    u32 i = 0;
    bool first = true;
    
    while (true) {
        read.findActionLine(first);
        if (read.findNextValueInCurrentLine(buffer, 2) && buffer[0] == ':')
            i++;

        first = false;

        if (read.isReachedEnd())
            return i;
    }
}

u32 TextReader::getCurrentLineNum() const {
    u32 i = 1;

    for (const char* pParse = mFileData; pParse < mStartOfCurrentLine; pParse++) {
        if (pParse[0] == '\n')
            i++;
    }

    return i;
}

void TextReader::resetPosToStartOfFile() {
    mStartOfCurrentLine = mFileData;
    mCurrentPos = mFileData;
    findAndSetEndOfCurrentLine();
}

void TextReader::resetPosToStartOfCurrentLine() {
    mCurrentPos = mStartOfCurrentLine;
}

bool TextReader::tryConvertStrToInt(const char* pStr, u32* pResult) const {
    char* pEnd;
    u32 val = strtoul(pStr, &pEnd, 0);

    if (pEnd == pStr || (!isCharMarkEndOfLine(pEnd[0]) && !isInvalidChar(pEnd[0]))) {
        pResult[0] = 0;
        return false;
    }
    else {
        pResult[0] = val;
        return true;
    }
}








bool TextReader::isCharMarkEndOfLine(char c) {
    return c == '\n' || c == '\0';
}

bool TextReader::isInvalidChar(char c) const {
    return c == ' ' || c == '\t' || c == '\r' || c == mCommentChar;
}

void TextReader::findAndSetEndOfCurrentLine() {
    mEndOfCurrentLine = mStartOfCurrentLine;

    while (true) {
        char c = mEndOfCurrentLine[0];

        if (isCharMarkEndOfLine(c) || c == mCommentChar)
            break;

        mEndOfCurrentLine++; // advance by one char
    }
}

void TextReader::findAndSetStartOfNextLine() {
    if (isReachedEnd())
        return;

    mStartOfCurrentLine = mEndOfCurrentLine;

    // find end of line
    while (!isCharMarkEndOfLine(mStartOfCurrentLine[0]))
        mStartOfCurrentLine++;

    mStartOfCurrentLine++;
    mCurrentPos = mStartOfCurrentLine;
    findAndSetEndOfCurrentLine();
}

void TextReader::findActionLine(bool IncludeCurrent) {
    if (!IncludeCurrent)
        findAndSetStartOfNextLine();

    while (!isReachedEnd() && isCurrentLineEmpty())
        findAndSetStartOfNextLine();
}