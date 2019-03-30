 

////////////////////////////////////////////////////////////////////////
// Arduino Bluetooth Interface with Mindwave
//
// This is example code provided by NeuroSky, Inc. and is provided
// license free.
////////////////////////////////////////////////////////////////////////

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

/*EEPROM notes*/
/*adress 0 - minutes*/
/*adress 1 - seconds*/

LiquidCrystal_I2C lcd(0x27,16,2);
//SCL A5
//SDA A4

 
#define LED_Quality 13
#define LED_attention 15
#define LED_meditation 11
#define ModePin 5
#define controlPin 3
#define BAUDRATE 57600
#define DEBUGOUTPUT 0
//NOTE: Pin 9 used as gnd, 7 used as stable 5V

// my variebles
int clk[2]={0,0}; // (seconds,minutes)
int record[2]={0,0}; // (seconds,minutes)
bool good=0;
//int val; 
bool Mode=0;
bool deBug=1;
long noBTmessageDelay=0;
long buttonPressedTimer=0;
int power=0;
 
// checksum variables
byte generatedChecksum = 0;
byte checksum = 0;
int payloadLength = 0;
byte payloadData[64] = {0};
byte poorQuality = 0;
byte attention = 0;
byte meditation = 0;
 
// system variables
long lastReceivedPacket = 0;
boolean bigPacket = false;
 
//////////////////////////
// Microprocessor Setup //
//////////////////////////

void ledControl(bool Mode){
  if (!Mode){
    digitalWrite(LED_attention, HIGH);
    digitalWrite(LED_meditation, LOW);
  }
  else{
    digitalWrite(LED_attention, LOW);
    digitalWrite(LED_meditation, HIGH);    
  }
}

void normalize(int val){
  if ( (val>75) && (power<60) ){
    power+=4;
  }
  if ( (val<=75) && (power>0) ){
    power-=4;
  }
}

void noBTprint(){
  lcd.setCursor(0, 0);
  lcd.print("    Mind-Fan    ");
  lcd.setCursor(0, 1);
  lcd.print("No BT Connection"); 
  digitalWrite(LED_meditation, LOW);
  digitalWrite(LED_attention, LOW);
}
/*
void debugOutput(bool Mode,byte attention, byte meditation){
    lcd.setCursor(0, 0);
    lcd.print("Attention:  ");
    lcd.print(attention,DEC);
    if (!Mode)
      lcd.print("<- ");
    else
      lcd.print("   ");              
    lcd.setCursor(0, 1);
    lcd.print("Meditation: ");
    lcd.print(meditation,DEC); 
    if (Mode)
      lcd.print("<- ");
    else
      lcd.print("   ");    
}
*/

void debugOutput(bool Mode,byte attention, byte meditation){
    if (!Mode){
      lcd.setCursor(0, 0);
      lcd.print("Attention:  ");
      lcd.print(attention,DEC);
      lcd.print("   ");
      lcd.setCursor(0, 1);
      lcd.print("Power:  ");
      lcd.print(power,DEC);
      lcd.print("   ");
      
    }
    else{
      lcd.setCursor(0, 0);
      lcd.print("Meditation:  ");
      lcd.print(meditation,DEC);
      lcd.print("   ");
      lcd.setCursor(0, 1);
      lcd.print("Power:  ");
      lcd.print(power,DEC);
      lcd.print("   ");
    }
} 


void clok(bool Mode,byte attention, byte meditation){
  if ( ( (!Mode) && (attention > 50) ) || ( (Mode) &&  (meditation>50) ) )
    good=1;
  else
    good=0;
  if (good){
    clk[0]++;
    if (clk[0]==60){
      clk[1]++;
      clk[0]=0;
    }
  }
  else{
    clk[1]=0;
    clk[0]=0;
  }  
}

void playerOutput(bool Mode){
  int val;
  
  if (!Mode)
    val=0; //Attention
  else
    val=2; //Meditation
  if (clk[1]>=record[1]){ /*UPDATE SELF RECORD*/
    if (clk[0]>record[0]){
      record[1]=clk[1];
      record[0]=clk[0];
    }
  }
  if ( (!clk[0]) && (!clk[1]) ){
      if (record[1]>=EEPROM.read(val)){ /*UPDATE TOP SCORE*/
        if (record[0]>EEPROM.read(val+1)){
          EEPROM.update(val, record[1]);
          EEPROM.update(val+1, record[0]);
        }
      }
      record[0]=0;
      record[1]=0; 
  }
  lcd.setCursor(0, 0);
  lcd.print("Top:    ");
  if (EEPROM.read(val)<10){
    lcd.print(0,DEC);
    lcd.print(EEPROM.read(val),DEC);
  }
  else
    lcd.print(EEPROM.read(val),DEC);
  lcd.print(":");
  if (EEPROM.read(val+1)<10){
    lcd.print(0,DEC);
    lcd.print(EEPROM.read(val+1),DEC);
  }
  else
    lcd.print(EEPROM.read(val+1),DEC);
  lcd.setCursor(0, 1);
  lcd.print("Score:  ");
  if (clk[1]<10){
    lcd.print(0,DEC);
    lcd.print(clk[1],DEC);
  }
  else
    lcd.print(clk[1],DEC);
  lcd.print(":");
  if (clk[0]<10){
    lcd.print(0,DEC);
    lcd.print(clk[0],DEC);
  }
  else
    lcd.print(clk[0],DEC);
  lcd.print("   ");
  //Serial.println(clk[0]);
  
}

