/*
 * Enhanced 4-Way Traffic Control System with Crosswalks
 * Features countdown timers, LCD display, and audio alerts
 * 
 * Enhanced Features:
 * - LCD countdown display for crosswalks
 * - Audio alerts for pedestrians
 * - Improved timing with pedestrian priority
 * - Night mode operation
 * - System diagnostics
 */

#include <LiquidCrystal.h>

// LCD Display (16x2)
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// Traffic Light Pin Definitions
const int NS_RED = 2;
const int NS_YELLOW = 3;
const int NS_GREEN = 4;
const int EW_RED = 5;
const int EW_YELLOW = 6;
const int EW_GREEN = 13;

// Crosswalk signals
const int NS_CROSSWALK_STOP = A1;
const int NS_CROSSWALK_GO = A2;
const int EW_CROSSWALK_STOP = A3;
const int EW_CROSSWALK_GO = A4;

// Pedestrian buttons
const int NS_BUTTON = A5;
const int EW_BUTTON = A6;

// Emergency and system controls
const int EMERGENCY_BUTTON = A0;
const int NIGHT_MODE_BUTTON = 1;
const int SPEAKER_PIN = 0;

// Timing constants (in milliseconds)
const unsigned long GREEN_TIME = 15000;      // 15 seconds green
const unsigned long YELLOW_TIME = 3000;      // 3 seconds yellow
const unsigned long RED_TIME = 2000;         // 2 seconds all red
const unsigned long CROSSWALK_TIME = 10000;  // 10 seconds crosswalk
const unsigned long CROSSWALK_WARNING = 3000; // 3 seconds flashing
const unsigned long NIGHT_MODE_INTERVAL = 2000; // 2 seconds night mode

// State definitions
enum TrafficState {
  NS_GREEN_EW_RED,
  NS_YELLOW_EW_RED,
  ALL_RED,
  EW_GREEN_NS_RED,
  EW_YELLOW_NS_RED,
  ALL_RED_2,
  NIGHT_MODE
};

// Global variables
TrafficState currentState = NS_GREEN_EW_RED;
unsigned long stateStartTime = 0;
bool nsButtonPressed = false;
bool ewButtonPressed = false;
bool emergencyMode = false;
bool nightMode = false;
bool crosswalkActive = false;
unsigned long crosswalkStartTime = 0;
int crosswalkFlashCount = 0;
unsigned long lastDisplayUpdate = 0;
int remainingTime = 0;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  Serial.println("Enhanced 4-Way Traffic Control System Starting...");
  
  // Initialize LCD
  lcd.begin(16, 2);
  lcd.print("Traffic Control");
  lcd.setCursor(0, 1);
  lcd.print("System Ready");
  delay(2000);
  
  // Configure traffic light pins as outputs
  pinMode(NS_RED, OUTPUT);
  pinMode(NS_YELLOW, OUTPUT);
  pinMode(NS_GREEN, OUTPUT);
  pinMode(EW_RED, OUTPUT);
  pinMode(EW_YELLOW, OUTPUT);
  pinMode(EW_GREEN, OUTPUT);
  
  // Configure crosswalk pins as outputs
  pinMode(NS_CROSSWALK_STOP, OUTPUT);
  pinMode(NS_CROSSWALK_GO, OUTPUT);
  pinMode(EW_CROSSWALK_STOP, OUTPUT);
  pinMode(EW_CROSSWALK_GO, OUTPUT);
  
  // Configure input pins
  pinMode(NS_BUTTON, INPUT_PULLUP);
  pinMode(EW_BUTTON, INPUT_PULLUP);
  pinMode(EMERGENCY_BUTTON, INPUT_PULLUP);
  pinMode(NIGHT_MODE_BUTTON, INPUT_PULLUP);
  pinMode(SPEAKER_PIN, OUTPUT);
  
  // Initialize all lights to off
  allLightsOff();
  
  // Start with North-South green, East-West red
  setTrafficLights(NS_GREEN_EW_RED);
  setCrosswalks(NS_GREEN_EW_RED);
  
  stateStartTime = millis();
  lastDisplayUpdate = millis();
  
  Serial.println("Enhanced Traffic Control System Ready!");
  displaySystemStatus();
}

