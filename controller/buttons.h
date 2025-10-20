#pragma once

void buttons_setup();
void buttons_update();

bool isButtonEventPending();
int getNextButtonEvent();

bool check_button_event();
int get_pressedButtonIndex();
int get_pressedButtonType();

void reset_ButtonEvent();
