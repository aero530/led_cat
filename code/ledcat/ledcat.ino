#include <Wire.h>
#include <OPT3001.h>
#include <FastLED.h>

#define NUM_LEDS 5

#define micPin 5  
#define ledPin 13
#define rightPaw 9
#define leftPaw 17
#define noseButton 1
#define neoPin 6
#define speakerEnPin 10
#define speakerPin 5



// NB: ALL NOTES DEFINED WITH STANDARD ENGLISH NAMES, EXCEPT FROM "A" 
//THAT IS CALLED WITH THE ITALIAN NAME "LA" BECAUSE A0,A1...ARE THE ANALOG PINS ON ARDUINO.
// (Ab IS CALLED Ab AND NOT LAb)
#define  C0 16.35
#define Db0 17.32
#define D0  18.35
#define Eb0 19.45
#define E0  20.60
#define F0  21.83
#define Gb0 23.12
#define G0  24.50
#define Ab0 25.96
#define LA0 27.50
#define Bb0 29.14
#define B0  30.87
#define C1  32.70
#define Db1 34.65
#define D1  36.71
#define Eb1 38.89
#define E1  41.20
#define F1  43.65
#define Gb1 46.25
#define G1  49.00
#define Ab1 51.91
#define LA1 55.00
#define Bb1 58.27
#define B1  61.74
#define C2  65.41
#define Db2 69.30
#define D2  73.42
#define Eb2 77.78
#define E2  82.41
#define F2  87.31
#define Gb2 92.50
#define G2  98.00
#define Ab2 103.83
#define LA2 110.00
#define Bb2 116.54
#define B2  123.47
#define C3  130.81
#define Db3 138.59
#define D3  146.83
#define Eb3 155.56
#define E3  164.81
#define F3  174.61
#define Gb3 185.00
#define G3  196.00
#define Ab3 207.65
#define LA3 220.00
#define Bb3 233.08
#define B3  246.94
#define C4  261.63
#define Db4 277.18
#define D4  293.66
#define Eb4 311.13
#define E4  329.63
#define F4  349.23
#define Gb4 369.99
#define G4  392.00
#define Ab4 415.30
#define LA4 440.00
#define Bb4 466.16
#define B4  493.88
#define C5  523.25
#define Db5 554.37
#define D5  587.33
#define Eb5 622.25
#define E5  659.26
#define F5  698.46
#define Gb5 739.99
#define G5  783.99
#define Ab5 830.61
#define LA5 880.00
#define Bb5 932.33
#define B5  987.77
#define C6  1046.50
#define Db6 1108.73
#define D6  1174.66
#define Eb6 1244.51
#define E6  1318.51
#define F6  1396.91
#define Gb6 1479.98
#define G6  1567.98
#define Ab6 1661.22
#define LA6 1760.00
#define Bb6 1864.66
#define B6  1975.53
#define C7  2093.00
#define Db7 2217.46
#define D7  2349.32
#define Eb7 2489.02
#define E7  2637.02
#define F7  2793.83
#define Gb7 2959.96
#define G7  3135.96
#define Ab7 3322.44
#define LA7 3520.01
#define Bb7 3729.31
#define B7  3951.07
#define C8  4186.01
#define Db8 4434.92
#define D8  4698.64
#define Eb8 4978.03
// DURATION OF THE NOTES 
#define BPM 120    //  you can change this value changing all the others
#define H 2*Q //half 2/4
#define Q 60000/BPM //quarter 1/4 
#define E Q/2   //eighth 1/8
#define S Q/4 // sixteenth 1/16
#define W 4*Q // whole 4/4


opt3001 lightsensor = opt3001();

// Define the array of leds
CRGB leds[NUM_LEDS];

int nightlightmode = 0;
uint8_t volume_in = 0;
//uint8_t gHue = 0;

void setup() {
  // put your setup code here, to run once:
  lightsensor.begin();
  pinMode(ledPin, OUTPUT);
  pinMode(speakerEnPin, OUTPUT);
  digitalWrite(speakerEnPin, 0);
  pinMode(speakerPin, OUTPUT);
  pinMode(rightPaw, INPUT);
  pinMode(leftPaw, INPUT);
  pinMode(noseButton, INPUT);
  pinMode(micPin, INPUT);
  FastLED.addLeds<WS2812, neoPin>(leds, NUM_LEDS);
}

