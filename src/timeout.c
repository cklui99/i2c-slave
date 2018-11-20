/**
 * \file
 *
 * \brief Timeout driver.
 *
 (c) 2018 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms,you may use this software and
    any derivatives exclusively with Microchip products.It is your responsibility
    to comply with third party license terms applicable to your use of third party
    software (including open source software) that may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 */

/**
 \defgroup doc_driver_timer_timeout Timeout Driver
 \ingroup doc_driver_timer

 \section doc_driver_timer_timeout_rev Revision History
 - v0.0.0.1 Initial Commit

@{
*/
#include <stdio.h>
#include "timeout.h"
#include "atomic.h"

absolutetime_t TIMER_0_dummy_handler(void *payload)
{
	return 0;
};

void           TIMER_0_start_timer_at_head(void);
void           TIMER_0_enqueue_callback(timer_struct_t *timer);
void           TIMER_0_set_timer_duration(absolutetime_t duration);
absolutetime_t TIMER_0_make_absolute(absolutetime_t timeout);
absolutetime_t TIMER_0_rebase_list(void);

timer_struct_t *TIMER_0_list_head          = NULL;
timer_struct_t *TIMER_0_execute_queue_head = NULL;

timer_struct_t          TIMER_0_dummy                         = {TIMER_0_dummy_handler};
volatile absolutetime_t TIMER_0_absolute_time_of_last_timeout = 0;
volatile absolutetime_t TIMER_0_last_timer_load               = 0;
volatile bool           TIMER_0_is_running                    = false;

void TIMER_0_timeout_init(void)
{

	// TCA0.SINGLE.CMP0 = 0x0; /* Compare Register 0: 0x0 */

	// TCA0.SINGLE.CMP1 = 0x0; /* Compare Register 1: 0x0 */

	// TCA0.SINGLE.CMP2 = 0x0; /* Compare Register 2: 0x0 */

	// TCA0.SINGLE.CNT = 0x0; /* Count: 0x0 */

	// TCA0.SINGLE.CTRLB = 0 << TCA_SINGLE_ALUPD_bp /* Auto Lock Update: disabled */
	//		 | 0 << TCA_SINGLE_CMP0EN_bp /* Compare 0 Enable: disabled */
	//		 | 0 << TCA_SINGLE_CMP1EN_bp /* Compare 1 Enable: disabled */
	//		 | 0 << TCA_SINGLE_CMP2EN_bp /* Compare 2 Enable: disabled */
	//		 | TCA_SINGLE_WGMODE_NORMAL_gc; /*  */

	// TCA0.SINGLE.CTRLC = 0 << TCA_SINGLE_CMP0OV_bp /* Compare 0 Waveform Output Value: disabled */
	//		 | 0 << TCA_SINGLE_CMP1OV_bp /* Compare 1 Waveform Output Value: disabled */
	//		 | 0 << TCA_SINGLE_CMP2OV_bp; /* Compare 2 Waveform Output Value: disabled */

	TCA0.SINGLE.DBGCTRL = 1 << TCA_SINGLE_DBGRUN_bp; /* Debug Run: enabled */

	// TCA0.SINGLE.EVCTRL = 0 << TCA_SINGLE_CNTEI_bp /* Count on Event Input: disabled */
	//		 | TCA_SINGLE_EVACT_POSEDGE_gc; /* Count on positive edge event */

	TCA0.SINGLE.INTCTRL = 0 << TCA_SINGLE_CMP0_bp   /* Compare 0 Interrupt: disabled */
	                      | 0 << TCA_SINGLE_CMP1_bp /* Compare 1 Interrupt: disabled */
	                      | 0 << TCA_SINGLE_CMP2_bp /* Compare 2 Interrupt: disabled */
	                      | 1 << TCA_SINGLE_OVF_bp; /* Overflow Interrupt: enabled */

	// TCA0.SINGLE.PER = 0xffff; /* Period: 0xffff */

	TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc    /* System Clock */
	                    | 1 << TCA_SINGLE_ENABLE_bp; /* Module Enable: enabled */
}

