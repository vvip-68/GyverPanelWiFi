// this example will play a random track from all on the sd 
//
// it expects the sd card to contain some mp3 files

#include <SoftwareSerial.h>
#include <DFMiniMp3.h>

// forward declare the notify class, just the name
//
class Mp3Notify; 

// define a handy type using serial and our notify class
//
typedef DFMiniMp3<HardwareSerial, Mp3Notify> DfMp3; 

// instance a DfMp3 object, 
//
DfMp3 dfmp3(Serial1);

// Some arduino boards only have one hardware serial port, so a software serial port is needed instead.
// comment out the above definitions and use these
//SoftwareSerial secondarySerial(10, 11); // RX, TX
//typedef DFMiniMp3<SoftwareSerial, Mp3Notify> DfMp3;
// DfMp3 dfmp3(secondarySerial);

// implement a notification class,
// its member methods will get called 
//
class Mp3Notify
{
public:
  static void PrintlnSourceAction(DfMp3_PlaySources source, const char* action)
  {
    if (source & DfMp3_PlaySources_Sd) 
    {
        Serial.print("SD Card, ");
    }
    if (source & DfMp3_PlaySources_Usb) 
    {
        Serial.print("USB Disk, ");
    }
    if (source & DfMp3_PlaySources_Flash) 
    {
        Serial.print("Flash, ");
    }
    Serial.println(action);
  }
  static void OnError(DfMp3& mp3, uint16_t errorCode)
  {
    // see DfMp3_Error for code meaning
    Serial.println();
    Serial.print("Com Error ");
    Serial.println(errorCode);
  }
  static void OnPlayFinished(DfMp3& mp3, DfMp3_PlaySources source, uint16_t track)
  {
    Serial.print("Play finished for #");
    Serial.println(track);  
  }
  static void OnPlaySourceOnline(DfMp3& mp3, DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "online");
  }
  static void OnPlaySourceInserted(DfMp3& mp3, DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "inserted");
  }
  static void OnPlaySourceRemoved(DfMp3& mp3, DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "removed");
  }
};


void setup() 
{
  Serial.begin(115200);

  Serial.println("initializing...");
  
  dfmp3.begin();
  dfmp3.reset(); 
  
  // show some properties and set the volume
  uint16_t volume = dfmp3.getVolume();
  Serial.print("volume ");
  Serial.println(volume);
  dfmp3.setVolume(24);
  
  uint16_t count = dfmp3.getTotalTrackCount(DfMp3_PlaySource_Sd);
  Serial.print("files ");
  Serial.println(count);

  uint16_t mode = dfmp3.getPlaybackMode();
  Serial.print("playback mode ");
  Serial.println(mode);
  
  Serial.println("starting...");
  
  dfmp3.playRandomTrackFromAll(); // random of all folders on sd
}

void loop() 
{
  // calling dfmp3.loop() periodically allows for notifications 
  // to be handled without interrupts
  dfmp3.loop();
}
