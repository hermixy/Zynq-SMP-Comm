/*
 * util.h
 *
 *  Created on: Mar 17, 2017
 *      Author: kulich
 */

#ifndef SRC_UTIL_H_
#define SRC_UTIL_H_

#include "xscugic.h"

int InitInterrupt( XScuGic *GicInstancePtr );
void EnableInterrupt( XScuGic *GicInstancePtr );
void DisableInterrupt( XScuGic *GicInstancePtr );

#endif /* SRC_UTIL_H_ */
