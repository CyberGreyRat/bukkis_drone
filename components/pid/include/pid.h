#ifndef PID_H
#define PID_H

typedef struct {
    float kp;
    float ki;
    float kd;
    
    float i_limit;   // Maximalwert für das Integral (Anti-Windup)
    float out_limit; // Maximaler Output des Reglers insgesamt
    
    float integral;
    float prev_error;
    float prev_derivative;
} pid_ctrl_t;

void pid_init(pid_ctrl_t *pid, float kp, float ki, float kd, float i_limit, float out_limit);
float pid_compute(pid_ctrl_t *pid, float setpoint, float measured, float dt);
void pid_reset(pid_ctrl_t *pid);

#endif // PID_H