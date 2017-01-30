/*
 * proyecto.c
 * 
 * Copyright 2017 Jesus Duran Hernandez and Rebeca Barcena Orero
 * 					<jdhez.14@gmail.com> and <rebeca.barcena.o@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ev3c-addons.h>
#include <ev3c.h>
#include <error_checks.h>
#include <timespec_operations.h>

#include "config.h"
#include "pid.h"

typedef enum stop_mode_enum
{
	COAST, BRAKE, HOLD
} stop_mode;

static char *STOP_MODE_STRING[] = {
		"coast","brake","hold"
};

typedef enum commands_enum
{
	RUN_ABS_POS, RUN_REL_POS, RUN_TIMED, RUN_DIRECT, RUN_FOREVER, STOP
} commands;

static char *COMMANDS_STRING[] = {
		"run-to-abs-pos","run-to-rel-pos","run-timed","run-direct", "run-forever", "stop"
};

typedef enum color_command_enum
{
	COL_REFLECT=0, COL_AMBIENT=1, COL_COLOR=2
} color_command;

typedef enum color_received {
	ROJO, VERDE, AMARILLO, OTRO, NO_COLOR
} Color_rec;

volatile bool is_running = true;
volatile bool is_pressed = false;
Color_rec c;

/**
 * Reads the button sensor and updates the value of the variable is_pressed
 */
void *funcionBoton(){
	ev3_sensor_ptr sensors = NULL; //  List of available sensors
	ev3_sensor_ptr button  = NULL;
	int value;

	// Loading all sensors
	sensors = ev3_load_sensors();
	if (sensors == NULL)pthread_exit(NULL);

	// Get button by port
	button = ev3_search_sensor_by_port( sensors, 3);
	if (button == NULL)pthread_exit(NULL);

	// Init button
	button = ev3_open_sensor(button);
	if (button == NULL)pthread_exit(NULL);

	ev3_update_sensor_val(button);
	value = button->val_data[0].s32;

	while(value==0){
		sleep(1);
		ev3_update_sensor_val(button);
		value = button->val_data[0].s32;
	}
	is_pressed = true;

	pthread_exit(NULL);
}

/**
 * Checks the color detected by the LED sensor and updates the value of
 * the variable c
 */
void checkColor() {
	printf ("\n Color thread \n");

	ev3_sensor_ptr sensors       = NULL; //  List of available sensors
	ev3_sensor_ptr color_sensor  = NULL;

	// Loading all sensors
	sensors = ev3_load_sensors();
	if (sensors == NULL)pthread_exit(NULL);

	// Get color sensor by port
	color_sensor = ev3_search_sensor_by_port( sensors, 3);
	if (color_sensor == NULL)pthread_exit(NULL);

	// Init sensor
	color_sensor = ev3_open_sensor(color_sensor);
	if (color_sensor == NULL)pthread_exit(NULL);

	// Set mode
	ev3_mode_sensor (color_sensor, COL_COLOR);
	bool goOn = true;

	while(goOn){
		ev3_update_sensor_val(color_sensor);
		int value = color_sensor->val_data[0].s32;

		if (value==3){
			c = VERDE;
			printf("Verde\n");
		}
		else if (value==5){
			c = ROJO;
			printf("Rojo\n");
			goOn = false;
		}
		else if (value==0){
			c = NO_COLOR;
			printf("Ninguno\n");
		}
		else {
			c = OTRO;
			printf("Otro\n");
		}

		usleep(500);
	}

	ev3_delete_sensors (sensors);
	ev3_quit_led();

	pthread_exit(NULL);
}

/**
 * Gives power to the engines to move WallE forward, waits until the pieces
 * are not over the conveyor belt, and goes backwards to stop it
 */
void *runWalle () {

	//  Regular variables
	//int index            = 1;    // Loop variable
	ev3_motor_ptr motors = NULL; // List of motors


	motors  = ev3_load_motors(); // Loading all motors
	if (motors == NULL) {
		printf ("Error on ev3_load_motors\n");
		exit (-1);
	}

	// Get the target motor
	ev3_motor_ptr right = ev3_search_motor_by_port(motors, 'A');
	ev3_motor_ptr left = ev3_search_motor_by_port(motors, 'D');

	// Init motor
	ev3_reset_motor(right);
	ev3_open_motor(right);
	ev3_reset_motor(left);
	ev3_open_motor(left);

	// Motor configuration
	ev3_stop_command_motor_by_name(right, STOP_MODE_STRING[BRAKE]);
	ev3_set_duty_cycle_sp(right, 25);  // Power to the engine
	ev3_set_position(right, 0);

	ev3_stop_command_motor_by_name(left, STOP_MODE_STRING[BRAKE]);
	ev3_set_duty_cycle_sp(left, 25);  // Power to the engine
	ev3_set_position(left, 0);

	ev3_command_motor_by_name( right, COMMANDS_STRING[RUN_FOREVER]); // Action!
	ev3_command_motor_by_name( left, COMMANDS_STRING[RUN_FOREVER]); // Action!

	while(!is_pressed);

	ev3_command_motor_by_name( right, COMMANDS_STRING[STOP]); // Action!
	ev3_command_motor_by_name( left, COMMANDS_STRING[STOP]);

	sleep(5);

	ev3_stop_command_motor_by_name(right, STOP_MODE_STRING[BRAKE]);
	ev3_set_duty_cycle_sp(right, -25);  // Power to the engine
	ev3_set_position(right, 0);

	ev3_stop_command_motor_by_name(left, STOP_MODE_STRING[BRAKE]);
	ev3_set_duty_cycle_sp(left, -25);  // Power to the engine
	ev3_set_position(left, 0);

	ev3_command_motor_by_name( right, COMMANDS_STRING[RUN_FOREVER]); // Action!
	ev3_command_motor_by_name( left, COMMANDS_STRING[RUN_FOREVER]); // Action!

	sleep(1);

	ev3_command_motor_by_name( right, COMMANDS_STRING[STOP]); // Action!
	ev3_command_motor_by_name( left, COMMANDS_STRING[STOP]);

	ev3_reset_motor(right);
	ev3_reset_motor(left);
	ev3_delete_motors(motors);
	pthread_exit(NULL);
}

/**
 * Says "Wall E"
 */
void *sayWalle() {
	char text[10] = "Wall E";

	speak_say(text,ENGLISH);
	pthread_exit(NULL);
}

/**
 * Main function which creates all the threads and waits until they have finished
 */
int main() {

	pthread_t tid1, tid2, tid3;

	// inicializamos la botonera
	ev3_init_button();

	pthread_create(&tid1, NULL, *funcionBoton, (void *)NULL);
	pthread_create(&tid2, NULL, *runWalle, (void *)NULL);

	pthread_join(tid1,NULL);
	pthread_join(tid2,NULL);

	sleep(2);
	pthread_create(&tid3, NULL, *sayWalle, (void *)NULL);

	pthread_join(tid3,NULL);

	//  Finish & close devices
	printf ("\n*** Stopping Wall-E...***\n");

	return 0;
}
