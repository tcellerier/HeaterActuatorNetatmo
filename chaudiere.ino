/// Script pour réguler la température maison via une vanne motorisée de la chaudière (signal 2-10 V) et le thermostat intelligent Netatmo
//  https://github.com/tcellerier/HeaterActuatorNetatmo
//
// Thomas Cellerier
// 
// v1 - 2022-02-28 -> Ouvre ou ferme la vanne de X % toutes les 10 minutes selon le statut du contact sec du thermostat
// v2 - 2022-03-01 ->  Compensation lors des changements d'état pour contrer les osciallations de température (ex: après long arrêt, on augmente plus fort et on attend plus longtemps pour le coup d'après)
// v3 - 2022-03-01 -> Amélioration de l'initialisation de la vanne au démarrage
// v4 - 2022-08-15  -> Ajout des ajustements via 1/ thermomètre au Sol et 2/ Thermomètre sortie chaudière (pour chauffe eau) 

#include <OneWire.h>
#include <DallasTemperature.h>


int PinInputThermostat,PinOutputDAC;
int ValuePercentage,ValuePercentageAdjusted;
int ThermostatValue,ThermostatPreviousValue;
int TimeDelay;
int TimeDelaySub;
int StatusCount;
float TempHeat,TempGround;

// Data wire is plugged into digital pin XX on the Arduino
#define ONE_WIRE_BUS 31
// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);	
// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);


// Adresse des capteurs à récuperer avec le script thermometres_CALIBRATION
uint8_t SensorGroundAddress[8] = { 0x28, 0xFF, 0x64, 0x02, 0xE9, 0x7C, 0x34, 0x2C };
uint8_t SensorHeatAddress[8] = { 0x28, 0xFF, 0x64, 0x02, 0xEE, 0x30, 0xDD, 0xF5 };


