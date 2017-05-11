/*	wav2flash												*/
/*	convert 8bit/8kHz/mono WAVE to flash memory structure 	*/



/*				INCLUDES __BEGIN__	*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <iostream>
#include <unistd.h>
#include <math.h>
using namespace std;
/*				INCLUDES __END__	*/


//////////////////////////////////////////////////////////////////////////////
// main                                                                     //
//////////////////////////////////////////////////////////////////////////////

#define SOUND_COUNT_OFFSET 0x0
#define FILE_INFO_SIZE 0x5
#define SOUND_INFO_OFFSET 0x1
#define SOUND_DATA_OFFSET 0x51
#define PCM_HEADER_LENGTH 0x2C
/* 88M -> 8bit, 8kHz, Mono */ 
#define DATA_LENGTH_FIELDS_COUNT 4
#define DATA_LENGTH_OFFSET 0x28

#define FLASH_SIZE_2MB 262144


/*				DATA STRUCTURES __BEGIN__	*/

struct flashSoundInfo{

	uint8_t addr[3];
	uint16_t length;
	void setAddr(uint8_t* _address){
		addr[0] = _address[0];
		addr[1] = _address[1];
		addr[2] = _address[2];
	}
	uint8_t blockSize(){
		return 5;
	}
};

/*				DATA STRUCTURES __END__		*/

/*------------------------------------------*/

/* 				PROTOTYPES __BEGIN__		*/

flashSoundInfo* parseWaveToFlash(char* _fileName, uint8_t _fileCount, uint32_t _offset, FILE *_fo);
void showProgress(int _currentSample, int _dataLength);
void fillEmptyFile(FILE *_fileStream, uint32_t _fileSize);
/*				PROTOTYPES __END__			*/


uint32_t totalWaveFiles;
uint16_t soundDataOffset;
int main(int argc, char *argv[])
{
	printf("\n\n");
	printf("[  wav2flash v0.3a by Ubi de Feo / Future Tailors   ]\n");
	printf("[loosely based on wav2c by Dark Fader / BlackThunder]\n\n");
	printf("Command Line Utility to convert 8 bit, 8kHz Mono Wave files\nto data for SPI Flash Memory (or as you please)\n");
	printf("\n");
	char *cvalue = NULL;
	char *outputFileName = NULL;
	int inputFilesIndex;
	int c;
	uint32_t outputFileSize;

	while ((c = getopt (argc, argv, "o:s:i:")) != -1)
	{
		 //fprintf (stderr, "Option: [-%c].\nArgument: [%s]\nAt position: [%i]\n", optopt, optarg, optind);
		 if(optopt == 's'){
		 	outputFileSize = atoi(optarg);
		 	//printf("output ROM size: %i\n", outputFileSize);
		 }else{
		 	outputFileSize = FLASH_SIZE_2MB;
		 }
		 if(optopt == 'o'){
		 	outputFileName = optarg;
		 }
		 if(optopt == 'i'){
		 	inputFilesIndex = optind;
		 }
	}

	//fprintf(stderr, "end of options at %c\n", optind);
	/*
	uint8_t addr[3] = {0x1, 0xff, 0xf3};
	soundDataObject.setAddr(addr);
	soundDataObject.length = 21360;

	printf("flashSoundInfo address: {0x%x,0x%x,0x%x}", soundDataObject.addr[0], soundDataObject.addr[1], soundDataObject.addr[2]);
	*/
	if (argc < 2) { 
		fprintf(stderr, "usage:\nwav2flash -o outputFileName [-s romSize] -i inputFile(s)\ninputFiles can be a directory containing multiple WAV files\ni.e.: -i soundsFolder/*\n");
		fprintf(stderr, "romSize defaults to 262144 (2Mbit)\n-i option should be used as last.\n");
		return -1;
	}
		
	//printf("input argv[1]: %s\n", argv[1]);
	
	// XXX remove return 0 to continue
	//return 0;

	/*	create and open ouput flash file 	*/
	//FILE *fo = fopen("soundFlashImage.rom", "wt+");
	FILE *fo = fopen(outputFileName, "wt+");
	if (!fo) { printf("Could not open output file!\n"); return -1; }
	fillEmptyFile(fo, outputFileSize);
	/*
	fclose(fo);
	return 0;
	*/
	uint8_t waveFilesCount = 0;
	
	/*	cycle through input WAV files 	*/
	flashSoundInfo* lastParsedSoundInfo;

	
	totalWaveFiles = argc - (inputFilesIndex-1);
	soundDataOffset = SOUND_INFO_OFFSET + totalWaveFiles*FILE_INFO_SIZE;
	uint32_t currentFileIndex = 0;
	for(uint8_t argId = inputFilesIndex-1; argId < argc; argId++){
		printf("[ Parsing %s ]\n", argv[argId]);
		/*	parse WAV file 	*/
		
		lastParsedSoundInfo = parseWaveToFlash(argv[argId], waveFilesCount, soundDataOffset, fo);

		// write sound info
		fseek(fo, SOUND_INFO_OFFSET + FILE_INFO_SIZE*waveFilesCount, SEEK_SET);
		fwrite((char*)&lastParsedSoundInfo->addr,3,1,fo);
		fseek(fo, SOUND_INFO_OFFSET + FILE_INFO_SIZE*waveFilesCount + 3, SEEK_SET);
		//fwrite((char*)&lastParsedSoundInfo->length,2,1,fo);
		fputc(lastParsedSoundInfo->length >> 8, fo);
		fputc(lastParsedSoundInfo->length & 0x00ff, fo);
		// 
		soundDataOffset += lastParsedSoundInfo->length;
		waveFilesCount++;

		delete lastParsedSoundInfo;
	}
	/*									*/
	
	/* 	close output flash file 	*/
	fclose(fo);
	
	
	
	
	return 0;
}

