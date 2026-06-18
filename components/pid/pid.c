#include "pid.h"

void pid_init(pid_ctrl_t *pid, float kp, float ki, float kd, float limit) {
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->limit = limit;
}

float pid_compute(pid_ctrl_t *pid, float setpoint, float measured, float dt) {
    if (dt <= 0.0f) return 0.0f;

    float error = setpoint - measured;
    
    // Integral mit Anti-Windup (Begrenzung)
    pid->integral += error * dt;
    if (pid->integral > pid->limit) pid->integral = pid->limit;
    if (pid->integral < -pid->limit) pid->integral = -pid->limit;

    // Ableitung (Derivativ)
    float derivative = (error - pid->prev_error) / dt;
    pid->prev_error = error;

    // PID Formel
    float output = (pid->kp * error) + (pid->ki * pid->integral) + (pid->kd * derivative);

    // Output begrenzen
    if (output > pid->limit) output = pid->limit;
    if (output < -pid->limit) output = -pid->limit;

    return output;
}