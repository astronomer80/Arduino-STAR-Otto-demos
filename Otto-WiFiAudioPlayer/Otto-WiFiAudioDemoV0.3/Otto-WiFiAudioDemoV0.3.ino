/*

 Otto-WiFiAudioDemo

 Arduino Wifi Audio player
 Use the Otto - Play Audio.html for the graphical interface

 This example for the Arduino shows how to use the
 to access to the audio player
 on the board through REST calls. It demonstrates how
 you can create your own REST audio player

 Possible commands created in this shetch:

 * "/arduino/play/"     -> play the audio file

 This example code is part of the public domain

 created 11 March 2017
 by astronomer80

  /arduino/arduino/play/
  /arduino/arduino/pause/
  /arduino/arduino/stop/
  /arduino/arduino/volume/<volume level 0-100>

examples:

 * "http://192.168.240.1:8080/arduino/volume/70"  -> set the volume level to 70
 * 
 */

#include <SPI.h>
#include <WiFiLink.h>
#include <SD.h>
#include <Audio.h>

char ssid[] = "yourNetwork";      // your network SSID (name)
char pass[] = "secretPassword";   // your network password
int keyIndex = 0;                 // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;

bool pause=false;
bool playing=false;
int volumelevel=50;
String playerCommand="";
int count = 0;
const int S = 1024; // Number of samples to read in block
uint32_t buffer[S];

File myFile;
WiFiClient client;
WiFiServer server(8080);
void setup() {
  Serial.begin(115200);
  
  delay(1000);
  Serial.println("Init");
  /* SD Init */
  while (SD.begin(SD_DETECT_PIN) != TRUE)
  {
    Serial.println("Opening SD card...");
    delay(1000);
  }

  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  server.begin();
  // you're connected now, so print out the status:
  printWifiStatus();

}

void blink(){
  pinMode(13, OUTPUT);
  for(int i=0;i<5;i++){
      digitalWrite(13, HIGH);
      delay(100);
      digitalWrite(13, LOW);
      delay(100);
      
  }
}

void loop() {
  // listen for incoming clients
  client = server.available();
  if (client) {
    // Process request
    process(client);
    // Close connection and free resources.
    client.stop();
    delay(1);

    //Manage the REST command
    if(playerCommand=="play"){      
      if(!playing){
        //If is not still playing initialize play audio
        play();
      }
      else{
        //Reset the volume level
        BSP_AUDIO_OUT_SetVolume(volumelevel);        
        //otherwise resume playing
        pause=false;          
      }
    }
    else if(playerCommand=="pause"){
      //To avoid noise the volume level is set to 0
      BSP_AUDIO_OUT_SetVolume(0);
      pause=true;
    }
    else if(playerCommand=="stop"){
      //stop playing and close the file
      stop();
    }
    else if(playerCommand=="volume"){
      if(playing && !pause)
        BSP_AUDIO_OUT_SetVolume(volumelevel);
    }
    
    Serial.println("Command:" + playerCommand);
    playerCommand="";
  }
  
  //If the player is initialized and is not in pause play the wave
  if(playing && !pause){
    //Serial.println("Playing...");
    if (myFile.available()) {
      //Serial.println("ms1:" + String(millis()));
      // Every 100 block print a '.'
      count++;
      if (count == 1000) {
        Serial.print(".");
        count = 0;
      }
      // read from the file into buffer
      myFile.read(buffer, sizeof(buffer));
  
      // Feed samples to audio
      Audio.write(buffer, sizeof(buffer));
      //Serial.println("ms2:" + String(millis()));
    }else  
      //Stop playing and close the file
      stop();
  }
}

//Parse the command
void process(WiFiClient client) {
  // read the command//
  if(listen_service(client, "arduino")){
    String command = client.readStringUntil('/');
    // is a valid command for the player?
    if (command == "play" || command=="pause" || command=="stop") {
      client.print(command);
      playerCommand=command;
    }
    else if (command == "volume") {
      volumelevelcommand(client);
      playerCommand=command;
    }
    // unknow command
    else{
      client.print(F("error: Unknow -"));
      client.print(command);
      client.print(F("- command!"));
      return;
    }
  }
}

void volumelevelcommand(WiFiClient client) {
  // Read pin number
  String levelstring;
  char c = client.read();
  // If the next character is a '/' it means we have an URL
  // with a value like: "/digital/13/1"
  // If the next character is a ' ' it means we have an URL
  // with a value like: "/digital/13"

  while(c != ' ' && c != '/'){
    levelstring+=c;
    c = client.read();
  }
  volumelevel = levelstring.toInt();
  
  // Send feedback to client
  client.print(F("Volume level:"));
  client.print(volumelevel);
}

bool listen_service(WiFiClient client, String service){

    //check service
    String currentLine="";
    while (client.connected()) {
      if (client.available()){
         char c= client.read();
         currentLine +=c;
         if (currentLine.endsWith(service+'/')){
           //client.read();
           return 1;
         }
      }
      else{
         client.print(F("error: Not found "));
         client.print(service);
         client.print(F(" service in your request"));
         client.println(F(", please check it"));
         return 0;
      }
  }

}
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: Arduino-Star-Otto-");
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.print(mac[2],HEX);
  Serial.print(mac[1],HEX);
  Serial.println(mac[0],HEX);

  // print your WiFi shield's IP address:
  //Serial.print("IP Address: 192.168.240.1");
  Serial.println();
  Serial.println("Surf the webpage http://192.168.240.1 to see the webpanel");
  Serial.println();
  Serial.println("The following REST api are enabled:");
  Serial.println();
  Serial.println("  /arduino/arduino/play/");
  Serial.println("  /arduino/arduino/pause/");
  Serial.println("  /arduino/arduino/stop/");
  Serial.println("  /arduino/arduino/volume/<volume level 0-100>");


  Serial.println();
  Serial.println("Examples:");
  Serial.println();

 Serial.println("  http://192.168.240.1:8080/arduino/play/");
 Serial.println("  http://192.168.240.1:8080/arduino/pause/");
 Serial.println("  http://192.168.240.1:8080/arduino/stop/");
 Serial.println("  http://192.168.240.1:8080/arduino/volume/70");
 
}


//Initialize audio player
void play() {
  playing=true;
  WAVE_FormatTypeDef WaveFormat;

  int duration;

  //Open the wav file
  myFile = SD.open("happymaking2.wav");
  if (!myFile.available()) {
    // if the file didn't open, print an error and stop
    Serial.println("error opening test.wav");
    while (true);
  } else {
    Serial.println("test.wav open OK");
  }

  myFile.read((void*) &WaveFormat, sizeof(WaveFormat));

  delay(1000);
  Serial.println("STARTUP AUDIO\r\n");
  delay(1000);
  Audio.begin(WaveFormat.SampleRate, 100);

  delay(1000);

  // Prepare samples
  Audio.prepare(NULL, S, volumelevel);

  delay(1000);

  
}

//Stop the sound and close the file
void stop(){
  /* reaching end of file */
  Serial.println("End of file. Thank you for listening!");
  client.print(F("STOP"));
  Audio.end();
  myFile.close();
  playing=false;
}