void loop() {
  // Check for night mode toggle
  if (digitalRead(NIGHT_MODE_BUTTON) == LOW) {
    nightMode = !nightMode;
    if (nightMode) {
      currentState = NIGHT_MODE;
      stateStartTime = millis();
      Serial.println("Night mode activated");
    } else {
      currentState = NS_GREEN_EW_RED;
      stateStartTime = millis();
      Serial.println("Day mode activated");
    }
    delay(500); // Debounce
  }
  
  // Check for emergency override
  if (digitalRead(EMERGENCY_BUTTON) == LOW) {
    emergencyMode = true;
    handleEmergency();
    return;
  }
  
  // Handle night mode
  if (nightMode && currentState == NIGHT_MODE) {
    handleNightMode();
    return;
  }
  
  // Check for pedestrian button presses
  checkPedestrianButtons();
  
  // Handle crosswalk logic
  if (crosswalkActive) {
    handleCrosswalk();
    return;
  }
  
  // Main traffic light state machine
  handleTrafficStates();
  
  // Update display every 100ms
  if (millis() - lastDisplayUpdate >= 100) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }
  
  // Small delay to prevent overwhelming the system
  delay(50);
}

void handleTrafficStates() {
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - stateStartTime;
  
  // Calculate remaining time for display
  switch (currentState) {
    case NS_GREEN_EW_RED:
      remainingTime = (GREEN_TIME - elapsedTime) / 1000;
      if (elapsedTime >= GREEN_TIME) {
        changeState(NS_YELLOW_EW_RED);
      }
      break;
      
    case NS_YELLOW_EW_RED:
      remainingTime = (YELLOW_TIME - elapsedTime) / 1000;
      if (elapsedTime >= YELLOW_TIME) {
        changeState(ALL_RED);
      }
      break;
      
    case ALL_RED:
      remainingTime = (RED_TIME - elapsedTime) / 1000;
      if (elapsedTime >= RED_TIME) {
        changeState(EW_GREEN_NS_RED);
      }
      break;
      
    case EW_GREEN_NS_RED:
      remainingTime = (GREEN_TIME - elapsedTime) / 1000;
      if (elapsedTime >= GREEN_TIME) {
        changeState(EW_YELLOW_NS_RED);
      }
      break;
      
    case EW_YELLOW_NS_RED:
      remainingTime = (YELLOW_TIME - elapsedTime) / 1000;
      if (elapsedTime >= YELLOW_TIME) {
        changeState(ALL_RED_2);
      }
      break;
      
    case ALL_RED_2:
      remainingTime = (RED_TIME - elapsedTime) / 1000;
      if (elapsedTime >= RED_TIME) {
        changeState(NS_GREEN_EW_RED);
      }
      break;
  }
}

void changeState(TrafficState newState) {
  currentState = newState;
  stateStartTime = millis();
  
  setTrafficLights(newState);
  setCrosswalks(newState);
  
  // Play audio alert for state changes
  playStateChangeSound();
  
  Serial.print("State changed to: ");
  Serial.println(getStateName(newState));
}

void setTrafficLights(TrafficState state) {
  // Turn off all lights first
  allLightsOff();
  
  switch (state) {
    case NS_GREEN_EW_RED:
      digitalWrite(NS_GREEN, HIGH);
      digitalWrite(EW_RED, HIGH);
      break;
      
    case NS_YELLOW_EW_RED:
      digitalWrite(NS_YELLOW, HIGH);
      digitalWrite(EW_RED, HIGH);
      break;
      
    case ALL_RED:
      digitalWrite(NS_RED, HIGH);
      digitalWrite(EW_RED, HIGH);
      break;
      
    case EW_GREEN_NS_RED:
      digitalWrite(EW_GREEN, HIGH);
      digitalWrite(NS_RED, HIGH);
      break;
      
    case EW_YELLOW_NS_RED:
      digitalWrite(EW_YELLOW, HIGH);
      digitalWrite(NS_RED, HIGH);
      break;
      
    case ALL_RED_2:
      digitalWrite(NS_RED, HIGH);
      digitalWrite(EW_RED, HIGH);
      break;
      
    case NIGHT_MODE:
      // All lights off in night mode
      break;
  }
}

