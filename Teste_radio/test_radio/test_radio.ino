/* http://www.youtube.com/c/electronoobs
 * 
 * This is an example where we control two axis drone with PID 
 * and data from the radio receiver and the MPU6050 module
 * 
 * Arduino pin    |   MPU6050
 * 5V             |   Vcc
 * GND            |   GND
 * A4             |   SDA
 * A5             |   SCL
 * 
 * F_ Left__motor |   D4
 * F_ Right__motor|   D7
 * B_Left__motor  |   D5
 * B_Right__motor |   D6
  */
#include <Wire.h>
#include <Servo.h>

Servo L_F_prop;
Servo L_B_prop;
Servo R_F_prop;
Servo R_B_prop;


//We create variables for the time width values of each PWM input signal
unsigned long counter_1, counter_2, counter_3, counter_4, current_count;

//We create 4 variables to stopre the previous value of the input signal (if LOW or HIGH)
byte last_CH1_state, last_CH2_state, last_CH3_state, last_CH4_state;

//To store the 1000us to 2000us value we create variables and store each channel
int input_YAW;      //In my case channel 4 of the receiver and pin D12 of arduino
int input_PITCH;    //In my case channel 2 of the receiver and pin D9 of arduino
int input_ROLL;     //In my case channel 1 of the receiver and pin D8 of arduino
int input_THROTTLE; //In my case channel 3 of the receiver and pin D10 of arduino




/*MPU-6050 gives you 16 bits data so you have to create some float constants
*to store the data for accelerations and gyro*/

//Gyro Variables
float elapsedTime, time, timePrev;        //Variables for time control
int gyro_error=0;                         //We use this variable to only calculate once the gyro data error
float Gyr_rawX, Gyr_rawY, Gyr_rawZ;       //Here we store the raw data read 
float Gyro_angle_x, Gyro_angle_y;         //Here we store the angle value obtained with Gyro data
float Gyro_raw_error_x, Gyro_raw_error_y; //Here we store the initial gyro data error

//Acc Variables
int acc_error=0;                            //We use this variable to only calculate once the Acc data error
float rad_to_deg = 180/3.141592654;         //This value is for pasing from radians to degrees values
float Acc_rawX, Acc_rawY, Acc_rawZ;         //Here we store the raw data read 
float Acc_angle_x, Acc_angle_y;             //Here we store the angle value obtained with Acc data
float Acc_angle_error_x, Acc_angle_error_y; //Here we store the initial Acc data error

float Total_angle_x, Total_angle_y;

//More variables for the code
int i;
int mot_activated=0;
long activate_count=0;
long des_activate_count=0;

//////////////////////////////PID FOR ROLL///////////////////////////
float roll_PID, pwm_L_F, pwm_L_B, pwm_R_F, pwm_R_B, roll_error, roll_previous_error;
float roll_pid_p=0;
float roll_pid_i=0;
float roll_pid_d=0;
///////////////////////////////ROLL PID CONSTANTS////////////////////
double roll_kp=0.7;//3.55
double roll_ki=0.006;//0.003
double roll_kd=1.2;//2.05
float roll_desired_angle = 0;     //This is the angle in which we whant the

//////////////////////////////PID FOR PITCH//////////////////////////
float pitch_PID, pitch_error, pitch_previous_error;
float pitch_pid_p=0;
float pitch_pid_i=0;
float pitch_pid_d=0;
///////////////////////////////PITCH PID CONSTANTS///////////////////
double pitch_kp=0.72;//3.55
double pitch_ki=0.006;//0.003
double pitch_kd=1.22;//2.05
float pitch_desired_angle = 0;     //This is the angle in which we whant the

                              

