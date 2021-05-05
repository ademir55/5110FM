/*Serial.begin(9600);
Serial.print("Seri haberlesme: ");
Serial.println(sayac);*/
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

Adafruit_PCD8544 display =  Adafruit_PCD8544(6, 5, 4, 3, 2 ); //Pinout:(SCLK, DIN, DC, CS, RST)
#include <TEA5767.h>
#include <Wire.h>
#include <Button.h>

TEA5767 Radio;   //Pinout SLC and SDA - Arduino pins A5 and A4

double old_frequency;
double frequency;
int search_mode = 0;
int search_direction;
unsigned long last_pressed;

// Frekans Seçme
//                           1    2   3     4   5     6    7   8   9  10  11   12   13   14   15  16    17   18   19    20   21   22    23    24    25   26                           
double    ai_Stations[26]={88.8,89.3,90.8,91.2,91.8,92.2,92.4,93.2,94,95.8,96,96.8,98.1,98.4,99.3,99.5,100,100.5,104.6,105,105.4,105.6,106,106.5,106.8,107.9};
int    i_sidx=0;        // Maksimum İstasyon Endeksi sıfırdan sayılan istasyon sayısı)
int    i_smax=23;       // Maks İstasyon Dizini - 0..22 = 23 :) (maksimum istasyon sayısı - sıfırdan sayılır)
int    i_smin=1;

Button btn_forward(A2, PULLUP); // HAFIZA RADYO BULMA YUKARI
Button btn_backward(A3, PULLUP); // HAFIZA RADYO BULMA AŞAĞI

#define incet 8  // volume - 
#define tare 9  // volume +
#define inainte A0    // search +
#define inapoi A1   // search -

byte f1, f2, f3, f4;  // her frekans bölgesi için numara
double frecventa;
int frecventa10;      // frekans x 10 

#include <EEPROM.h>

int PotWiperVoltage0 = 0;
int RawVoltage0 = 0;
float Voltage0 = 0;
int PotWiperVoltage1 = 1;
int RawVoltage1 = 0;
float Voltage1 = 0;
const int wiper0writeAddr = B00000000;
const int wiper1writeAddr = B00010000;
const int tconwriteAddr = B01000000;
const int tcon_0off_1on = B11110000;
const int tcon_0on_1off = B00001111;
const int tcon_0off_1off = B00000000;
const int tcon_0on_1on = B11111111;
const int slaveSelectPin = 10;   
const int shutdownPin = 7;

// transform lin to log
float a = 0.2;     // for x = 0 to 63% => y = a*x;
float x1 = 63;
float y1 = 12.6;   // 
float b1 = 2.33;     // for x = 63 to 100% => y = a1 + b1*x;
float a1 = -133.33;

int level, level1, level2, stepi;

int lung = 300;    // buton için duraklat
int scurt = 25;   // tekrar döngüsü için duraklat

byte sunet = 0;    
byte volmax = 15;

void setup () {
//Serial.begin(9600);
   // slaveSelectPin'i bir çıktı olarak ayarlayın:
  pinMode (slaveSelectPin, OUTPUT);
  //shutdownPin'i bir çıktı olarak ayarlayın:
  pinMode (shutdownPin, OUTPUT);
  // tüm kapların kapanmasıyla başla
  digitalWrite(shutdownPin,LOW);
  // SPI'yı başlat:
  SPI.begin(); 
 
digitalPotWrite(wiper0writeAddr, 0); // Sileceği ayarla 0 
digitalPotWrite(wiper1writeAddr, 0); // Sileceği ayarla 0 

// son frekansın okuma değeri
f1 = EEPROM.read(101);
f2 = EEPROM.read(102);
f3 = EEPROM.read(103);
f4 = EEPROM.read(104);
stepi = EEPROM.read(105);

// numarayı kurtar
frecventa = 100*f1 + 10*f2 + f3 + 0.1*f4;

pinMode(inainte, INPUT);
pinMode(inapoi, INPUT);
pinMode(incet, INPUT);
pinMode(tare, INPUT);
digitalWrite(inainte, HIGH);
digitalWrite(inapoi, HIGH);
digitalWrite(incet, HIGH);
digitalWrite(tare, HIGH);

  Wire.begin();
  Radio.init();
  Radio.set_frequency(frecventa); 
  i_sidx=7; // İstasyonda indeks = 15 ile başlayın (Eski - istasyon 15'den başlıyoruz, ancak sıfırdan sayıyoruz)
  Radio.set_frequency(ai_Stations[i_sidx]); 


  display.begin();
  // başlatma tamamlandı
  
  // en iyi görüntüleme için!
  display.setContrast(50);  // 50..100 ekranı uyarlamak için kontrastı değiştirebilirsiniz
  display.clearDisplay();
  // LCD'ye bir logo mesajı yazdırın.
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("Aydin");
  display.setCursor(24, 8);
  display.print("DEMIR");
  display.setTextSize(2);
  display.setCursor(1, 16);
  display.print("FM");  
  display.setTextSize(1);
  display.setCursor(30, 20);
  display.setTextColor(WHITE, BLACK);
  display.print(" radio "); 
  display.setCursor(0, 32);
  display.setTextColor(BLACK);
  display.print("24/02/2021"); 
  display.display();
  delay (5000);
  display.clearDisplay(); 

int procent1 = stepi * 6.66;
    if (procent1 < 63)  // for x = 0 to 63% => y = a*x;
    {
    level = a * procent1;
    }
    else               // for x = 63 to 100% => y = y1 + b*x;
    {
    level = a1 + b1 * procent1;
    }  
level1 = map(level, 0, 100, 0, 255);      // 256 adımda% 100 dönüştürün 
digitalPotWrite(wiper0writeAddr, level1); // Sileceği 0 ile 255 arasında ayarlayın
digitalPotWrite(wiper1writeAddr, level1); // Sileceği 0 ile 255 arasında ayarlayın
digitalWrite(shutdownPin,HIGH); //Turn off shutdown
}

