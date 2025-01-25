/*Sunsoft (GB/GBC) to MIDI converter*/
/*By Will Trowbridge*/
/*Portions based on code by ValleyBell*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define bankSize 16384

FILE* rom, * txt;
long bank;
long offset;
long tablePtrLoc;
long tableOffset;
int i, j;
char outfile[1000000];
int songNum;
long seqPtrs[4];
long songPtr;
int chanMask;
long bankAmt;
int foundTable = 0;
long firstPtr = 0;
int version = 0;

unsigned static char* romData;

const char MagicBytesA[5] = { 0x87, 0x5F, 0x16, 0x00, 0x21};
const char MagicBytesB[6] = { 0x6D, 0x07, 0x6E, 0x07, 0x6F, 0x07 };

/*Function prototypes*/
unsigned short ReadLE16(unsigned char* Data);
static void Write8B(unsigned char* buffer, unsigned int value);
static void WriteBE32(unsigned char* buffer, unsigned long value);
static void WriteBE24(unsigned char* buffer, unsigned long value);
static void WriteBE16(unsigned char* buffer, unsigned int value);
void song2txt(int songNum, long ptr);

/*Convert little-endian pointer to big-endian*/
unsigned short ReadLE16(unsigned char* Data)
{
	return (Data[0] << 0) | (Data[1] << 8);
}

static void Write8B(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = value;
}

static void WriteBE32(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF000000) >> 24;
	buffer[0x01] = (value & 0x00FF0000) >> 16;
	buffer[0x02] = (value & 0x0000FF00) >> 8;
	buffer[0x03] = (value & 0x000000FF) >> 0;

	return;
}

static void WriteBE24(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF0000) >> 16;
	buffer[0x01] = (value & 0x00FF00) >> 8;
	buffer[0x02] = (value & 0x0000FF) >> 0;

	return;
}

static void WriteBE16(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = (value & 0xFF00) >> 8;
	buffer[0x01] = (value & 0x00FF) >> 0;

	return;
}

int main(int args, char* argv[])
{
	printf("Sunsoft (GB/GBC) to TXT converter\n");
	if (args != 3)
	{
		printf("Usage: SS2TXT <rom> <bank>\n");
		return -1;
	}
	else
	{
		if ((rom = fopen(argv[1], "rb")) == NULL)
		{
			printf("ERROR: Unable to open file %s!\n", argv[1]);
			exit(1);
		}
		else
		{
			if ((rom = fopen(argv[1], "rb")) == NULL)
			{
				printf("ERROR: Unable to open file %s!\n", argv[1]);
				exit(1);
			}
			else
			{
				bank = strtol(argv[2], NULL, 16);
				if (bank != 1)
				{
					bankAmt = bankSize;
				}
				else
				{
					bankAmt = 0;
				}
				fseek(rom, ((bank - 1) * bankSize), SEEK_SET);
				romData = (unsigned char*)malloc(bankSize);
				fread(romData, 1, bankSize, rom);
				fclose(rom);

				/*Try to search the bank for song table loader - Method 1*/
				for (i = 0; i < bankSize; i++)
				{
					if ((!memcmp(&romData[i], MagicBytesA, 5)) && foundTable != 1)
					{
						tablePtrLoc = bankAmt + i + 5;
						printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
						tableOffset = ReadLE16(&romData[tablePtrLoc - bankAmt]);
						printf("Song table starts at 0x%04x...\n", tableOffset);
						foundTable = 1;
						version = 1;
					}
				}

				/*Method 2: frequency table*/
				for (i = 0; i < bankSize; i++)
				{
					if ((!memcmp(&romData[i], MagicBytesB, 6)) && foundTable != 1)
					{
						tableOffset = i + bankAmt + 6;
						printf("Song table starts at 0x%04x...\n", tableOffset);
						foundTable = 1;
					}
				}

				if (foundTable == 1)
				{
					i = tableOffset - bankAmt;
					songNum = 1;
					while ((ReadLE16(&romData[i])) > bankAmt && (ReadLE16(&romData[i])) < 0x8000)
					{
						songPtr = ReadLE16(&romData[i]);
						printf("Song %i: 0x%04X\n", songNum, songPtr);
						song2txt(songNum, songPtr);
						i += 2;
						songNum++;
					}
					printf("The operation was successfully completed!\n");
					exit(0);
				}
				else
				{
					printf("ERROR: Magic bytes not found!\n");
					exit(-1);
				}

			}
		}
	}
}

