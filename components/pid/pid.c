#include "pid.h"
#include "esp_log.h"
#include <math.h>

// Eine Hilfsfunktion, die wie das Arduino-constrain() funktioniert
static float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void pid_init(pid_ctrl_t *pid, float kp, float ki, float kd, float i_limit, float out_limit) {
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    
    pid->i_limit = i_limit;
    pid->out_limit = out_limit;
    
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->prev_derivative = 0.0f; // Für den D-Term Filter
}

float pid_compute(pid_ctrl_t *pid, float setpoint, float measured, float dt) {
    // 1. Fehler berechnen (Soll - Ist)
    float error = setpoint - measured;

    // 2. Proportional (P)
    float p_out = pid->kp * error;

    // 3. Integral (I)
    pid->integral += error * dt;
    
    // Anti-Windup: Das Integral darf nicht unendlich groß werden!
    // (Wie im Crazyflie-Code)
    pid->integral = clamp(pid->integral, -pid->i_limit, pid->i_limit);
    float i_out = pid->ki * pid->integral;

    // 4. Derivative (D)
    // Berechnet die Änderungsgeschwindigkeit des Fehlers
    float derivative = (error - pid->prev_error) / dt;
    
    // --- DER D-TERM FILTER (Sehr wichtig!) ---
    // Statt des komplexen lpf2p (Crazyflie) nutzen wir hier einen einfachen, 
    // aber extrem effektiven Tiefpassfilter.
    // 0.5f bedeutet: 50% der neuen Änderung, 50% der alten Änderung. 
    // Das schluckt hochfrequente Motorvibrationen sofort.
    derivative = 0.5f * derivative + 0.5f * pid->prev_derivative;
    
    float d_out = pid->kd * derivative;

    // Werte für den nächsten Durchlauf speichern
    pid->prev_error = error;
    pid->prev_derivative = derivative;

    // 5. PID zusammenrechnen
    float output = p_out + i_out + d_out;

    // Gesamtausgabe limitieren (Sicherheitsfunktion)
    return clamp(output, -pid->out_limit, pid->out_limit);
}

// Wenn Gas < 2% ist, rufen wir das hier auf, damit die Werte nicht weiterlaufen
void pid_reset(pid_ctrl_t *pid) {
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->prev_derivative = 0.0f;
}