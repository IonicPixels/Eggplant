#pragma once

#include <iostream>
#include "include/Types.h"
#include "include/BinaryReader.h"
#include "include/TextWriter.h"
#include "include/TextReader.h"
#include "include/BSTN.h"
#include "midi-parser.h"

namespace BMS {
    struct OperandCommandTable {
        const char* mCommandName;
        const u16 mParamCount;
        const u16 mParamTypes;
    };
    
    static const OperandCommandTable cCommandTable[] = {
        { 0, 0, 0 }, //_0
        { 0, 0, 0 }, //_1
        { 0, 0, 0 }, //_2
        { 0, 0, 0 }, //_3
        { 0, 0, 0 }, //_4
        { 0, 0, 0 }, //_5
        { 0, 0, 0 }, //_6
        { 0, 0, 0 }, //_7
        { 0, 0, 0 }, //_8
        { 0, 0, 0 }, //_9
        { 0, 0, 0 }, //_A
        { 0, 0, 0 }, //_B
        { 0, 0, 0 }, //_C
        { 0, 0, 0 }, //_D
        { 0, 0, 0 }, //_E
        { 0, 0, 0 }, //_F
        { 0, 0, 0 }, //_10
        { "CMD_NOTE_ON", 3, 0 }, //_11
        { "CMD_NOTE_OFF", 1, 0 }, //_12
        { "CMD_NOTE", 4, 0x40 }, //_13
        { "CMD_SET_LAST_NOTE", 1, 0 }, //_14
        { 0, 0, 0 }, //_15
        { 0, 0, 0 }, //_16
        { 0, 0, 0 }, //_17
        { "CMD_PARAM_E", 2, 0 }, //_18
        { "CMD_PARAM_I", 2, 0x4 }, //_19
        { "CMD_PARAM_EI", 3, 0x10 }, //_1A
        { "CMD_PARAM_II", 3, 0x14 }, //_1B
        { 0, 0, 0 }, //_1C
        { 0, 0, 0 }, //_1D
        { 0, 0, 0 }, //_1E
        { 0, 0, 0 }, //_1F
        { 0, 0, 0 }, //_20
        { "CMD_OPEN_TRACK", 2, 0x8 }, //_21
        { "CMD_CLOSE_TRACK", 1, 0 }, //_22
        { "CMD_CALL", 1, 0x2 }, //_23
        { "CMD_CALL_F", 2, 0x8 }, //_24
        { "CMD_RETURN", 0, 0 }, //_25
        { "CMD_RETURN_F", 1, 0 }, //_26
        { "CMD_JUMP", 1, 0x2 }, //_27
        { "CMD_JUMP_F", 2, 0x8 }, //_28
        { "CMD_JUMP_TABLE", 2, 0xB }, //_29
        { "CMD_CALL_TABLE", 2, 0xB }, //_2A
        { "CMD_LOOP_START", 1, 0x1 }, //_2B
        { "CMD_LOOP_END", 0, 0 }, //_2C
        { 0, 0, 0 }, //_2D
        { 0, 0, 0 }, //_2E
        { 0, 0, 0 }, //_2F
        { "CMD_READ_PORT", 2, 0 }, //_30
        { "CMD_WRITE_PORT", 2, 0xC }, //_31
        { "CMD_CHECK_PORT_IMPORT", 1, 0 }, //_32
        { "CMD_CHECK_PORT_EXPORT", 1, 0 }, //_33
        { "CMD_PARENT_WRITE_PORT", 2, 0xC }, //_34
        { "CMD_CHILD_WRITE_PORT", 2, 0xC }, //_35
        { "CMD_PARENT_READ_PORT", 2, 0 }, //_36
        { "CMD_CHILD_READ_PORT", 2, 0 }, //_37
        { "CMD_REG_LOAD", 2, 0x4 }, //_38
        { "CMD_REG", 3, 0x30 }, //_39
        { "CMD_REG_INT", 3, 0x10 }, //_3A
        { "CMD_REG_UNI", 2, 0 }, //_3B
        { "CMD_REG_TABLE_LOAD", 4, 0xE0 }, //_3C
        { 0, 0, 0 }, //_3D
        { 0, 0, 0 }, //_3E
        { 0, 0, 0 }, //_3F
        { "CMD_TEMPO", 1, 0x1 }, //_40
        { "CMD_BANK_PROGRAM", 2, 0 }, //_41, originally { "CMD_BANK_PROGRAM", 1, 0x1 }
        { "CMD_BANK", 1, 0 }, //_42
        { "CMD_PROGRAM", 1, 0 }, //_43
        { 0, 0, 0 }, //_44
        { 0, 0, 0 }, //_45
        { 0, 0, 0 }, //_46
        { "CMD_ENV_SCALE_SET", 2, 0x4 }, //_47
        { "CMD_ENV_SET", 2, 0x8 }, //_48
        { "CMD_SIMPLE_ADSR", 5, 0x155 }, //_49
        { "CMD_BUS_CONNECT", 2, 0x4 }, //_4A
        { "CMD_IIR_CUT_OFF", 1, 0 }, //_4B
        { "CMD_IIR_SET", 4, 0x55 }, //_4C
        { "CMD_FIR_SET", 1, 0x2 }, //_4D
        { 0, 0, 0 }, //_4E
        { 0, 0, 0 }, //_4F
        { "CMD_WAIT", 0, 0 }, //_50
        { "CMD_WAIT_BYTE", 1, 0 }, //_51
        { 0, 0, 0 }, //_52
        { "CMD_SET_INTERRUPT_TABLE", 1, 0x2 }, //_53
        { "CMD_SET_INTERRUPT", 1, 0x1 }, //_54
        { "CMD_DISRUPT_INTERRUPT", 1, 0x1 }, //_55
        { "CMD_RETURN_INTERRUPT", 0, 0 }, //_56
        { "CMD_CLEAR_INTERRUPT", 0, 0 }, //_57
        { "CMD_INTERRUPT_TIMER", 2, 0x4 }, //_58
        { "CMD_SYNC_CPU", 1, 0x1 }, //_59
        { 0, 0, 0 }, //_5A
        { 0, 0, 0 }, //_5B
        { 0, 0, 0 }, //_5C
        { "CMD_PRINTF", 0, 0 }, //_5D
        { "CMD_NOP", 0, 0 }, //_5E
        { "CMD_FINISH", 0, 0 } //_5F
    };

