#include <cstring>
#include <cmath>

#include "include/BMS.h"

namespace BMS {
    Disassembler::Disassembler(BinaryReader* pBMSBinary, BSTNHolder* pBstn) : mBMSBinary(pBMSBinary), mOutTxt(0), mLabelMask(0), mNoticeMask(0), mIsSeBms(false), mBSTN(pBstn), mSoundIds(0) {}

    void Disassembler::init() {
        mBMSBinary->resetPosition();

        if (mBMSBinary->readU16BE() == 'SC') {
            mIsSeBms = true;
            u32 NumCat = mBMSBinary->readU16BE();

            std::cout << "The SE BMS has " << NumCat << " categories:\n";

            for (u32 i = 0; i < NumCat; i++) {
                // gather number of entries for each category and create array
                mBMSBinary->setPosition(mBMSBinary->readU32BE(8 + i * 4)); // load category array offset
                std::cout << "Category number " << i << " has " << mBMSBinary->readU32BE() << " sound entries.\n";
            }

            std::cout << "\n";
        }
        else {
            mIsSeBms = false;
        }

        // init label mask
        mLabelMask = new u8 [mBMSBinary->mFileSize];
        for (size_t i = 0; i < mBMSBinary->mFileSize; i++)
            mLabelMask[i] = LABEL_NULL;
        
        // init notice mask
        mNoticeMask = new bool [mBMSBinary->mFileSize];
        for (size_t i = 0; i < mBMSBinary->mFileSize; i++)
            mNoticeMask[i] = false;

        // init sound id labels
        if (mBSTN) {
            mSoundIds = new u32 [mBMSBinary->mFileSize];
            for (size_t i = 0; i < mBMSBinary->mFileSize; i++)
                mSoundIds[i] = 0xFFFFFFFF;
        }

        // write label mask
        std::cout << "Creating Labels...\n";

        while (true) {
            if (!disassemble(0))
                break;

            bool isRestart = false;
            for (u32 f = 0; f < mBMSBinary->mFileSize; f++) {
                if (mNoticeMask[f]) {
                    isRestart = true;
                    break;
                }
            }

            if (!isRestart)
                break;
        }

        delete[] mNoticeMask;
        mNoticeMask = 0;
    }



    bool Disassembler::disassemble(TextWriter* pWrt) {
        mOutTxt = pWrt;
        u32 EndOfData = mBMSBinary->mFileSize;
        
        if (mIsSeBms) {
            mBMSBinary->setPosition(8); // set position to after the header
            EndOfData = mBMSBinary->readU32BE(4);
            writeStringToTxt("SEBMS");
            writeLineToTxt();
            writeLineToTxt();

            // add category labels and advance
            for (u32 i = 0; i < getNumCategories(); i++)
                mLabelMask[mBMSBinary->readU32BE()] = LABEL_SOUND_ARRAY;
        }
        else {
            mLabelMask[0] = LABEL_START;
            mBMSBinary->resetPosition();
        }

        while (mBMSBinary->mCurrentPos < EndOfData && !mBMSBinary->mHasReachedEndOfFile) {
            u8 labelType = mLabelMask[mBMSBinary->mCurrentPos];

             // no longer notice address!
            if (mNoticeMask) mNoticeMask[mBMSBinary->mCurrentPos] = false;

            if (labelType != LABEL_NULL) {
                if (labelType == LABEL_SOUND_ARRAY)
                    writeCategoryList();
                else {
                    writeLabelStartWithType(mBMSBinary->mCurrentPos); // create label

                    if (labelType == LABEL_JUMPTABLE || labelType == LABEL_CALLTABLE)
                        writeJumpOrCallTable();
                    else if (labelType == LABEL_REG_TABLE_U8 || labelType == LABEL_REG_TABLE_U16 || labelType == LABEL_REG_TABLE_U24 || labelType == LABEL_REG_TABLE_U32 || labelType == LABEL_REG_TABLE_U32_ABS)
                        writeRegTable();
                    else if (!parseJASSeq())     // the real shit
                        return false;
                }

                writeLineToTxt();
            }
            else
                mBMSBinary->mCurrentPos++; // if no label found, keep parsing
        }

        return true;
    }

    void Disassembler::writeAndNoticeLabel(u32 adr, LabelType type) {
        if (!mOutTxt && mNoticeMask) { // set label type
            if (mLabelMask[adr] == LABEL_NULL)
                mNoticeMask[adr] = true; // notice address!
            
            if (mLabelMask[adr] == LABEL_NULL || (mLabelMask[adr] == LABEL_GENERIC && type != LABEL_GENERIC) || type == LABEL_SOUND)
                mLabelMask[adr] = type; // prevent changing the label name afterwards
        }

        writeLabelRefWithSpaceBeforeToTxt(getLabelNameByType(adr), adr);
    }

    void Disassembler::writeLabelStartWithType(u32 adr) {
        u8 type = mLabelMask[adr];

        if (type != LABEL_GENERIC)
            writeLineToTxt();
        
        writeLabelStartToTxt(getLabelNameByType(adr), adr);
    }

    const char* Disassembler::getLabelNameByType(u32 adr) {
        switch (mLabelMask[adr]) {
            case LABEL_GENERIC:
                return "label";

            case LABEL_SOUND: {
                if (mBSTN && mSoundIds && mSoundIds[adr] != 0xFFFFFFFF) {
                    const char* pSoundName = mBSTN->getSoundNameFromId(mSoundIds[adr]);
                    if (pSoundName)
                        return pSoundName;
                }
                
                return "sound";
            }

            case LABEL_TRACK:
                return "track";

            case LABEL_FUNC:
                return "func";

            case LABEL_JUMPTABLE:
                return "jumptable";
            
            case LABEL_CALLTABLE:
                return "calltable";

            // 7 = sound category table start

            case LABEL_REG_TABLE_U8:
            case LABEL_REG_TABLE_U16:
            case LABEL_REG_TABLE_U24:
            case LABEL_REG_TABLE_U32:
            case LABEL_REG_TABLE_U32_ABS:
                return "regtable";

            case LABEL_START:
                return "start";
        }

        return 0;
    }



    void Disassembler::writeCategoryList() {
        // find category
        u32 cat = 0;
        for (u32 i = 0; i < getNumCategories(); i++) {
            if (mBMSBinary->readU32BE(8 + i * 4) == mBMSBinary->mCurrentPos) {
                cat = i;
                break;
            }
        }

        if (mOutTxt)
            std::cout << "Write category list " << cat << "...\n";
        
        writeHeaderMarkerToTxt();
        writeStringToTxt("CATEGORY");
        writeHexWithSpaceBeforeToTxt(cat);
        writeHeaderMarkerToTxt();

        if (mBSTN) {
            const char* pCategoryName = mBSTN->getCategoryNameFromId(cat << 16);
            if (pCategoryName) {
                writeLineToTxt();
                writeStringToTxt("# ");
                writeStringToTxt(pCategoryName);
                writeLineToTxt();
            }
        }
        
        u32 num = mBMSBinary->readU32BE();

        for (u32 i = 0; i < num; i++) {
            u32 SoundAdr = mBMSBinary->readU32BE();
            if (mSoundIds) {
                if (mSoundIds[SoundAdr] != 0xFFFFFFFF)
                    std::cout << "WARNING! Two or more sounds use the same sound data!\n";
                else
                    mSoundIds[SoundAdr] = (cat << 16) | (i & 0xFFFF);
            }

            writeLineToTxt();
            writeStringToTxt("SOUND");
            writeHexWithSpaceBeforeToTxt(i);
            writeAndNoticeLabel(SoundAdr, LABEL_SOUND);
        }
    }

    bool Disassembler::parseJASSeq() {
        while (!mBMSBinary->mHasReachedEndOfFile) {
            u8 opcode = mBMSBinary->readU8();
            s32 out = 0;

            if (opcode < 0x80)
                out = parseNoteOn(opcode);
            else if (opcode < 0x90)
                out = parseNoteOff(opcode & 0b00001111);
            else if (opcode < 0xA0)
                out = parseRegCommand((opcode & 0b00000111) + 1);
            else
                out = parseCommand(opcode, 0);
            
            if (out > 0)
                return out == 1;
            
            if (mLabelMask[mBMSBinary->mCurrentPos] != LABEL_NULL)
                return true;
            
            writeLineToTxt();
        }

        std::cout << "WARNING! Reached end of file without finishing the last set of instructions!\n";
        return false;
    }
    
