#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>

#include "include/BMS.h"
#include "include/CIT.h"

/*#include "src/BinaryReader.cpp"
#include "src/BMS.cpp"
#include "src/FileHolder.cpp"
#include "src/TextWriter.cpp"
#include "src/TextReader.cpp"*/

namespace Eggplant {
    bool disassemble(const char* pInFile, const char* pOutFile, const char* pBstnFile) {
        BinaryReader in_bms (pInFile);
        TextWriter out_txt (pOutFile);
        BSTNHolder* bstn = 0;

        if (!in_bms.loadFile()) {
            std::cout << "ERROR! The input file could not be opened.\n";
            return false;
        }

        if (!out_txt.openFile()) {
            std::cout << "ERROR! The output file could not be opened.\n";
            return false;
        }

        if (pBstnFile) {
            bstn = new BSTNHolder(pBstnFile);

            if (!bstn->loadFile()) {
                std::cout << "ERROR! The input BSTN file could not be opened.\n";
                return false;
            }
        }

        std::cout << "Loaded file.\n";

        BMS::Disassembler se_bms (&in_bms, bstn);
        se_bms.init();

        std::cout << "Now starting to disassemble...\n";

        if (se_bms.disassemble(&out_txt)) {
            out_txt.closeFile();
            return true;
        }
        else {
            out_txt.closeFile();
            return false;
        }
    }



    bool assemble(const char* pInFile, const char* pOutFile) {
        TextReader text_in (pInFile);
        BinaryReader out_bms (pOutFile);

        if (!text_in.loadFile()) {
            std::cout << "The input file could not be opened.\n";
            return false;
        }

        std::cout << "Loaded file.\n";

        BMS::Assembler bms_asm (&text_in);
        if (!bms_asm.init())
            return false;
        std::cout << "Now starting to assemble...\n";
        bms_asm.createBinaryData(&out_bms);

        if (bms_asm.assemble(&out_bms)) {
            std::cout << "Writing out file...\n";
            if (out_bms.writeFile())
                return true;
            else
                std::cout << "ERROR! The file could not be written!\n";
        }

        return false;
    }
    
    
    
    
    
    
    
    
    bool midi(const char* pInFile, const char* pOutFile) {
        FileHolder midi_in (pInFile);
        TextWriter out_txt (pOutFile);

        if (!midi_in.loadFile()) {
            std::cout << "The input file could not be opened.\n";
            return false;
        }

        if (!out_txt.openFile()) {
            std::cout << "ERROR! The output file could not be opened.\n";
            return false;
        }

        std::cout << "Loaded file.\n";

        BMS::Midi2ASM midi2asm (&midi_in);

        std::cout << "Now starting to write file...\n";

        if (midi2asm.writeOutFile(&out_txt))
            return true;
        
        std::cout << "ERROR! The file could not be written!\n";

        return false;
    }




    bool cit2txt(const char* pInFile, const char* pOutFile) {
        BinaryReader in_cit (pInFile);
        TextWriter out_txt (pOutFile);

        if (!in_cit.loadFile()) {
            std::cout << "The input file could not be opened.\n";
            return false;
        }

        if (!out_txt.openFile()) {
            std::cout << "ERROR! The output file could not be opened.\n";
            return false;
        }

        std::cout << "Loaded file.\n";

        CITReader cit_reader;

        std::cout << "Now starting to write file...\n";

        if (cit_reader.writeOutFile(&in_cit, &out_txt))
            return true;
        
        std::cout << "ERROR! The file could not be written!\n";

        return false;
    }




    bool txt2cit(const char* pInFile, const char* pOutFile) {
        TextReader in_txt (pInFile);

        if (!in_txt.loadFile()) {
            std::cout << "The input file could not be opened.\n";
            return false;
        }

        std::cout << "Loaded file.\n";

        CITWriter cit_writer;
        BinaryReader out_cit (pOutFile);

        if (!cit_writer.writeOutFile(&out_cit, &in_txt))
            return false;

        std::cout << "Now starting to write file...\n";

        if (out_cit.writeFile())
            return true;
        
        std::cout << "ERROR! The file could not be written!\n";

        return false;
    }




    void addBankProgram(u16 bp, std::vector<u16>& rBnkPrg) {
        for (auto i = rBnkPrg.begin(); i != rBnkPrg.end(); i++) {
            if (*i == bp)
                return;
        }

        rBnkPrg.push_back(bp);
    }

