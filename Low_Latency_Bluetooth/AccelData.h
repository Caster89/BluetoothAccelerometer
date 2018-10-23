#ifndef AccelData_h
#define AccelData_h
#include "Arduino.h"

//
struct accelData {
  unsigned long time; //4bytes
  char limb; //1byte
  float qw;    //4bytes
  float qx;    //4bytes
  float qy;    //4bytes
  float qz;    //4bytes
  int16_t ax;  //2bytes
  int16_t ay;  //2bytes
  int16_t az;  //2bytes
};             //27bytes
#endif  // UserTypes_h