    #define COMMAND_TABLE_SIZE (sizeof(cCommandTable) / sizeof(OperandCommandTable))

    enum OpcodeList {
        OP_CMD_NOTE_ON = 0xB1,
        OP_CMD_NOTE_OFF = 0xB2,
        OP_CMD_NOTE = 0xB3,
        OP_CMD_SET_LAST_NOTE = 0xB4,

        OP_CMD_PARAM_E = 0xB8,
        OP_CMD_PARAM_I = 0xB9,
        OP_CMD_PARAM_EI = 0xBA,
        OP_CMD_PARAM_II = 0xBB,

        OP_CMD_OPEN_TRACK = 0xC1,
        OP_CMD_CLOSE_TRACK = 0xC2,
        OP_CMD_CALL = 0xC3,
        OP_CMD_CALL_F = 0xC4,
        OP_CMD_RETURN = 0xC5,
        OP_CMD_RETURN_F = 0xC6,
        OP_CMD_JUMP = 0xC7,
        OP_CMD_JUMP_F = 0xC8,
        OP_CMD_JUMP_TABLE = 0xC9,
        OP_CMD_CALL_TABLE = 0xCA,
        OP_CMD_LOOP_START = 0xCB,
        OP_CMD_LOOP_END = 0xCC,

        OP_CMD_READ_PORT = 0xD0,
        OP_CMD_WRITE_PORT = 0xD1,
        OP_CMD_CHECK_PORT_IMPORT = 0xD2,
        OP_CMD_CHECK_PORT_EXPORT = 0xD3,
        OP_CMD_PARENT_WRITE_PORT = 0xD4,
        OP_CMD_CHILD_WRITE_PORT = 0xD5,
        OP_CMD_PARENT_READ_PORT = 0xD6,
        OP_CMD_CHILD_READ_PORT = 0xD7,
        OP_CMD_REG_LOAD = 0xD8,
        OP_CMD_REG = 0xD9,
        OP_CMD_REG_INT = 0xDA,
        OP_CMD_REG_UNI = 0xDB,
        OP_CMD_REG_TABLE_LOAD = 0xDC,

