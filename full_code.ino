// Code to run a stepper motor driven dividing head using an 
// Arduino Uno with a display formed from 3 x 7 segment digit displays,
// driven by 3 x 4511 decoder integrated circuits. The constant 'reduction'
// is the mechanical reduction in the dividing head, and the constant 'step_rot'
// is the number of steps per rotation of the stepper motor.
// The unit uses a push button 'button_pin', and four toggle switches
// 'mode_pin', 'tog1_pin', 'tog2_pin', and 'fwd_tog_pin' to control
// the dividing head.
//
// enable serial port access: sudo chmod a+rw /dev/ttyACM0
//
// connect terminal for printing: screen /dev/ttyACM0
//
// specify Arduino pins
// can use pins 0 - 19
//  If you must use pin 13 as a digital input, set its pinMode() to INPUT and use an external pull down resistor. 

// output pins
const int step_pin = 18;
const int fwd_pin = 19;
const int A_pin = 2;   // the four inputs for 4511 decoders, A, B, C & D
const int B_pin = 3;   // A is the least significant bit
const int C_pin = 4;
const int D_pin = 5;
const int lat1_pin = 6; // latches for 4511 decoders, 1 for each of the three decoders
const int lat2_pin = 7; // lat1_pin is least significant digit
const int lat3_pin = 8;

// input pins
const int button_pin = 9;
const int mode_pin = 10;  // mode toggle switch
const int tog1_pin = 11;  // the two toggle switches to control how many steps per button press
const int tog2_pin = 12;
const int fwd_tog_pin = 14; // toggle whether a button press moves forward or backwards 

// global constants
const long reduction = 180;
const unsigned int step_rot = 400; // steps per rotation of motor
const unsigned int nramp = 36;  // ramping steps for stepper motor
const float fact = 1000000.0 * 60.0 / (float) step_rot; // factor for computing delay between steps in micro seconds
const float dT = 50.0 * 50.0; // kinetic energy change, for ramping
const long tot_steps = (long) step_rot * reduction; // total steps to rotate dividing head spindle one turn
const unsigned int debounce_count = 10; // software debouncing

// global variables
int ndivs; // number of divisions to be indexed by dividing head, user input
unsigned int ndivs_half;
unsigned int cur_div; // current division
long step_tally;
bool old_mode;
bool last_button_state;

// index the dividing head by moving stepper motor n steps
void index(unsigned long n)
{
    unsigned long i, nmid;
    unsigned long pause; // time between steps in microseconds
    unsigned long hpause;
    unsigned long nhalf;
    float omega = 0.0;
    int steps = 0;

    bool fwd = digitalRead(fwd_tog_pin);
    digitalWrite(fwd_pin, fwd);
    nhalf = n / 2;
    if(n == 1){
        omega = omega * omega + dT; 
        omega = sqrt(omega);
        pause = fact / omega; // pause in micro seconds
        hpause = pause / 2;
        // next 4 lines cause motor to undergo 1 step
        digitalWrite(step_pin, HIGH);
        delayMicroseconds(hpause);
        digitalWrite(step_pin, LOW);
        delayMicroseconds(pause - hpause);
        ++steps; 
        return;
    }
    if(nhalf > nramp) nhalf = nramp;
    nmid = n - 2 * nhalf;
    // Ramp up motor, constant rate of kinetic energy change
    for(i=0; i<nhalf; ++i){
        omega = omega * omega + dT;
        omega = sqrt(omega);
        pause = fact / omega; // pause in micro seconds
        hpause = pause / 2;
        ++steps;
        digitalWrite(step_pin, HIGH);
        delayMicroseconds(hpause);
        digitalWrite(step_pin, LOW);
        delayMicroseconds(pause - hpause);   
        //Serial.print("step up "); 
        //Serial.println(pause, DEC); 
    }
    for(i=0; i<nmid; ++i){   
        hpause = pause / 2;
        ++steps;
        digitalWrite(step_pin, HIGH);
        delayMicroseconds(hpause);
        digitalWrite(step_pin, LOW);
        delayMicroseconds(pause - hpause);
    }
    // Ramp down motor
    for(i=0; i<nhalf; ++i){
        pause = fact / omega; // pause in micro seconds 
        hpause = pause / 2;
        ++steps;
        digitalWrite(step_pin, HIGH);
        delayMicroseconds(hpause);
        digitalWrite(step_pin, LOW);
        delayMicroseconds(pause - hpause);
        omega = omega * omega - dT;
        omega = sqrt(omega);    
        //Serial.println("step down "); 
    }
    //Serial.println(n, DEC); 
}

