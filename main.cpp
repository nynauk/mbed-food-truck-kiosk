//Group 10: Ayaan Malik, Karim Huessein, Akram Halhoul Kaffal, Haci Aras, Isabel Geoff
//I2C library found here: https://github.com/sstaub/mbedLCDi2c/
#include "mbed.h"
#include "LCDi2c.h"
// Define I2C pins for the LCD
#define SDA_PIN PB_9
#define SCL_PIN PB_8

// Initialize the I2C-based LCD
LCDi2c lcd(SDA_PIN, SCL_PIN, LCD16x2, 0x27); // LCD16x2 for a 16x2 display, address 0x27

// LEDs
DigitalOut greenLED(D10, 1); // Turned ON by default
DigitalOut redLED(D12, 1);
DigitalOut whiteLED(D11, 1);

// Buttons with pull-down resistors
DigitalIn buttonIceCream(D5, PullDown);
DigitalIn buttonBurger(D6, PullDown);
DigitalIn buttonSoda(D7, PullDown);

// Servo control pin
PwmOut servoPin(PC_7);

// LM35 Temperature Sensor
AnalogIn lm35_sensor(A0);

// DFPlayer Serial Connection
#define DFTX D8
#define DFRX D2
UnbufferedSerial dfplayer(DFTX, DFRX, 9600); // DFPlayer using serial communication

// Timer for inactivity, scrolling, servo control, and LED flashing
Timer inactivityTimer;
Timer scrollTimer;
Timer servoTimer;
Timer flashTimer;

// Variables
const char* defaultMessage = "Welcome to our food truck! ";
const char* iceCreamMessage = "Ice Cream - Delicious Treats! ";
const char* burgerMessage = "Burger - Tasty and Juicy! ";
const char* sodaMessage = "Soda - Cool and Refreshing! ";

const char* currentMessage = defaultMessage; // Message currently being displayed
char scrollBuffer[17] = "";                  // Buffer for 16 visible characters + null terminator
int scrollIndex = 0;                         // Current index of scrolling message
bool servoActive = false;                    // Tracks if the servo is active
bool welcomePlayed = false;                  // Tracks if the welcome track has already been played
bool flashing = false;                       // Tracks if an LED is flashing
int flashingLED = -1;                        // Indicates which LED is flashing (0 = green, 1 = red, 2 = white)
bool ledState = true;                        // Tracks the current state of the flashing LED

// Constants
#define SCROLL_DELAY 200ms       // Delay between scrolling updates
#define INACTIVITY_TIMEOUT 10s   // Time after which the system defaults to welcome message
#define SERVO_RESET_TIMEOUT 10s  // Time after which the servo resets
#define FLASH_DURATION 10s       // Duration for which the LED should flash
#define FLASH_INTERVAL 500ms     // LED flashing interval
#define ADC_REF_MV 3300          // ADC reference voltage in millivolts
#define LM35_SCALING 10          // LM35 outputs 10mV/°C
#define TEMP_OFFSET 5            // Additional constant offset for temperature

// Function to send command to DFPlayer
void sendCommand(uint8_t command, uint8_t param1, uint8_t param2) {
    uint8_t buffer[10];
    buffer[0] = 0x7E; buffer[1] = 0xFF; buffer[2] = 0x06; buffer[3] = command;
    buffer[4] = 0x01; buffer[5] = param1; buffer[6] = param2;
    uint16_t checksum = 0xFFFF - (buffer[1] + buffer[2] + buffer[3] + buffer[4] + buffer[5] + buffer[6]) + 1;
    buffer[7] = (checksum >> 8) & 0xFF; buffer[8] = checksum & 0xFF; buffer[9] = 0xEF;
    dfplayer.write(buffer, sizeof(buffer));
    ThisThread::sleep_for(100ms);
}

// Function to scroll the current message
void scrollMessage() {
    int length = strlen(currentMessage);

    // Extract 16 characters starting from the current scroll index
    for (int i = 0; i < 16; i++) {
        scrollBuffer[i] = currentMessage[(scrollIndex + i) % length];
    }
    scrollBuffer[16] = '\0'; // Null-terminate the buffer

    lcd.locate(0, 1); // Bottom row
    lcd.printf("%s", scrollBuffer); // Display the scrolling message

    scrollIndex = (scrollIndex + 1) % length; // Update the scroll index
}

// Function to set the current message and reset scroll
void setMessage(const char* message) {
    currentMessage = message;
    scrollIndex = 0; // Reset the scroll index
    inactivityTimer.reset(); // Reset the inactivity timer
}

