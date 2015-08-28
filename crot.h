/*
 * crot.h
 *
 *  Created on: 27-08-2015
 *      Author: patryk
 */

#ifndef CROT_H_
#define CROT_H_

#define PI 3.141592653589793f

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>
#include <unistd.h>

#define r 0
#define g 1
#define b 2

int get(int, int, int, uint8_t*);
void set(int, int, int, int, uint8_t*);
int rep1(int, int, int, int, uint8_t*);
int rot1(int, int, int, uint8_t*, uint8_t*);
int resample1(int, int, uint8_t**);
int rep8(int, int, int, int, uint8_t*);
int rot8(int, int, int, uint8_t*, uint8_t*);
int resample8(int, int, uint8_t**);
inline static void mapgs(int*);
inline static void mapc(int*);
int rgb8ToConsole(int*);
void setup_screen(void);
void clean_screen(int);
int kbhit(void);

#endif /* CROT_H_ */
