#pragma once

void motor_setup();

void setMotorMoveTask(float revolutions, int direction, int speed);
void MotorExecMoveTask();
void unint_motor_move(float revolutions, int direction, int speed, unsigned long timeout_ms = 10000);
void motor_test();
