# SS2MID
Sunsoft (GB/GBC) to MIDI converter

This tool converts music from Game Boy and Game Boy Color games using Sunsoft's sound engine to MIDI format.

It works with ROM images. To use it, you must specify the name of the ROM followed by the number of the bank containing the sound data (in hex).
For games that contain multiple banks of music, you must run the program multiple times specifying where each different bank is located. However, in order to prevent files from being overwritten, the MIDI files from the previous bank must either be moved to a separate folder or renamed.
Examples:
* SS2MID "Batman (JU) [!].gb" 3
* SS2MID "Looney Tunes (E) [C].gbc" 8
* SS2MID "Trip World (E) [!].gb" D
* SS2MID "Trip World (E) [!].gb" E

This tool was based on my own reverse-engineering of the sound engine from the original Batman. As usual, a "prototype" converter, SS2TXT, is also included.

Supported games:
  * Batman
  * Batman: Return of the Joker
  * Blaster Master Jr.
  * Daffy Duck: Fowl Play
  * Daffy Duck: The Marvin Missions
  * Gremlins 2: The New Batch
  * Hisshatsu Pachinko Boy: CR Monster House
  * Looney Tunes
  * Shanghai Pocket
  * Trip World

## To do:
  * Panning support
  * Support for other versions of the sound engine (e.g. NES)
  * GBS file support
