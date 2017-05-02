#include <Adafruit_CircuitPlayground.h>
#include <Servo.h>


#define gLED 6            // define the relevent pins
#define rLED 10
#define lockMotor 1
#define programButton1 19
#define lockSensor 0
#define knockSensor 9
#define lockButton 4
#define servo 10
#define speaker 5

Servo myServo;
const int threshold = 200; // minimum knock *strength* required to register
const int maxKnocks = 6; // longest knock allowed
int maxTime = 1000;
int minTime = 20;
int secretCode[maxKnocks] = {100,100,100,100,100}; // array containing preprogrammed knock
int knocks[maxKnocks]; //array of length maxKnocks to store attempted knock
int knockSensorValue;
bool programming = false;
int rejectValue = 25;
int avgReject = 15;

void setup() {
  pinMode(lockMotor,OUTPUT);        // set outputs
  pinMode(gLED, OUTPUT);
  pinMode(rLED, OUTPUT);
  pinMode(programButton1, INPUT);
  CircuitPlayground.begin();
  Serial.begin(9600);

  myServo.attach(servo);            // initialize the servo motor
  myServo.write(25);
  //pinMode(programButton2, INPUT);



  CircuitPlayground.clearPixels();
  CircuitPlayground.setPixelColor(0,0,255,0); // set one pixel to green to signify on

}

void loop() {
  //listen for any knock to activate
  knockSensorValue = analogRead(knockSensor);     // read piezo sensor waiting for knock

  if (digitalRead(programButton1)){              // if we get a knock while the programming button is pushed down
    programming = true;                          //set that we are programming the code
    Serial.println("Programming...");
    CircuitPlayground.clearPixels();
    CircuitPlayground.setPixelColor(2,255,0,0);  // let the user know we are programming by turning on a pixel
    if (knockSensorValue >=threshold) {
      listenToKnock();
      delay(500);
    }
  }
  /*
  else if (lockButton) {                        // if we had used the right pushbutton to lock the door
    lockDoor();
  }
  */
  else {
    programming =false;               // set that we are not programming
    CircuitPlayground.setPixelColor(0,0,255,0);
    if (knockSensorValue >=threshold) {
      listenToKnock();
      delay(500);
    }
  }
}

void unlockDoor(){
  Serial.println("Unlocking door...");
  makeTone(speaker,440,500);    // let the user know we are unlocking the door
  myServo.write(25);            // move the servo so the lock is not engaged
  delay(500);
  lockDoor();
}

void lockDoor(){
   Serial.println("Locking door...");

   CircuitPlayground.setPixelColor(1,0,255,0); // set a pixel showing 10 seconds until close
   delay(5000);
   CircuitPlayground.setPixelColor(1,255,255,0); // set a pixel showing 5 seconds until close
   delay(5000);
   CircuitPlayground.setPixelColor(1,255,0,0);

   myServo.write(90);                           // move the servo so the lock is engaged

   makeTone(speaker,500,500);                   // let the user know that the box is locked

}

void makeTone (char speaker, int frequency, long duration) {
  int x;
  long delayAmount = (long)(1000000/frequency);
  long loopTime = (long)((duration*1000)/(delayAmount*2));
  for (x=0; x<loopTime; x++) {        // the wave will be symetrical (same time high & low)
     digitalWrite(speakerPin,HIGH);   // set speaker high
     delayMicroseconds(delayAmount);  // make wave peak
     digitalWrite(speakerPin,LOW);    // let speaker low
     delayMicroseconds(delayAmount);  // make wave minimum
  }
}

bool checkValid(){

  int currentKnockCount = 0;
  int secretKnockCount = 0;
  int maxKnockInterval = 0;
  int longest = 0;

  for (int i=0;i<maxKnocks;i++){
    // count number of knocks in the attempted knock
    if (knocks[i] > 0) {
      currentKnockCount++;
    }
    // count number of knocks in your secret knock
    if (secretCode[i] > 0) {
      secretKnockCount++;
    }
    if (knocks[i] > longest){
      longest = knocks[i]; // collect longest knock time for normalization below
    }
  }

  if (programming) {
    // normalize the new code knock times
    for (int i=0;i<currentKnockCount;i++){
      secretCode[i] = map(knocks[i],0, longest, 0, 100); // map from 0 to longest difference to between 0 and 100
    }

    return false; // don't unlock the door for new code
  }

  if (currentKnockCount != secretKnockCount){
    return false; // incorrect number of knocks- don't unlock the door
  }

  int diff = 0;
  int totalDiff = 0;
  for (int i=0;i<maxKnocks;i++){
    knocks[i] = map(knocks[i],0,longest,0,100); //normalize knocks
    diff = abs(knocks[i]-secretCode[i]); // normalized difference for knock vs. code
    if (diff > rejectValue){ // if the value is too different from the code
      return false;
    }
    totalDiff += diff; //accumulate total difference
  }
  // if the whole knock is on average too different from the code
  if ((totalDiff/secretKnockCount)>avgReject){
    return false;
  }
  // knock is correct
  return true;
}

void listenToKnock(){
   Serial.println("listening to knock...");

   // initialize array knocks to 0s
   for (int i=0; i<maxKnocks; i++) {
    knocks[i]=0;
   }

   int currentKnock = 0;
   int startTime = millis();
   int now = millis();

   bool knocking = true;
   while(knocking) {
    knockSensorValue = analogRead(knockSensor); // read from the piezo

    if (knockSensorValue >= threshold) { // if the knock is strong enough

      now = millis(); // take the current time at which the knock was registered
      if ((now-startTime)<minTime) { // take the difference in time between the current knock and the one before it
        continue;
      }
      Serial.println("knock...");
      knocks[currentKnock] = now-startTime; // set time difference between knocks
      Serial.println(knocks[currentKnock]);
      currentKnock++; // increment array variable for knock array


      if ((knocks[currentKnock] >= maxTime) || (currentKnock > maxKnocks)){ //check if knock is still happening
        Serial.println("Done knocking");
        knocking = false;

      }
      startTime = now; // reset startTime for the next knock
      Serial.println(startTime);
      Serial.println(now);
   }

  }

   // if we're not programming check to see if the knock is valid and unlock door
   if (!programming){
    if(checkValid()){ // if the knock is correct
      unlockDoor();   // unlock the door
    }
    else{             // if the knock is incorrect
      Serial.println("Incorrect code");
      makeTone(speaker,800, 250); // make two tones telling the user that the knock is incorrect
      delay(100);
      makeTone(speaker,800, 250);
    }
   }
    // if we're programming just check to see if the knock is valid
    else if (programming) {
      checkValid();

      Serial.println("New code stored");
      makeTone(speaker,250, 500);
      // blink the lights to show new code is stored
      CircuitPlayground.setPixelColor(2,0,0,0);

      CircuitPlayground.setPixelColor(1,0,255,0);
      delay(200);
      CircuitPlayground.setPixelColor(1,255,255,0);
      delay(200);
      CircuitPlayground.setPixelColor(1,0,255,0);

      }
}