    s32 Disassembler::parseNoteOn(u8 note) {
        u32 Flags = mBMSBinary->readU8();
        if ((Flags & 0b00000111) == 0) {
            writeStringToTxt("NOTE_ON_GATE");
            writeHexWithSpaceBeforeToTxt(note);
            writeHexWithSpaceBeforeToTxt(Flags >> 3); // other flags
            writeHexWithSpaceBeforeToTxt(mBMSBinary->readU8()); // velocity
            writeHexWithSpaceBeforeToTxt(readMidiValue()); // duration
        }
        else {
            writeStringToTxt("NOTE_ON");
            writeHexWithSpaceBeforeToTxt(note);
            writeHexWithSpaceBeforeToTxt(Flags & 0b00000111); // voice ID
            //writeHexWithSpaceBeforeToTxt(Flags >> 3); // other flags
            writeHexWithSpaceBeforeToTxt(mBMSBinary->readU8()); // velocity
        }

        return 0;
    }

    s32 Disassembler::parseNoteOff(u8 cmd) {
        writeStringToTxt("NOTE_OFF");
        writeHexWithSpaceBeforeToTxt(cmd);
        return 0;
    }
    
    s32 Disassembler::parseRegCommand(u8 cmd) {
        u32 outMask = 0;
        u32 value = 0b00000011;
        u32 mask = mBMSBinary->readU8();

        writeStringToTxt("REG");
        writeHexWithSpaceBeforeToTxt(cmd);
        writeHexWithSpaceBeforeToTxt(mask);
        writeStringToTxt(" ");

        for (u32 i = 0; i < cmd; i++) {
            if ((mask & 0b10000000) != 0)
                outMask |= value;
            
            mask = (mask & 0b01111111) << 1;
            value = (value & 0b0011111111111111) << 2;
        }
        
        return parseCommand(mBMSBinary->readU8(), outMask);
    }