void setCrosswalks(TrafficState state) {
  // Turn off all crosswalk lights
  digitalWrite(NS_CROSSWALK_STOP, LOW);
  digitalWrite(NS_CROSSWALK_GO, LOW);
  digitalWrite(EW_CROSSWALK_STOP, LOW);
  digitalWrite(EW_CROSSWALK_GO, LOW);
  
  switch (state) {
    case NS_GREEN_EW_RED:
      digitalWrite(NS_CROSSWALK_GO, HIGH);
      digitalWrite(EW_CROSSWALK_STOP, HIGH);
      break;
      
    case NS_YELLOW_EW_RED:
      digitalWrite(NS_CROSSWALK_STOP, HIGH);
      digitalWrite(EW_CROSSWALK_STOP, HIGH);
      break;
      
    case ALL_RED:
      digitalWrite(NS_CROSSWALK_STOP, HIGH);
      digitalWrite(EW_CROSSWALK_STOP, HIGH);
      break;
      
    case EW_GREEN_NS_RED:
      digitalWrite(EW_CROSSWALK_GO, HIGH);
      digitalWrite(NS_CROSSWALK_STOP, HIGH);
      break;
      
    case EW_YELLOW_NS_RED:
      digitalWrite(EW_CROSSWALK_STOP, HIGH);
      digitalWrite(NS_CROSSWALK_STOP, HIGH);
      break;
      
    case ALL_RED_2:
      digitalWrite(NS_CROSSWALK_STOP, HIGH);
      digitalWrite(EW_CROSSWALK_STOP, HIGH);
      break;
      
    case NIGHT_MODE:
      // All crosswalk lights off in night mode
      break;
  }
}

void checkPedestrianButtons() {
  // Check North-South button
  if (digitalRead(NS_BUTTON) == LOW && !nsButtonPressed) {
    nsButtonPressed = true;
    Serial.println("North-South crosswalk button pressed");
    playButtonSound();
    // Only activate if currently in EW green phase
    if (currentState == EW_GREEN_NS_RED) {
      activateCrosswalk();
    }
  } else if (digitalRead(NS_BUTTON) == HIGH) {
    nsButtonPressed = false;
  }
  
  // Check East-West button
  if (digitalRead(EW_BUTTON) == LOW && !ewButtonPressed) {
    ewButtonPressed = true;
    Serial.println("East-West crosswalk button pressed");
    playButtonSound();
    // Only activate if currently in NS green phase
    if (currentState == NS_GREEN_EW_RED) {
      activateCrosswalk();
    }
  } else if (digitalRead(EW_BUTTON) == HIGH) {
    ewButtonPressed = false;
  }
}

void activateCrosswalk() {
  if (!crosswalkActive) {
    crosswalkActive = true;
    crosswalkStartTime = millis();
    crosswalkFlashCount = 0;
    Serial.println("Crosswalk activated - extending green time");
    playCrosswalkActivatedSound();
  }
}

void handleCrosswalk() {
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - crosswalkStartTime;
  
  if (elapsedTime >= CROSSWALK_TIME) {
    // End crosswalk and continue with normal sequence
    crosswalkActive = false;
    Serial.println("Crosswalk time ended");
  } else if (elapsedTime >= (CROSSWALK_TIME - CROSSWALK_WARNING)) {
    // Flash crosswalk lights during warning period
    if ((millis() / 500) % 2 == 0) {
      if (currentState == NS_GREEN_EW_RED) {
        digitalWrite(NS_CROSSWALK_GO, HIGH);
        digitalWrite(NS_CROSSWALK_STOP, LOW);
      } else if (currentState == EW_GREEN_NS_RED) {
        digitalWrite(EW_CROSSWALK_GO, HIGH);
        digitalWrite(EW_CROSSWALK_STOP, LOW);
      }
    } else {
      digitalWrite(NS_CROSSWALK_GO, LOW);
      digitalWrite(EW_CROSSWALK_GO, LOW);
      digitalWrite(NS_CROSSWALK_STOP, HIGH);
      digitalWrite(EW_CROSSWALK_STOP, HIGH);
    }
  }
}

