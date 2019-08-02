#include <string>
#include <iostream>
#include <cstdlib>
#include "RtMidi.h"
#include <unistd.h>

int main(int argc, char** argv)
{
  if(argc != 8) { 
    std::cerr << "Usage: ./send <roll> <pitch>" << std::endl;
    exit(1);
  }

  RtMidiOut *midiout = new RtMidiOut();
  midiout->openPort( 0 );

  std::vector<unsigned char> message(9);
  // Check available ports.
  unsigned int nPorts = midiout->getPortCount();
  if ( nPorts == 0 ) {
    std::cout << "No ports available!\n";
    delete midiout;
    exit(2);
  }
    // Open first available port.
    // Send out a series of MIDI messages.
    // Program change: 192, 5
/*    message.push_back( 192 );
    message.push_back( 5 );
    midiout->sendMessage( &message );
    // Control Change: 176, 7, 100 (volume)
    message[0] = 176;
    message[1] = 7;
    message.push_back( 100 );
    midiout->sendMessage( &message );
    // Note On: 144, 64, 90
    message[0] = 144;
    message[1] = 64;
    message[2] = 90;
    midiout->sendMessage( &message );
    usleep( 500000 ); // Platform-dependent ... see example in tests directory.*/
    // Note Off: 128, 64, 40
    message[0] = 0xf0;
    message[1] = std::stoi(argv[1]);
    message[2] = std::stoi(argv[2]);
    message[3] = std::stoi(argv[3]);
    message[4] = std::stoi(argv[4]);
    message[5] = std::stoi(argv[5]);
    message[6] = std::stoi(argv[6]);
    message[7] = std::stoi(argv[7]);
    message[8] = 0xf7;


    midiout->sendMessage( &message );
  delete midiout;
  midiout = NULL;

  return 0;
}