    bool collectBankProgramParseTrack(TextReader& rTxtReader, std::vector<u16>& rBnkPrg) {
        u32 prg = 0xFFFFF, bnk = 0xFFFFF;
        u32 jumpNum = 0;
        char buffer[256];

        rTxtReader.findNextActionLineIncludeCurrentLine();

        while (true) {
            if (rTxtReader.findNextValueInCurrentLine(buffer, 256)) {
                if (!strcasecmp("CMD_BANK_PROGRAM", buffer)) {
                    if (!rTxtReader.findNextValueInCurrentLine(buffer, 256) || !rTxtReader.tryConvertStrToInt(buffer, &bnk) || !rTxtReader.findNextValueInCurrentLine(buffer, 256) || !rTxtReader.tryConvertStrToInt(buffer, &prg)) {
                        std::cout << "ERROR! Invalid arguments on line " << rTxtReader.getCurrentLineNum() << "!\n";
                        return false;
                    }
                    
                    addBankProgram(((bnk & 0xFF) << 8) | (prg & 0xFF), rBnkPrg);
                    return true;
                }
                else if (!strcasecmp("CMD_BANK", buffer)) {
                    if (!rTxtReader.findNextValueInCurrentLine(buffer, 256) || !rTxtReader.tryConvertStrToInt(buffer, &bnk)) {
                        std::cout << "ERROR! Invalid arguments on line " << rTxtReader.getCurrentLineNum() << "!\n";
                        return false;
                    }

                    if (prg != 0xFFFFF) {
                        addBankProgram(((bnk & 0xFF) << 8) | (prg & 0xFF), rBnkPrg);
                        return true;
                    }
                }
                else if (!strcasecmp("CMD_PROGRAM", buffer)) {
                    if (!rTxtReader.findNextValueInCurrentLine(buffer, 256) || !rTxtReader.tryConvertStrToInt(buffer, &prg)) {
                        std::cout << "ERROR! Invalid arguments on line " << rTxtReader.getCurrentLineNum() << "!\n";
                        return false;
                    }

                    if (bnk != 0xFFFFF) {
                        addBankProgram(((bnk & 0xFF) << 8) | (prg & 0xFF), rBnkPrg);
                        return true;
                    }
                }
                else if (!strcasecmp("CMD_OPEN_TRACK", buffer)) {
                    if (!rTxtReader.findNextValueInCurrentLine(buffer, 256) || !rTxtReader.findNextValueInCurrentLine(buffer, 256)) {
                        std::cout << "ERROR! Invalid arguments on line " << rTxtReader.getCurrentLineNum() << "!\n";
                        return false;
                    }
                    if (strlen(buffer) < 2 || buffer[0] != '$') {
                        std::cout << "ERROR! Child track does not have a valid label (" << (buffer + 1) << ") on line " << rTxtReader.getCurrentLineNum() << ".\n";
                        return false;
                    }
                    TextReader txtTmp (&rTxtReader, 0);
                    if (!txtTmp.findNextActionLineAfterLabel(buffer + 1)) {
                        std::cout << "ERROR! Label \":" << buffer + 1 << "\" could not be found! On line " << rTxtReader.getCurrentLineNum() << ".\n";
                        return false;
                    }
                    if (!collectBankProgramParseTrack(txtTmp, rBnkPrg))
                        return false;
                }
                else if (!strcasecmp("CMD_JUMP", buffer)) {
                    jumpNum++;
                    if (jumpNum > 16) {
                        std::cout << "WARNING! Ended parsing after 16 jump on line " << rTxtReader.getCurrentLineNum() << ".\n";
                        return true;
                    }

                    if (!rTxtReader.findNextValueInCurrentLine(buffer, 256) || strlen(buffer) < 2 || buffer[0] != '$') {
                        std::cout << "ERROR! Jump does not have a valid label on line " << rTxtReader.getCurrentLineNum() << ".\n";
                        return false;
                    }
                    if (!rTxtReader.findNextActionLineAfterLabel(buffer + 1)) {
                        std::cout << "ERROR! Label \":" << buffer + 1 << "\" could not be found! On line " << rTxtReader.getCurrentLineNum() << ".\n";
                        return false;
                    }
                    if (rTxtReader.isReachedEnd()) {
                        std::cout << "ERROR! End of file was reached unexpectedly!\n";
                        return false;
                    }
                    continue;
                }
                else if (!strcasecmp("CMD_FINISH", buffer) || !strcasecmp("CMD_RETURN", buffer) || !strcasecmp("CMD_LOOP_END", buffer))
                    return true;
                else if (!strcasecmp("CMD_JUMP_TABLE", buffer)) {
                    std::cout << "WARNING! Parsing stopped at " << rTxtReader.getCurrentLineNum() << " because of unsupported opcode.\n";
                    return true;
                }
            }
            
            if (rTxtReader.isReachedEnd()) {
                std::cout << "ERROR! End of file was reached unexpectedly!\n";
                return false;
            }

            rTxtReader.findNextActionLine();
        }

        return false;
    }

