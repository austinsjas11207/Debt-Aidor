#include <LiquidCrystal.h>
#include <Keypad.h>
#include <EEPROM.h>

// LCD pins: RS, EN, D4–D7
LiquidCrystal lcd(10, 11, 2, 3, 4, 5);

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {6, 7, 8, 9};
byte colPins[COLS] = {A2, A3, A4, A5};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

#define BACKSPACE_PIN 12
#define RESET_BUTTON_PIN A0
#define MAX_PLAYERS 6

String playerNames[MAX_PLAYERS];
int loans[MAX_PLAYERS];
int numPlayers = 0;
int currentPlayerIndex = 0;
int currentState = 0; // 0: player count, 1: name entry, 2: loan mode, 3: subtract mode
int nameIndex = 0;
String tempInput = "";

void setup() {
  lcd.begin(16, 2);
  pinMode(BACKSPACE_PIN, INPUT_PULLUP);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  showWelcome();

  // Load from EEPROM
  numPlayers = EEPROM.read(0);
  if (numPlayers >= 2 && numPlayers <= MAX_PLAYERS) {
    for (int i = 0; i < numPlayers; i++) {
      int addr = 1 + i * 2;
      loans[i] = EEPROM.read(addr) | (EEPROM.read(addr + 1) << 8);
    }
    currentPlayerIndex = 0;
    currentState = 2;
    showCurrentPlayer();
  } else {
    lcd.clear();
    lcd.print("Players (2-6)?");
    currentState = 0;
  }
}

void loop() {
  // In-game reset
  if (digitalRead(RESET_BUTTON_PIN) == LOW) {
    lcd.clear();
    lcd.print("Clearing Data...");
    resetEEPROM();
    delay(1000);
    lcd.clear();
    lcd.print("EEPROM Cleared!");
    delay(1500);
    resetGame();
    return;
  }

  if (digitalRead(BACKSPACE_PIN) == LOW) {
    delay(200);
    if (tempInput.length() > 0) {
      tempInput.remove(tempInput.length() - 1);
      if (currentState == 2) showCurrentPlayer();
      else if (currentState == 3) showSubtractPage();
    }
  }

  char key = keypad.getKey();
  if (key) {
    switch (currentState) {
      case 0: handlePlayerCount(key); break;
      case 1: handleNameEntry(key); break;
      case 2: handleLoanScreen(key); break;
      case 3: handleSubtractScreen(key); break;
    }
  }
}

void handlePlayerCount(char key) {
  if (key >= '2' && key <= '6') {
    numPlayers = key - '0';
    EEPROM.write(0, numPlayers);
    lcd.clear();
    lcd.print("Name P1:");
    nameIndex = 0;
    tempInput = "";
    currentState = 1;
  }
}

void handleNameEntry(char key) {
  if (key == '#') {
    if (tempInput.length() > 0) {
      playerNames[nameIndex] = tempInput;
      loans[nameIndex] = 0;
      saveLoanToEEPROM(nameIndex, 0);
      nameIndex++;
      tempInput = "";
      if (nameIndex < numPlayers) {
        lcd.clear();
        lcd.print("Name P");
        lcd.print(nameIndex + 1);
        lcd.print(":");
      } else {
        currentPlayerIndex = 0;
        currentState = 2;
        showCurrentPlayer();
      }
    }
  } else if (key == '*') {
    tempInput = "";
    lcd.setCursor(0, 1);
    lcd.print("                ");
  } else {
    if (tempInput.length() < 8) {
      tempInput += key;
      lcd.setCursor(0, 1);
      lcd.print(tempInput);
    }
  }
}

void handleLoanScreen(char key) {
  if (key >= '0' && key <= '9') {
    if (tempInput.length() < 5) {
      tempInput += key;
      showCurrentPlayer();

      // Check for 999 entry
      if (tempInput == "999") {
        tempInput = "";
        currentState = 3;
        showSubtractPage();
      }
    }
  } else if (key == '#') {
    if (tempInput != "") {
      int amt = tempInput.toInt();
      loans[currentPlayerIndex] += amt;
      saveLoanToEEPROM(currentPlayerIndex, loans[currentPlayerIndex]);
      tempInput = "";
      showCurrentPlayer();
    }
  } else if (key == '*') {
    currentPlayerIndex = (currentPlayerIndex + 1) % numPlayers;
    tempInput = "";
    showCurrentPlayer();
  }
}

void handleSubtractScreen(char key) {
  if (key >= '0' && key <= '9') {
    if (tempInput.length() < 5) {
      tempInput += key;
      showSubtractPage();
    }
  } else if (key == '#') {
    int amt = tempInput.toInt();
    if (amt <= loans[currentPlayerIndex]) {
      loans[currentPlayerIndex] -= amt;
    } else {
      loans[currentPlayerIndex] = 0;
    }
    saveLoanToEEPROM(currentPlayerIndex, loans[currentPlayerIndex]);
    tempInput = "";
    currentState = 2;
    showCurrentPlayer();
  }
}

void showCurrentPlayer() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Player");
  lcd.print(currentPlayerIndex + 1);
  lcd.print(":");
  lcd.print(playerNames[currentPlayerIndex].substring(0, 8));

  lcd.setCursor(0, 1);
  lcd.print("T:");
  lcd.print(loans[currentPlayerIndex]);

  lcd.setCursor(9, 1);
  lcd.print("N:");
  lcd.print(tempInput);
  for (int i = tempInput.length(); i < 5; i++) {
    lcd.print(" ");
  }
}

void showSubtractPage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter No:");
  lcd.setCursor(0, 1);
  lcd.print("T:");
  lcd.print(loans[currentPlayerIndex]);
  lcd.setCursor(9, 1);
  lcd.print(tempInput);
  for (int i = tempInput.length(); i < 5; i++) {
    lcd.print(" ");
  }
}

void resetGame() {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    loans[i] = 0;
    playerNames[i] = "";
  }
  numPlayers = 0;
  currentPlayerIndex = 0;
  nameIndex = 0;
  tempInput = "";
  currentState = 0;

  lcd.clear();
  lcd.print("Players (2-6)?");
}

void resetEEPROM() {
  EEPROM.write(0, 0);
  for (int i = 1; i <= MAX_PLAYERS * 2; i++) {
    EEPROM.write(i, 0);
  }
}

void saveLoanToEEPROM(int index, int value) {
  int addr = 1 + index * 2;
  EEPROM.write(addr, value & 0xFF);
  EEPROM.write(addr + 1, (value >> 8) & 0xFF);
}

void showWelcome() {
  String scrollText = "   Debtor Aid Welcomes You!   ";
  for (int i = 0; i < scrollText.length() - 15; i++) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(scrollText.substring(i, i + 16));
    delay(250);
  }
  delay(500);
}
