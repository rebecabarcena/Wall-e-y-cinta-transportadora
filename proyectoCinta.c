#include <stdio.h>
#include <ev3c.h>
#include <pthread.h>

#include "config.h"

#define BUTTON_NOT_AVAILABLE 	-1
#define BUTTON_PORT		     	 1

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

/**
 * @brief Init and set up the motor
 *
 * @param
 *
 * @return returns the motor that has been initializated
 */
ev3_motor_ptr init_motor() {

	ev3_motor_ptr motors = NULL;      // List of motors
	motors  = ev3_load_motors();      // Loading all motors

	if (motors == NULL) {
		printf ("Error on ev3_load_motors\n");
		exit (-1);
	}

	// Get the target motor
	ev3_motor_ptr the_motor = ev3_search_motor_by_port(motors, THE_MOTOR_PORT);

	// Init motor
	ev3_reset_motor(the_motor);
	ev3_open_motor(the_motor);

	// Motor configuration
	ev3_stop_command_motor_by_name(the_motor, STOP_MODE_STRING[COAST]);
	ev3_set_duty_cycle_sp(the_motor, 0);
	ev3_set_position(the_motor, 0 );

	ev3_command_motor_by_name( the_motor, COMMANDS_STRING[RUN_DIRECT]); // Action!
	usleep (10000);
	ev3_set_duty_cycle_sp(the_motor, 25);

	return the_motor;
}

/**
 * @brief Set the motor speed using the duty cycle
 *
 * @param motor: the target motor
 * @param duty_cycle: the duty cycle
 *
 * @return
 */
void set_motor_duty_cycle (ev3_motor_ptr motor, int duty_cycle) {
	if (duty_cycle > MAX_DUTY_CYCLE)
		duty_cycle = MAX_DUTY_CYCLE;

	if (duty_cycle < MIN_DUTY_CYCLE)
		duty_cycle = MIN_DUTY_CYCLE;

	ev3_set_duty_cycle_sp(motor, duty_cycle);
}

/**
 * Move the motor when the button is pressed and it stop the motor
 * when the button is not pressed.
 */
void *funcionBoton(){
	ev3_sensor_ptr sensors = NULL; //  List of available sensors
	ev3_sensor_ptr button  = NULL;
	ev3_motor_ptr the_motor= NULL;
	int value;

	// Loading all sensors
	sensors = ev3_load_sensors();
	if (sensors == NULL)pthread_exit(NULL);

	// Get button by port
	button = ev3_search_sensor_by_port( sensors, BUTTON_PORT);
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
	if(value!=0){
		the_motor = init_motor();
		while(value!=0){
			sleep(1);
			ev3_update_sensor_val(button);
			value = button->val_data[0].s32;
		}
		if(value==0){
			//Let's reset and close the motors
			ev3_reset_motor(the_motor);
			ev3_delete_motors(the_motor);
		}
	}

	pthread_exit(NULL);
}

int main () {
	pthread_t tid1;

	// Create the thread
	pthread_create(&tid1, NULL, funcionBoton, (void *)NULL);

	sleep(15);

	//  Finish & close devices
	printf ("\n*** Finishing wall-e application ***\n");

	return 0;
}