// display number 'num' on 3 digit display
void num_display(unsigned int num)
{
  int i1, i2, i3;
  // masks to be used with bitwise &
  const int mask1 = 1; // 1st bit high
  const int mask2 = 2; // 2nd bit high
  const int mask3 = 4; // 3rd bit high
  const int mask4 = 8; // 4th bit high
  bool b1, b2, b3, b4;
  
  num = num % 1000;
  // start with left most number
  if(num < 100){ // set to blank, binary 10
    digitalWrite(A_pin, LOW); 
    digitalWrite(B_pin, HIGH); 
    digitalWrite(C_pin, LOW);
    digitalWrite(D_pin, HIGH);
    digitalWrite(lat3_pin, LOW); // open 4511 decoder latch for half a milli second
    delayMicroseconds(500);
    digitalWrite(lat3_pin, HIGH);
  }
  else{
    i3 = num / 100; 
    b1 = i3 & mask1;
    b2 = i3 & mask2;
    b3 = i3 & mask3;
    b4 = i3 & mask4;
    digitalWrite(A_pin, b1); 
    digitalWrite(B_pin, b2); 
    digitalWrite(C_pin, b3);
    digitalWrite(D_pin, b4);
    digitalWrite(lat3_pin, LOW);
    delayMicroseconds(500);
    digitalWrite(lat3_pin, HIGH);  
  }
  // middle number
  if(num < 10){ // set to blank, binary 10
    digitalWrite(A_pin, LOW); 
    digitalWrite(B_pin, HIGH); 
    digitalWrite(C_pin, LOW);
    digitalWrite(D_pin, HIGH);
    digitalWrite(lat2_pin, LOW); // open 4511 decoder latch for half a milli second
    delayMicroseconds(500);
    digitalWrite(lat2_pin, HIGH);
  }
  else{
    i2 = num % 100;
    i2 = i2 / 10;
    b1 = i2 & mask1;
    b2 = i2 & mask2;
    b3 = i2 & mask3;
    b4 = i2 & mask4;
    digitalWrite(A_pin, b1); 
    digitalWrite(B_pin, b2); 
    digitalWrite(C_pin, b3);
    digitalWrite(D_pin, b4);
    digitalWrite(lat2_pin, LOW);
    delayMicroseconds(500);
    digitalWrite(lat2_pin, HIGH);  
  }
  // right most number
  i1 = num % 10;
  b1 = i1 & mask1;
  b2 = i1 & mask2;
  b3 = i1 & mask3;
  b4 = i1 & mask4; 
  digitalWrite(A_pin, b1); 
  digitalWrite(B_pin, b2); 
  digitalWrite(C_pin, b3);
  digitalWrite(D_pin, b4);
  digitalWrite(lat1_pin, LOW);
  delayMicroseconds(500);
  digitalWrite(lat1_pin, HIGH);  
}

// Obtain whether to increment 1, 2, 5, or 10 divisions forward or backwards depending on toggle switches
int GetIncr()
{
  int n = 0;
  bool state1, state2, fwd;
  
  state1 = digitalRead(tog1_pin);
  state2 = digitalRead(tog2_pin);
  if(state1 == HIGH) n = 1;
  if(state2 == HIGH) n += 2;
  switch(n)
  {
    case 0:
      n = 1;
      break;
    case 1:
      n = 5;
      break;
    case 2:
      n = 2;
      break;
    case 3:
      n = 10;
      break; 
  }
  fwd = digitalRead(fwd_tog_pin);
  if(fwd == HIGH) return n;
  else return -n;
}

void setup() {
  // output pins
  pinMode(step_pin, OUTPUT);
  pinMode(fwd_pin, OUTPUT);
  pinMode(A_pin, OUTPUT);
  pinMode(B_pin, OUTPUT);
  pinMode(C_pin, OUTPUT);
  pinMode(D_pin, OUTPUT);
  pinMode(lat1_pin, OUTPUT);
  pinMode(lat2_pin, OUTPUT);
  pinMode(lat3_pin, OUTPUT);
  // input pins
  pinMode(button_pin, INPUT);     
  pinMode(mode_pin, INPUT);
  pinMode(tog1_pin, INPUT);
  pinMode(tog2_pin, INPUT);
  pinMode(fwd_tog_pin, INPUT);
  // all pins set
  ndivs = 4;
  ndivs_half = ndivs / 2;
  cur_div = 0;
  step_tally = 0;
  old_mode = digitalRead(mode_pin);
  if(old_mode == HIGH) num_display(0); // run mode
  else num_display(4); // in set mode
  last_button_state = LOW;
  //Serial.begin(9600);
  digitalWrite(step_pin, LOW); 
}

void loop() {
  bool buttonState, modeState;
  int incr, new_div, i;
  long n, fsteps;
  
  modeState = digitalRead(mode_pin);
  if(modeState != old_mode){ // mode has been switched
    if(modeState == HIGH){ // run mode
      num_display(cur_div); 
    }
    else{ // set mode
      num_display(ndivs);   
    }
    old_mode = modeState;
  }
  buttonState = digitalRead(button_pin);
  if(buttonState == HIGH) { // button has been pressed
    incr = GetIncr(); // increment, to be determined from switches
    if(modeState == HIGH){ // in run mode
      if(buttonState != last_button_state){ // software: switch on rise only
        for(i=0; i<debounce_count; ++i){
          delay(1);
          if(digitalRead(button_pin) != HIGH) break; // button debounce
        }
        if(i == debounce_count){
          new_div = cur_div + incr;
          while(new_div < 0){
            new_div += ndivs;
            step_tally += tot_steps;
          }
          n = (new_div * tot_steps + ndivs_half) / ndivs; // total steps to get to new division
          fsteps = n - step_tally;
          index(abs(fsteps));
          //Serial.println(fsteps, DEC); 
          cur_div = new_div % ndivs;  
          step_tally += fsteps;
          step_tally = step_tally % tot_steps;
          num_display(cur_div);
        }
      }
    }
    else{ // in set mode
      ndivs += GetIncr();
      if(ndivs < 2) ndivs = 990;
      if(ndivs > 999) ndivs = 2;
      num_display(ndivs);
      ndivs_half = ndivs / 2;  
      cur_div = 0;
      step_tally = 0;
      delay(600);
    }
  }
  last_button_state = buttonState;
}
