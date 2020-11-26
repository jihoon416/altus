 /*핀 연결
  * #Bluetooth (HC-06)
  * Tx -> 10
  * Rx -> 9
  * #DS3231 (RTC)
  * SDA -> A4
  * SCL -> A5
  * #열선 패드
  * 모스펫 -> 5
  * #LED 띠
  * LED -> 6
  * #진동 모터
  * 전원 -> 3, 11
  * #MP3 모듈
  * 흰 -> 4
  * 노 -> 2
  * 
  * 나머지는 5V나 GND
  */

#include <SoftwareSerial.h> //블루투스 모듈과 시리얼 통신
#include <DS3231.h> //정확한 시간을 알려주는 RTC 모듈을 위한 라이브러리
#include <EEPROM.h> //아두이노가 전원 부족으로 사망해도 알람 시간을 기억할 수 있도록
#include <SPI.h>
#include "KT403A_Player.h" //MP3 모듈 라이브러리

DS3231  rtc(SDA, SCL); //RTC 모듈 객체 생성
Time t; //시간 변수 t

SoftwareSerial Bluetooth(10, 9); // TX, RX

//여기서부터는 MP3 모듈 셋업
#ifdef __AVR__
SoftwareSerial SSerial(2, 4); // RX, TX
#define COMSerial SSerial
#define ShowSerial Serial 

KT403A<SoftwareSerial> Mp3Player;
#endif
/*
#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
#define COMSerial Serial1
#define ShowSerial SerialUSB 

KT403A<Uart> Mp3Player;
#endif

#ifdef ARDUINO_ARCH_STM32F4
#define COMSerial Serial
#define ShowSerial SerialUSB 

KT403A<HardwareSerial> Mp3Player;
#endif
*/
//static uint8_t recv_cmd[8] = {};
//MP3 모듈 셋업 끝

//핀 번호
const int HEAT=5;
const int LED=6;
const int VIBR1=3;
const int VIBR2=11;

int alarm_start=5; //열과 빛이 몇 분 전부터 시작할지
int alarm_end=60; //알람이 정해진 시각 이후로 몇 초 동안 작동할지
int max_brightness=255; //빛의 최대 밝기(0~255)
int max_heat=255; //열선 패드의 최대 출력(0~255)
int max_vibration=127; //진동 모터의 최대 출력(0~255)
int volume_increase=1; 
int wake_sound=0;
int sleep_sound=11;

int count=0;

//전역 변수
char incoming; //블루투스 앱으로부터 수신받은 정보 저장

//시간 저장 - *RTC 모듈 오면 임의로 지정된 숫자(5,50,49,51) 지우기
int wake_alarm_hour;
int start_alarm_hour;
int end_alarm_hour;
//int sleep_alarm_hour;
int wake_alarm_minute;
int start_alarm_minute;
int end_alarm_minute;
//int sleep_alarm_minute;
int current_time_hour;
int current_time_minute;
int current_time_second;

int cur;

//플래그 변수
boolean settings = false;
boolean sleep = false;
boolean light = false;
boolean vibr = false;
boolean sound = false;
boolean sign_1 = false; //이 3개의 변수들은 시간이 지나면 딱 한 번만 실행되도록
boolean sign_2 = false;

void wait_for_reply() //사용자가 입력값을 주기 전까지 기다리도록 하는 function
{
  Bluetooth.flush(); while (!Bluetooth.available()); 
}

void Initialize_RTC()
{
  rtc.begin();   // rtc 객체 initialize

//#### 다음 주석처리된 코드는 처음에 시간을 설정할 때 주석 표시를 없애고 현재 시간으로 맞춰서 한 번 돌리면 된다.### 
//Serial.println("Starting"); 
//rtc.setDOW(TUESDAY);     // 요일 설정 (영어 모든 글자 대문자)
//rtc.setTime(18, 19, 40);     // 시간 설정 (24시간 기준)
//rtc.setDate(9, 3, 2019);   // 날짜 설정
//Serial.println("Finished");
}