    s32 Disassembler::parseCommand(u8 opcode, u16 RegisterMask) {
        const char* pName = cCommandTable[opcode - 0xA0].mCommandName;
        if (!pName) {
            printf("ERROR! Invalid or unsupported opcode %d %X at 0x%X!\n", opcode, opcode, mBMSBinary->mCurrentPos);
            return 2;
        }

        u32 uIntBuffer [16];
        u32 pos = mBMSBinary->mCurrentPos;
        u32 ParamCount = cCommandTable[opcode - 0xA0].mParamCount;
        u32 ParamTypes = cCommandTable[opcode - 0xA0].mParamTypes | RegisterMask;
        writeStringToTxt(pName);

        for (u32 i = 0; i < ParamCount; i++) {
            u32 val = 0;
            switch (ParamTypes & 0b00000011) {
                case 0:
                    val = mBMSBinary->readU8();
                    break;

                case 1:
                    val = mBMSBinary->readU16BE();
                    break;

                case 2:
                    val = mBMSBinary->readU24BE();
                    break;

                case 3:
                    val = mBMSBinary->readU8(); // usually reads from register
                    break;
            }

            uIntBuffer[i] = val;
            ParamTypes = ParamTypes >> 2;
        }

        switch (opcode) {
            // notes
            case OP_CMD_NOTE_ON: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]); // key
                writeHexWithSpaceBeforeToTxt(uIntBuffer[1]); // voice ID
                writeHexWithSpaceBeforeToTxt(uIntBuffer[2]); // velocity
                break;
            }

            case OP_CMD_NOTE: {
                if ((RegisterMask & 0b00000011) != 0b00000011) // make sure not to shift the register value
                    writeHexWithSpaceBeforeToTxt(uIntBuffer[0] >> 3); // flags
                else
                    writeHexWithSpaceBeforeToTxt(uIntBuffer[0]); // flags from reg
                
                writeHexWithSpaceBeforeToTxt(uIntBuffer[1]); // key
                writeHexWithSpaceBeforeToTxt(uIntBuffer[2]); // velocity
                writeHexWithSpaceBeforeToTxt(uIntBuffer[3]); // duration
                break;
            }

            case OP_CMD_SET_LAST_NOTE: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                break;
            }





            // parameters
            case OP_CMD_PARAM_E: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[1]);
                break;
            }

            case OP_CMD_PARAM_I: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[1]);
                break;
            }

            case OP_CMD_PARAM_EI: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[1]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[2]);
                break;
            }

            case OP_CMD_PARAM_II: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[1]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[2]);
                break;
            }






            // branches
            case OP_CMD_OPEN_TRACK: {
                if (RegisterMask & 0b00001100)
                    warnBranchAddressREGTakeover(pos);
                
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeAndNoticeLabel(uIntBuffer[1], LABEL_TRACK);
                break;
            }

            case OP_CMD_CLOSE_TRACK: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                break;
            }

            case OP_CMD_CALL: {
                if (RegisterMask > 0)
                    warnBranchAddressREGTakeover(pos);

                writeAndNoticeLabel(uIntBuffer[0], LABEL_FUNC);
                break;
            }

            case OP_CMD_CALL_F: {
                if (RegisterMask & 0b00001100)
                    warnBranchAddressREGTakeover(pos);

                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeAndNoticeLabel(uIntBuffer[1], LABEL_FUNC);
                break;
            }

            case OP_CMD_RETURN: {
                return 1;
            }

            case OP_CMD_JUMP: {
                if (RegisterMask > 0)
                    warnBranchAddressREGTakeover(pos);

                writeAndNoticeLabel(uIntBuffer[0], LABEL_GENERIC);
                return 1;
            }

            case OP_CMD_JUMP_F: {
                if (RegisterMask & 0b00001100)
                    warnBranchAddressREGTakeover(pos);

                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeAndNoticeLabel(uIntBuffer[1], LABEL_GENERIC);
                break;
            }

            case OP_CMD_JUMP_TABLE: { // a real bitch
                if (RegisterMask & 0b00001100)
                    warnBranchAddressREGTakeover(pos);

                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeAndNoticeLabel(uIntBuffer[1], LABEL_JUMPTABLE);
                return 1;
            }

            case OP_CMD_CALL_TABLE: { // a real bitch
                if (RegisterMask & 0b00001100)
                    warnBranchAddressREGTakeover(pos);

                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeAndNoticeLabel(uIntBuffer[1], LABEL_CALLTABLE);
                break;
            }






            // registers
            case OP_CMD_READ_PORT: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[1]);
                break;
            }

            case OP_CMD_WRITE_PORT: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[1]);
                break;
            }

            case OP_CMD_CHILD_WRITE_PORT: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[1]);
                break;
            }

            case OP_CMD_PARENT_READ_PORT: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[1]);
                break;
            }

            case OP_CMD_REG_LOAD: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[1]);
                break;
            }

            case OP_CMD_REG: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[1]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[2]);
                break;
            }

            case OP_CMD_REG_INT: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[1]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[2]);
                break;
            }

            case OP_CMD_REG_UNI: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[1]);
                break;
            }

            case OP_CMD_REG_TABLE_LOAD: { // even more of a bitch
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]); // type, 12 = u8, 13 = u16, 14 = u24, 15 = u32, 16 = u32 absolute
                LabelType label = LABEL_REG_TABLE_U8;
                switch (uIntBuffer[0]) {
                    case 12:
                        break;
                    case 13:
                        label = LABEL_REG_TABLE_U16;
                        break;
                    case 14:
                        label = LABEL_REG_TABLE_U24;
                        break;
                    case 15:
                        label = LABEL_REG_TABLE_U32;
                        break;
                    case 16:
                        label = LABEL_REG_TABLE_U32_ABS;
                        break;
                    default:
                        printf("ERROR! An illegal REG_TABLE type has been found! %d %X at 0x%X\n", uIntBuffer[0], uIntBuffer[0], pos);
                        return 2;
                }
                writeHexWithSpaceBeforeToTxt(uIntBuffer[1]); // targer register
                writeAndNoticeLabel(uIntBuffer[2], label); // address
                writeHexWithSpaceBeforeToTxt(uIntBuffer[3]); // register to for loading
                break;
            }






            // midi events
            case OP_CMD_TEMPO: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                break;
            }

            case OP_CMD_BANK_PROGRAM: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[1]);
                break;
            }

            case OP_CMD_BANK: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                break;
            }

            case OP_CMD_PROGRAM: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                break;
            }

            case OP_CMD_SIMPLE_ATTACK_DECAY_SUSTAIN_RELEASE: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[1]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[2]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[3]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[4]);
                break;
            }

            case OP_CMD_BUS_CONNECT: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                writeHexWithSpaceBeforeToTxt(uIntBuffer[1]);
                break;
            }

            case OP_CMD_IIR_CUT_OFF: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                break;
            }






            // misc
            case OP_CMD_WAIT: {
                writeHexWithSpaceBeforeToTxt(readMidiValue());
                break;
            }

            case OP_CMD_WAIT_BYTE: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                break;
            }

            case OP_CMD_SYNC_CPU: {
                writeHexWithSpaceBeforeToTxt(uIntBuffer[0]);
                break;
            }

            case OP_CMD_PRINTF: { // dog shit function
                parsePrintf();
                break;
            }

            case OP_CMD_FINISH: {
                return 1;
            }

            default: {
                printf("ERROR! Unimplemented opcode %d %X at 0x%X\n", opcode, opcode, pos - 1);
                return 2;
            }
        }
        
        return 0;
    }

    u32 Disassembler::readMidiValue() {
        u32 midiVal = mBMSBinary->readU8();
        if ((midiVal & 0b10000000) == 0)
            return midiVal;

        midiVal &= 0b01111111;

        for (u32 i = 0; i <= 2; i++) {
            u8 NextVal = mBMSBinary->readU8();
            midiVal = (midiVal << 7) | (NextVal & 0b01111111);
            if ((NextVal & 0b10000000) == 0)
                break;
        }

        return midiVal;
    }

    void Disassembler::readReg(u32 val) {
        writeHexWithSpaceBeforeToTxt(val); // seems not to advance the array
    }

    void Disassembler::writeJumpOrCallTable() {
        u32 DataSize = mBMSBinary->readU32BE(4);
        bool IsCallTable = mLabelMask[mBMSBinary->mCurrentPos] == LABEL_CALLTABLE;
        writeStringToTxt("TBL24");

        while (true) {
            u32 read = mBMSBinary->readU24BE();

            if (read >= DataSize || mBMSBinary->mCurrentPos >= DataSize || mBMSBinary->mHasReachedEndOfFile) {
                mBMSBinary->mCurrentPos -= 3; // set pos back
                return;
            }

            writeAndNoticeLabel(read, IsCallTable ? LABEL_FUNC : LABEL_GENERIC);

            if (mLabelMask[mBMSBinary->mCurrentPos] != LABEL_NULL) {
                return;
            }
        }
    }

    void Disassembler::writeRegTable() {
        u32 DataSize = mBMSBinary->readU32BE(4);
        u8 LabelType = mLabelMask[mBMSBinary->mCurrentPos];
        u32 TypeSize = 1;
        const char* pTableType = "TBL8";

        switch (LabelType) {
            case LABEL_REG_TABLE_U8:
                break;
            case LABEL_REG_TABLE_U16:
                TypeSize = 2;
                pTableType = "TBL16";
                break;
            case LABEL_REG_TABLE_U24:
                TypeSize = 3;
                pTableType = "TBL24";
                break;
            case LABEL_REG_TABLE_U32:
            case LABEL_REG_TABLE_U32_ABS:
                TypeSize = 4;
                pTableType = "TBL32";
                break;
            default:
                return; // invalid type!
        }

        writeStringToTxt(pTableType);

        while (true) {
            u32 before = mBMSBinary->mCurrentPos;
            u32 read = mBMSBinary->readValueBE(TypeSize);

            if (mBMSBinary->mCurrentPos >= DataSize || mBMSBinary->mHasReachedEndOfFile) {
                mBMSBinary->mCurrentPos = before; // set pos back
                return;
            }

            writeHexWithSpaceBeforeToTxt(read);

            if (mLabelMask[mBMSBinary->mCurrentPos] != LABEL_NULL) {
                return;
            }
        }
    }

    void Disassembler::parsePrintf() {
        u32 argCount = 0;
        writeStringToTxt(" \"");
        
        while (true) {
            u8 c = mBMSBinary->readU8();

            if (c == '\0')
                break;
            else {
                if (c == '%')
                    argCount++;
                
                writeCharToTxt(c);
            }
        }

        writeStringToTxt("\"");

        for (u32 i = 0; i < argCount; i++)
            writeHexWithSpaceBeforeToTxt(mBMSBinary->readU8());
    }













    Assembler::Assembler(TextReader* pTxt) : mInTxt(pTxt), mBMSBinary(0), mCategoryAdr(0), mNumSounds(0), mNumCategories(0), mLabels(0), mNumLabels(0), mIsSeBms(false), mIsDoneParseLabels(false), mOutBmsSize(0) {}

    bool Assembler::init() {
        char buffer [256];
        BinaryReader reader;
        
        mIsDoneParseLabels = false;
        mInTxt->resetPosToStartOfFile();
        mInTxt->findNextActionLineIncludeCurrentLine();

        if (mInTxt->findNextValueInCurrentLine(buffer, 256) && !strcasecmp(buffer, "SEBMS")) {
            mNumCategories = mInTxt->countValueInFileStr("CATEGORY");
            mCategoryAdr = new u32 [mNumCategories];
            mNumSounds = new u32 [mNumCategories];

            for (u32 i = 0; i < mNumCategories; i++) {
                mCategoryAdr[i] = 0xFFFFFFFF;
                mNumSounds[i] = 0x0;
            }

            std::cout << "The input file has " << mNumCategories << " sound categories.\n";

            mIsSeBms = true;
        }
        else {
            mIsSeBms = false;
        }

        std::cout << "Creating label data...\n";
        mNumLabels = mInTxt->countLabelsInFile();
        mLabels = new LabelData [mNumLabels];

        if (!assemble(&reader))
            return false;

        mOutBmsSize = reader.mCurrentPos;
        mIsDoneParseLabels = true;
        return true;
    }

    void Assembler::createBinaryData(BinaryReader* pBin) const {
        size_t size = mOutBmsSize;

        while (size % 0x20) // add padding
            size++;

        pBin->mFileData = new char [size];
        memset(pBin->mFileData, 0, size);
        pBin->mFileSize = size;
    }

    bool Assembler::assemble(BinaryReader* pWriter) {
        mBMSBinary = pWriter;
        mBMSBinary->resetPosition();
        mInTxt->resetPosToStartOfFile();

        if (mIsSeBms) {
            mBMSBinary->writeU16BE('SC');
            mBMSBinary->writeU16BE(mNumCategories);
            mBMSBinary->writeU32BE(mOutBmsSize);

            for (u32 i = 0; i < mNumCategories; i++)
                mBMSBinary->writeU32BE(mCategoryAdr[i]);
        }

        bool isFirst = true;
        char buffer [256];
        u32 NumDiscLabels = 0;

        // do main parse loop
        do {
            if (isFirst) {
                mInTxt->findNextActionLineIncludeCurrentLine();
                isFirst = false;
            }
            else
                mInTxt->findNextActionLine();

            SkipToNextLine:
            if (mInTxt->isCurrentLineEmpty())
                break;
            
            mInTxt->findNextValueInCurrentLine(buffer, 256);

            if (!strcasecmp(buffer, "SEBMS")) {
                // skip
            }
            else if (buffer[0] == ':') { // only register when in first parse
                if (!mIsDoneParseLabels && NumDiscLabels < mNumLabels)
                    mLabels[NumDiscLabels++].regist(buffer + 1, mBMSBinary->mCurrentPos); // register label
            }
            else if (mIsSeBms && !strcasecmp(buffer, "CATEGORY")) {
                if (!parseCategory())
                    return false;
                else
                    goto SkipToNextLine;
            }
            else if (!strcasecmp(buffer, "TBL8")) {
                parseTBL(1);
            }
            else if (!strcasecmp(buffer, "TBL16")) {
                parseTBL(2);
            }
            else if (!strcasecmp(buffer, "TBL24")) {
                parseTBL(3);
            }
            else if (!strcasecmp(buffer, "TBL32")) {
                parseTBL(4);
            }
            else if (!parseJASSeq()) {
                return false;
            }

            if (mInTxt->findNextValueInCurrentLine(buffer, 1)) {
                std::cout << "ERROR! Line " << mInTxt->getCurrentLineNum() << ": There are more arguments than expected.\n";
                return false;
            }
            
        } while (!mInTxt->isReachedEnd());

        return true;
    }






    bool Assembler::parseCategory() {
        u32 catNum = findNextValueAndConvert(0);

        if (mIsDoneParseLabels)
            std::cout << "Writing category " << catNum << " with " << mNumSounds[catNum] << " sounds in line " << mInTxt->getCurrentLineNum() << "...\n";

        if (catNum >= mNumCategories) {
            std::cout << "ERROR! Line " << mInTxt->getCurrentLineNum() << ": The category uses an invalid ID \"" << catNum << "\"!\n";
            return false;
        }

        if (!mIsDoneParseLabels && mCategoryAdr[catNum] != 0xFFFFFFFF) { // only when parsing first time
            std::cout << "ERROR! Line " << mInTxt->getCurrentLineNum() << ": The category \"" << catNum << "\" has already been written!\nMake sure you don't use an ID twice.\n";
            return false;
        }

        mCategoryAdr[catNum] = mBMSBinary->mCurrentPos;
        mBMSBinary->writeU32BE(0); // num sounds dummy, to advance the pos
        char buff[32];
        u32 numSounds = 0;
        u32 pos = mBMSBinary->mCurrentPos;

        do {
            mInTxt->findNextActionLine();
            if (mInTxt->findNextValueInCurrentLine(buff, 32)) {
                if (!strcasecmp(buff, "SOUND")) {
                    numSounds++;

                    if (mIsDoneParseLabels) {
                        u32 SndID = findNextValueAndConvert(0);
                        u32 SndAdr = findNextValueAndConvert(0);

                        if (mBMSBinary->readU32BE(pos + SndID * 4) != 0x0) {
                            std::cout << "ERROR! Line " << mInTxt->getCurrentLineNum() << ": The sound \"" << SndID << " has already been written!\nMake sure you don't use an ID twice.\n";
                            return false;
                        }
                        else if (SndID >= mNumSounds[catNum]) {
                            std::cout << "ERROR! Line " << mInTxt->getCurrentLineNum() << ": The sound uses an invalid ID \"" << SndID << "\"!\n";
                            return false;
                        }

                        mBMSBinary->writeU32BE(pos + SndID * 4, SndAdr);
                    }
                    
                    mBMSBinary->mCurrentPos += 4; // advance pos
                }
                else
                    break;
            }

        } while (!mInTxt->isReachedEnd());

        mBMSBinary->writeU32BE(mCategoryAdr[catNum], numSounds); // write num of sounds
        mNumSounds[catNum] = numSounds;
        mInTxt->resetPosToStartOfCurrentLine();
        return true;
    }

    void Assembler::parseTBL(u32 size) {
        bool isNotFoundEnd;

        while (true) {
            u32 val = findNextValueAndConvert(&isNotFoundEnd);
            if (!isNotFoundEnd)
                return;
            
            mBMSBinary->writeValueBE(size, val);
        }
    }

    bool Assembler::parseJASSeq() {
        char buff [32];
        mInTxt->resetPosToStartOfCurrentLine();
        mInTxt->findNextValueInCurrentLine(buff, 32);
        
        if (!strcasecmp(buff, "NOTE_ON")) {
            return parseOpNoteOn(false);
        }
        else if (!strcasecmp(buff, "NOTE_ON_GATE")) {
            return parseOpNoteOn(true);
        }
        else if (!strcasecmp(buff, "NOTE_OFF")) {
            return parseOpNoteOff();
        }
        else if (!strcasecmp(buff, "REG")) {
            return parseOpReg();
        }
        else {
            return parseOpCmd(buff, 0);
        }
    }
    
    bool Assembler::parseOpNoteOn(bool isGate) {
        u32 val = findNextValueAndConvert(0);
        if (val >= 0x80) {
            std::cout << "ERROR! Line " << mInTxt->getCurrentLineNum() << ": The Note \"" << val << "\" is out of range!\nOnly use values between 0 and 127.\n";
            return false;
        }
        mBMSBinary->writeU8(val);

        if (isGate)
            mBMSBinary->writeU8(findNextValueAndConvert(0) << 3); // flag
        else {
            val = findNextValueAndConvert(0);
            if (val > 7 || val == 0) {
                std::cout << "ERROR! Line " << mInTxt->getCurrentLineNum() << ": The Voice ID \"" << val << "\" is out of range!\nOnly use values between 1 and 7.\n";
                return false;
            }
            mBMSBinary->writeU8(val);
        }

        val = findNextValueAndConvert(0);
        if (val >= 0x80) {
            std::cout << "ERROR! Line " << mInTxt->getCurrentLineNum() << ": The Velocity \"" << val << "\" is out of range!\nOnly use values between 0 and 127.\n";
            return false;
        }
        mBMSBinary->writeU8(val);

        if (isGate)
            makeMidiValue(findNextValueAndConvert(0)); // duration

        return true;
    }
    
    bool Assembler::parseOpNoteOff() {
        u32 val = findNextValueAndConvert(0);

        if (val > 7 || val == 0) {
            std::cout << "ERROR! Line " << mInTxt->getCurrentLineNum() << ": The Voice ID \"" << val << "\" is out of range!\nOnly use values between 1 and 7.\n";
            return false;
        }

        mBMSBinary->writeU8(val | 0x80);
        return true;
    }
    
    bool Assembler::parseOpReg() {
        u32 iter = findNextValueAndConvert(0);
        if (iter > 8 || iter == 0) {
            std::cout << "ERROR! Line " << mInTxt->getCurrentLineNum() << ": The Iteration number \"" << iter << "\" is out of range!\nOnly use values between 1 and 8.\n";
            return false;
        }
        mBMSBinary->writeU8((iter - 1) | 0x90);

        u32 outMask = 0;
        u32 value = 0b00000011;
        u32 mask = findNextValueAndConvert(0);
        mBMSBinary->writeU8(mask);

        for (u32 i = 0; i < iter; i++) {
            if ((mask & 0b10000000) != 0)
                outMask |= value;
            
            mask = (mask & 0b01111111) << 1;
            value = (value & 0b0011111111111111) << 2;
        }

        char buff [32];
        mInTxt->findNextValueInCurrentLine(buff, 32);
        return parseOpCmd(buff, outMask);
    }
    
    bool Assembler::parseOpCmd(const char* pOp, u16 Mask) {
        // search in table
        u32 i = 0;
        while (true) {
            if (i >= COMMAND_TABLE_SIZE) {
                std::cout << "ERROR! Line " << mInTxt->getCurrentLineNum() << ": The operator \"" << pOp << "\" is invalid!\n";
                return false;
            }

            if (cCommandTable[i].mCommandName && !strcasecmp(cCommandTable[i].mCommandName, pOp))
                break;
            
            i++;
        }

        u32 ParamCount = cCommandTable[i].mParamCount;
        u32 ParamTypes = cCommandTable[i].mParamTypes | Mask;
        i += 0xA0;
        mBMSBinary->writeU8(i);

        switch (i) {
            // notes
            case OP_CMD_NOTE_ON:
                break;

            case OP_CMD_NOTE: {
                if ((Mask & 0b00000011) != 0b00000011) { // only write when the mask is not forcing the flag to be a register
                    mBMSBinary->writeU8(findNextValueAndConvert(0) << 3); // flags
                    ParamCount--;
                    ParamTypes = ParamTypes >> 2;
                }
                break;
            }

            case OP_CMD_SET_LAST_NOTE:
                break;




            // parameters
            case OP_CMD_PARAM_E:
            case OP_CMD_PARAM_I:
            case OP_CMD_PARAM_EI:
            case OP_CMD_PARAM_II:
                break;





            // branches
            case OP_CMD_OPEN_TRACK:
            case OP_CMD_CALL:
            case OP_CMD_CALL_F:
            case OP_CMD_RETURN:
            case OP_CMD_JUMP:
            case OP_CMD_JUMP_F:
            case OP_CMD_JUMP_TABLE:
            case OP_CMD_CALL_TABLE:
                break;






            // registers
            case OP_CMD_READ_PORT:
            case OP_CMD_WRITE_PORT:
            case OP_CMD_CHILD_WRITE_PORT:
            case OP_CMD_PARENT_READ_PORT:
            case OP_CMD_REG_LOAD:
            case OP_CMD_REG:
            case OP_CMD_REG_INT:
            case OP_CMD_REG_UNI:
            case OP_CMD_REG_TABLE_LOAD:
                break;






            // midi events
            case OP_CMD_TEMPO:
            case OP_CMD_BANK_PROGRAM:
            case OP_CMD_BANK:
            case OP_CMD_PROGRAM:
            case OP_CMD_SIMPLE_ATTACK_DECAY_SUSTAIN_RELEASE:
            case OP_CMD_BUS_CONNECT:
            case OP_CMD_IIR_CUT_OFF:
            case OP_CMD_IIR_SET:
            case OP_CMD_FIR_SET:
                break;






            // misc
            case OP_CMD_WAIT: {
                makeMidiValue(findNextValueAndConvert(0));
                return true;
            }

            case OP_CMD_WAIT_BYTE:
                break;

            case OP_CMD_PRINTF: {
                return parsePrintf();
            }

            case OP_CMD_FINISH:
                break;

            default: {
                std::cout << "ERROR! Line " << mInTxt->getCurrentLineNum() << ": Unimplemented opcode \"" << pOp << "\n.";
                return false;
            }
        }

        for (u32 i = 0; i < ParamCount; i++) {
            u32 val = findNextValueAndConvert(0);
            switch (ParamTypes & 0b00000011) {
                case 0:
                    mBMSBinary->writeU8(val);
                    break;

                case 1:
                    mBMSBinary->writeU16BE(val);
                    break;

                case 2:
                    mBMSBinary->writeU24BE(val);
                    break;

                case 3:
                    mBMSBinary->writeU8(val); // usually reads from register
                    break;
            }

            ParamTypes = ParamTypes >> 2;
        }

        return true;
    }

    void Assembler::makeMidiValue(u32 midiVal) {
        // lazy mode
        if (midiVal < 0x80)
            mBMSBinary->writeU8(midiVal);
        else if (midiVal < 0x4000) {
            mBMSBinary->writeU8((midiVal >> 7) | 0b10000000);
            mBMSBinary->writeU8(midiVal & 0b01111111);
        }
        else if (midiVal < 0x200000) {
            mBMSBinary->writeU8((midiVal >> 14) | 0b10000000);
            mBMSBinary->writeU8((midiVal >> 7) | 0b10000000);
            mBMSBinary->writeU8(midiVal & 0b01111111);
        }
        else {
            mBMSBinary->writeU8((midiVal >> 21) | 0b10000000);
            mBMSBinary->writeU8((midiVal >> 14) | 0b10000000);
            mBMSBinary->writeU8((midiVal >> 7) | 0b10000000);
            mBMSBinary->writeU8(midiVal & 0b01111111);
        }
    }

    bool Assembler::parsePrintf() {
        char buff [512];

        // copy string
        
        if (!mInTxt->findNextValueInCurrentLineStr(buff, 512)) {
            std::cout << "ERROR! Line " << mInTxt->getCurrentLineNum() << ": String is not valid or non-existent!\n";
            return false;
        }

        u32 len = strlen(buff) + 1;
        for (u32 i = 0; i < len; i++)
            mBMSBinary->writeU8(buff[i]);

        bool needKeepLooking = false;
        
        while (true) {
            len = findNextValueAndConvert(&needKeepLooking);

            if (!needKeepLooking)
                break;
            
            mBMSBinary->writeU8(len);
        }

        return true;
    }



    u32 Assembler::findNextValueAndConvert(bool* pIsFound) {
        char buffer [256];
        
        if (!mInTxt->findNextValueInCurrentLine(buffer, 256)) {
            if (!pIsFound) {
                std::cout << "ERROR! Line " << mInTxt->getCurrentLineNum() << ": Too few arguments!\n";
                exit(1);
            }
            else
                pIsFound[0] = false;
            
            return 0;
        }
        
        if (pIsFound) pIsFound[0] = true;

        // get label address, if value is a label
        if (buffer[0] == '$') {
            LabelData* pLabel = findLabelByName(buffer + 1);

            if (!pLabel) {
                if (mIsDoneParseLabels) {
                    std::cout << "ERROR! Line " << mInTxt->getCurrentLineNum() << ": The label \"" << (buffer + 1) << "\" was not found!\n";
                    exit(1);
                }
                return 0;
            }

            return pLabel->mAdrInBinary;
        }

        u32 val = 0;

        if (!mInTxt->tryConvertStrToInt(buffer, &val)) {
            std::cout << "ERROR! Line " << mInTxt->getCurrentLineNum() << ": Invalid argument \"" << buffer << "\"!\n";
            exit(1);
        }

        return val;
    }

    Assembler::LabelData* Assembler::findLabelByName(const char* pName) const {
        for (u32 i = 0; i < mNumLabels; i++) {
            if (mLabels[i].mName && !strcmp(mLabels[i].mName, pName))
                return &mLabels[i];
        }

        return 0;
    }















    Midi2ASM::Midi2ASM(const FileHolder* pPar) : mMidiFile(pPar), mMidiStatus(MIDI_PARSER_EOB), mOutTxt(0), mLoopStartTime(0xFFFFFFFFFFFFFFFF), mLoopEndTime(0xFFFFFFFFFFFFFFFF) {
        for (s32 i = 0; i < 16; i++)
            mChannelSlots[i] = 0;
        
        resetMidiParser();
    }

    void Midi2ASM::resetMidiParser() {
        mMidi.state = MIDI_PARSER_INIT;
        mMidi.size = mMidiFile->mFileSize;
        mMidi.in = (const u8*) mMidiFile->mFileData;
    }

    bool Midi2ASM::findTrack(s32 i) {
        resetMidiParser();
        s32 FoundTrack = 0;

        while (true) {
            if (!parseMidi()) {
                printf("ERROR: Track %d was not found in midi!\n", i);
                return false;
            }
            else if (mMidiStatus == MIDI_PARSER_TRACK) {
                if (FoundTrack == i)
                    return true;
                else
                    FoundTrack++;
            }
        }
    }

    bool Midi2ASM::parseMidi() {
        mMidi.vtime = 0;
        mMidiStatus = midi_parse(&mMidi);

        if (mMidiStatus == MIDI_PARSER_ERROR) {
            printf("ERROR: An error occured while parsing the midi!\nYour midi file might not be supported.\n");
            return false;
        }

        if (mMidiStatus == MIDI_PARSER_EOB)
            return false;

        return true;
    }




    bool Midi2ASM::writeOutFile(TextWriter* pTxt) {
        mOutTxt = pTxt;

        findAndSetLoopData();

        if (!writeStartTrack())
            return false;
        
        if (!writeRootTrack())
            return false;

        if (!writeTracks())
            return false;

        return true;
    }

    u64 Midi2ASM::writeWaitWithDifference(u64 CurrTime, u64 DstTime) {
        if (DstTime - CurrTime > 0) {
            mOutTxt->writeString("CMD_WAIT");
            mOutTxt->writeHexWithSpaceBefore(DstTime - CurrTime);
            mOutTxt->writeLine();
        }

        return DstTime;
    }

    void Midi2ASM::findAndSetLoopData() {
        resetMidiParser();
        u64 CurrTime = 0;
        u64 LongestTime = 0;

        while (true) {
            if (!parseMidi())
                break;
            else if (mMidiStatus == MIDI_PARSER_TRACK)
                CurrTime = 0; // reset time because new track starts
            else if (mMidiStatus == MIDI_PARSER_TRACK_MIDI || mMidiStatus == MIDI_PARSER_TRACK_META || mMidiStatus == MIDI_PARSER_TRACK_SYSEX) {
                if (LongestTime < CurrTime)
                    LongestTime = CurrTime;

                CurrTime += mMidi.vtime;

                if (mMidiStatus == MIDI_PARSER_TRACK_META && mMidi.meta.type == MIDI_META_MAKER) {
                    if (mLoopStartTime == 0xFFFFFFFFFFFFFFFF)
                        mLoopStartTime = CurrTime;
                    else if (mLoopEndTime == 0xFFFFFFFFFFFFFFFF) {
                        mLoopEndTime = CurrTime;
                        break;
                    }

                    /*if (!strncasecmp((const char*)mMidi.meta.bytes, "LoopStart", mMidi.meta.length))
                        mLoopStartTime = CurrTime;
                    else if (!strncasecmp((const char*)mMidi.meta.bytes, "LoopEnd", mMidi.meta.length))
                        mLoopEndTime = CurrTime;*/
                }
            }
        }

        if (mLoopEndTime == 0xFFFFFFFFFFFFFFFF)
            mLoopEndTime = LongestTime;
    }

    bool Midi2ASM::writeStartTrack() {
        resetMidiParser();
        parseMidi();

        if (mMidiStatus != MIDI_PARSER_HEADER) {
            printf("ERROR: no header found in midi file!\n");
            return false;
        }

        if (mMidi.header.format == MIDI_FILE_FORMAT_MULTIPLE_SONGS) {
            printf("ERROR: The file format of the given midi is not supported!\n");
            return false;
        }

        if (mMidi.header.tracks_count <= 0) {
            printf("ERROR: The midi does not have any tracks to convert!\n");
            return false;
        }

        printf("Midi file has %d tracks...\n", mMidi.header.tracks_count);

        mOutTxt->writeString("# BMS assembly made from MIDI using Eggplant");
        mOutTxt->writeLine();
        mOutTxt->writeLine();

        mOutTxt->writeLabelStart("start", 0);
        mOutTxt->writeString("CMD_OPEN_TRACK");
        mOutTxt->writeHexWithSpaceBefore(0);
        mOutTxt->writeLabelRefWithSpaceBefore("root_start", 0);
        mOutTxt->writeLine();

        mOutTxt->writeString("CMD_REG_LOAD");
        mOutTxt->writeHexWithSpaceBefore(98);
        mOutTxt->writeHexWithSpaceBefore(mMidi.header.time_division);
        mOutTxt->writeLine();
        
        // find all root track events (CMD_TEMPO)
        resetMidiParser();
        s32 EvtCount = 0;

        while (true) {
            if (findMidiMetaEvent(MIDI_META_SET_TEMPO) && mMidi.meta.length > 0)
                EvtCount++;
            else
                break;
        }

        resetMidiParser();
        u64* pTimeTable = new u64 [EvtCount];
        u16* pTempoVal = new u16 [EvtCount];
        u64 CurrTime = 0;
        s32 Count = 0;

        for (s32 i = 0; i < EvtCount; i++) {
            pTimeTable[i] = 0;
            pTempoVal[i] = 0;
        }

        while (true) {
            if (!parseMidi())
                break;
            else if (mMidiStatus == MIDI_PARSER_TRACK)
                CurrTime = 0; // reset time because new track starts
            else if (mMidiStatus == MIDI_PARSER_TRACK_MIDI || mMidiStatus == MIDI_PARSER_TRACK_META || mMidiStatus == MIDI_PARSER_TRACK_SYSEX) {
                CurrTime += mMidi.vtime;

                if (mMidiStatus == MIDI_PARSER_TRACK_META && mMidi.meta.type == MIDI_META_SET_TEMPO && mMidi.meta.length > 0) {
                    BinaryReader reader ((char*)mMidi.meta.bytes, mMidi.meta.length);
                    pTempoVal[Count] = round(60000000.0 / (f64)reader.readValueBE(0, mMidi.meta.length));
                    pTimeTable[Count++] = CurrTime;
                }
            }
        }

        CurrTime = 0;
        bool IsWriteLoopStart = false;
        bool IsWriteLoopStartData = false;
        u16 TempoAtLoop = 120; u16 CurrentTempo = 120;

        for (s32 i = 0; i < EvtCount; i++) {
            s32 TmpEntry = 0;

            // get smallest entry
            for (s32 c = 0; c < EvtCount; c++) {
                if (pTimeTable[c] < pTimeTable[TmpEntry])
                    TmpEntry = c;
            }

            if (pTimeTable[TmpEntry] > mLoopEndTime)
                break;

            if (isValidLoopData() && !IsWriteLoopStart && pTimeTable[TmpEntry] >= mLoopStartTime) {
                CurrTime = writeWaitWithDifference(CurrTime, mLoopStartTime);
                mOutTxt->writeLabelStart("loop_start", 0);
                IsWriteLoopStart = true;
            }

            if (IsWriteLoopStart && !IsWriteLoopStartData && pTimeTable[TmpEntry] > mLoopStartTime) {
                IsWriteLoopStartData = true;
                TempoAtLoop = CurrentTempo;
            }

            CurrTime = writeWaitWithDifference(CurrTime, pTimeTable[TmpEntry]);

            mOutTxt->writeString("CMD_TEMPO");
            mOutTxt->writeHexWithSpaceBefore(pTempoVal[TmpEntry]);
            mOutTxt->writeLine();

            CurrentTempo = pTempoVal[TmpEntry];

            CurrTime = pTimeTable[TmpEntry];
            pTimeTable[TmpEntry] = 0xFFFFFFFFFFFFFFFF; // invalidate entry
        }

        if (isValidLoopData()) {
            if (!IsWriteLoopStart) {
                CurrTime = writeWaitWithDifference(CurrTime, mLoopStartTime);
                mOutTxt->writeLabelStart("loop_start", 0);
            }
        }

        writeWaitWithDifference(CurrTime, mLoopEndTime);

        if (isValidLoopData()) {
            if (IsWriteLoopStartData) { // check, if a loop start was even set before no more events occured
                if (TempoAtLoop != CurrentTempo) {
                    mOutTxt->writeString("CMD_TEMPO");
                    mOutTxt->writeHexWithSpaceBefore(TempoAtLoop);
                    mOutTxt->writeLine();
                }
            }
            mOutTxt->writeString("CMD_JUMP");
            mOutTxt->writeLabelRefWithSpaceBefore("loop_start", 0);
        }
        else
            mOutTxt->writeString("CMD_FINISH");
        


        delete[] pTimeTable;
        delete[] pTempoVal;

        return true;
    }

    bool Midi2ASM::writeRootTrack() {
        for (s32 i = 0; i < 16; i++)
            mChannelSlots[i] = 0;
        
        resetMidiParser();

        mOutTxt->writeLine();
        mOutTxt->writeLine();
        mOutTxt->writeLine();
        mOutTxt->writeLabelStart("root_start", 0);

        // write open tracks
        bool FindNextTrack = true;
        s32 track = 0;

        while (true) {
            if (!parseMidi())
                break;
            else if (mMidiStatus == MIDI_PARSER_TRACK) {
                if (!FindNextTrack)
                    printf("WARNING: Track %d has no midi data! Skipping...\n", track - 1);
                track++;
                FindNextTrack = false; // reset because new track starts
            }
            else if (mMidiStatus == MIDI_PARSER_TRACK_MIDI) {
                if (!FindNextTrack) {
                    if (mMidi.midi.channel > 15) {
                        printf("ERROR: Track %d has an invalid channel number %d!\n", track, mMidi.midi.channel);
                        return false;
                    }

                    if (mChannelSlots[mMidi.midi.channel] == 0) {
                        mOutTxt->writeString("CMD_OPEN_TRACK");
                        mOutTxt->writeHexWithSpaceBefore(mMidi.midi.channel);
                        mOutTxt->writeLabelRefWithSpaceBefore("track_start", mMidi.midi.channel);
                        mOutTxt->writeLine();
                    }

                    FindNextTrack = true;
                    mChannelSlots[mMidi.midi.channel]++;
                }
            }
        }
        if (!FindNextTrack)
            printf("WARNING: Track %d has no midi data! Skipping...\n", track - 1);

        if (isValidLoopData()) {
            writeWaitWithDifference(0, mLoopStartTime);
            mOutTxt->writeLabelStart("root_loop_start", 0);
            writeWaitWithDifference(mLoopStartTime, mLoopEndTime);
            mOutTxt->writeString("CMD_JUMP");
            mOutTxt->writeLabelRefWithSpaceBefore("root_loop_start", 0);
        }
        else {
            writeWaitWithDifference(0, mLoopEndTime);
            mOutTxt->writeString("CMD_FINISH");
        }

        // check and split channel tracks
        for (s32 i = 0; i < 16; i++) {
            if (mChannelSlots[i] > 15) {
                printf("ERROR: Too many tracks share the channel number %d!\n", i);
                return false;
            }

            if (mChannelSlots[i] > 1) {
                mOutTxt->writeLine();
                mOutTxt->writeLine();
                mOutTxt->writeLine();
                mOutTxt->writeLabelStart("track_start", i);

                for (s32 f = 0; f < mChannelSlots[i]; f++) {
                    mOutTxt->writeString("CMD_OPEN_TRACK");
                    mOutTxt->writeHexWithSpaceBefore(f);
                    mOutTxt->writeLabelRefWithSpaceBeforeSplit("track_start", i, f);
                    mOutTxt->writeLine();
                }

                if (isValidLoopData()) {
                    writeWaitWithDifference(0, mLoopStartTime);
                    mOutTxt->writeLabelStart("track_loop_start", i);
                    writeWaitWithDifference(mLoopStartTime, mLoopEndTime);
                    mOutTxt->writeString("CMD_JUMP");
                    mOutTxt->writeLabelRefWithSpaceBefore("track_loop_start", i);
                }
                else {
                    writeWaitWithDifference(0, mLoopEndTime);
                    mOutTxt->writeString("CMD_FINISH");
                }
            }
        }

        return true;
    }

    bool Midi2ASM::writeTracks() {
        u8 ChannelSlotsTmp[16];
        for (s32 i = 0; i < 16; i++)
            ChannelSlotsTmp[i] = 0;

        for (s32 i = 0; i < mMidi.header.tracks_count; i++) {

            // calc note max
            if (!findTrack(i))
                return false;

            s32 NumNotesMax = 0;
            s32 NumCurrentNotesActive = 0;
            u8 TrackChannel = 0xFF;

            while (true) {
                if (!parseMidi())
                    break;
                else if (mMidiStatus == MIDI_PARSER_TRACK)
                    break;
                else if (mMidiStatus == MIDI_PARSER_TRACK_MIDI) {
                    if (TrackChannel == 0xFF)
                        TrackChannel = mMidi.midi.channel;

                    if (mMidi.midi.status == MIDI_STATUS_NOTE_ON)
                        NumCurrentNotesActive++;
                    else if (mMidi.midi.status == MIDI_STATUS_NOTE_OFF)
                        NumCurrentNotesActive--;

                    if (NumNotesMax < NumCurrentNotesActive)
                        NumNotesMax = NumCurrentNotesActive;
                }
            }

            if (TrackChannel == 0xFF) {
                // skipping because no midi data
                continue;
            }

            s32 NumSubChannels = NumNotesMax / 7;

            if (NumSubChannels > 15) {
                printf("ERROR: Too many notes are playing at once in track %d!", i);
                return false;
            }

            s32 SplitNum = (mChannelSlots[TrackChannel] > 1) ? ChannelSlotsTmp[TrackChannel]++ : -1;
            
            mOutTxt->writeLine();
            mOutTxt->writeLine();
            mOutTxt->writeLine();
            mOutTxt->writeLabelStartSplit("track_start", TrackChannel, SplitNum);

            for (s32 s = 0; s < NumSubChannels; s++) {
                mOutTxt->writeString("CMD_OPEN_TRACK");
                mOutTxt->writeHexWithSpaceBefore(s);
                mOutTxt->writeLabelRefWithSpaceBeforeSplitAndSub("track_start", TrackChannel, s, SplitNum);
                mOutTxt->writeLine();
            }

            printf("The maximum number of notes in Track %d: %d - Number of resulting sub channels: %d\n", i, NumNotesMax, NumSubChannels);

            writeTracksSub(i, 0, TrackChannel, SplitNum);

            for (s32 s = 0; s < NumSubChannels; s++) {
                mOutTxt->writeLine();
                mOutTxt->writeLine();
                mOutTxt->writeLine();
                mOutTxt->writeLabelStartSplitAndSub("track_start", TrackChannel, s, SplitNum);
                writeTracksSub(i, (s + 1) * 7, TrackChannel, SplitNum);
            }
        }

        return true;
    }

    void Midi2ASM::writeTracksSub(s32 Track, s32 SubNote, u32 TrackChannel, s32 SplitNum) {
        findTrack(Track);

        u64 CurrTime = 0;
        u64 LastTime = 0;

        f32 Volume = 1.0f;
        f32 Expression = 1.0f;

        u8 VolumeAtLoop = 0x7F; u8 CurrentVolume = 0x7F;
        u8 PanAtLoop = 0x40; u8 CurrentPan = 0x40;
        u16 PitchAtLoop = 0x0; u16 CurrentPitch = 0x0;
        u8 ProgramAtLoop = 0x0; u8 CurrentProgram = 0x0;
        u8 BankAtLoop = 0x0; u8 CurrentBank = 0x0;
        u8 ReverbAtLoop = 0x0; u8 CurrentReverb = 0x0;
        u8 DolbyAtLoop = 0x0; u8 CurrentDolby = 0x0;
        u8 TremoloAtLoop = 0x0; u8 CurrentTremolo = 0x0;

        bool IsWriteLoopStart = false;
        bool IsWriteLoopStartData = false;
        //mOutTxt->writeString("CMD_BANK_PROGRAM 0x0 0x0");
        //mOutTxt->writeLine();

        for (s32 i = 0; i < sizeof(mLastNote) / sizeof(u8); i++)
            mLastNote[i] = 0xFF;

        while (true) {
            if (!parseMidi())
                break;
            else if (mMidiStatus == MIDI_PARSER_TRACK)
                break;
            else if (mMidiStatus == MIDI_PARSER_TRACK_MIDI || mMidiStatus == MIDI_PARSER_TRACK_META || mMidiStatus == MIDI_PARSER_TRACK_SYSEX) {
                CurrTime += mMidi.vtime;

                if (CurrTime > mLoopEndTime)
                    break;

                if (isValidLoopData() && !IsWriteLoopStart && CurrTime >= mLoopStartTime) {
                    LastTime = writeWaitWithDifference(LastTime, mLoopStartTime);

                    if (SubNote <= 0)
                        mOutTxt->writeLabelStartSplit("track_loop_start", TrackChannel, SplitNum);
                    else
                        mOutTxt->writeLabelStartSplitAndSub("track_loop_start", TrackChannel, SubNote / 7 - 1, SplitNum);
                    
                    IsWriteLoopStart = true;
                }

                if (IsWriteLoopStart && !IsWriteLoopStartData && CurrTime > mLoopStartTime) {
                    IsWriteLoopStartData = true;
                    VolumeAtLoop = CurrentVolume;
                    PanAtLoop = CurrentPan;
                    PitchAtLoop = CurrentPitch;
                    ProgramAtLoop = CurrentProgram;
                    BankAtLoop = CurrentBank;
                    ReverbAtLoop = CurrentReverb;
                    DolbyAtLoop = CurrentDolby;
                    TremoloAtLoop = CurrentTremolo;
                }

                if (mMidiStatus == MIDI_PARSER_TRACK_MIDI) {
                    if (mMidi.midi.status == MIDI_STATUS_NOTE_ON) {
                        s32 noteSlot = findNoteSlot(0xFF, mMidi.midi.param1);
                        if (noteSlot > SubNote && noteSlot <= SubNote + 7) {
                            LastTime = writeWaitWithDifference(LastTime, CurrTime);
                            mOutTxt->writeString("NOTE_ON");
                            mOutTxt->writeHexWithSpaceBefore(mMidi.midi.param1); // key
                            mOutTxt->writeHexWithSpaceBefore(noteSlot - SubNote);
                            mOutTxt->writeHexWithSpaceBefore(mMidi.midi.param2); // velocity
                            mOutTxt->writeLine();
                        }
                    }
                    else if (mMidi.midi.status == MIDI_STATUS_NOTE_OFF) {
                        s32 noteSlot = findNoteSlot(mMidi.midi.param1, 0xFF);
                        if (noteSlot > SubNote && noteSlot <= SubNote + 7) {
                            LastTime = writeWaitWithDifference(LastTime, CurrTime);
                            mOutTxt->writeString("NOTE_OFF");
                            mOutTxt->writeHexWithSpaceBefore(noteSlot - SubNote);
                            mOutTxt->writeLine();
                        }
                    }
                    else if (mMidi.midi.status == MIDI_STATUS_PGM_CHANGE) {
                        LastTime = writeWaitWithDifference(LastTime, CurrTime);
                        mOutTxt->writeString("CMD_PROGRAM");
                        mOutTxt->writeHexWithSpaceBefore(mMidi.midi.param1);
                        mOutTxt->writeLine();

                        CurrentProgram = mMidi.midi.param1;
                    }
                    else if (mMidi.midi.status == MIDI_STATUS_PITCH_BEND && SubNote <= 0) {
                        LastTime = writeWaitWithDifference(LastTime, CurrTime);
                        mOutTxt->writeString("CMD_PARAM_I");
                        mOutTxt->writeHexWithSpaceBefore(1);
                        u16 Pitch = ((mMidi.midi.param2 << 7) | mMidi.midi.param1) - 8192;
                        mOutTxt->writeHexWithSpaceBefore(Pitch);
                        mOutTxt->writeLine();

                        CurrentPitch = Pitch;
                    }
                    else if (mMidi.midi.status == MIDI_STATUS_CC) {
                        if (mMidi.midi.param1 == 0) { // bank
                            LastTime = writeWaitWithDifference(LastTime, CurrTime);
                            mOutTxt->writeString("CMD_BANK");
                            mOutTxt->writeHexWithSpaceBefore(mMidi.midi.param2);
                            mOutTxt->writeLine();

                            CurrentBank = mMidi.midi.param2;
                        }
                        else if (mMidi.midi.param1 == 1) { // modulation => tremolo depth
                            LastTime = writeWaitWithDifference(LastTime, CurrTime);
                            mOutTxt->writeString("CMD_REG_LOAD");
                            mOutTxt->writeHexWithSpaceBefore(0x70);
                            mOutTxt->writeHexWithSpaceBefore(mMidi.midi.param2);
                            mOutTxt->writeLine();

                            CurrentTremolo = mMidi.midi.param2;
                        }
                        else if (SubNote <= 0) {
                            if (mMidi.midi.param1 == 7) { // volume
                                Volume = (f32)mMidi.midi.param2 / 127.0f;
                                CurrentVolume = roundf(Volume * Expression * 127.0f);

                                LastTime = writeWaitWithDifference(LastTime, CurrTime);
                                mOutTxt->writeString("CMD_PARAM_E");
                                mOutTxt->writeHexWithSpaceBefore(0);
                                mOutTxt->writeHexWithSpaceBefore(CurrentVolume);
                                mOutTxt->writeLine();
                            }
                            else if (mMidi.midi.param1 == 10) { // pan
                                LastTime = writeWaitWithDifference(LastTime, CurrTime);
                                mOutTxt->writeString("CMD_PARAM_E");
                                mOutTxt->writeHexWithSpaceBefore(3);
                                mOutTxt->writeHexWithSpaceBefore(mMidi.midi.param2);
                                mOutTxt->writeLine();

                                CurrentPan = mMidi.midi.param2;
                            }
                            else if (mMidi.midi.param1 == 11) { // expression
                                Expression = (f32)mMidi.midi.param2 / 127.0f;
                                CurrentVolume = roundf(Volume * Expression * 127.0f);

                                LastTime = writeWaitWithDifference(LastTime, CurrTime);
                                mOutTxt->writeString("CMD_PARAM_E");
                                mOutTxt->writeHexWithSpaceBefore(0);
                                mOutTxt->writeHexWithSpaceBefore(CurrentVolume);
                                mOutTxt->writeLine();
                            }
                            else if (mMidi.midi.param1 == 91) { // reverb
                                LastTime = writeWaitWithDifference(LastTime, CurrTime);
                                mOutTxt->writeString("CMD_PARAM_E");
                                mOutTxt->writeHexWithSpaceBefore(2);
                                mOutTxt->writeHexWithSpaceBefore(mMidi.midi.param2);
                                mOutTxt->writeLine();

                                CurrentReverb = mMidi.midi.param2;
                            }
                            else if (mMidi.midi.param1 == 93) { // dolby / Chorus
                                LastTime = writeWaitWithDifference(LastTime, CurrTime);
                                mOutTxt->writeString("CMD_PARAM_E");
                                mOutTxt->writeHexWithSpaceBefore(4);
                                mOutTxt->writeHexWithSpaceBefore(mMidi.midi.param2);
                                mOutTxt->writeLine();

                                CurrentDolby = mMidi.midi.param2;
                            }
                        }
                    }
                }
            }
        }

        if (isValidLoopData()) {
            if (!IsWriteLoopStart) {
                LastTime = writeWaitWithDifference(LastTime, mLoopStartTime);
                if (SubNote <= 0)
                    mOutTxt->writeLabelStartSplit("track_loop_start", TrackChannel, SplitNum);
                else
                    mOutTxt->writeLabelStartSplitAndSub("track_loop_start", TrackChannel, SubNote / 7 - 1, SplitNum);
            }
        }

        writeWaitWithDifference(LastTime, mLoopEndTime);

        if (isValidLoopData()) {
            if (IsWriteLoopStartData) {
                if (SubNote <= 0) {
                    if (CurrentVolume != VolumeAtLoop) {
                        // write volume
                        mOutTxt->writeString("CMD_PARAM_E");
                        mOutTxt->writeHexWithSpaceBefore(0);
                        mOutTxt->writeHexWithSpaceBefore(VolumeAtLoop);
                        mOutTxt->writeLine();
                    }
                    if (CurrentPitch != PitchAtLoop) {
                        // write pitch
                        mOutTxt->writeString("CMD_PARAM_I");
                        mOutTxt->writeHexWithSpaceBefore(1);
                        mOutTxt->writeHexWithSpaceBefore(PitchAtLoop);
                        mOutTxt->writeLine();
                    }
                    if (CurrentReverb != ReverbAtLoop) {
                        // write reverb
                        mOutTxt->writeString("CMD_PARAM_E");
                        mOutTxt->writeHexWithSpaceBefore(2);
                        mOutTxt->writeHexWithSpaceBefore(ReverbAtLoop);
                        mOutTxt->writeLine();
                    }
                    if (CurrentPan != PanAtLoop) {
                        // write pan
                        mOutTxt->writeString("CMD_PARAM_E");
                        mOutTxt->writeHexWithSpaceBefore(3);
                        mOutTxt->writeHexWithSpaceBefore(PanAtLoop);
                        mOutTxt->writeLine();
                    }
                    if (CurrentDolby != DolbyAtLoop) {
                        // write dolby
                        mOutTxt->writeString("CMD_PARAM_E");
                        mOutTxt->writeHexWithSpaceBefore(4);
                        mOutTxt->writeHexWithSpaceBefore(DolbyAtLoop);
                        mOutTxt->writeLine();
                    }
                }
                // tremolo
                if (TremoloAtLoop != CurrentTremolo) {
                    mOutTxt->writeString("CMD_REG_LOAD");
                    mOutTxt->writeHexWithSpaceBefore(0x70);
                    mOutTxt->writeHexWithSpaceBefore(TremoloAtLoop);
                    mOutTxt->writeLine();
                }
                // write prg and bnk
                if (CurrentBank != BankAtLoop) {
                    mOutTxt->writeString("CMD_BANK");
                    mOutTxt->writeHexWithSpaceBefore(BankAtLoop);
                    mOutTxt->writeLine();
                }
                if (CurrentProgram != ProgramAtLoop) {
                    mOutTxt->writeString("CMD_PROGRAM");
                    mOutTxt->writeHexWithSpaceBefore(ProgramAtLoop);
                    mOutTxt->writeLine();
                }
            }

            mOutTxt->writeString("CMD_JUMP");
            if (SubNote <= 0)
                mOutTxt->writeLabelRefWithSpaceBeforeSplit("track_loop_start", TrackChannel, SplitNum);
            else
                mOutTxt->writeLabelRefWithSpaceBeforeSplitAndSub("track_loop_start", TrackChannel, SubNote / 7 - 1, SplitNum);
        }
        else
            mOutTxt->writeString("CMD_FINISH");
    }

    s32 Midi2ASM::findNoteSlot(u32 Val, u32 Data) {
        for (s32 i = 0; i < sizeof(mLastNote) / sizeof(u8); i++) {
            if (mLastNote[i] == Val) {
                mLastNote[i] = Data;
                return i + 1;
            }
        }
        
        return -1;
    }

    bool Midi2ASM::findMidiMetaEvent(midi_meta meta) {
        while (true) {
            if (!parseMidi())
                return false;

            if (mMidiStatus == MIDI_PARSER_TRACK_META && mMidi.meta.type == meta)
                return true;
        }
    }
}