// Function to control the servo
void handleServo() {
    // Move servo to max angle (180 degrees)
    servoPin.pulsewidth(0.002f); // Max position
    servoTimer.reset(); // Reset the timer whenever a servo action starts
    servoActive = true;
}

void resetServo() {
    // Move servo back to the default position
    servoPin.pulsewidth(0.001f); // Default position
    servoActive = false;
}

// Function to play track based on the current state (without looping)
void playTrack(uint8_t track) {
    sendCommand(0x03, 0x00, track); // Loop parameter is always set to 0x00 (no loop)
}


// Function to start flashing a specific LED
void startFlashing(int ledIndex) {
    flashing = true;
    flashingLED = ledIndex;
    flashTimer.reset();
    flashTimer.start();
    ledState = true; // Start with LED ON
}

// Function to handle LED flashing
void handleFlashing() {
    if (flashing) {
        if (flashTimer.elapsed_time() < FLASH_DURATION) {
            if (flashTimer.elapsed_time() % FLASH_INTERVAL < FLASH_INTERVAL / 2) {
                ledState = !ledState; // Toggle LED state
            }

            // Update the state of the flashing LED
            if (flashingLED == 0) greenLED = ledState;
            if (flashingLED == 1) redLED = ledState;
            if (flashingLED == 2) whiteLED = ledState;
        } else {
            // Stop flashing after the duration
            flashing = false;
            flashingLED = -1;
            greenLED = 1; // Reset all LEDs to ON
            redLED = 1;
            whiteLED = 1;
        }
    }
}

int main() {
    ThisThread::sleep_for(1s); // Allow components to initialize
    lcd.cls(); // Clear the LCD
    inactivityTimer.start(); // Start the inactivity timer
    scrollTimer.start(); // Start the scroll timer
    servoTimer.start(); // Start the servo timer
    flashTimer.stop();

    // Initialize servo to the default position
    servoPin.period(0.02f); // 20ms period (50Hz)
    servoPin.pulsewidth(0.001f); // Default to 0 degrees

    setMessage(defaultMessage); // Start with the default welcome message

    while (true) {
        // **Temperature Display (Top Row)**
        uint16_t adcValue = lm35_sensor.read_u16(); // Read raw ADC value (0 to 65535)
        uint32_t millivolts = (adcValue * ADC_REF_MV) / 65536; // Convert ADC value to millivolts
        uint16_t temperature = (millivolts / LM35_SCALING) + TEMP_OFFSET; // Convert to temperature in °C with offset

        lcd.locate(0, 0); // Top row
        lcd.printf("Temp: %d C       ", temperature); // Display temperature

        // Inside the main loop, modify the playTrack calls as follows:

        if (buttonIceCream.read()) {
            setMessage(iceCreamMessage);
            startFlashing(0); // Flash Green LED
            handleServo();    // Start servo movement
            playTrack(1);     // Play Ice Cream track (once)
            welcomePlayed = false; // Reset welcome track flag
        }

        if (buttonBurger.read()) {
            setMessage(burgerMessage);
            startFlashing(1); // Flash Red LED
            handleServo();    // Start servo movement
            playTrack(2);     // Play Burger track (once)
            welcomePlayed = false; // Reset welcome track flag
        }

        if (buttonSoda.read()) {
            setMessage(sodaMessage);
            startFlashing(2); // Flash White LED
            handleServo();    // Start servo movement
            playTrack(3);     // Play Soda track (once)
            welcomePlayed = false; // Reset welcome track flag
        }

        // Handle inactivity to play the welcome track once
        if (inactivityTimer.elapsed_time() > INACTIVITY_TIMEOUT) {
            if (!welcomePlayed) {
                playTrack(4);     // Play Welcome track only once
                welcomePlayed = true; // Set the flag to prevent looping
            }
            setMessage(defaultMessage); // Reset to default message
        }
        // **Check Servo Inactivity**
        if (servoActive && servoTimer.elapsed_time() > SERVO_RESET_TIMEOUT) {
            resetServo(); // Reset servo after inactivity
        }

        // **Handle LED Flashing**
        handleFlashing();

        // **Scroll the Current Message**
        if (scrollTimer.elapsed_time() > SCROLL_DELAY) {
            scrollMessage(); // Scroll the message
            scrollTimer.reset(); // Reset the scroll timer
        }

        // Sleep briefly to maintain a consistent refresh rate
        ThisThread::sleep_for(50ms);
    }
}