//unsigned long int start_time;
//unsigned long int light_start;
void setup() {
  // put your setup code here, to run once:
  //start_time = millis();
  
  Initialize_RTC();
  Bluetooth.begin(9600); //앱과 블루투스 통신
  Serial.begin(9600); //디버깅을 위한 시리얼 모니터

  //EEPROM에 전에 저장된 것이 있으면 알람 변수들로 복사
  //sleep_alarm_hour = EEPROM.read(0);
  //sleep_alarm_minute = EEPROM.read(1);
  wake_alarm_hour =  EEPROM.read(2); //블루투스로 시간을 받도록 코드 바뀌면 주석 해제하기
  wake_alarm_minute = EEPROM.read(3); //블루투스로 시간을 받도록 코드 바뀌면 주석 해제하기
  
  max_heat = EEPROM.read(4);
  max_vibration = EEPROM.read(5);
  max_brightness = EEPROM.read(6);
  alarm_start = EEPROM.read(7);
  alarm_end = EEPROM.read(8);
  
  wake_sound = EEPROM.read(9);
  sleep_sound = EEPROM.read(10);

  //ShowSerial.begin(9600); 
  COMSerial.begin(9600); //MP3모듈과 통신
  //while (!ShowSerial);
  while (!COMSerial);
  Mp3Player.init(COMSerial);
  //for(int i=0;i<100;i+=1) {Mp3Player.volumeUp();} //MP3 모듈 볼륨 최대한 올리기
  Serial.println("Start");
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:
  Bluetooth.listen();
  settings=true; //true는 블루투스 앱과의 연동이 될 준비라는 뜻
  Interactive_BT(); //사용자가 블루투스로 접속하려 하는지 확인
  
  t = rtc.getTime(); //현재 시각 #RTC 모듈 사용 시 주석 해제
  current_time_hour = t.hour; //현재 시각 시 #RTC 모듈 사용 시 주석 해제
  current_time_minute = t.min; //현재 시각 분 #RTC 모듈 사용 시 주석 해제
  current_time_second = t.sec; //현재 시각 초 #RTC 모듈 사용 시 주석 해제
  //current_time_hour=5; //*RTC 모듈 사용 시 코드 제거
  //current_time_minute=start_alarm_minute+(millis()-start_time)/60000; //*RTC 모듈 사용 시 코드 제거

  //디버깅용 시리얼 모니터 출력
  //Serial.print("At time: "); Serial.print(t.hour); Serial.print (" : "); Serial.println (t.min);
  //Serial.print("Wake up at :"); Serial.print(wake_alarm_hour); Serial.print (" : "); Serial.println (wake_alarm_minute);
  /*
  Serial.print(max_heat); Serial.print(" & ");
  Serial.print(max_vibration); Serial.print(" & ");
  Serial.print(max_brightness); Serial.print(" &|& ");
  Serial.print(alarm_start); Serial.print(" & ");
  Serial.print(alarm_end); Serial.println();
  */
  //디버깅 끝

  if(sleep==true) {
    start_sleep();
    sleep=false;
  }
  
  if(current_time_hour==start_alarm_hour&&current_time_minute==start_alarm_minute&&sign_1==false) {
    start_heat();
    start_light();
    sign_1=true;
  }
  
  if(current_time_hour==wake_alarm_hour&&current_time_minute==wake_alarm_minute&&sign_2==false) {
    start_vibr();
    start_sound();
    cur = millis();
    sign_2=true;
  }

  int seconds_past = ((current_time_minute-start_alarm_minute+60)%60)*60+current_time_second; //60으로 나눈 나머지로 분 계산
  int seconds_past2 = ((current_time_minute-wake_alarm_minute+60)%60)*60+current_time_second;
  if(seconds_past2==alarm_end&&sign_2==true) {
    end_heat();
    end_light();
    end_vibr();
    end_sound();
    sign_1=false;
    sign_2=false;
  }
  
  if(light==true) {
    analogWrite(LED, (int)((float)max_brightness*(float)seconds_past/(float)(alarm_start*60.0+alarm_end)));
    //Serial.print(seconds_past);
    //Serial.print(" : ");
    //Serial.println((int)((float)max_brightness*(float)seconds_past/((float)(alarm_start+alarm_end)*60.0)));
  }

  if(vibr==true) {
    analogWrite(VIBR1, (int)((float)max_vibration*(float)seconds_past2/(float)(alarm_end)));
    analogWrite(VIBR2, (int)((float)max_vibration*(float)seconds_past2/(float)(alarm_end)));
    //Serial.print(seconds_past);
    //Serial.print(" : ");
    //Serial.println((int)((float)max_brightness*(float)seconds_past/((float)(alarm_start+alarm_end)*60.0)));
  }

  if(sound==true&&(millis()-cur)%(1000/volume_increase)==0) {
    COMSerial.listen(); Mp3Player.volumeUp(); Bluetooth.listen();
    count+=1;
    Serial.println(count);
  }
}

