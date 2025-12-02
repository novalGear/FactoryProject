#pragma once

const unsigned long DFLT_TIMEOUT = 10000;
const int DFLT_SPEED = 150;

void motor_setup();



long get_encoder();

void setMotorMoveTask(unsigned long ticks, int direction, int speed);
bool MotorExecMoveTask();
bool unint_motor_move(unsigned long ticks, int direction, int speed = DFLT_SPEED, unsigned long timeout_ms = DFLT_TIMEOUT);

int change_pos(int pos);
int get_current_position_index();

void motor_test();
