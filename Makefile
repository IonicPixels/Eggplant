CPP := g++

CPP_FILES := src/Eggplant.cpp src/BinaryReader.cpp src/BMS.cpp src/CIT.cpp src/TextWriter.cpp src/FileHolder.cpp src/TextReader.cpp src/BSTN.cpp midi/src/midi-parser.c

ifeq ($(OS),Windows_NT)
	Eggplant := Eggplant.exe
else
	Eggplant := Eggplant
endif

all:
	$(CPP) -Os -s -I . -I ./midi/include -static -Wno-multichar -Wall -o $(Eggplant) $(CPP_FILES)
	
clean:
	rm -f $(Eggplant)