void setup() {
  // Start up the Temperature library DS18B20
  sensors.begin(); 

  // Pin numbers
  PinInputThermostat = 12; // Le 13 est réservé pour l'usage de la LED
  PinOutputDAC = DAC1;

  // Setup pin function
  pinMode(PinInputThermostat, INPUT_PULLUP);
  pinMode(PinOutputDAC, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // Initialisation
  StatusCount = 0;
  ThermostatValue = digitalRead(PinInputThermostat);
  ThermostatPreviousValue = ThermostatValue;
  // First value of percentage
  if (ThermostatValue == LOW) { // Thermostat ON au démarrage
    ValuePercentage = 41;  // à 45 après 1ere loop
  } else {  // Thermostat OFF au démarrage
    ValuePercentage = 29;  // à 25 après 1ère loop 
  }

  // Debug -> à commenter si PROD
  //Serial.begin(9600);
}



void loop() {
  
  TimeDelay = 600000;  // default 10 min = 600000. ATTENTION: doit être divisible par 10
  ThermostatValue = digitalRead(PinInputThermostat);

  // LED
  if (ThermostatValue == HIGH) {
      digitalWrite(LED_BUILTIN, LOW);
  } 
  else {
      digitalWrite(LED_BUILTIN, HIGH);
  }

  /***** si thermostat ON *****/
  if (ThermostatValue == LOW) { 

    if (ThermostatPreviousValue == LOW) { // si thermostatPrecedent ON

      StatusCount += -2;
      StatusCount = constrain(StatusCount, -30, 35); // Parametres a ajuster

      // Idée de réduire l'augmentation si statuscount augmente trop
      // VarPercentage = 4 - abs(floor(StatusCount * 1.0 / 15.0))
      ValuePercentage = UpdatePercentage(ValuePercentage, 4);
      
    } 
    else { // si thermostatPrecedent OFF

      // Idée à tester pour réduire oscillaction : diviser timedelay par 2
      ValuePercentage = UpdatePercentage(ValuePercentage, StatusCount);
      TimeDelay += 60000 * abs(StatusCount);
      StatusCount = -5; // changement de status

    }
  } 

  /***** si thermostat OFF *****/
  else {  // ThermostatValue == HIGH  

    if (ThermostatPreviousValue == LOW) {  // Si ThermostatPrecedent ON

      ValuePercentage = UpdatePercentage(ValuePercentage, StatusCount);
      TimeDelay += 60000 * abs(StatusCount);
      StatusCount = 3; // changement de status

    } 
    else { // Si ThermostatPrecedent OFF

      StatusCount += 2;
      StatusCount = constrain(StatusCount, -30, 35); // Parametres a ajuster

      // Idée de réduire la baisse si statuscount augmente trop
      // VarPercentage = 4 - abs(floor(StatusCount * 1.0 / 20.0))
      ValuePercentage = UpdatePercentage(ValuePercentage, -4);

    }
  }


  // On découpe le délai en 10 pour gérer des évenements intermédiaires
  TimeDelaySub = TimeDelay / 10;
  for (int i = 0; i < 10; i++) {

    sensors.requestTemperatures(); 
    TempGround = sensors.getTempC(SensorGroundAddress);
    TempHeat = sensors.getTempC(SensorHeatAddress);

    // Si temp départ chaudière >= XX° (ie chauffe-eau en marche) -> on divise le pourcentage par 2 
    ValuePercentageAdjusted = UpdatePercentageWaterHeat(ValuePercentage);

    // Si temp sol garage < X°C -> on augmente le pourcentage pour éviter le gel
    ValuePercentageAdjusted = UpdatePercentageGround(ValuePercentageAdjusted);

    int ValuePWM = ConvertPercentageToRange(ValuePercentageAdjusted);
    analogWrite(PinOutputDAC, ValuePWM);

    // Debug
    /*Serial.print((i+1));
    Serial.print("/10  ");
    Serial.print(ValuePercentage);
    Serial.print("%  ajuste en ");
    Serial.print(ValuePercentageAdjusted);
    Serial.print("% -  PWM: ");
    Serial.print(ValuePWM);
    Serial.print(" - StatusCount: ");
    Serial.println(StatusCount); */
    
    delay(TimeDelaySub);  // default wait 600 / 10 = 60s
  }

  ThermostatPreviousValue = ThermostatValue;
}


// Si température départ chaudière > XX°C (ce qui veut dire que le chauffe-eau est en route), on réduit l'ouverture de la vanne
int UpdatePercentageWaterHeat(int PercentageInput) {
 
  int PercentageOutput = PercentageInput;
  //Serial.print("Temp. eau depart chaudiere: ");
  //Serial.println(TempHeat);

  if (TempHeat >= 70) {
    PercentageOutput = PercentageInput * 1.0 / 3.0;
  }
  else if(TempHeat >= 60) {
    PercentageOutput = PercentageInput * 1.0 / 2.0;
  }
  else if(TempHeat >= 50) {
    PercentageOutput = PercentageInput * 1.0 / 1.5;
  }
  return PercentageOutput;
}


// Si température sol garage < 5°, on augmente l'ouverture de la vanne
int UpdatePercentageGround(int PercentageInput) {

  int PercentageOutput = PercentageInput;
  //Serial.print("Temp. sol garage : ");
  //Serial.println(TempGround);

  if (TempGround < 5) {
    PercentageOutput = UpdatePercentage(PercentageInput, 25); // On rajoute X pps 
  }
  return PercentageOutput;
}


// Convert Percentage (from 0 to 100) into PWM values (from 0 to 255) -> 0.54 to 2.71V
int ConvertPercentageToRange(int x) {
  int result = map(x, 0, 100, 0, 255);  // map(value, fromLow, fromHigh, toLow, toHigh)
  return result;
}

// Update percentage with new value
// Take into consideration Min and Max percentage values
int UpdatePercentage(int oldvalue, int variation) {

  // Parameters
  int MaxPercentage = 80;  // Max 80% ->  en realité, le max est atteint à 95% avec une tension de 10V. On ne monte pas trop haut pour éviter les retours trop froids qui génèrent de la condensation
  int MinPercentage = 1;   // Min 1% -> 1% est le min avec le setup actuel, à environ 2.17V (mais pas tout à fait au min du bouton en mode manuel)

  int result = oldvalue + variation;
  result = constrain(result, MinPercentage, MaxPercentage);

  return result;
}
