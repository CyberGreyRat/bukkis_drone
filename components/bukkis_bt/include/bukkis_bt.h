#ifndef BUKKIS_BT_H
#define BUKKIS_BT_H

#include <stdint.h>

// Macht die Variablen für die Hauptdatei sichtbar
extern int32_t bt_throttle;
extern int32_t bt_yaw;
extern int32_t bt_pitch;
extern int32_t bt_roll;

// Macht die Startfunktion für die Hauptdatei sichtbar
void bluetooth_init(void);

#endif