#pragma once

#include "include/FileHolder.h"

class TextReader : public FileHolder {
public:
    TextReader(const char*);
    TextReader(const TextReader*, const char* pCurrentLine);

    inline void setCommentChar(char c) { mCommentChar = c; }
    bool loadFile();
    // finds the next action line in the text file, that is not empty, including the current line
    void findNextActionLineIncludeCurrentLine();
    // finds the next action line in the text file, that is not empty
    void findNextActionLine();
    // find next Action Line after label
    bool findNextActionLineAfterLabel(const char*);
    // gets the action value of the line. starts before the first value AND set the line position after it
    bool findNextValueInCurrentLine(char* pDstOut, u32 MaxStrLen);
    // find next value in line string, returns true if succeeded
    bool findNextValueInCurrentLineStr(char* pDstOut, u32 MaxStrLen);
    // find next value in line integer, returns true if succeeded
    bool isCurrentLineEmpty() const;
    // this checks, if we reached the last line!
    bool isReachedEnd() const;
    // count value
    u32 countValueInFileStr(const char* pValueToSearch) const;
    u32 countLabelsInFile() const;
    u32 getCurrentLineNum() const;
    void resetPosToStartOfFile();
    void resetPosToStartOfCurrentLine();
    bool tryConvertStrToInt(const char*, u32*) const;

private:
    // see if the char marks the end of the line
    static bool isCharMarkEndOfLine(char);
    // checks if the char is action text or just empty
    bool isInvalidChar(char) const;
    // find the start of the next line
    void findAndSetStartOfNextLine();
    // find the end of the current line and sets it
    void findAndSetEndOfCurrentLine();
    void findActionLine(bool);

    const char* mStartOfCurrentLine;
    const char* mEndOfCurrentLine;
    const char* mCurrentPos;
    char mCommentChar;
};