void song2txt(int songNum, long ptr)
{
	int activeChan[4];
	long romPos = 0;
	long seqPos = 0;
	int curTrack = 0;
	int tempCurTrack = 0;
	int actTrack = 0;
	int seqEnd = 0;
	int curNote = 0;
	int curNoteLen = 0;
	int curDelay = 0;
	int holdNote = 0;
	int transpose = 0;
	long macroPos = 0;
	long macroRet = 0;
	int macroTimes = 0;
	int repeat1 = 0;
	long repeat1Pos = 0;
	int repeat2 = 0;
	long repeat2Pos = 0;
	int autoLen = 0;
	int autoLenNum = 0;
	int k = 0;
	int inMacro = 0;
	unsigned char command[4];

	sprintf(outfile, "song%i.txt", songNum);
	if ((txt = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file seqs.txt!\n");
		exit(2);
	}
	else
	{
		romPos = songPtr - bankAmt;

		for (curTrack = 0; curTrack < 4; curTrack++)
		{
			activeChan[curTrack] = 0;
		}

		while (romData[romPos] != 0xFF)
		{
			tempCurTrack = romData[romPos];
			switch (tempCurTrack)
			{
			case 0x00:
				curTrack = 0;
				actTrack = (romData[romPos + 1]) + 1;
				activeChan[0] = ReadLE16(&romData[romPos + 2]);
				fprintf(txt, "Music channel %i (%i): 0x%04X\n", (curTrack + 1), actTrack, activeChan[curTrack]);
				romPos += 4;
				break;
			case 0x01:
				curTrack = 1;
				actTrack = (romData[romPos + 1]) + 1;
				activeChan[1] = ReadLE16(&romData[romPos + 2]);
				fprintf(txt, "Music channel %i (%i): 0x%04X\n", (curTrack + 1), actTrack, activeChan[curTrack]);
				romPos += 4;
				break;
			case 0x02:
				curTrack = 2;
				actTrack = (romData[romPos + 1]) + 1;
				activeChan[2] = ReadLE16(&romData[romPos + 2]);
				fprintf(txt, "Music channel %i (%i): 0x%04X\n", (curTrack + 1), actTrack, activeChan[curTrack]);
				romPos += 4;
				break;
			case 0x03:
				curTrack = 3;
				actTrack = (romData[romPos + 1]) + 1;
				activeChan[3] = ReadLE16(&romData[romPos + 2]);
				fprintf(txt, "Music channel %i (%i): 0x%04X\n", (curTrack + 1), actTrack, activeChan[curTrack]);
				romPos += 4;
				break;
			case 0x04:
				curTrack = 0;
				actTrack = (romData[romPos + 1]) + 1;
				activeChan[0] = ReadLE16(&romData[romPos + 2]);
				fprintf(txt, "SFX channel %i (%i): 0x%04X\n", (curTrack + 1), actTrack, activeChan[curTrack]);
				romPos += 4;
				break;
			case 0x05:
				curTrack = 1;
				actTrack = (romData[romPos + 1]) + 1;
				activeChan[1] = ReadLE16(&romData[romPos + 2]);
				fprintf(txt, "SFX channel %i (%i): 0x%04X\n", (curTrack + 1), actTrack, activeChan[curTrack]);
				romPos += 4;
				break;
			case 0x06:
				curTrack = 2;
				actTrack = (romData[romPos + 1]) + 1;
				activeChan[2] = ReadLE16(&romData[romPos + 2]);
				fprintf(txt, "SFX channel %i (%i): 0x%04X\n", (curTrack + 1), actTrack, activeChan[curTrack]);
				romPos += 4;
				break;
			case 0x07:
				curTrack = 3;
				actTrack = (romData[romPos + 1]) + 1;
				activeChan[3] = ReadLE16(&romData[romPos + 2]);
				fprintf(txt, "SFX channel %i (%i): 0x%04X\n", (curTrack + 1), actTrack, activeChan[curTrack]);
				romPos += 4;
				break;
			default:
				curTrack = 0;
				actTrack = (romData[romPos + 1]) + 1;
				activeChan[0] = ReadLE16(&romData[romPos + 2]);
				fprintf(txt, "Music channel %i (%i): 0x%04X\n", (curTrack + 1), actTrack, activeChan[curTrack]);
				romPos += 4;
				break;
			}
		}

		fprintf(txt, "\n");
		for (curTrack = 0; curTrack < 4; curTrack++)
		{
			if (activeChan[curTrack] != 0 && activeChan[curTrack] >= bankAmt && activeChan[curTrack] < (bankSize * 2))
			{
				fprintf(txt, "Channel %i:\n", (curTrack + 1));
				seqPos = activeChan[curTrack] - bankAmt;
				seqEnd = 0;
				autoLen = 0;
				curDelay = 0;
				inMacro = 0;

				while (seqEnd == 0)
				{
					command[0] = romData[seqPos];
					command[1] = romData[seqPos + 1];
					command[2] = romData[seqPos + 2];
					command[3] = romData[seqPos + 3];

					if (command[0] < 0xC8)
					{
						if (autoLen != 1)
						{
							curNote = command[0];
							curNoteLen = command[1];
							fprintf(txt, "Play note: %01X, length: %01X\n", curNote, curNoteLen);
							seqPos += 2;
						}
						else if (autoLen == 1)
						{
							curNote = command[0];
							fprintf(txt, "Play note: %01X\n", curNote);
							seqPos++;
						}

					}

					else if (command[0] == 0xC8)
					{
						fprintf(txt, "Set panning? (v1): %01X\n", command[1]);
						seqPos += 2;
					}

					else if (command[0] == 0xC9)
					{
						fprintf(txt, "Set panning? (v2): %01X\n", command[1]);
						seqPos += 2;
					}

					else if (command[0] == 0xCA)
					{
						fprintf(txt, "Set panning? (v3): %01X\n", command[1]);
						seqPos += 2;
					}

					else if (command[0] >= 0xCB && command[0] <= 0xCE)
					{
						if (autoLen != 1)
						{
							curNote = command[0];
							curNoteLen = command[1];
							fprintf(txt, "Play drum note: %01X, length: %01X\n", curNote, curNoteLen);
							seqPos += 2;
						}
						else if (autoLen == 1)
						{
							curNote = command[0];
							fprintf(txt, "Play drum note: %01X\n", curNote);
							seqPos++;
						}
					}

					else if (command[0] == 0xCF)
					{
						fprintf(txt, "Set noise parameters (v1): %01X, %01X, %01X\n", command[1], command[2], command[3]);
						seqPos += 4;
					}

					else if (command[0] == 0xD0)
					{
						fprintf(txt, "Set noise parameters (v2): %01X, %01X, %01X\n", command[1], command[2], command[3]);
						seqPos += 4;
					}

					else if (command[0] == 0xD1)
					{
						fprintf(txt, "Set noise parameters (v3): %01X, %01X, %01X\n", command[1], command[2], command[3]);
						seqPos += 4;
					}

					else if (command[0] == 0xD2)
					{
						fprintf(txt, "Set noise parameters (v4): %01X, %01X, %01X\n", command[1], command[2], command[3]);
						seqPos += 4;
					}

					else if (command[0] == 0xD3)
					{
						autoLen = 0;
						fprintf(txt, "Turn off automatic note length\n");
						seqPos++;
					}

					else if (command[0] >= 0xD4 && command[0] <= 0xD9)
					{
						curNoteLen = command[1];
						fprintf(txt, "Tom tom note: %01X\n", command[0]);
						seqPos++;
					}

					else if (command[0] == 0xDA)
					{

						fprintf(txt, "Set waveform with volume (v1): %04X, %01X, %01X\n", ReadLE16(&romData[seqPos + 1]), command[2], command[3]);
						seqPos += 3;
					}

					else if (command[0] == 0xDB)
					{

						fprintf(txt, "Set waveform with volume (v2): %04X, %01X, %01X\n", ReadLE16(&romData[seqPos + 1]), command[2], command[3]);
						seqPos += 3;
					}

					else if (command[0] == 0xDC)
					{

						fprintf(txt, "Set waveform with volume (v3): %04X, %01X, %01X\n", ReadLE16(&romData[seqPos + 1]), command[2], command[3]);
						seqPos += 3;
					}

					else if (command[0] == 0xDD)
					{

						fprintf(txt, "Set waveform with volume (v4): %04X, %01X, %01X\n", ReadLE16(&romData[seqPos + 1]), command[2], command[3]);
						seqPos += 3;
					}

					else if (command[0] == 0xDE)
					{

						fprintf(txt, "Set waveform with volume (v5): %04X, %01X, %01X\n", ReadLE16(&romData[seqPos + 1]), command[2], command[3]);
						seqPos += 3;
					}

					else if (command[0] == 0xDF)
					{

						fprintf(txt, "Set waveform with volume (v6): %04X, %01X, %01X\n", ReadLE16(&romData[seqPos + 1]), command[2], command[3]);
						seqPos += 3;
					}

					else if (command[0] >= 0xE0 && command[0] <= 0xE4)
					{
						fprintf(txt, "Set sweep?: %01X\n", command[0]);
						seqPos++;
					}

					else if (command[0] >= 0xE5 && command[0] <= 0xE7)
					{
						fprintf(txt, "Set volume: %01X\n", command[0]);
						seqPos++;
					}

					else if (command[0] == 0xE8)
					{
						fprintf(txt, "Sweep effect: %01X\n", command[1]);
						seqPos += 2;
					}

					else if (command[0] == 0xE9)
					{
						fprintf(txt, "Fade note effect (v1)\n");
						seqPos++;
					}

					else if (command[0] == 0xEA)
					{
						fprintf(txt, "Fade note effect (v2)\n");
						seqPos++;
					}

					else if (command[0] == 0xEB)
					{
						if (autoLen != 1)
						{
							holdNote = command[1];
							fprintf(txt, "Hold note: %01X\n", holdNote);
							seqPos += 2;
						}
						else if (autoLen == 1)
						{
							holdNote = curNoteLen;
							fprintf(txt, "Hold note\n");
							seqPos++;
						}

					}

					else if (command[0] == 0xEC)
					{
						fprintf(txt, "Set duty: %01X\n", command[1]);
						seqPos += 2;
					}

					else if (command[0] == 0xED)
					{
						fprintf(txt, "Exit macro\n");
						seqPos++;
					}

					else if (command[0] == 0xEE)
					{
						macroPos = ReadLE16(&romData[seqPos + 1]);
						macroRet = seqPos += 3;
						fprintf(txt, "Go to macro: 0x%04X\n", macroPos);
						seqPos += 3;
					}

					else if (command[0] == 0xEF)
					{
						repeat1 = command[1];
						repeat1Pos = ReadLE16(&romData[seqPos + 2]);
						fprintf(txt, "Repeat position (v1): 0x%04X, %i times\n", repeat1Pos, repeat1);
						seqPos += 4;
					}

					else if (command[0] == 0xF0)
					{
						repeat2 = command[1];
						repeat2Pos = ReadLE16(&romData[seqPos + 2]);
						fprintf(txt, "Repeat position (v2): 0x%04X, %i times\n", repeat2Pos, repeat2);
						seqPos += 4;
					}

					else if (command[0] == 0xF1)
					{
						fprintf(txt, "Go to loop point: 0x%04X\n\n", ReadLE16(&romData[seqPos + 1]));
						seqEnd = 1;
					}

					else if (command[0] == 0xF2)
					{
						autoLen = 1;
						autoLenNum = command[1];
						fprintf(txt, "Turn on automatic note length: %01X\n", autoLenNum);
						seqPos += 2;
					}

					else if (command[0] == 0xF3)
					{
						fprintf(txt, "Set tuning: %01X\n", command[1]);
						seqPos += 2;
					}

					else if (command[0] == 0xF4)
					{
						transpose = (signed char)command[1];
						fprintf(txt, "Set transpose: %i\n", transpose);
						seqPos += 2;
					}

					else if (command[0] == 0xF5)
					{
						fprintf(txt, "Set echo?: %01X\n", command[1]);
						seqPos += 2;
					}

					else if (command[0] == 0xF6)
					{
						curDelay = command[1];
						fprintf(txt, "Rest: %01X\n", curDelay);
						seqPos += 2;
					}

					else if (command[0] == 0xF7)
					{
						fprintf(txt, "Set vibrato delay time: %01X\n", command[1]);
						seqPos += 2;
					}

					else if (command[0] == 0xF8)
					{
						fprintf(txt, "Pointer to vibrato parameters: %04X\n", ReadLE16(&romData[seqPos + 1]));
						seqPos += 3;
					}

					else if (command[0] == 0xF9)
					{
						fprintf(txt, "Set envelope: %01X\n", command[1]);
						seqPos += 2;
					}

					else if (command[0] == 0xFA)
					{
						fprintf(txt, "Pointer to envelope parameters: %04X\n", ReadLE16(&romData[seqPos + 1]));
						seqPos += 3;
					}

					else if (command[0] == 0xFB)
					{
						fprintf(txt, "Pointer to waveform: %04X\n", ReadLE16(&romData[seqPos + 1]));
						seqPos += 3;
					}

					else if (command[0] == 0xFC)
					{
						fprintf(txt, "Pointer to envelope fade parameters: %04X\n", ReadLE16(&romData[seqPos + 1]));
						seqPos += 3;
					}

					else if (command[0] == 0xFD)
					{
						fprintf(txt, "Set noise sweep strength? (v1): %01X\n", command[1]);
						seqPos += 2;
					}

					else if (command[0] == 0xFE)
					{
						fprintf(txt, "Set noise sweep strength? (v1): %01X\n", command[1]);
						seqPos += 2;
					}

					else if (command[0] == 0xFF)
					{
						fprintf(txt, "End of sequence\n\n");
						seqEnd = 1;
					}

					else
					{
						fprintf(txt, "Unknown command: %01X\n", command[0]);
						seqPos++;
					}
				}
			}

		}
		fclose(txt);
	}
}