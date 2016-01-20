#include <SoftwareSerial.h>
#include <Wire.h>
#include <EEPROM.h>

SoftwareSerial BTserial(4, 5);
int led[4]={11,10,9,8};
unsigned long kolvo=0,t=0;
byte i=0,sens=0,level=0,tme=0,stp=0,alarm=0;

void i2c_wr(byte reg,byte val) {
   Wire.beginTransmission(0x1D);
   Wire.write(reg);
   Wire.write(val);
   Wire.endTransmission();
}

byte i2c_read(byte reg) {
  byte x=0xFF;
  Wire.beginTransmission(0x1D);
  Wire.write(reg);
  Wire.requestFrom(0x1D, 1);
  if (Wire.available()) {
    x=Wire.read();
  }
  Wire.endTransmission();
  return x;
}

void rst_device() {
// Выключим звук
  noTone(6);

// Выключим все светодиоды
  for (i=0;i<4;i++) digitalWrite(led[i],0);

// Включим все светодиоды по очереди
// для видимости того, что МК работает
  for (i=0;i<4;i++) {
    digitalWrite(led[i],1);
    delay(100);
  }

// Выключим все светодиоды по очереди
  for (i=4;i>0;i--) {
    digitalWrite(led[i-1],0);
    delay(100);
  }

// Считываем параметры из памяти МК
  sens = EEPROM.read(0);
  level = EEPROM.read(1);
  tme = EEPROM.read(2);
  stp = EEPROM.read(3);

// Настройка акселлерометра
  i2c_wr(0x16,0x46);
  i2c_wr(0x18,0x00);
  i2c_wr(0x19,0x00);
  i2c_wr(0x1A,sens);
  i2c_wr(0x17,0x03);
  i2c_wr(0x17,0x00);
  
// Обнуляем количество срабатываний
  kolvo=0;
  alarm=0;
  t=millis();
}

byte set_alarm_state() {
  byte k_led=kolvo/level;
  if (k_led>4) k_led=4;
  for (i=0;i<4;i++) digitalWrite(led[i],0);
  for (i=0;i<k_led;i++) digitalWrite(led[i],1);
  if (k_led==4) { return 1; } else { return 0; }
}

void setup() {
  pinMode(3,INPUT); // кнопка
  pinMode(8,OUTPUT); // светодиод 1
  pinMode(9,OUTPUT); // светодиод 2
  pinMode(10,OUTPUT); // светодиод 3
  pinMode(11,OUTPUT); // светодиод 4
  pinMode(6,OUTPUT); // динамик
  pinMode(2,INPUT); // INT1 от акселлерометра

  BTserial.begin(9600);
//  BTserial.setTimeout(100);  
  Wire.begin();
  
  rst_device();
  
}

void loop() {
// если нажата кнопка на устройстве
  if (digitalRead(3)==1) {
    delay(70); // задержка от "дребезга" контактов
    while (digitalRead(3)); // ожидание пока кнопку отпустят
    delay(70); // задержка от "дребезга" контактов
    rst_device(); // сброс устройства в исходное состояние
  }
// Если что-то пришло с канала связи Bluetooth
  if (BTserial.available()>0) {
    delay(1000); // задержка чтобы успело все прийти в буфер
    switch (BTserial.peek()) {
      case 'S': // если первый символ "S" - то есть установка значений
                sens = BTserial.parseInt();
                level = BTserial.parseInt();
                tme = BTserial.parseInt();
                stp = BTserial.parseInt();
                
                // Запись значений в память МК 
                EEPROM.write(0,sens);
                EEPROM.write(1,level);
                EEPROM.write(2,tme);
                EEPROM.write(3,stp);
                break;
      case 'G': // если первый символ "G" - получить параметры с МК
                BTserial.print(sens);
                BTserial.print(" ");
                BTserial.print(level);
                BTserial.print(" ");
                BTserial.print(tme);
                BTserial.print(" ");
                BTserial.println(stp);
                break;
    }
    
    // очищаем буфер приема
    while (BTserial.available()>0) BTserial.read();
  }
// если сработал акселлерометр
  if (digitalRead(2)==1) {
    kolvo++;

// сброс состояния акселлерометра и подготовка его к следующей регистрации    
    i2c_wr(0x17,0x03);
    i2c_wr(0x17,0x00);

// Установка индикации и состояния тревоги
    alarm=set_alarm_state();
  }
  if ((millis()-t)>tme) {
    t=millis();
    if (kolvo<=stp) { kolvo=0; } else { kolvo=kolvo-stp; }
    alarm=set_alarm_state();
  }
  if (alarm==1) {
    tone(6,analogRead(0)*10);
    t=millis();
    BTserial.println(1);
    while (digitalRead(3)==0) {
      if ((millis()-t)>5000) {
        BTserial.println(1);
        t=millis();
      }
    }
    delay(70);
    while (digitalRead(3)==1);
    delay(70);
    rst_device();
  }
}

