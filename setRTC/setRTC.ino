/*
 * RTCSync - A utility to set the DS1307 RTC time via Serial Monitor.
 *
 * INSTRUCTIONS:
 * 1. Connect your Arduino to the DS1307 RTC module (SDA, SCL, VCC, GND).
 * 2. Upload this sketch to your Arduino.
 * 3. Open the Serial Monitor (Ctrl+Shift+M or Tools -> Serial Monitor).
 * 4. Make sure the baud rate is set to 9600.
 * 5. Make sure the line ending is set to "Newline" or "Both NL & CR".
 * 6. You will see a prompt asking for the time.
 * 7. Type the current date and time in the format YYYY,MM,DD,hh,mm,ss
 *    - YYYY: 4-digit year (e.g., 2025)
 *    - MM: Month (1-12)
 *    - DD: Day (1-31)
 *    - hh: Hour in 24-hour format (0-23)
 *    - mm: Minute (0-59)
 *    - ss: Second (0-59)
 *
 *    EXAMPLE: For September 7, 2025, at 4:30:15 PM, you would enter:
 *    2025,9,7,16,30,15
 *
 * 8. Press the "Send" button or hit Enter.
 * 9. The sketch will confirm that the RTC time has been updated.
 * 10. You can now load your main clock sketch back onto the Arduino. The RTC will keep the correct time.
 */

#include <Wire.h>
#include <DS1307RTC.h> // Uses the TimeLib library internally
#include <TimeLib.h>

void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for Serial port to connect, needed for some Arduinos

  Wire.begin();

  Serial.println("--- RTC Time Setter ---");
  Serial.println();

  // Read and display the current time on the RTC
  Serial.print("Current RTC time: ");
  printCurrentTime();
  Serial.println();

  // Print instructions
  Serial.println("Please enter the new date and time in the format:");
  Serial.println("YYYY,MM,DD,hh,mm,ss");
  Serial.println("Example: 2025,9,7,16,30,15");
  Serial.println("Then press Send or Enter.");
  Serial.println("---------------------------------");
}

void loop() {
  // Check if data has been sent from the Serial Monitor
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim(); // Remove any leading/trailing whitespace

    if (parseAndSetTime(input)) {
      Serial.println("\nSUCCESS: RTC time has been updated!");
      Serial.print("New RTC time is now: ");
      printCurrentTime();
      Serial.println("---------------------------------");
      Serial.println("You can now upload your main clock sketch.");
      // Stop the loop after successfully setting the time.
      while (true) {}
    } else {
      Serial.println("\nERROR: Invalid format.");
      Serial.println("Please use the format YYYY,MM,DD,hh,mm,ss and try again.");
      Serial.println("---------------------------------");
    }
  }
}

// Parses the input string and sets the RTC time
bool parseAndSetTime(String input) {
  tmElements_t tm;
  int year, month, day, hour, minute, second;

  // sscanf is a C function to parse formatted strings.
  // It returns the number of items successfully matched. We expect 6.
  int itemsParsed = sscanf(input.c_str(), "%d,%d,%d,%d,%d,%d",
                           &year, &month, &day, &hour, &minute, &second);

  if (itemsParsed != 6) {
    return false; // The format was incorrect
  }

  // Populate the tmElements_t struct
  tm.Year   = CalendarYrToTm(year); // Convert year to years since 1970
  tm.Month  = month;
  tm.Day    = day;
  tm.Hour   = hour;
  tm.Minute = minute;
  tm.Second = second;

  // Write the new time to the RTC module
  if (RTC.write(tm)) {
    return true; // Success
  } else {
    Serial.println("ERROR: Failed to write to RTC module. Check wiring.");
    return false;
  }
}

// Reads the time from the RTC and prints it in a friendly format
void printCurrentTime() {
  tmElements_t currentTime;
  if (RTC.read(currentTime)) {
    // Zero-pad single digit values for cleaner output
    char timeBuffer[50];
    sprintf(timeBuffer, "%d-%02d-%02d %02d:%02d:%02d",
            tmYearToCalendar(currentTime.Year), currentTime.Month, currentTime.Day,
            currentTime.Hour, currentTime.Minute, currentTime.Second);
    Serial.println(timeBuffer);
  } else {
    if (RTC.chipPresent()) {
        Serial.println("The DS1307 is stopped. Please set the time.");
    } else {
        Serial.println("DS1307 read error! Please check the wiring.");
    }
  }
}