void TIMER_0_stop_timeouts(void)
{
	TCA0.SINGLE.INTCTRL &= ~TCA_SINGLE_OVF_bm;
	TIMER_0_absolute_time_of_last_timeout = 0;
	TIMER_0_is_running                    = 0;
}

inline void TIMER_0_set_timer_duration(absolutetime_t duration)
{
	TIMER_0_last_timer_load = 65535 - duration;
	TCA0.SINGLE.CNT         = TIMER_0_last_timer_load;
}

inline absolutetime_t TIMER_0_make_absolute(absolutetime_t timeout)
{
	timeout += TIMER_0_absolute_time_of_last_timeout;
	timeout += TIMER_0_is_running ? TCA0.SINGLE.CNT - TIMER_0_last_timer_load : 0;

	return timeout;
}

inline absolutetime_t TIMER_0_rebase_list(void)
{
	timer_struct_t *base_point = TIMER_0_list_head;
	absolutetime_t  base       = TIMER_0_list_head->absolute_time;

	while (base_point != NULL) {
		base_point->absolute_time -= base;
		base_point = base_point->next;
	}

	TIMER_0_absolute_time_of_last_timeout -= base;
	return base;
}

inline void TIMER_0_print_list(void)
{
	timer_struct_t *base_point = TIMER_0_list_head;
	while (base_point != NULL) {
		printf("%ld -> ", (uint32_t)base_point->absolute_time);
		base_point = base_point->next;
	}
	printf("NULL\n");
}

// Returns true if the insert was at the head, false if not
bool TIMER_0_sorted_insert(timer_struct_t *timer)
{
	absolutetime_t  timer_absolute_time = timer->absolute_time;
	uint8_t         at_head             = 1;
	timer_struct_t *insert_point        = TIMER_0_list_head;
	timer_struct_t *prev_point          = NULL;
	timer->next                         = NULL;

	if (timer_absolute_time < TIMER_0_absolute_time_of_last_timeout) {
		timer_absolute_time += 65535 - TIMER_0_rebase_list() + 1;
		timer->absolute_time = timer_absolute_time;
	}

	while (insert_point != NULL) {
		if (insert_point->absolute_time > timer_absolute_time) {
			break; // found the spot
		}
		prev_point   = insert_point;
		insert_point = insert_point->next;
		at_head      = 0;
	}

	if (at_head == 1) // the front of the list.
	{
		TIMER_0_set_timer_duration(65535);
		TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;

		timer->next       = (TIMER_0_list_head == &TIMER_0_dummy) ? TIMER_0_dummy.next : TIMER_0_list_head;
		TIMER_0_list_head = timer;
		return true;
	} else // middle of the list
	{
		timer->next = prev_point->next;
	}
	prev_point->next = timer;
	return false;
}

void TIMER_0_start_timer_at_head(void)
{
	TCA0.SINGLE.INTCTRL &= ~TCA_SINGLE_OVF_bm;

	if (TIMER_0_list_head == NULL) // no timeouts left
	{
		TIMER_0_stop_timeouts();
		return;
	}

	absolutetime_t period = ((TIMER_0_list_head != NULL) ? (TIMER_0_list_head->absolute_time) : 0)
	                        - TIMER_0_absolute_time_of_last_timeout;

	// Timer is too far, insert dummy and schedule timer after the dummy
	if (period > 65535) {
		TIMER_0_dummy.absolute_time = TIMER_0_absolute_time_of_last_timeout + 65535;
		TIMER_0_dummy.next          = TIMER_0_list_head;
		TIMER_0_list_head           = &TIMER_0_dummy;
		period                      = 65535;
	}

	TIMER_0_set_timer_duration(period);

	TCA0.SINGLE.INTCTRL |= TCA_SINGLE_OVF_bm;
	TIMER_0_is_running = 1;
}

void TIMER_0_timeout_flush_all(void)
{
	TIMER_0_stop_timeouts();
	TIMER_0_list_head = NULL;
}