void loop() {
  // put your main code here, to run repeatedly:


  Serial.print(lightsensor.readResult(),DEC);
  Serial.print(" lux \n\r");


  if (nightlightmode == 0) {
    int mn = 1024;     // mn only decreases
    int mx = 0;        // mx only increases
  
    // Perform 10000 reads. Update mn and mx for each one.
    for (int i = 0; i < 100; ++i) {
      int val = analogRead(micPin);
      mn = min(mn, val);
      mx = max(mx, val);
    }
    volume_in = (mx-mn) / 4;
  
    // Send min, max and delta over Serial
    //Serial.print("m=");
    //Serial.print(mn);  
    //Serial.print(" M=");
    //Serial.print(mx);  
    //Serial.print(" D=");
    //Serial.print(mx-mn);  
    //Serial.println();  

    // set LED on back of board
    //analogWrite(ledPin, volume_in);  // analogRead values go from 0 to 1023, analogWrite values from 0 to 255

    // set front leds
    if (volume_in>200) {
      confetti();

    } else {
      fill_solid( &(leds[0]), 5 /*number of leds*/, CRGB( 0, 0, (volume_in)) );  
    }
    
    FastLED.show();
  } else {
    
    //fill_solid( &(leds[0]), 5 /*number of leds*/, CRGB( 0, 0, 56 )); // nathan
    fill_solid( &(leds[0]), 5 /*number of leds*/, CRGB( 40, 0, 20 ) ); // kaylin
    FastLED.show();
    
  }



  if (digitalRead(1)==1) {

    if (nightlightmode == 1) {
      nightlightmode = 0;
    } else if (nightlightmode==0) {
      nightlightmode = 1;
    }
    playscale();
  }
  if (digitalRead(9)==0) {
    starwars();
  }
  if (digitalRead(17)==0) {
    pawpatrol();
  }
  /*
  Serial.print(digitalRead(1));
  Serial.print(" ");
  Serial.print(digitalRead(9));
  Serial.print(" ");
  Serial.print(digitalRead(17));
  Serial.print("\n\r");
*/

  delay(1);
}



void confetti() 
{
  for (int i=0; i<50; i++) {
    // random colored speckles that blink in and fade smoothly
    fadeToBlackBy( leds, NUM_LEDS, 50);
    int pos = random16(NUM_LEDS);
    leds[pos] += CHSV( random8(255), 200, 180);
    //leds[pos] += CHSV( gHue + random8(64), 200, 255);
    FastLED.show();  
    // insert a delay to keep the framerate modest
    FastLED.delay(30);
  }
}



void playscale() {
  digitalWrite(speakerEnPin, 1);
  for (int i = 100; i < 500; i=i+10) {
    tone(speakerPin,i); 
    delay(50);
  }
  digitalWrite(speakerEnPin, 0);
}

void pawpatrol() {

  //B A G B A G G E G G E B A
  digitalWrite(speakerEnPin, 1);
  
  colornote(B4,Q); // paw
  colornote(LA4,E); // pa-
  colornote(G4,H); // trol
  colornote(B4,Q); // paw
  colornote(LA4,E); // pa-
  colornote(G4,H); // trol
  colornote(G4,E); // we'll
  colornote(E4,Q); // be 
  colornote(G4,E); // there 
  colornote(G4,Q); // on 
  colornote(E4,E); // the 
  colornote(B4,Q); // dou-
  colornote(LA4,H); //ble
  
  digitalWrite(speakerEnPin, 0);
}

void colornote(int note, long duration) {
  fill_rainbow( &(leds[0]), 5 /*led count*/, note-256 /*starting hue*/);
  FastLED.show();
  tone(speakerPin,note,duration);
  delay(1+duration);  //delay duration should always be 1 ms more than the note in order to separate them.
}