void setup() {
    
  PCICR |= (1 << PCIE0);    //enable PCMSK0 scan                                                 
  PCMSK0 |= (1 << PCINT4);  //Set pin D8 trigger an interrupt on state change. 
  PCMSK0 |= (1 << PCINT5);  //Set pin D9 trigger an interrupt on state change.                                             
  PCMSK0 |= (1 << PCINT6);  //Set pin D10 trigger an interrupt on state change.                                               
  PCMSK0 |= (1 << PCINT7);  //Set pin D12 trigger an interrupt on state change.  

  
//  DDRB |= B00100000;  //D13 as output
//  PORTB &= B11011111; //D13 set to LOW
//
//  L_F_prop.attach(4); //left front motor
//  L_B_prop.attach(5); //left back motor
//  R_F_prop.attach(7); //right front motor 
//  R_B_prop.attach(6); //right back motor 
//  /*in order to make sure that the ESCs won't enter into config mode
//  *I send a 1000us pulse to each ESC.*/
//  L_F_prop.writeMicroseconds(1000); 
//  L_B_prop.writeMicroseconds(1000);
//  R_F_prop.writeMicroseconds(1000); 
//  R_B_prop.writeMicroseconds(1000);
  
  
//  Wire.begin();                           //begin the wire comunication
//  Wire.beginTransmission(0x68);           //begin, Send the slave adress (in this case 68)              
//  Wire.write(0x6B);                       //make the reset (place a 0 into the 6B register)
//  Wire.write(0x00);
//  Wire.endTransmission(true);             //end the transmission
//  
//  Wire.beginTransmission(0x68);           //begin, Send the slave adress (in this case 68) 
//  Wire.write(0x1B);                       //We want to write to the GYRO_CONFIG register (1B hex)
//  Wire.write(0x10);                       //Set the register bits as 00010000 (100dps full scale)
//  Wire.endTransmission(true);             //End the transmission with the gyro
//
//  Wire.beginTransmission(0x68);           //Start communication with the address found during search.
//  Wire.write(0x1C);                       //We want to write to the ACCEL_CONFIG register (1A hex)
//  Wire.write(0x10);                       //Set the register bits as 00010000 (+/- 8g full scale range)
//  Wire.endTransmission(true);  
//  
//  Serial.begin(9600);
//  delay(1000);
//  time = millis();                         //Start counting time in milliseconds


/*Here we calculate the gyro data error before we start the loop
 * I make the mean of 200 values, that should be enough*/
//  if(gyro_error==0)
//  {
//    for(int i=0; i<200; i++)
//    {
//      Wire.beginTransmission(0x68);            //begin, Send the slave adress (in this case 68) 
//      Wire.write(0x43);                        //First adress of the Gyro data
//      Wire.endTransmission(false);
//      Wire.requestFrom(0x68,4,true);           //We ask for just 4 registers 
//         
//      Gyr_rawX=Wire.read()<<8|Wire.read();     //Once again we shif and sum
//      Gyr_rawY=Wire.read()<<8|Wire.read();
//   
//      /*---X---*/
//      Gyro_raw_error_x = Gyro_raw_error_x + (Gyr_rawX/32.8); 
//      /*---Y---*/
//      Gyro_raw_error_y = Gyro_raw_error_y + (Gyr_rawY/32.8);
//      if(i==199)
//      {
//        Gyro_raw_error_x = Gyro_raw_error_x/200;
//        Gyro_raw_error_y = Gyro_raw_error_y/200;
//        gyro_error=1;
//      }
//    }
//  }//end of gyro error calculation   


/*Here we calculate the acc data error before we start the loop
 * I make the mean of 200 values, that should be enough*/
//  if(acc_error==0)
//  {
//    for(int a=0; a<200; a++)
//    {
//      Wire.beginTransmission(0x68);
//      Wire.write(0x3B);                       //Ask for the 0x3B register- correspond to AcX
//      Wire.endTransmission(false);
//      Wire.requestFrom(0x68,6,true); 
//      
//      Acc_rawX=(Wire.read()<<8|Wire.read())/4096.0 ; //each value needs two registres
//      Acc_rawY=(Wire.read()<<8|Wire.read())/4096.0 ;
//      Acc_rawZ=(Wire.read()<<8|Wire.read())/4096.0 ;
//
//      
//      /*---X---*/
//      Acc_angle_error_x = Acc_angle_error_x + ((atan((Acc_rawY)/sqrt(pow((Acc_rawX),2) + pow((Acc_rawZ),2)))*rad_to_deg));
//      /*---Y---*/
//      Acc_angle_error_y = Acc_angle_error_y + ((atan(-1*(Acc_rawX)/sqrt(pow((Acc_rawY),2) + pow((Acc_rawZ),2)))*rad_to_deg)); 
//      
//      if(a==199)
//      {
//        Acc_angle_error_x = Acc_angle_error_x/200;
//        Acc_angle_error_y = Acc_angle_error_y/200;
//        acc_error=1;
//      }
//    }
//  }//end of acc error calculation  
//}//end of setup loop

void loop() {
  
  
  /////////////////////////////I M U/////////////////////////////////////
  timePrev = time;  // the previous time is stored before the actual time read
  time = millis();  // actual time read
  elapsedTime = (time - timePrev) / 1000;     
  /*The tiemStep is the time that elapsed since the previous loop. 
  *This is the value that we will use in the formulas as "elapsedTime" 
  *in seconds. We work in ms so we have to divide the value by 1000 
  to obtain seconds*/
  /*Reed the values that the accelerometre gives.
  * We know that the slave adress for this IMU is 0x68 in
  * hexadecimal. For that in the RequestFrom and the 
  * begin functions we have to put this value.*/   
 //////////////////////////////////////Gyro read/////////////////////////////////////
//  Wire.beginTransmission(0x68);            //begin, Send the slave adress (in this case 68) 
//  Wire.write(0x43);                        //First adress of the Gyro data
//  Wire.endTransmission(false);
//  Wire.requestFrom(0x68,4,true);           //We ask for just 4 registers        
//  Gyr_rawX=Wire.read()<<8|Wire.read();     //Once again we shif and sum
//  Gyr_rawY=Wire.read()<<8|Wire.read();
  /*Now in order to obtain the gyro data in degrees/seconds we have to divide first
  the raw value by 32.8 because that's the value that the datasheet gives us for a 1000dps range*/
  /*---X---*/
//  Gyr_rawX = (Gyr_rawX/32.8) - Gyro_raw_error_x; 
//  /*---Y---*/
//  Gyr_rawY = (Gyr_rawY/32.8) - Gyro_raw_error_y;  
//  /*Now we integrate the raw value in degrees per seconds in order to obtain the angle
//  * If you multiply degrees/seconds by seconds you obtain degrees */
//    /*---X---*/
//  Gyro_angle_x = Gyr_rawX*elapsedTime;
//  /*---X---*/
//  Gyro_angle_y = Gyr_rawY*elapsedTime;


    
  
//  //////////////////////////////////////Acc read/////////////////////////////////////
//  Wire.beginTransmission(0x68);     //begin, Send the slave adress (in this case 68) 
//  Wire.write(0x3B);                 //Ask for the 0x3B register- correspond to AcX
//  Wire.endTransmission(false);      //keep the transmission and next
//  Wire.requestFrom(0x68,6,true);    //We ask for next 6 registers starting withj the 3B  
  /*We have asked for the 0x3B register. The IMU will send a brust of register.
  * The amount of register to read is specify in the requestFrom function.
  * In this case we request 6 registers. Each value of acceleration is made out of
  * two 8bits registers, low values and high values. For that we request the 6 of them  
  * and just make then sum of each pair. For that we shift to the left the high values 
  * register (<<) and make an or (|) operation to add the low values.
  If we read the datasheet, for a range of+-8g, we have to divide the raw values by 4096*/    
//  Acc_rawX=(Wire.read()<<8|Wire.read())/4096.0 ; //each value needs two registres
//  Acc_rawY=(Wire.read()<<8|Wire.read())/4096.0 ;
//  Acc_rawZ=(Wire.read()<<8|Wire.read())/4096.0 ; 
 /*Now in order to obtain the Acc angles we use euler formula with acceleration values
 after that we substract the error value found before*/  
 /*---X---*/
// Acc_angle_x = (atan((Acc_rawY)/sqrt(pow((Acc_rawX),2) + pow((Acc_rawZ),2)))*rad_to_deg) - Acc_angle_error_x;
// /*---Y---*/
// Acc_angle_y = (atan(-1*(Acc_rawX)/sqrt(pow((Acc_rawY),2) + pow((Acc_rawZ),2)))*rad_to_deg) - Acc_angle_error_y;   


 //////////////////////////////////////Total angle and filter/////////////////////////////////////
 /*---X axis angle---*/
// Total_angle_x = 0.98 *(Total_angle_x + Gyro_angle_x) + 0.02*Acc_angle_x;
// /*---Y axis angle---*/
// Total_angle_y = 0.98 *(Total_angle_y + Gyro_angle_y) + 0.02*Acc_angle_y;







/*///////////////////////////P I D///////////////////////////////////*/
roll_desired_angle = map(input_ROLL,1000,2000,-10,10);
pitch_desired_angle = map(input_PITCH,1000,2000,-10,10);
throttle_desired_angle = map(input_THROTTLE,1000,2000,-10,10);
Serial.print("ROLL:");
Serial.print(roll_desired_angle);
Serial.print(" ");
Serial.print("PITCH:");
Serial.print(pitch_desired_angle);
Serial.print(" ");
Serial.print("THROTTLE:");
Serial.print(throttle_desired_angle);
Serial.println("");

///*First calculate the error between the desired angle and 
//*the real measured angle*/
//roll_error = Total_angle_y - roll_desired_angle;
//pitch_error = Total_angle_x - pitch_desired_angle;    
///*Next the proportional value of the PID is just a proportional constant
//*multiplied by the error*/
//roll_pid_p = roll_kp*roll_error;
//pitch_pid_p = pitch_kp*pitch_error;
/*The integral part should only act if we are close to the
desired position but we want to fine tune the error. That's
why I've made a if operation for an error between -2 and 2 degree.
To integrate we just sum the previous integral value with the
error multiplied by  the integral constant. This will integrate (increase)
the value each loop till we reach the 0 point*/
//if(-3 < roll_error <3)
//{
//  roll_pid_i = roll_pid_i+(roll_ki*roll_error);  
//}
//if(-3 < pitch_error <3)
//{
//  pitch_pid_i = pitch_pid_i+(pitch_ki*pitch_error);  
//}
/*The last part is the derivate. The derivate acts upon the speed of the error.
As we know the speed is the amount of error that produced in a certain amount of
time divided by that time. For taht we will use a variable called previous_error.
We substract that value from the actual error and divide all by the elapsed time. 
Finnaly we multiply the result by the derivate constant*/
//roll_pid_d = roll_kd*((roll_error - roll_previous_error)/elapsedTime);
//pitch_pid_d = pitch_kd*((pitch_error - pitch_previous_error)/elapsedTime);
/*The final PID values is the sum of each of this 3 parts*/
//roll_PID = roll_pid_p + roll_pid_i + roll_pid_d;
//pitch_PID = pitch_pid_p + pitch_pid_i + pitch_pid_d;
/*We know taht the min value of PWM signal is 1000us and the max is 2000. So that
tells us that the PID value can/s oscilate more than -1000 and 1000 because when we
have a value of 2000us the maximum value taht we could substract is 1000 and when
we have a value of 1000us for the PWM signal, the maximum value that we could add is 1000
to reach the maximum 2000us. But we don't want to act over the entire range so -+400 should be enough*/
//if(roll_PID < -400){roll_PID=-400;}
//if(roll_PID > 400) {roll_PID=400; }
//if(pitch_PID < -4000){pitch_PID=-400;}
//if(pitch_PID > 400) {pitch_PID=400;}

/*Finnaly we calculate the PWM width. We sum the desired throttle and the PID value*/
//pwm_R_F  = 115 + input_THROTTLE - roll_PID - pitch_PID;
//pwm_R_B  = 115 + input_THROTTLE - roll_PID + pitch_PID;
//pwm_L_B  = 115 + input_THROTTLE + roll_PID + pitch_PID;
//pwm_L_F  = 115 + input_THROTTLE + roll_PID - pitch_PID;



/*Once again we map the PWM values to be sure that we won't pass the min
and max values. Yes, we've already maped the PID values. But for example, for 
throttle value of 1300, if we sum the max PID value we would have 2300us and
that will mess up the ESC.*/
////Right front
//if(pwm_R_F < 1100)
//{
//  pwm_R_F= 1100;
//}
//if(pwm_R_F > 2000)
//{
//  pwm_R_F=2000;
//}
//
////Left front
//if(pwm_L_F < 1100)
//{
//  pwm_L_F= 1100;
//}
//if(pwm_L_F > 2000)
//{
//  pwm_L_F=2000;
//}
//
////Right back
//if(pwm_R_B < 1100)
//{
//  pwm_R_B= 1100;
//}
//if(pwm_R_B > 2000)
//{
//  pwm_R_B=2000;
//}
//
////Left back
//if(pwm_L_B < 1100)
//{
//  pwm_L_B= 1100;
//}
//if(pwm_L_B > 2000)
//{
//  pwm_L_B=2000;
//}
//
//roll_previous_error = roll_error; //Remember to store the previous error.
//pitch_previous_error = pitch_error; //Remember to store the previous error.

/*
 Serial.print("RF: ");
 Serial.print(pwm_R_F);
 Serial.print("   |   ");
 Serial.print("RB: ");
 Serial.print(pwm_R_B);
 Serial.print("   |   ");
 Serial.print("LB: ");
 Serial.print(pwm_L_B);
 Serial.print("   |   ");
 Serial.print("LF: ");
 Serial.print(pwm_L_F);

 Serial.print("   |   ");
 Serial.print("Xº: ");
 Serial.print(Total_angle_x);
 Serial.print("   |   ");
 Serial.print("Yº: ");
 Serial.print(Total_angle_y);
 Serial.println(" ");
*/







/*now we can write the values PWM to the ESCs only if the motor is activated
*/
//
//  if(mot_activated)
//  {
//  L_F_prop.writeMicroseconds(pwm_L_F); 
//  L_B_prop.writeMicroseconds(pwm_L_B);
//  R_F_prop.writeMicroseconds(pwm_R_F); 
//  R_B_prop.writeMicroseconds(pwm_R_B);
//  }
//  if(!mot_activated)
//  {
//    L_F_prop.writeMicroseconds(1000); 
//    L_B_prop.writeMicroseconds(1000);
//    R_F_prop.writeMicroseconds(1000); 
//    R_B_prop.writeMicroseconds(1000);
//  }
//
//  if(input_THROTTLE < 1100 && input_YAW > 1800 && !mot_activated)
//  {
//    if(activate_count==200)
//    {
//      mot_activated=1;   
//      PORTB |= B00100000; //D13 LOW   
//      
//    }
//    activate_count=activate_count+1;
//  }
//  if(!(input_THROTTLE < 1100 && input_YAW > 1800) && !mot_activated)
//  {
//    activate_count=0;    
//  }
//
//   if(input_THROTTLE < 1100 && input_YAW < 1100 && mot_activated)
//  {
//    if(des_activate_count==300)
//    {
//      mot_activated=0;       
//      PORTB &= B11011111; //D13 LOW   
//    }
//    des_activate_count=des_activate_count+1;
//  }
//  if(!(input_THROTTLE < 1100 && input_YAW < 1100) && mot_activated)
//  {
//    des_activate_count=0;
//  }
 
}















ISR(PCINT0_vect){
//First we take the current count value in micro seconds using the micros() function
  
  current_count = micros();
  ///////////////////////////////////////Channel 1
  if(PINB & B00000001){                              //We make an AND with the pin state register, We verify if pin 8 is HIGH???
    if(last_CH1_state == 0){                         //If the last state was 0, then we have a state change...
      last_CH1_state = 1;                            //Store the current state into the last state for the next loop
      counter_1 = current_count;                     //Set counter_1 to current value.
    }
  }
  else if(last_CH1_state == 1){                      //If pin 8 is LOW and the last state was HIGH then we have a state change      
    last_CH1_state = 0;                              //Store the current state into the last state for the next loop
    input_ROLL = current_count - counter_1;   //We make the time difference. Channel 1 is current_time - timer_1.
  }



  ///////////////////////////////////////Channel 2
  if(PINB & B00000010 ){                             //pin D9 -- B00000010                                              
    if(last_CH2_state == 0){                                               
      last_CH2_state = 1;                                                   
      counter_2 = current_count;                                             
    }
  }
  else if(last_CH2_state == 1){                                           
    last_CH2_state = 0;                                                     
    input_PITCH = current_count - counter_2;                             
  }


  
  ///////////////////////////////////////Channel 3
  if(PINB & B00000100 ){                             //pin D10 - B00000100                                         
    if(last_CH3_state == 0){                                             
      last_CH3_state = 1;                                                  
      counter_3 = current_count;                                               
    }
  }
  else if(last_CH3_state == 1){                                             
    last_CH3_state = 0;                                                    
    input_THROTTLE = current_count - counter_3;                            

  }


  
  ///////////////////////////////////////Channel 4
  if(PINB & B00010000 ){                             //pin D12  -- B00010000                      
    if(last_CH4_state == 0){                                               
      last_CH4_state = 1;                                                   
      counter_4 = current_count;                                              
    }
  }
  else if(last_CH4_state == 1){                                             
    last_CH4_state = 0;                                                  
    input_YAW = current_count - counter_4;                            
  }


 
}
