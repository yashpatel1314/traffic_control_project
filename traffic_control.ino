/*
 * 4-Way Traffic Control System with Crosswalks
 * Based on standard traffic light timing patterns
 * 
 * Features:
 * - 4-way intersection with traffic lights
 * - Pedestrian crosswalk signals
 * - Automatic timing sequences
 * - Emergency override capability
 */

// Traffic Light Pin Definitions
// North-South direction
const int NS_RED = 2;
const int NS_YELLOW = 3;
const int NS_GREEN = 4;

// East-West direction  
const int EW_RED = 5;
const int EW_YELLOW = 6;
const int EW_GREEN = 7;

// Crosswalk signals
const int NS_CROSSWALK_STOP = 8;    // Red hand
const int NS_CROSSWALK_GO = 9;      // Walking person
const int EW_CROSSWALK_STOP = 10;   // Red hand
const int EW_CROSSWALK_GO = 11;     // Walking person

// Pedestrian buttons
const int NS_BUTTON = 12;
const int EW_BUTTON = 13;

// Emergency override button
const int EMERGENCY_BUTTON = A0;

// Timing constants (in milliseconds)
const unsigned long GREEN_TIME = 15000;      // 15 seconds green
const unsigned long YELLOW_TIME = 3000;      // 3 seconds yellow
const unsigned long RED_TIME = 2000;         // 2 seconds all red
const unsigned long CROSSWALK_TIME = 10000;  // 10 seconds crosswalk
const unsigned long CROSSWALK_WARNING = 3000; // 3 seconds flashing

// State definitions
enum TrafficState {
  NS_GREEN_EW_RED,
  NS_YELLOW_EW_RED,
  ALL_RED,
  EW_GREEN_NS_RED,
  EW_YELLOW_NS_RED,
  ALL_RED_2
};

// Global variables
TrafficState currentState = NS_GREEN_EW_RED;
unsigned long stateStartTime = 0;
bool nsButtonPressed = false;
bool ewButtonPressed = false;
bool emergencyMode = false;
bool crosswalkActive = false;
unsigned long crosswalkStartTime = 0;
int crosswalkFlashCount = 0;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  Serial.println("4-Way Traffic Control System Starting...");
  
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
  
  // Initialize all lights to off
  allLightsOff();
  
  // Start with North-South green, East-West red
  setTrafficLights(NS_GREEN_EW_RED);
  setCrosswalks(NS_GREEN_EW_RED);
  
  stateStartTime = millis();
  
  Serial.println("Traffic Control System Ready!");
}

void loop() {
  // Check for emergency override
  if (digitalRead(EMERGENCY_BUTTON) == LOW) {
    emergencyMode = true;
    handleEmergency();
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
  
  // Small delay to prevent overwhelming the system
  delay(100);
}

void handleTrafficStates() {
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - stateStartTime;
  
  switch (currentState) {
    case NS_GREEN_EW_RED:
      if (elapsedTime >= GREEN_TIME) {
        changeState(NS_YELLOW_EW_RED);
      }
      break;
      
    case NS_YELLOW_EW_RED:
      if (elapsedTime >= YELLOW_TIME) {
        changeState(ALL_RED);
      }
      break;
      
    case ALL_RED:
      if (elapsedTime >= RED_TIME) {
        changeState(EW_GREEN_NS_RED);
      }
      break;
      
    case EW_GREEN_NS_RED:
      if (elapsedTime >= GREEN_TIME) {
        changeState(EW_YELLOW_NS_RED);
      }
      break;
      
    case EW_YELLOW_NS_RED:
      if (elapsedTime >= YELLOW_TIME) {
        changeState(ALL_RED_2);
      }
      break;
      
    case ALL_RED_2:
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
      // North-South can cross
      digitalWrite(NS_CROSSWALK_GO, HIGH);
      digitalWrite(EW_CROSSWALK_STOP, HIGH);
      break;
      
    case NS_YELLOW_EW_RED:
      // North-South warning, East-West stop
      digitalWrite(NS_CROSSWALK_STOP, HIGH);
      digitalWrite(EW_CROSSWALK_STOP, HIGH);
      break;
      
    case ALL_RED:
      // All stop
      digitalWrite(NS_CROSSWALK_STOP, HIGH);
      digitalWrite(EW_CROSSWALK_STOP, HIGH);
      break;
      
    case EW_GREEN_NS_RED:
      // East-West can cross
      digitalWrite(EW_CROSSWALK_GO, HIGH);
      digitalWrite(NS_CROSSWALK_STOP, HIGH);
      break;
      
    case EW_YELLOW_NS_RED:
      // East-West warning, North-South stop
      digitalWrite(EW_CROSSWALK_STOP, HIGH);
      digitalWrite(NS_CROSSWALK_STOP, HIGH);
      break;
      
    case ALL_RED_2:
      // All stop
      digitalWrite(NS_CROSSWALK_STOP, HIGH);
      digitalWrite(EW_CROSSWALK_STOP, HIGH);
      break;
  }
}

void checkPedestrianButtons() {
  // Check North-South button
  if (digitalRead(NS_BUTTON) == LOW && !nsButtonPressed) {
    nsButtonPressed = true;
    Serial.println("North-South crosswalk button pressed");
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
    changeState(NS_GREEN_EW_RED);
    Serial.println("Emergency mode deactivated");
  }
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
    default: return "Unknown";
  }
}
