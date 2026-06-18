#pragma once

typedef struct {
    float kp;
    float ki;
    float kd;
    float integral;
    float prev_error;
    float limit; // Maximales Output-Limit (z.B. 100% Motorleistung)
} pid_ctrl_t;

// Ref: SYS003 - PID Regler Initialisierung
void pid_init(pid_ctrl_t *pid, float kp, float ki, float kd, float limit);

// Ref: SYS003 - PID Berechnung (dt = Zeit in Sekunden seit letztem Aufruf)
float pid_compute(pid_ctrl_t *pid, float setpoint, float measured, float dt);