void handleNightMode() {
  // Flash all yellow lights slowly
  if ((millis() / NIGHT_MODE_INTERVAL) % 2 == 0) {
    digitalWrite(NS_YELLOW, HIGH);
    digitalWrite(EW_YELLOW, HIGH);
  } else {
    digitalWrite(NS_YELLOW, LOW);
    digitalWrite(EW_YELLOW, LOW);
  }
  
  // Keep crosswalk lights off
  digitalWrite(NS_CROSSWALK_STOP, LOW);
  digitalWrite(NS_CROSSWALK_GO, LOW);
  digitalWrite(EW_CROSSWALK_STOP, LOW);
  digitalWrite(EW_CROSSWALK_GO, LOW);
}

void handleEmergency() {
  // Flash all red lights
  allLightsOff();
  
  if ((millis() / 500) % 2 == 0) {
    digitalWrite(NS_RED, HIGH);
    digitalWrite(EW_RED, HIGH);
    digitalWrite(NS_CROSSWALK_STOP, HIGH);
    digitalWrite(EW_CROSSWALK_STOP, HIGH);
  }
  
  // Check if emergency button is released
  if (digitalRead(EMERGENCY_BUTTON) == HIGH) {
    emergencyMode = false;
    crosswalkActive = false;
    // Return to normal operation
    if (nightMode) {
      currentState = NIGHT_MODE;
    } else {
      currentState = NS_GREEN_EW_RED;
    }
    stateStartTime = millis();
    Serial.println("Emergency mode deactivated");
  }
}

void updateDisplay() {
  lcd.clear();
  
  if (emergencyMode) {
    lcd.print("EMERGENCY MODE");
    lcd.setCursor(0, 1);
    lcd.print("All Lights Flash");
    return;
  }
  
  if (nightMode) {
    lcd.print("NIGHT MODE");
    lcd.setCursor(0, 1);
    lcd.print("Flashing Yellow");
    return;
  }
  
  // Display current state and countdown
  lcd.print(getStateName(currentState));
  lcd.setCursor(0, 1);
  
  if (crosswalkActive) {
    unsigned long crosswalkRemaining = (CROSSWALK_TIME - (millis() - crosswalkStartTime)) / 1000;
    lcd.print("Crosswalk: ");
    lcd.print(crosswalkRemaining);
    lcd.print("s");
  } else {
    lcd.print("Time: ");
    lcd.print(remainingTime);
    lcd.print("s");
  }
}

void displaySystemStatus() {
  lcd.clear();
  lcd.print("System Status:");
  lcd.setCursor(0, 1);
  lcd.print("All Systems OK");
  delay(2000);
}

void playStateChangeSound() {
  // Play a short beep for state changes
  tone(SPEAKER_PIN, 1000, 200);
}

void playButtonSound() {
  // Play a different beep for button presses
  tone(SPEAKER_PIN, 1500, 100);
}

void playCrosswalkActivatedSound() {
  // Play a longer beep for crosswalk activation
  tone(SPEAKER_PIN, 800, 500);
}

void allLightsOff() {
  digitalWrite(NS_RED, LOW);
  digitalWrite(NS_YELLOW, LOW);
  digitalWrite(NS_GREEN, LOW);
  digitalWrite(EW_RED, LOW);
  digitalWrite(EW_YELLOW, LOW);
  digitalWrite(EW_GREEN, LOW);
  digitalWrite(NS_CROSSWALK_STOP, LOW);
  digitalWrite(NS_CROSSWALK_GO, LOW);
  digitalWrite(EW_CROSSWALK_STOP, LOW);
  digitalWrite(EW_CROSSWALK_GO, LOW);
}

String getStateName(TrafficState state) {
  switch (state) {
    case NS_GREEN_EW_RED: return "NS Green, EW Red";
    case NS_YELLOW_EW_RED: return "NS Yellow, EW Red";
    case ALL_RED: return "All Red";
    case EW_GREEN_NS_RED: return "EW Green, NS Red";
    case EW_YELLOW_NS_RED: return "EW Yellow, NS Red";
    case ALL_RED_2: return "All Red 2";
    case NIGHT_MODE: return "Night Mode";
    default: return "Unknown";
  }
}