void setup()
 
{
  //EEPROM init
  /*for (int j=0;j<4;j++)
    EEPROM.update(j,0); */
  //
  Serial.begin(BAUDRATE);
  TCCR2A = 0x23;
  TCCR2B = 0x09;  // select clock
  OCR2A = 79;  // aiming for 25kHz 
  pinMode(controlPin, OUTPUT);
  OCR2B = 62;  //meyutar
  pinMode(LED_Quality, OUTPUT);           
  pinMode(LED_attention, OUTPUT);   
  pinMode(LED_meditation, OUTPUT);
  pinMode(9, OUTPUT); 
  digitalWrite(9,LOW);
  pinMode(7, OUTPUT); 
  digitalWrite(7,HIGH);
  lcd.init();   // initialize the lcd 
 
  lcd.backlight();
  pinMode(ModePin, INPUT_PULLUP);
  noBTprint();

}
 
////////////////////////////////
// Read data from Serial UART //
////////////////////////////////
byte ReadOneByte()
 
{
  int ByteRead;
  noBTmessageDelay=millis();
  while(!Serial.available()){
    if (millis()>noBTmessageDelay+5000)
      noBTprint();
  }
  ByteRead = Serial.read();
  if (!digitalRead(ModePin)){
    Serial.println("clickkkkkkkk");
    delay(150);
    Mode=!Mode;
    clk[1]=0;
    clk[0]=0;
    buttonPressedTimer=millis();
    while (!digitalRead(ModePin)){
      Serial.println(millis()-buttonPressedTimer);
      if (millis()>buttonPressedTimer+7000){
        Serial.println("success");
        deBug=!deBug;
        lcd.clear();
        break;
      }
    }
    
  }
 
#if DEBUGOUTPUT 
  Serial.print((char)ByteRead);   // echo the same byte out the USB serial (for debug purposes)
#endif
 
  return ByteRead;
}
 
/////////////
//MAIN LOOP//
/////////////
void loop()
 
{ 
  // Look for sync bytes
  if(ReadOneByte() == 170)
  {
    if(ReadOneByte() == 170)
    {
        payloadLength = ReadOneByte();
     
        if(payloadLength > 169)          //Payload length can not be greater than 169
        return;
        generatedChecksum = 0;       
        for(int i = 0; i < payloadLength; i++)
        { 
        payloadData[i] = ReadOneByte();            //Read payload into memory
        generatedChecksum += payloadData[i];
        }  
 
        checksum = ReadOneByte();         //Read checksum byte from stream     
        generatedChecksum = 255 - generatedChecksum;   //Take one's compliment of generated checksum
 
        if(checksum == generatedChecksum)
        {   
          poorQuality = 200;
          attention = 0;
          meditation = 0;
 
          for(int i = 0; i < payloadLength; i++)
          {                                          // Parse the payload
          switch (payloadData[i])
          {
          case 2:
            i++;           
            poorQuality = payloadData[i];
            bigPacket = true;           
            break;
          case 4:
            i++;
            attention = payloadData[i];                       
            break;
          case 5:
            i++;
            meditation = payloadData[i];
            break;
          case 0x80:
            i = i + 3;
            break;
          case 0x83:
            i = i + 25;     
            break;
          default:
            break;
          } // switch
        } // for loop
 
#if !DEBUGOUTPUT
 
        // *** Add your code here ***
 
        if(bigPacket)
        {
          ledControl(Mode);
          if(poorQuality == 0)
            digitalWrite(LED_Quality, HIGH);
          else
            digitalWrite(LED_Quality, LOW);
         
          Serial.print("PoorQuality: ");
          Serial.print(poorQuality, DEC);
          Serial.print(" Attention: ");
          Serial.print(attention, DEC);
          Serial.print(" Meditation: ");
          Serial.print(meditation, DEC);
          Serial.print(" Time since last packet: ");
          Serial.print(millis() - lastReceivedPacket, DEC);
          lastReceivedPacket = millis();
          Serial.print("\n");
          Serial.println("Mode: "+(String)Mode);
          Serial.println("debug: "+(String)deBug);
          Serial.println(!digitalRead(ModePin));
          if (!digitalRead(ModePin)){
            delay(150);
            Serial.println("clickkkkkkkk");
            Mode=!Mode;
            clk[1]=0;
            clk[0]=0;
            buttonPressedTimer=millis();
            while (!digitalRead(ModePin)){
              Serial.println("kaki");
              if (millis()>=buttonPressedTimer+7000){
                deBug=!deBug;
                lcd.clear();
                break;
              }
            }            
          }
          if (!Mode){
            /*val = map(attention, 0, 100, 0, 79);
            OCR2B = val;*/
            normalize(attention);
            OCR2B = power;
          }
          else{
            /*val = map(meditation, 0, 100, 0, 79);
            OCR2B = val;   */      
            normalize(meditation); 
            OCR2B = power;  
          }
          if (!deBug)
            debugOutput(Mode,attention,meditation);   
          else{
            clok(Mode,attention,meditation);
            playerOutput(Mode);
          }
        }
#endif       
        bigPacket = false;       
      }
      else {
        // Checksum Error
      }  // end if else for checksum
    } // end if read 0xAA byte
  } // end if read 0xAA byte
}
 
//- See more at: https://www.pantechsolutions.net/interfacing-mindwave-mobile-with-arduino#sthash.n4c2cB3S.dpuf