        OP_CMD_TEMPO = 0xE0,
        OP_CMD_BANK_PROGRAM = 0xE1,
        OP_CMD_BANK = 0xE2,
        OP_CMD_PROGRAM = 0xE3,

        OP_CMD_ENV_SCALE_SET = 0xE7,
        OP_CMD_ENV_SET = 0xE8,
        OP_CMD_SIMPLE_ATTACK_DECAY_SUSTAIN_RELEASE = 0xE9,
        OP_CMD_BUS_CONNECT = 0xEA,
        OP_CMD_IIR_CUT_OFF = 0xEB,
        OP_CMD_IIR_SET = 0xEC,
        OP_CMD_FIR_SET = 0xED,

        OP_CMD_WAIT = 0xF0,
        OP_CMD_WAIT_BYTE = 0xF1,

        OP_CMD_SET_INTERRUPT_TABLE = 0xF3,
        OP_CMD_SET_INTERRUPT = 0xF4,
        OP_CMD_DISRUPT_INTERRUPT = 0xF5,
        OP_CMD_RETURN_INTERRUPT = 0xF6,
        OP_CMD_CLEAR_INTERRUPT = 0xF7,
        OP_CMD_INTERRUPT_TIMER = 0xF8,
        OP_CMD_SYNC_CPU = 0xF9,

        OP_CMD_PRINTF = 0xFD,
        OP_CMD_NOP = 0xFE,
        OP_CMD_FINISH = 0xFF
    };

    class Disassembler {
    public:
        enum LabelType {
            LABEL_NULL = 0,
            LABEL_GENERIC,
            LABEL_SOUND,
            LABEL_TRACK,
            LABEL_FUNC,
            LABEL_JUMPTABLE,
            LABEL_CALLTABLE,
            LABEL_SOUND_ARRAY,
            LABEL_REG_TABLE_U8,
            LABEL_REG_TABLE_U16,
            LABEL_REG_TABLE_U24,
            LABEL_REG_TABLE_U32,
            LABEL_REG_TABLE_U32_ABS,
            LABEL_START
        };

        Disassembler(BinaryReader* pBMSBinary, BSTNHolder*);

        inline u16 getNumCategories() const {
            return mBMSBinary->readU16BE(2);
        }

        void init();
        bool disassemble(TextWriter*);
        void writeAndNoticeLabel(u32, LabelType);
        void writeLabelStartWithType(u32);
        const char* getLabelNameByType(u32 adr);

        void writeCategoryList();
        bool parseJASSeq();
        s32 parseNoteOn(u8);
        s32 parseNoteOff(u8);
        s32 parseRegCommand(u8);
        s32 parseCommand(u8, u16);
        u32 readMidiValue();
        void readReg(u32);
        void writeJumpOrCallTable();
        void writeRegTable();
        void parsePrintf();

