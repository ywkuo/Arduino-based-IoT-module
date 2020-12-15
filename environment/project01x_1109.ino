#include<OneWire.h> //DS18b20
#include<DallasTemperature.h> //DS18b20
#include<Wire.h> //RtcDS3231
#include<RtcDS3231.h> //RtcDS3231

#define ONE_WIRE_BUS 2 //DS18b20_DataPin
#define TEMPERATURE_PRECISION 12 //DS18b20
#define Thermometer0_Offset 0.6
#define Thermometer1_Offset 0
#define Thermometer2_Offset 0.25

#define countof(a) (sizeof(a)/sizeof(a[0]))

OneWire oneWire(ONE_WIRE_BUS); //DS18b20
DallasTemperature DS18b20(&oneWire); //DS18b20
DeviceAddress Thermometer0,Thermometer1,Thermometer2; //DS18b20
RtcDS3231<TwoWire> Rtc(Wire); //RtcDS3231
RtcDateTime now; //RtcDS3231

const double referenceVolts=2.56;
const int batteryPin=A3;

bool gy39_check(unsigned char,int);
double gy39_Illumination(); //return gy39_Illumination
double gy39_Temperature(); //return gy39_Temperature
double gy39_AirPressure(); //return gy39_AirPressure
double gy39_Humidity(); //return gy39_Humidity
double gy39_Altitude(); //return gy39_Altitude
double battery_Volts(); //return battery_Volts
void flush_serial(); //flush Serial
void sendtoLora(String);
String to_String(int);

void printAddress(DeviceAddress deviceAddress){ //DS18b20
  for (uint8_t i=0;i<8;i++){ 
    if (deviceAddress[i]<16)Serial.print("0");
    Serial.print(deviceAddress[i],HEX);
  }
}

void printTemperature(DeviceAddress deviceAddress){ //DS18b20
  double tempC=DS18b20.getTempC(deviceAddress);
  if (tempC==DEVICE_DISCONNECTED_C){
    Serial.println("Error: Could not read temperature data");
    return;
  }
  Serial.print("Temp C: ");
  Serial.print(tempC);
  Serial.print(" Temp F: ");
  Serial.print(DallasTemperature::toFahrenheit(tempC));
}

void printResolution(DeviceAddress deviceAddress) { //DS18b20
  Serial.print("Resolution: ");
  Serial.print(DS18b20.getResolution(deviceAddress));
  Serial.println();
}

void printData(DeviceAddress deviceAddress) { //DS18b20
  Serial.print("Device Address: ");
  printAddress(deviceAddress);
  Serial.print(" ");
  printTemperature(deviceAddress);
  Serial.println();
}

void printDateTime(const RtcDateTime& dt){ //RtcDS3231
  char datestring[20];
  snprintf_P(datestring, 
      countof(datestring),
      PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
      dt.Month(),
      dt.Day(),
      dt.Year(),
      dt.Hour(),
      dt.Minute(),
      dt.Second() );
    Serial.print(datestring);
}

void(* resetFunc) (void) = 0; //sleep


