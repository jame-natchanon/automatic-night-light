// Program: NatchanonNightLight.c
// Author:  Nuansanit, Natchanon
// Date:    29-JUL-2026
// Purpose: This program uses a LabJack U3-HV to run an automatic night light.
//          AIN0 reads a light sensor (LDR). AIN1 is the MODE button. AIN2 is the
//          kill switch. Timer0 on FIO4 sends a PWM signal to the main LED.
//          Timer1 on FIO5 sends a PWM signal to a piezo buzzer, the same buzzer
//          as Lab 6. The program has 4 modes. Each press of the MODE button goes
//          to the next mode, and the buzzer beeps to say which mode it is now.
//          Mode 0 makes the LED follow the room light, so a dark room gives a
//          bright LED. Modes 1, 2 and 3 make the LED blink slow, medium or fast.
//          The kill switch fades the LED off and ends the program.



#include <stdio.h>
#include <stdlib.h>
#include <windows.h>                          // Needed for the Sleep() function.
#include "C:\Program Files (x86)\LabJack\Drivers\LabJackUD.h"

int main()
{
    // SECTION 1 - Set up variables for the program.

    LJ_ERROR lj_cue;                          // Every LabJack call is set equal to this.
    LJ_HANDLE lj_handle = 0;                  // The handle to the LabJack.
    double ldr = 0, mode = 0, kill = 0;       // The three input readings, in volts.
    double last = 0;                          // The MODE button reading from last time.
    int state = 0;                            // 0 dim, 1 slow, 2 medium, 3 fast.
    int tick = 0;                             // Counts loops. Used to time the blink.
    int beep = 0;                             // How many loops of beeping are left.
    int period = 40;                          // How many loops one blink takes.
    long target = 65280;                      // The brightness we want.
    long now = 65280;                         // The brightness we have right now.
    long value = 65280;                       // The number we send to the LED.

    // SECTION 2 - Open the LabJack and reset it to original settings.

    lj_cue = OpenLabJack(LJ_dtU3, LJ_ctUSB, "1", 1, &lj_handle);
             // Find the first LabJack U3 on USB and save its handle.
    lj_cue = ePut(lj_handle, LJ_ioPIN_CONFIGURATION_RESET, 0, 0, 0);
             // Put every pin back to how it comes from the factory.

    // SECTION 3 - Set up TWO PWM signals, on FIO4 and FIO5.

    lj_cue = AddRequest(lj_handle, LJ_ioPUT_CONFIG, LJ_chTIMER_COUNTER_PIN_OFFSET, 4, 0, 0);
             // Start the timers at pin 4. So the first PWM is FIO4 and the next one is FIO5.
             // The timers always take pins side by side. They cannot skip a pin.
    lj_cue = AddRequest(lj_handle, LJ_ioPUT_CONFIG, LJ_chTIMER_CLOCK_BASE, LJ_tc48MHZ_DIV, 0, 0);
             // Pick the clock. (TCB = 48MHz) Both timers must share this.
    lj_cue = AddRequest(lj_handle, LJ_ioPUT_CONFIG, LJ_chTIMER_CLOCK_DIVISOR, 47, 0, 0);
             // Divide the clock by 47. Both timers must share this too.
    lj_cue = AddRequest(lj_handle, LJ_ioPUT_CONFIG, LJ_chNUMBER_TIMERS_ENABLED, 2, 0, 0);
             // Turn on 2 timers. Timer0 is FIO4 (LED). Timer1 is FIO5 (buzzer).
    lj_cue = AddRequest(lj_handle, LJ_ioPUT_TIMER_MODE, 0, LJ_tmPWM8, 0, 0);
             // Timer0 is 8-bit PWM. (TM = 256)
    lj_cue = AddRequest(lj_handle, LJ_ioPUT_TIMER_MODE, 1, LJ_tmPWM8, 0, 0);
             // Timer1 is 8-bit PWM as well. f = TCB / (DIV * TM) = 3989 Hz on both pins.
             // Both parts want a fast signal, so both timers use the same mode. 3989 Hz is
             // too fast for the eye to see, so the LED looks steady. It is also the best
             // pitch for this buzzer. The blinking is NOT made by slowing the PWM down.
             // It is made by counting loops in SECTION 4, so the buzzer never changes pitch.
    lj_cue = AddRequest(lj_handle, LJ_ioPUT_TIMER_VALUE, 0, 65280, 0, 0);
             // LED off. (100%=0, 75%=16384, 50%=32768, 25%=49152, 0%=65280)
    lj_cue = AddRequest(lj_handle, LJ_ioPUT_TIMER_VALUE, 1, 0, 0, 0);
             // Buzzer off. Same as Lab 6. A value of 0 holds the pin HIGH and never
             // switches, and a buzzer only makes a sound when the pin keeps switching.
             // Careful: 0 means the LED is FULL bright but the buzzer is SILENT.
    lj_cue = Go();
             // Send all of the lines above to the LabJack.

    lj_cue = ePut(lj_handle, LJ_ioPUT_DAC, 0, 0, 0);
             // DAC0 = 0V turns the PNP on, so the green status LED lights up.
             // The PNP works backwards. If the LED is wrong, swap the 0 and the 5.

    printf("Mode 0 dim, 1 slow blink, 2 medium blink, 3 fast blink.\n");
    printf("Press the MODE button (AIN1) to change mode. Kill switch (AIN2) quits.\n");
    system("pause");                          // Wait for a key press before starting.

    // SECTION 4 - Main loop. Read the 3 inputs, then set the 2 PWM signals.

    while (kill < 2.5)                        // Both buttons are pull-down, so pressed = HIGH.
    {
        lj_cue = AddRequest(lj_handle, LJ_ioGET_AIN, 0, 0, 0, 0);      // Ask for AIN0.
        lj_cue = AddRequest(lj_handle, LJ_ioGET_AIN, 1, 0, 0, 0);      // Ask for AIN1.
        lj_cue = AddRequest(lj_handle, LJ_ioGET_AIN, 2, 0, 0, 0);      // Ask for AIN2.
        lj_cue = Go();                                                 // Send all 3 asks.
        lj_cue = GetResult(lj_handle, LJ_ioGET_AIN, 0, &ldr);          // Save AIN0 in ldr.
        lj_cue = GetResult(lj_handle, LJ_ioGET_AIN, 1, &mode);         // Save AIN1 in mode.
        lj_cue = GetResult(lj_handle, LJ_ioGET_AIN, 2, &kill);         // Save AIN2 in kill.

        tick = tick + 1;                      // One more loop done. Used for the blink.

        if (last < 2.5 && mode > 2.5)         // Was low, is now high. So it was just pressed.
        {
            state = (state + 1) % 4;          // Go to the next mode. After 3 it goes back to 0.
            beep = (state + 1) * 8;           // Beep (mode + 1) times. Each beep takes 8 loops.
            tick = 0;                         // Start the blink from the beginning.
            printf("Mode %d\n", state);
        }
        last = mode;                          // Save this reading to compare next loop.

        if (state == 1) period = 40;          // Slow blink. 40 loops x 25 ms = 1 second.
        else if (state == 2) period = 16;     // Medium blink. 16 loops = 0.4 second.
        else if (state == 3) period = 8;      // Fast blink. 8 loops = 0.2 second.

        if (state == 0)                       // Mode 0. The LED follows the room light.
        {
            if (ldr < 1.0) target = 0;                    // Dark room. LED full bright.
            else if (ldr > 4.0) target = 65280;           // Bright room. LED off.
            else target = (long)((ldr - 1.0) / 3.0 * 65280);
                 // In between. More volts means a brighter room, so a dimmer LED.

            if (now < target - 512) now = now + 512;      // Move 512 closer to the target.
            else if (now > target + 512) now = now - 512; // Move 512 closer the other way.
            else now = target;                            // Near enough. Just use the target.
            value = now;                                  // Small steps make a soft fade.
        }
        else                                  // Modes 1, 2 and 3. The LED blinks.
        {
            if (tick % period < period / 2) value = 0;    // First half. LED full bright.
            else value = 65280;                           // Second half. LED off.
            now = value;                                  // Keep now the same for SECTION 5.
        }

        if (kill > 2.5) value = 65280;        // Kill switch pressed. Turn the LED off.

        lj_cue = AddRequest(lj_handle, LJ_ioPUT_TIMER_VALUE, 0, value, 0, 0);
                 // Send the brightness to Timer0, the main LED.
        if (beep > 0 && beep % 8 >= 4)
        {
            lj_cue = AddRequest(lj_handle, LJ_ioPUT_TIMER_VALUE, 1, 32768, 0, 0);
                     // 50% is the loudest for this buzzer. 4 loops on, 4 loops off = 1 beep.
        }
        else
            lj_cue = AddRequest(lj_handle, LJ_ioPUT_TIMER_VALUE, 1, 0, 0, 0);
                     // Buzzer off, same as Lab 6.
        if (beep > 0) beep = beep - 1;        // Take one loop off the beeping.
        lj_cue = Go();
                 // Send both timer values together.

        printf("mode %d   LDR %.2f V   value %ld      \r", state, ldr, value);
        Sleep(25);                            // Wait 25 ms. This also helps the button reading.
    }

    // SECTION 5 - Fade the LED off, then put all the pins back.

    while (now < 65280)                       // Keep going so the LED does not just snap off.
    {
        now = now + 512;
        if (now > 65280) now = 65280;
        lj_cue = ePut(lj_handle, LJ_ioPUT_TIMER_VALUE, 0, now, 0);
        Sleep(25);
    }

    lj_cue = ePut(lj_handle, LJ_ioPUT_DAC, 0, 5, 0);
             // DAC0 = 5V turns the PNP off, so the green status LED goes out.
    lj_cue = ePut(lj_handle, LJ_ioPIN_CONFIGURATION_RESET, 0, 0, 0);
             // Put every pin back to how it comes from the factory.
    lj_cue = eGet(lj_handle, LJ_ioGET_DIGITAL_BIT, 4, 0, 0);   // Make FIO4 an input again.
    lj_cue = eGet(lj_handle, LJ_ioGET_DIGITAL_BIT, 5, 0, 0);   // Make FIO5 an input again.

    // SECTION 6 - Pause, Close the LabJack, then exit the program.

    system("pause");                          // Wait for a key press.
    Close();                                  // Close the LabJack.
    return 0;                                 // End the program.
}