        // write utils
        void writeStringToTxt(const char* pStr)                              { if (mOutTxt) mOutTxt->writeString(pStr); }
        void writeHexToTxt(u32 num)                                          { if (mOutTxt) mOutTxt->writeHex(num); }
        void writeHexWithSpaceBeforeToTxt(u32 num)                           { if (mOutTxt) mOutTxt->writeHexWithSpaceBefore(num); }
        void writeLineToTxt()                                                { if (mOutTxt) mOutTxt->writeLine(); }
        void writeLabelStartToTxt(const char* pStr, u32 num)                 { if (mOutTxt) mOutTxt->writeLabelStart(pStr, num); }
        void writeLabelRefToTxt(const char* pStr, u32 num)                   { if (mOutTxt) mOutTxt->writeLabelRef(pStr, num); }
        void writeLabelRefWithSpaceBeforeToTxt(const char* pStr, u32 num)    { if (mOutTxt) mOutTxt->writeLabelRefWithSpaceBefore(pStr, num); }
        void writeHeaderMarkerToTxt()                                        { if (mOutTxt) mOutTxt->writeHeaderMarker(); }
        void writeCharToTxt(char c)                                          { if (mOutTxt) mOutTxt->writeChar(c); }

        static void warnBranchAddressREGTakeover(u32 pos) {
            printf("WARNING! At 0x%X a 'REG' is trying to take over the branch address.\nThis might cause errors when rebuilding and the offset of this instruction was changed.\n", pos - 3);
        }

        BinaryReader* mBMSBinary;
        TextWriter* mOutTxt;
        u8* mLabelMask;
        bool* mNoticeMask;
        bool mIsSeBms;
        BSTNHolder* mBSTN;
        u32* mSoundIds;
    };

    class Assembler {
    public:
        class LabelData {
        public:
            LabelData() : mName(0), mAdrInBinary(0), mIsAlreadyRegist(false) {}

            // ONLY INPUT VALUES WITHOUT THE ":"
            void regist(const char* pName, u32 Adr) {
                char* name = new char [strlen(pName) + 1];
                strcpy(name, pName);
                mName = name;
                mAdrInBinary = Adr;
                mIsAlreadyRegist = true;
            }

            const char* mName;
            u32 mAdrInBinary;
            bool mIsAlreadyRegist;
        };
        
        Assembler(TextReader*);
        bool init();
        void createBinaryData(BinaryReader*) const;
        bool assemble(BinaryReader*);
        bool parseCategory();
        void parseTBL(u32 size);
        bool parseJASSeq();
        bool parseOpNoteOn(bool);
        bool parseOpNoteOff();
        bool parseOpReg();
        bool parseOpCmd(const char*, u16 Mask);
        void makeMidiValue(u32);
        bool parsePrintf();
        u32 findNextValueAndConvert(bool*);
        LabelData* findLabelByName(const char*) const;

        TextReader* mInTxt;
        BinaryReader* mBMSBinary;
        u32* mCategoryAdr;
        u32* mNumSounds;
        u32 mNumCategories;
        LabelData* mLabels;
        u32 mNumLabels;
        bool mIsSeBms;
        bool mIsDoneParseLabels;
        u32 mOutBmsSize;
    };

    class Midi2ASM {
    public:
        Midi2ASM(const FileHolder*);

        void resetMidiParser();
        bool findTrack(s32);
        bool parseMidi();

        bool writeOutFile(TextWriter*);
        u64 writeWaitWithDifference(u64 CurrTime, u64 DstTime);

        void findAndSetLoopData();
        inline bool isValidLoopData() const { return mLoopStartTime != 0xFFFFFFFFFFFFFFFF; }

        bool writeStartTrack();
        bool writeRootTrack();
        bool writeTracks();
        void writeTracksSub(s32 Track, s32 SubNote, u32 TrackChannel, s32 SplitNum);
        s32 findNoteSlot(u32 Val, u32 Data);

        bool findMidiMetaEvent(midi_meta);

        const FileHolder* mMidiFile;
        midi_parser mMidi;
        midi_parser_status mMidiStatus;
        TextWriter* mOutTxt;
        u64 mLoopStartTime;
        u64 mLoopEndTime;
        u8 mLastNote[128];
        u8 mChannelSlots[16];
    };
}