void setup() {
  delay(100);
// setup ADC reference
  analogReference(INTERNAL2V56);
    
  Serial.begin(9600); //Serial baud_rate
  Serial1.begin(9600); //gy39 baud_rate
  Serial2.begin(115200); //LoRa baud_rate
  
  Rtc.Begin();
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeAlarmTwo);
  now=Rtc.GetDateTime();
  Serial.println("now.Minute(): "+String(now.Minute()));
  if((now.Minute()%10)!=0){
    Serial.println("Go to Sleep.");
    Rtc.LatchAlarmsTriggeredFlags(); // clear alarm and then power off
  }

  Serial.println("Start:");

  DS18b20.begin(); //DS18b20
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(DS18b20.getDeviceCount(), DEC);
  Serial.println(" devices.");
  Serial.print("Parasite power is: ");
  if(DS18b20.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  if(!DS18b20.getAddress(Thermometer0,0))Serial.println("Unable to find address for Device 0");
  if(!DS18b20.getAddress(Thermometer1,1))Serial.println("Unable to find address for Device 1");
  if(!DS18b20.getAddress(Thermometer2,2))Serial.println("Unable to find address for Device 2");
  Serial.print("Device 0 Address: ");
  printAddress(Thermometer0);
  Serial.println();
  Serial.print("Device 1 Address: ");
  printAddress(Thermometer1);
  Serial.println();
  Serial.print("Device 2 Address: ");
  printAddress(Thermometer2);
  Serial.println();
  DS18b20.setResolution(Thermometer0,TEMPERATURE_PRECISION);
  DS18b20.setResolution(Thermometer1,TEMPERATURE_PRECISION);
  DS18b20.setResolution(Thermometer2,TEMPERATURE_PRECISION);
  Serial.print("Device 0 Resolution: ");
  Serial.print(DS18b20.getResolution(Thermometer0), DEC);
  Serial.println();
  Serial.print("Device 1 Resolution: ");
  Serial.print(DS18b20.getResolution(Thermometer1), DEC);
  Serial.println();
  Serial.print("Device 2 Resolution: ");
  Serial.print(DS18b20.getResolution(Thermometer2), DEC);
  Serial.println();

  Serial1.write(0xA5); //設定gy39
  Serial1.write(0x00);
  Serial1.write(0xA5);
  delay(100);
}
void loop() {
  int i,j[3];
  bool flag;
  String sendLora;
  float Thermometer0_Value=0,Thermometer1_Value=0,Thermometer2_Value=0,result;
  double Illumination_Value=0,Temperature_Value=0,AirPressure_Value=0,Humidity_Value=0,Altitude_Value=0;
  double Volts_Value=0,temp;
  
  //print Time
  now=Rtc.GetDateTime();
  printDateTime(now);
  Serial.println("");
  //delay(1000);
  //read Thermometer0_Value 28FF745281140247


  for (i=0;i<3;i++)
    j[3]=0; 
  Thermometer0_Value=0;
  Thermometer1_Value=0;
  Thermometer2_Value=0;
  i=0;
  do
  {
    flag = false;
    DS18b20.requestTemperatures();
    delay(100);
    if (j[0]<3)
    {
      flag=true;
      result = DS18b20.getTempC(Thermometer0);
      Serial.println("Thermometer0_Value: "+String(result));
      if ((result<70)&&(result>-10))
      {
        Thermometer0_Value += result;
        j[0]++;
      }
    }
    if (j[1]<3)
    {
      flag=true;
      result = DS18b20.getTempC(Thermometer1);
      Serial.println("Thermometer1_Value: "+String(result));
      if ((result<70)&&(result>-10))
      {
        Thermometer1_Value += result;
        j[1]++;
      }
    }
    if (j[2]<3)
    {
      flag=true;
      result = DS18b20.getTempC(Thermometer2);
      Serial.println("Thermometer2_Value: "+String(result));
      if ((result<70)&&(result>-10))
      {
        Thermometer2_Value += result;
        j[2]++;
      }
    }    
  } while((i<10)&&(flag));
  if (j[0]!=0)
    Thermometer0_Value = Thermometer0_Value/j[0];
  else
    Thermometer0_Value = 0;  // default value
  if (j[1]!=0)
    Thermometer1_Value = Thermometer1_Value/j[1];
  else
    Thermometer1_Value = 0;  // default value
  if (j[2]!=0)
    Thermometer2_Value = Thermometer2_Value/j[2];
  else
    Thermometer2_Value = 0;  // default value 

  Serial.println("Temp values: "+String(Thermometer0_Value)+", "+String(Thermometer1_Value)+", "+String(Thermometer2_Value));
 

  //read Illumination_Value
  Illumination_Value=0;
  Serial.println("read Illumination_Value"); 
  for(int i=0;i<5;i++){
    temp=gy39_Illumination();
    Serial.println(String(i)+": "+String(temp));
    Illumination_Value+=temp;
   // delay(200);
  }
  Illumination_Value/=5;
  Serial.println("Illumination_Value: "+String(Illumination_Value));

  //read Temperature_Value
  Temperature_Value = 0;
  Serial.println("read Temperature_Value"); 
  for(int i=0;i<5;i++){
    temp=gy39_Temperature();
    Serial.println(String(i)+": "+String(temp));
    Temperature_Value+=temp;
   // delay(200);
  }
  Temperature_Value/=5;
  Serial.println("Temperature_Value: "+String(Temperature_Value));

  //read AirPressure_Value
  AirPressure_Value = 0;
  Serial.println("read AirPressure_Value"); 
  for(int i=0;i<5;i++){
    temp=gy39_AirPressure();
    Serial.println(String(i)+": "+String(temp));
    AirPressure_Value+=temp;
  //  delay(200);
  }
  AirPressure_Value/=5;
  Serial.println("AirPressure_Value: "+String(AirPressure_Value));

  //read Humidity_Value
  Humidity_Value = 0;
  Serial.println("read Humidity_Value"); 
  for(int i=0;i<5;i++){
    temp=gy39_Humidity();
    Serial.println(String(i)+": "+String(temp));
    Humidity_Value+=temp;
   // delay(200);
  }
  Humidity_Value/=5;
  Serial.println("Humidity_Value: "+String(Humidity_Value));

  //read Altitude_Value
  Altitude_Value = 0;
  Serial.println("read Altitude_Value"); 
  for(int i=0;i<5;i++){
    temp=gy39_Altitude();
    Serial.println(String(i)+": "+String(temp));
    Altitude_Value+=temp;
   // delay(200);
  }
  Altitude_Value/=5;
  Serial.println("Altitude_Value: "+String(Altitude_Value));

  //read Volts_Value
  Volts_Value = 0;
  Serial.println("read Volts_Value"); 
  for(int i=0;i<5;i++){
    temp=battery_Volts();
    Serial.println(String(i)+": "+String(temp));
    Volts_Value+=temp;
   // delay(200);
  }
  Volts_Value/=5;
  Serial.println("Volts_Value: "+String(Volts_Value));

  Serial.println("Thermometer0_Value: "+String(Thermometer0_Value)+" C, Thermometer1_Value: "+String(Thermometer1_Value)+" C, Thermometer2_Value: "+String(Thermometer2_Value)
                 +" C, Illumination_Value: "+String(Illumination_Value)+" lux, Temperature_Value: "+String(Temperature_Value)+" C, AirPressure_Value: "+String(AirPressure_Value)
                 +" pa, Humidity_Value: "+String(Humidity_Value)+" %, Altitude_Value: "+String(Altitude_Value)+" m, Volts_Value: "+String(Volts_Value)+" V");
  sendLora=to_String(Thermometer0_Value*100)+to_String(AirPressure_Value/100)+to_String(Thermometer1_Value*100)+to_String(Altitude_Value)+to_String(Thermometer2_Value*100)+to_String(Volts_Value*100)+to_String(Temperature_Value*100)+to_String(Humidity_Value*100)+to_String(0)+to_String(sqrt(Illumination_Value)*100);
  Serial.println("Thermometer0_Value*100 | AirPressure_Value/100 | Thermometer1_Value*100 | Altitude_Value | Thermometer2_Value*100 | Volts_Value*100 | Temperature_Value*100 | Humidity_Value*100 | 0 | sqrt(Illumination_Value)*100");
  //              pHValue                  orpValue                temp                     ECValue          battery_volts            do_value          air_temp                humidity          water_level        bright
  
  Serial.println(sendLora);

  sendtoLora(sendLora);

  Serial.println("Done.Go to Sleep.\n-------------------------------------------------------\n");
  Rtc.LatchAlarmsTriggeredFlags(); // clear alarm and then power off

  delay(1000); 
  resetFunc(); 
}