void start_heat() {analogWrite(HEAT, max_heat);}
void start_light() {light=true; }//light_start=millis();}
void start_vibr() {vibr=true;}
void start_sound() {sound=true; COMSerial.listen(); for(int i=0;i<100;i+=1){Mp3Player.volumeDown();} Mp3Player.playSongMP3(wake_sound); Bluetooth.listen();} //음악 재생 안 되면 여기가 문제
void start_sleep() {COMSerial.listen(); for(int i=0;i<100;i+=1){Mp3Player.volumeUp();} Mp3Player.playSongMP3(sleep_sound); 
Bluetooth.listen();}
void end_heat() {analogWrite(HEAT, 0);}
void end_light() {light=false; analogWrite(LED, 0);}
void end_vibr() {vibr=false; analogWrite(VIBR1, 0); analogWrite(VIBR2, 0); Serial.print("Vibration has finished");}
void end_sound() {COMSerial.listen(); Mp3Player.pause(); Bluetooth.listen(); sound=false;}

void Interactive_BT() //블루투스 관련 작업들을 하나로 모아놓은 곳 - 앱이랑 연동되게 만든 코드를 여기에 녹이기!!
{
  if (Bluetooth.available() > 0 && settings == true) { //if the user has sent something
  
    incoming = Bluetooth.read(); //read and clear the stack
    Serial.println(incoming); 
    /*
    Bluetooth.println("0-> Set Alarm "); Bluetooth.println("1 -> Control Lamp"); Bluetooth.println("x -> Exit Anytime"); //Display the options 
    wait_for_reply();
    incoming = Bluetooth.read(); //read what the user has sent 
    */
//Based on user request 
  if (incoming == '0')
         {
            /*
             * Bluetooth.println("Wake me at:"); wake_alarm_hour = get_hour(); wake_alarm_minute = get_minute(); 
            Bluetooth.print("Wake up alarm set at: "); Bluetooth.print(wake_alarm_hour); Bluetooth.print(" : "); Bluetooth.println(wake_alarm_minute);
            위 두 줄의 코드 대신에 앱에서 따로 받는 방법을 넣어야 함*/
            digitalWrite(LED_BUILTIN, HIGH);
            wake_alarm_hour=100; wake_alarm_minute=1000; //불가능한 숫자
            start_alarm_hour=100; start_alarm_minute=1000; //불가능한 숫자
            end_alarm_hour=100; end_alarm_minute=1000; //불가능한 숫자
            EEPROM.write(2, wake_alarm_hour);  EEPROM.write(3, wake_alarm_minute); //아두이노가 꺼지는 경우에 대비해서 저장
         }

    if (incoming == '1')
    {
      /*
      Bluetooth.println("Wake me at:"); wake_alarm_hour = get_hour(); wake_alarm_minute = get_minute(); 
      Bluetooth.print("Wake up alarm set at: "); Bluetooth.print(wake_alarm_hour); Bluetooth.print(" : "); Bluetooth.println(wake_alarm_minute);
            EEPROM.write(2, wake_alarm_hour);  EEPROM.write(3, wake_alarm_minute);
            */
            digitalWrite(LED_BUILTIN, LOW);
            delay(100);
            //wake_alarm_hour=13; wake_alarm_minute=40;
            wake_alarm_hour = get_two(); wake_alarm_minute = get_two();
            EEPROM.write(2, wake_alarm_hour);  EEPROM.write(3, wake_alarm_minute);
            if(wake_alarm_minute<alarm_start) {start_alarm_minute=wake_alarm_minute+60-alarm_start; if(wake_alarm_hour==0) {start_alarm_hour=23;} else {start_alarm_hour=wake_alarm_hour-1;}}
            else {start_alarm_minute=wake_alarm_minute-alarm_start; start_alarm_hour=wake_alarm_hour;}
            if(wake_alarm_minute+alarm_end>=60) {end_alarm_minute=wake_alarm_minute-60+alarm_end; if(wake_alarm_hour==23) {end_alarm_hour=0;} else {end_alarm_hour=wake_alarm_hour+1;}}
            else {end_alarm_minute=wake_alarm_minute+alarm_end; end_alarm_hour=wake_alarm_hour;}
            //start_time=millis();
            sign_1=false;
            sign_2=false;
            end_heat();
            end_light();
            end_vibr();
            end_sound();
    }

    if (incoming == '2')
    {
      max_heat = get_three();
      EEPROM.write(4, max_heat);
      max_vibration = get_three();
      EEPROM.write(5, max_vibration);
      max_brightness = get_three();
      EEPROM.write(6, max_brightness);
      alarm_start = get_two();
      EEPROM.write(7, alarm_start);
      alarm_end = get_three();
      EEPROM.write(8, alarm_end);

      if(wake_alarm_minute<alarm_start) {start_alarm_minute=wake_alarm_minute+60-alarm_start; if(wake_alarm_hour==0) {start_alarm_hour=23;} else {start_alarm_hour=wake_alarm_hour-1;}}
      else {start_alarm_minute=wake_alarm_minute-alarm_start; start_alarm_hour=wake_alarm_hour;}
    }

    if (incoming == '3')
    {
      wake_sound = get_two();
      EEPROM.write(9, wake_sound);
    }

    if (incoming == '4')
    {
      sleep=true;
      sleep_sound=get_two();
      sleep_sound+=10;
      EEPROM.write(10, sleep_sound);
      Serial.println(sleep_sound);
    }

/*
    if (incoming == 'x') //exit from Bluetooth mode
    {
      Bluetooth.flush();
      incoming = Bluetooth.read();
      incoming = 0;
      settings= false;
      Bluetooth.println("Back to main");
    }
*/
   }
 }

 int get_two() //설정하고 싶은 알람의 시 설정 - 블루투스 앱에 맞게 바꿔야 함!! (시간 정보 형식만)
{
  delay(100);
  char UD; char LD; //upper digit and lower digit 
  //Bluetooth.println("Enter hours");
  wait_for_reply(); //wait for user to enter something
  UD = Bluetooth.read(); delay (100); //Read the first digit 
  wait_for_reply(); //wait for user to enter something
  LD = Bluetooth.read();//Read the lower digit

  UD= int(UD)-48; LD= int(LD)-48; //convert the char to int by subtracting 48 from it

  return (UD*10)+ LD; // Comine the uper digit and lowe digit to form the number which is hours 
}

int get_three()
{
  delay(100);
  char A; char B; char C;
  wait_for_reply();
  A = Bluetooth.read(); delay(100);
  wait_for_reply();
  B = Bluetooth.read(); delay(100);
  wait_for_reply();
  C = Bluetooth.read();

  A = int(A)-48;
  B = int(B)-48;
  C = int(C)-48;

  return A*100+B*10+C;
}