flashSoundInfo* parseWaveToFlash(char* _fileName, uint8_t _fileCount, uint32_t _offset, FILE *_fo){
	char buf[1024], *s = buf;
	strcpy(s, _fileName);
	char *p = strstr(s, ".wav");
	
	//printf("name %s\n", soundName);
	//printf("input *p: %p\n", p);
	if (!p) { printf("Please specify a 8KHz, 8-bit wav file!\n"); return NULL; }
	uint8_t dotPos = strcspn(s, ".");
	char soundName[dotPos];
	soundName[dotPos] = '\0';
	strncpy(soundName, s, dotPos);


	FILE *fi = fopen(s, "rb");
	if (!fi) { printf("Could not open input file!\n"); return NULL; }
	strcpy(p, ".rom");
	

	fseek(fi, PCM_HEADER_LENGTH, SEEK_SET);
	p = strrchr(s, '\\'); if (p) s = p+1;
	p = strrchr(s, '/'); if (p) s = p+1;
	p = strrchr(s, '.'); if (p) *p = 0;
	uint32_t dataLength = 0;
	uint32_t lengthBytePartial;
	fseek(fi, DATA_LENGTH_OFFSET, SEEK_SET);
	for(uint8_t field = 0; field < DATA_LENGTH_FIELDS_COUNT; field++){
		lengthBytePartial = fgetc(fi);
		lengthBytePartial <<= 8*field;
		dataLength |= lengthBytePartial;
	}
	
	flashSoundInfo *soundDataObject = new flashSoundInfo;
	uint8_t addr[3];

	//uint32_t addr = _offset;
	addr[0] = _offset >> 16;
	addr[1] = (_offset >> 8) & 0x00ff;
	addr[2] = (_offset & 0x0000ff);
	soundDataObject->setAddr(addr);
	soundDataObject->length = dataLength;
	
	
	fseek(fi, PCM_HEADER_LENGTH, SEEK_SET);
	fseek(_fo, 0, SEEK_SET);
	fputc((_fileCount+1), _fo);

	/* rework following statements to write a 32 bit number at beginning */
	//uint32_t countData = 0xaa55aa55;
	//fwrite((char*)&countData,3,1,_fo);

	/*	add address+length [A0-A1, L0-L1-L2]	*/

	fseek(_fo, _offset, SEEK_SET);
	printf("Processing file %i of %i\n", _fileCount+1, totalWaveFiles);
	printf("Data length: %i\n", dataLength);
	printf("At offset: %i\n", _offset);

	//fprintf(fo, "/* %s */\nconst uint32_t %sSound_length=%i;\n", soundName, soundName, dataLength);
	//fprintf(fo, "const uint8_t %sSound_data[] PROGMEM =\n{", s);
	
	int l=0;
	float progress = 0.0;
	int barWidth = 40;
	while (1)
	{
		if (l++ % 100 == 0) { 
			//fprintf(fo, "\n\t"); 
		}
		
		uint8_t a = fgetc(fi);
		//if (a == EOF) break;
		fputc(a, _fo);
		//showProgress(l, dataLength);
		
		// FUTURE: move progress display to showProgress()
		int pos = barWidth * progress;
		cout << "[";
		for(uint8_t i = 0; i < barWidth; i++){
			if (i <= round(pos)) cout << "=";
	        else cout << "*";
		}
		cout << "]";
		progress+=(1.0f/dataLength);

		cout << " progress: " << round(progress*100) << "\r";
		cout.flush();
		if(l == dataLength){
			//fprintf(fo, "%d", a);	
			break;		
		}else{
			//fprintf(fo, "%d,", a);
		}
		usleep(10);
	}
	cout << endl;
	//fprintf(fo, "\n};\n");	

	fclose(fi);
	return soundDataObject;
}
// showProgress(...) currently not in use
void showProgress(int _currentSample, int _dataLength){
	/*cout << "[";
	for(uint8_t iProg = 0; iProg < 40; iProg++){
		cout << "progress" << "%\r";
		cout.flush();
	}
	cout << endl;
	*/
	float progress = 0.0;
	while (progress < 1.0) {
	    int barWidth = 70;

	    cout << "[";
	    int pos = barWidth * progress;
	    for (int i = 0; i < barWidth; ++i) {
	        if (i < pos) cout << "=";
	        else if (i == pos) cout << ">";
	        else cout << " ";
	    }
	    cout << "] " << int(progress * 100.0) << " %\r";
	    cout.flush();

	    progress += 0.16; // for demonstration only
	}
	cout << endl;
}
void fillEmptyFile(FILE *_fileStream, uint32_t _fileSize){
	printf("[ Filling ROM file with 0xFF ]\n");
	fseek(_fileStream, 0, SEEK_SET);
	for(int seekPosition = 0; seekPosition < _fileSize; seekPosition++){
		
		fputc(0xff, _fileStream);
	
	}
}
/*
float progress = 0.0;
while (progress < 1.0) {
    int barWidth = 70;

    cout << "[";
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) cout << "=";
        else if (i == pos) cout << ">";
        else cout << " ";
    }
    cout << "] " << int(progress * 100.0) << " %\r";
    cout.flush();

    progress += 0.16; // for demonstration only
}
cout << endl;
*/