bool gy39_check(unsigned char data[],int len){
  int total=0;
  String a,b=String(data[len-1],HEX);
  for(int i=0;i<len;i++){
    Serial.print(String(data[i],HEX)+", ");
  }
  Serial.println();
  for(int i=0;i<len-1;i++){
    total+=data[i];
  }
  a=String(total,HEX);
  if(a.length()>=2){
    a=String(a[a.length()-2])+String(a[a.length()-1]);
  }else if(a.length()==1){
    a="0"+String(a[a.length()-1]);
  }else{
    a="00";
  }
  if(b.length()==1){
    b="0"+b;
  }
  Serial.print("a:"+a+" b:"+b+" ");

  if(a==b){
    Serial.println(" ->  good, return 0");
    return 0;
  }else{
    Serial.println(" ->  bad, return 1");
    return 1;
  }
}

double gy39_Illumination(){
  double Illumination=0;
  unsigned char data[9];
  do{
    flush_serial(Serial1);
    Serial1.write(0xA5);
    Serial1.write(0x51);
    Serial1.write(0xF6);
    delay(100);
    while(Serial1.available()){
      for(int i=0;i<9;i++){
        data[i]=Serial1.read();
      }
    }
  }while(gy39_check(data,9));
  Illumination=(data[4]*16777216.0+data[5]*65536.0+data[6]*256.0+data[7])/100;
  return Illumination;
}