void TIMER_0_timeout_delete(timer_struct_t *timer)
{
	if (TIMER_0_list_head == NULL)
		return;

	// Guard in case we get interrupted, we cannot safely compare/search and get interrupted
	TCA0.SINGLE.INTCTRL &= ~TCA_SINGLE_OVF_bm;

	// Special case, the head is the one we are deleting
	if (timer == TIMER_0_list_head) {
		TIMER_0_list_head = TIMER_0_list_head->next; // Delete the head
		TIMER_0_start_timer_at_head();               // Start the new timer at the head
	} else {                                         // More than one timer in the list, search the list.
		timer_struct_t *find_timer = TIMER_0_list_head;
		timer_struct_t *prev_timer = NULL;
		while (find_timer != NULL) {
			if (find_timer == timer) {
				prev_timer->next = find_timer->next;
				break;
			}
			prev_timer = find_timer;
			find_timer = find_timer->next;
		}
		TCA0.SINGLE.INTCTRL |= TCA_SINGLE_OVF_bm;
	}
}

inline void TIMER_0_enqueue_callback(timer_struct_t *timer)
{
	timer_struct_t *tmp;
	timer->next = NULL;

	// Special case for empty list
	if (TIMER_0_execute_queue_head == NULL) {
		TIMER_0_execute_queue_head = timer;
		return;
	}

	// Find the end of the list and insert the next expired timer at the back of the queue
	tmp = TIMER_0_execute_queue_head;
	while (tmp->next != NULL)
		tmp = tmp->next;

	tmp->next = timer;
}

void TIMER_0_timeout_call_next_callback(void)
{

	if (TIMER_0_execute_queue_head == NULL)
		return;

	// Critical section needed if TIMER_0_timeout_call_next_callback()
	// was called from polling loop, and not called from ISR.
	ENTER_CRITICAL(T);
	timer_struct_t *callback_timer = TIMER_0_execute_queue_head;

	// Done, remove from list
	TIMER_0_execute_queue_head = TIMER_0_execute_queue_head->next;

	EXIT_CRITICAL(T); // End critical section

	absolutetime_t reschedule = callback_timer->callback_ptr(callback_timer->payload);

	// Do we have to reschedule it? If yes then add delta to absolute for reschedule
	if (reschedule) {
		TIMER_0_timeout_create(callback_timer, reschedule);
	}
}

void TIMER_0_timeout_create(timer_struct_t *timer, absolutetime_t timeout)
{
	TCA0.SINGLE.INTCTRL &= ~TCA_SINGLE_OVF_bm;

	timer->absolute_time = TIMER_0_make_absolute(timeout);

	// We only have to start the timer at head if the insert was at the head
	if (TIMER_0_sorted_insert(timer)) {
		TIMER_0_start_timer_at_head();
	} else {
		if (TIMER_0_is_running)
			TCA0.SINGLE.INTCTRL |= TCA_SINGLE_OVF_bm;
	}
}

// NOTE: assumes the callback completes before the next timer tick
ISR(TCA0_OVF_vect)
{
	timer_struct_t *next                  = TIMER_0_list_head->next;
	TIMER_0_absolute_time_of_last_timeout = TIMER_0_list_head->absolute_time;
	TIMER_0_last_timer_load               = 0;

	if (TIMER_0_list_head != &TIMER_0_dummy)
		TIMER_0_enqueue_callback(TIMER_0_list_head);

	// Remove expired timer for the list now (it is always the one at the head)
	TIMER_0_list_head = next;

	TIMER_0_start_timer_at_head();

	TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}

// These methods are for calculating the elapsed time in stopwatch mode.
// TIMER_0_timeout_start_timer will start a
// timer with (maximum range)/2. You cannot time more than
// this and the timer will stop after this time elapses
void TIMER_0_timeout_start_timer(timer_struct_t *timer)
{
	absolutetime_t i = -1;
	TIMER_0_timeout_create(timer, i >> 1);
}

// This funciton stops the "stopwatch" and returns the elapsed time.
absolutetime_t TIMER_0_timeout_stop_timer(timer_struct_t *timer)
{
	absolutetime_t now = TIMER_0_make_absolute(0); // Do this as fast as possible for accuracy
	absolutetime_t i   = -1;
	i >>= 1;

	TIMER_0_timeout_delete(timer);

	absolutetime_t diff = timer->absolute_time - now;

	// This calculates the (max range)/2 minus (remaining time) which = elapsed time
	return (i - diff);
}