void starwars() {
  digitalWrite(speakerEnPin, 1);
    colornote(LA3,Q);
    colornote(LA3,Q);
    colornote(LA3,Q);
    colornote(F3,E+S);
    colornote(C4,S);
    
    colornote(LA3,Q);
    colornote(F3,E+S);
    colornote(C4,S);
    colornote(LA3,H);
    /*
    tone(speakerPin,E4,Q); 
    delay(1+Q); 
    tone(speakerPin,E4,Q);
    delay(1+Q);
    tone(speakerPin,E4,Q);
    delay(1+Q);
    tone(speakerPin,F4,E+S);
    delay(1+E+S);
    tone(speakerPin,C4,S);
    delay(1+S);
    
    tone(speakerPin,Ab3,Q);
    delay(1+Q);
    tone(speakerPin,F3,E+S);
    delay(1+E+S);
    tone(speakerPin,C4,S);
    delay(1+S);
    tone(speakerPin,LA3,H);
    delay(1+H);
    
    tone(speakerPin,LA4,Q);
    delay(1+Q);
    tone(speakerPin,LA3,E+S);
    delay(1+E+S);
    tone(speakerPin,LA3,S);
    delay(1+S);
    tone(speakerPin,LA4,Q);
    delay(1+Q);
    tone(speakerPin,Ab4,E+S);
    delay(1+E+S);
    tone(speakerPin,G4,S);
    delay(1+S);
    
    tone(speakerPin,Gb4,S);
    delay(1+S);
    tone(speakerPin,E4,S);
    delay(1+S);
    tone(speakerPin,F4,E);
    delay(1+E);
    delay(1+E);//PAUSE
    tone(speakerPin,Bb3,E);
    delay(1+E);
    tone(speakerPin,Eb4,Q);
    delay(1+Q);
    tone(speakerPin,D4,E+S);
    delay(1+E+S);
    tone(speakerPin,Db4,S);
    delay(1+S);
    
    tone(speakerPin,C4,S);
    delay(1+S);
    tone(speakerPin,B3,S);
    delay(1+S);
    tone(speakerPin,C4,E);
    delay(1+E);
    delay(1+E);//PAUSE QUASI FINE RIGA
    tone(speakerPin,F3,E);
    delay(1+E);
    tone(speakerPin,Ab3,Q);
    delay(1+Q);
    tone(speakerPin,F3,E+S);
    delay(1+E+S);
    tone(speakerPin,LA3,S);
    delay(1+S);
    
    tone(speakerPin,C4,Q);
    delay(1+Q);
     tone(speakerPin,LA3,E+S);
    delay(1+E+S);
    tone(speakerPin,C4,S);
    delay(1+S);
    tone(speakerPin,E4,H);
    delay(1+H);
    
     tone(speakerPin,LA4,Q);
    delay(1+Q);
    tone(speakerPin,LA3,E+S);
    delay(1+E+S);
    tone(speakerPin,LA3,S);
    delay(1+S);
    tone(speakerPin,LA4,Q);
    delay(1+Q);
    tone(speakerPin,Ab4,E+S);
    delay(1+E+S);
    tone(speakerPin,G4,S);
    delay(1+S);
    
    tone(speakerPin,Gb4,S);
    delay(1+S);
    tone(speakerPin,E4,S);
    delay(1+S);
    tone(speakerPin,F4,E);
    delay(1+E);
    delay(1+E);//PAUSE
    tone(speakerPin,Bb3,E);
    delay(1+E);
    tone(speakerPin,Eb4,Q);
    delay(1+Q);
    tone(speakerPin,D4,E+S);
    delay(1+E+S);
    tone(speakerPin,Db4,S);
    delay(1+S);
    
    tone(speakerPin,C4,S);
    delay(1+S);
    tone(speakerPin,B3,S);
    delay(1+S);
    tone(speakerPin,C4,E);
    delay(1+E);
    delay(1+E);//PAUSE QUASI FINE RIGA
    tone(speakerPin,F3,E);
    delay(1+E);
    tone(speakerPin,Ab3,Q);
    delay(1+Q);
    tone(speakerPin,F3,E+S);
    delay(1+E+S);
    tone(speakerPin,C4,S);
    delay(1+S);
    
    tone(speakerPin,LA3,Q);
    delay(1+Q);
     tone(speakerPin,F3,E+S);
    delay(1+E+S);
    tone(speakerPin,C4,S);
    delay(1+S);
    tone(speakerPin,LA3,H);
    delay(1+H);
    */
    
  digitalWrite(speakerEnPin, 0);
}