double gy39_Temperature(){
  double Temperature=0;
  unsigned char data[15];
  do{
    flush_serial(Serial1);
    Serial1.write(0xA5);
    Serial1.write(0x52);
    Serial1.write(0xF7);
    delay(100);
    while(Serial1.available()){
      for(int i=0;i<15;i++){
        data[i]=Serial1.read();
      }
    }
  }while(gy39_check(data,15));
  Temperature=(data[4]*256.0+data[5])/100;
  return Temperature;
}

double gy39_AirPressure(){
  double AirPressure=0;
  unsigned char data[15];
  do{
    flush_serial(Serial1);
    Serial1.write(0xA5);
    Serial1.write(0x52);
    Serial1.write(0xF7);
    delay(100);
    while(Serial1.available()){
      for(int i=0;i<15;i++){
        data[i]=Serial1.read();
      }
    }
  }while(gy39_check(data,15));
  AirPressure=(data[6]*16777216.0+data[7]*65536.0+data[8]*256.0+data[9])/100;
  return AirPressure;
}

double gy39_Humidity() {
  double Humidity=0;
  unsigned char data[15];
  flush_serial(Serial1);
  Serial1.write(0xA5);
  Serial1.write(0x52);
  Serial1.write(0xF7);
  delay(100);
  while(Serial1.available()){
    for(int i=0;i<15;i++){
      data[i]=Serial1.read();
    }
  }
  gy39_check(data,15);
  Humidity=(data[10]*256.0+data[11])/100;
  return Humidity;
}

double gy39_Altitude(){
  double Altitude=0;
  unsigned char data[15];
  do{
    flush_serial(Serial1);
    Serial1.write(0xA5);
    Serial1.write(0x52);
    Serial1.write(0xF7);
    delay(100);
    while (Serial1.available()){
      for (int i=0;i<15;i++){
        data[i]=Serial1.read();
      }
    }
  }while(gy39_check(data,15));
  Altitude=data[12]*256.0+data[13];
  return Altitude;
}

double battery_Volts(){
  double Volts=0;
  Volts=analogRead(batteryPin)/1024.0*referenceVolts*2;
  Serial.println(analogRead(batteryPin));
  return Volts; 
}

void flush_serial(Stream &serial){
  String temp="flush: ";
  while(serial.available()>0){
    temp+=serial.read();
    delay(10);
    if(serial.available()==0)Serial.println(temp);
  }
}

void sendtoLora(String sendLora){
  String response="";
  Serial.println("sendtoLora: mac join abp");
  Serial2.print("mac join abp");
  delay(3000);
  while(Serial2.available()>0){
    response+=char(Serial2.read());
  }
  Serial.println("response: "+response);
  response="";
  Serial.println("sendtoLora: mac tx ucnf 1 "+sendLora);
  Serial2.print("mac tx ucnf 1 "+sendLora);
  delay(5000);
  while(Serial2.available()>0){
    response+=char(Serial2.read());
  }
  Serial.println("response: "+response);

}

String to_String(int temp){ //int(DEC) to 4bits string(HEX) //e.g. 45 -> 002d 
  String value=String(temp,HEX);
  int j=4-value.length();
  for(int i=0;i<j;i++){
    value="0"+value;
  }
  Serial.println(String(temp)+"\t-> "+value);
  return value;
}
