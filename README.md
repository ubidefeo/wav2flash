#wav2flash

Initially a rework of the popular [wav2c](https://github.com/olleolleolle/wav2c) used to convert 8 bit, 8kHz, Mono Wave files to c headers, this evolved into a way to parse said wave files into data for an SPI Flash memory.

Based on how data is organised in a RIFF/WAVE file, the sound data portion is transposed into a _rom_ file and _offset_/_length_ are stored as a group of 5 bytes in the initial section of the binary file.

The first byte represents the amount of wave chunks present in the binary, let's call this _N_.
Following, _N*5_ Bytes contain _address_ (3 Bytes) and _length (2 Bytes) of each chunk.

Data is organised linearly to be read from a microcontroller over SPI or other protocol.


### build
`make all` will build the source into a binary using c++
`make install` will build and copy the binary to /usr/local/sbin (you may have to change this)

### usage
`wav2flash -o outputFile [-s ROM size] [-i inputFile(s)]`
`-s may be omitted and defaults to a 2Mbit (262144)`
`-i is added as last option/argument to allow expanding of multiple files (i.e.: folder/*.wav)`

### using it with flashrom
using [flashrom](https://www.flashrom.org/Flashrom) and a [Bus Pirate 3.6 or 4](https://www.seeedstudio.com/Bus-Pirate-v4-p-740.html) the data file can be uploaded to an SPI chip or dumped to a local file.
`make dump` will extract the Flash content to `flashDump.rom`
`make flash` will take the `outputFile` from the `rom` folder and flash it.
The above process can be a bit cumbersome because of issues with the Bus Pirate firmware, but once you're up-to-date it should be fine.