void loop () {
  
  unsigned char buf[5];
  int stereo;
  int signal_level;
  double current_freq;
  unsigned long current_millis = millis();
  
  if (Radio.read_status(buf) == 1) {
    current_freq =  floor (Radio.frequency_available (buf) / 100000 + .5) / 10;
    stereo = Radio.stereo(buf);
    signal_level = Radio.signal_level(buf);
   display.setTextSize(2);
   display.setTextColor(BLACK);
   display.setCursor(0,0);
   if (current_freq < 100) display.print(" ");
   display.print(current_freq,1);
   display.setTextSize(1);
   display.print("MHz");
   // FM sinyalinin görüntülenme seviyesi ..
   display.setCursor(50,40);
   display.setTextSize(1);
   if (signal_level<10) display.print(" ");
   display.print(signal_level);
   display.print("/15");
// **************************************************
// istasyon adlarını görüntüleme
    printpost(current_freq);
// **************************************************
if (stepi <=0) stepi = 0;
if (stepi >=volmax) stepi = volmax;
   display.setCursor(0,40);
   display.setTextSize(1);
   display.setTextColor(BLACK);
   if (stepi<10) display.print("");
   display.print("Ses ");
   display.setTextColor(WHITE, BLACK);
   display.print(stepi);
   display.display();
   delay (500);
   display.clearDisplay(); 

// sinyal değerini çiz
int sus = 8;
int sl = signal_level;
   for (int x = 0; x < sl; x++)
   { 
   display.drawLine(60+2*x, 45-sus, 60+2*x, 45-x-sus, BLACK);
   }
   if (stereo) {
   display.setCursor(45,16);
   display.setTextSize(1);
   display.setTextColor(WHITE,BLACK);
   display.print("STEREO");
   }
    else
   {
    display.setCursor(45,16);
    display.setTextSize(1);
    display.setTextColor(WHITE, BLACK);  
    display.print(" MONO ");
   }     
  }
   if (search_mode == 1) {
      if (Radio.process_search (buf, search_direction) == 1) {
          search_mode = 0;
      }
           if (Radio.read_status(buf) == 1) {  
      frecventa =  floor (Radio.frequency_available (buf) / 100000 + .5) / 10;
      frecventa10 = 10*frecventa;
 f1 = frecventa10/1000;
 frecventa10 = frecventa10 - 1000*f1;
 f2 = frecventa10/100;
 frecventa10 = frecventa10 - 100*f2;
 f3 = frecventa10/10;
 f4 = frecventa10 - 10*f3;
EEPROM.write(101,f1);
EEPROM.write(102,f2);
EEPROM.write(103,f3);
EEPROM.write(104,f4);
frecventa = 100*f1 + 10*f2 + f3 + 0.1*f4;
Radio.set_frequency(frecventa);
      }
  }
if (sunet == 1) {
if (stepi <= 0) stepi = 0;
if (stepi >= volmax) stepi = volmax;
  EEPROM.write(105,stepi);
int procent1 = stepi * 6.66;
    if (procent1 < 63)  // for x = 0 to 63% => y = a*x;
    {
    level = a * procent1;
    }
    else               // for x = 63 to 100% => y = y1 + b*x;
    {
    level = a1 + b1 * procent1;
    }  
level1 = map(level, 0, 100, 0, 255);      // convert 100% in 256 steps  

if (level1 >255) level1 = 255;
digitalPotWrite(wiper0writeAddr, level1); // Set wiper 0 to 255
digitalPotWrite(wiper1writeAddr, level1); // Set wiper 0 to 255
sunet = 0;
}

//istasyon değişikliği yukarı
   if (btn_forward.isPressed()) {
   i_sidx++;
   if (i_sidx>i_smax){i_sidx=0;
   }
   Radio.set_frequency(ai_Stations[i_sidx]);
   delay(lung/5);
  }
  //istasyon değişikliği aşağı
   if (btn_backward.isPressed()) {
   i_sidx--;
   if (i_sidx<i_smin){i_sidx=23;
   }
   Radio.set_frequency(ai_Stations[i_sidx]);
   delay(lung/5);
  }
  
 if (digitalRead(inainte) == LOW) { 
    last_pressed = current_millis;
    search_mode = 1;
    search_direction = TEA5767_SEARCH_DIR_UP;
    Radio.search_up(buf);
    delay(lung/5);
  }
   
 if (digitalRead(inapoi) == LOW) { 
    last_pressed = current_millis;
    search_mode = 1;
    search_direction = TEA5767_SEARCH_DIR_DOWN;
    Radio.search_down(buf);
    delay(lung/5);
  } 
  
 if (digitalRead(incet) == LOW) { 
    stepi = stepi -1;
    if (stepi <= 0) stepi == 0;
   sunet = 1;
    delay(lung/5);
   } 
   
 if (digitalRead(tare) == LOW) { 
    stepi = stepi +1;
    if (stepi > volmax ) stepi == volmax;
    sunet = 1;
    delay(lung/5);
  } 
    delay(scurt);
}

 // Bu işlev, pota SPI verilerinin gönderilmesini sağlar.
  void digitalPotWrite(int address, int value) {
  // çipi seçmek için SS pinini düşük tutun:
  digitalWrite(slaveSelectPin,LOW);
 
  //  SPI aracılığıyla adresi ve değeri gönder:
  SPI.transfer(address);
  SPI.transfer(value);
  //çipin seçimini kaldırmak için SS pinini yükseğe alın:
  digitalWrite(slaveSelectPin,HIGH); 
  
}
// istasyon adlarını görüntüleme 
 void printpost(double current_freq)
{
 // switch(current_freq)
  {    
   if (current_freq==88.8) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("Show Radyo");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("1");}
   
   if (current_freq==89.3) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("ALEM FM");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("2");}
   
   if (current_freq==90.8) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("SUPER FM");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("3");}
   
   if (current_freq==91.2) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("DENIZLI NET");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("4");}
       
   if (current_freq==91.8) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("Radyo 20");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("5");}
   
    if (current_freq==92.2) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("TRT FM");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("6");}
   
   if (current_freq==92.4) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("RADYO SES");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("7");}

    if (current_freq==93.2) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("TRT FM");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("8");}
   
   if (current_freq==94.0) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("TRT TURkU");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("9");}
   
   if (current_freq==95.8) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("TRT NaGme");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("10");}
   
   if (current_freq==96.0) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("METRO FM");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("11");}
   
   if (current_freq==96.8) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("AKRA FM");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("12");}

   if (current_freq==98.1) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("TATLISES");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("13");}

   if (current_freq==98.4) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("BEST FM");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("14");}

   if (current_freq==99.3) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("Pal Nostalji");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("15");}

  if (current_freq==99.5) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("Radyo EGE");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("16");}

    if (current_freq==100.0) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("POWER FM");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("17");}

   if (current_freq==100.5) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MUJDE FM");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("18");}

    if (current_freq==104.6) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("polis Rad");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("19");}

   if (current_freq==105.0) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("RADYO HOROZ");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("20");}

   if (current_freq==105.4) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("GURBETCI");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("21");}

   if (current_freq==105.6) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("HABERTURK");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("22");}

   if (current_freq==106.0) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("Dogus FM");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("23");}

   if (current_freq==106.5) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("JOYTURK FM");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("24");}

   if (current_freq==106.8) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("Dostlar FM");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("25");}

   if (current_freq==107.9) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("TRT Haber");
   display.setCursor(0,16);display.setTextSize(1);
   display.setTextColor(BLACK);display.print("MEM ");
   display.print("26");}
  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx 
 
   if (current_freq==89.6) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("Kafa RADYO");}
    
   if (current_freq==90.0) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("TRT RADYO3");}
   
   if (current_freq==90.2) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("TURKUVAZ");}

   if (current_freq==94.2) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("TGRT FM");}

   if (current_freq==95.2) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("TRT RADYO1");}

   if (current_freq==97.8) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("TRT NAGME");}

    if (current_freq==99.6) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("Radyo SEYMEN");}

    if (current_freq==99.8) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("KRAL POP");}

   if (current_freq==102.2) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("ART FM");}
   
    if (current_freq==103.5) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("DEHA");}

   if (current_freq==104.2) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("RADYOSPOR");}

   if (current_freq==107.7) { display.setCursor(0,28);
   display.setTextSize(1);
   display.setTextColor(BLACK);display.print("RADYO 7");}
   
  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx          
 } 
}