    bool collectBankProgram(const char* pInTxt, const char* pOutCsv, const char* pInBstn) {
        TextReader txt (pInTxt);
        std::ofstream csv;
        BSTNHolder* bstn = 0;

        if (!txt.loadFile()) {
            std::cout << "The input file could not be opened.\n";
            return false;
        }

        csv.open(pOutCsv);

        if (!csv.is_open()) {
            std::cout << "The output file could not be opened.\n";
            return false;
        }

        if (pInBstn) {
            bstn = new BSTNHolder(pInBstn);

            if (!bstn->loadFile()) {
                std::cout << "The BSTN file could not be opened.\n";
                return false;
            }
        }

        char buffer[256];
        std::vector<u16> bankPrg;
        txt.findNextActionLineIncludeCurrentLine();

        while (true) {
            if (txt.findNextValueInCurrentLine(buffer, 256) && !strcasecmp(buffer, "CATEGORY")) {
                u32 cat;
                if (!txt.findNextValueInCurrentLine(buffer, 256) || !txt.tryConvertStrToInt(buffer, &cat)) {
                    std::cout << "ERROR! Category does not have a valid ID on line " << txt.getCurrentLineNum() << ".\n";
                    return false;
                }

                while (!txt.isReachedEnd()) {
                    txt.findNextActionLine();

                    if (txt.findNextValueInCurrentLine(buffer, 256) && !strcasecmp(buffer, "SOUND")) {
                        u32 soundID;
                        if (!txt.findNextValueInCurrentLine(buffer, 256) || !txt.tryConvertStrToInt(buffer, &soundID)) {
                            std::cout << "ERROR! Sound does not have a valid ID on line " << txt.getCurrentLineNum() << ".\n";
                            return false;
                        }
                        soundID = (cat << 16) | (soundID & 0xFFFF);

                        if (!txt.findNextValueInCurrentLine(buffer, 256) || strlen(buffer) < 2 || buffer[0] != '$') {
                            std::cout << "ERROR! Sound does not have a valid label on line " << txt.getCurrentLineNum() << ".\n";
                            return false;
                        }

                        TextReader txtTmp (&txt, 0);
                        if (!txtTmp.findNextActionLineAfterLabel(buffer + 1)) {
                            std::cout << "ERROR! Label \":" << buffer + 1 << "\" could not be found! On line " << txt.getCurrentLineNum() << ".\n";
                            return false;
                        }

                        bankPrg.clear();
                        if (!collectBankProgramParseTrack(txtTmp, bankPrg)) {
                            std::cout << "Occured while trying to process category " << cat << " sound " << (soundID & 0xFFFF) << " (" << (buffer + 1) << ")\n";
                            return false;
                        }

                        for (auto i = bankPrg.begin(); i != bankPrg.end(); i++) {
                            csv << std::dec << soundID << "," << *i << "," << std::hex << soundID << "," << *i;

                            if (bstn) {
                                csv << ",";
                                const char* pSoundName = bstn->getSoundNameFromId(soundID);
                                if (pSoundName)
                                    csv << pSoundName;
                            }

                            csv << "\n";
                        }
                    }
                    else
                        break; // end of SOUND entries
                }
            }

            if (txt.isReachedEnd())
                break;
            
            txt.findNextActionLine();
        }

        csv.close();
        return true;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "To disassemble a bms file:\n\t" << argv[0] << " disassemble inFile.bms outFile.txt [inBstn.bstn]\n\n";
        std::cout << "To assemble a bms file:\n\t" << argv[0] << " assemble inFile.txt outFile.bms\n\n";
        std::cout << "To disassemble a midi to bms:\n\t" << argv[0] << " midi inFile.mid outFile.txt\n\n\n";
        std::cout << "To disassemble a cit to txt:\n\t" << argv[0] << " cit2txt inFile.cit outFile.txt\n\n";
        std::cout << "To assemble a txt to cit:\n\t" << argv[0] << " txt2cit inFile.txt outFile.cit\n\n\n";
        std::cout << "To collect bank and program information from txt to csv:\n\t" << argv[0] << " collectbnkprg inFile.txt outFile.csv [inBstn.bstn]\n";
        return 1;
    }

    if (!strcmp(argv[1], "disassemble")) {
        if (!Eggplant::disassemble(argv[2], argv[3], argc >= 5 ? argv[4] : 0))
            return 1;
    }
    else if (!strcmp(argv[1], "assemble")) {
        if (!Eggplant::assemble(argv[2], argv[3]))
            return 1;
    }
    else if (!strcmp(argv[1], "midi")) {
        if (!Eggplant::midi(argv[2], argv[3]))
            return 1;
    }
    else if (!strcmp(argv[1], "cit2txt")) {
        if (!Eggplant::cit2txt(argv[2], argv[3]))
            return 1;
    }
    else if (!strcmp(argv[1], "txt2cit")) {
        if (!Eggplant::txt2cit(argv[2], argv[3]))
            return 1;
    }
    else if (!strcmp(argv[1], "collectbnkprg")) {
        if (!Eggplant::collectBankProgram(argv[2], argv[3], argc >= 5 ? argv[4] : 0))
            return 1;
    }
    else {
        std::cout << "Wrong argument \"" << argv[1] << "\"\n";
        return 1;
    }
    
    std::cout << "DONE! :D\n";
    return 0;
}