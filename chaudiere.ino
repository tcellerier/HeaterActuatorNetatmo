/// Script pour réguler la température maison via une vanne motorisée de la chaudière (signal 2-10 V) et le thermostat intelligent Netatmo
//  https://github.com/tcellerier/HeaterActuatorNetatmo
//
// Thomas Cellerier
// 
// v1 - 2022-02-28 -> Ouvre ou ferme la vanne de X % toutes les 10 minutes selon le statut du contact sec du thermostat
// v2 - 2022-03-01 ->  Compensation lors des changements d'état pour contrer les osciallations de température (ex: après long arrêt, on augmente plus fort et on attend plus longtemps pour le coup d'après)


int PinInputThermostat;
int PinOutputDAC;
int ValuePercentage;
int ValuePercentageAdjusted;
int ThermostatValue;
int ThermostatPreviousValue;
int TimeDelay;
int StatusCount;


void setup() {
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
  
  ThermostatValue = digitalRead(PinInputThermostat);
  TimeDelay = 600000;  // default 10 min = 600000

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
      StatusCount = constrain(StatusCount, -30, 35); // Paramètres à ajuster

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
      StatusCount = constrain(StatusCount, -30, 35); // Paramètres à ajuster

      ValuePercentage = UpdatePercentage(ValuePercentage, -4);

    }
  }



  // Phase d'ajustement post-régulation

  // Par défaut, pas d'ajustement
  ValuePercentageAdjusted = ValuePercentage;


  int ValuePWM = ConvertPercentageToRange(ValuePercentageAdjusted);
  analogWrite(PinOutputDAC, ValuePWM);

  // Debug
  //Serial.print(ValuePercentage);
  //Serial.print("%  ajusté en ")
  //Serial.print(ValuePercentageAdjusted)
  //Serial.print("% -  PWM: ");
  //Serial.print(ValuePWM);
  //Serial.print(" - StatusCount: ");
  //Serial.print(StatusCount);
  //Serial.println("");
  

  ThermostatPreviousValue = ThermostatValue;
  delay(TimeDelay);  // default wait 600s

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
  int MaxPercentage = 80;  // Max 80% -> en realité, le max est atteint à 95% avec une tension de 10V. On ne monte pas trop haut pour éviter les retours trop froids qui génèrent de la condensation
  int MinPercentage = 1;   // Min 1% -> 1% est le min avec le setup actuel

  int result = oldvalue + variation;
  result = constrain(result, MinPercentage, MaxPercentage);

  return result;
}
