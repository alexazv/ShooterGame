#include <LiquidCrystal.h>

#define MAX_PROJ 4
#define MAX_EN 3

#define NOTE_C3 131
#define NOTE_C4 262
#define NOTE_A4 440

#define UP 0
#define DOWN 1

#define ENABLED 0
#define DISABLED 1
#define MISSED 2
#define DESTROYED 3

#define BUZZ_PIN 8

int score;
bool buttonPressed = false;
byte control;
int analogInput = A0;

typedef struct Entity{
  int xPos;
  int yPos;
  int level;
  byte sprite[8];
  byte state;
  byte charByte;
};

Entity projectiles[MAX_PROJ];
Entity enemies[MAX_EN];
Entity ship;

int previousInput = 0;

LiquidCrystal lcd(12, 11, 5, 4, 3, 6);

void setup() {
  pinMode(analogInput, INPUT);
  pinMode(2, INPUT);

  //attachInterrupt(0, directionISR, CHANGE);
  attachInterrupt(0, shootButtonISR, HIGH);
  
  lcd.begin(16, 2);
  lcd.display();
  lcd.noBlink();
  Serial.begin(9600);
  resetGame();
}

void resetGame(){
  ship = {.xPos = 0, .yPos = -1, .level = UP, .sprite = {0,0,0,0,0,0,0,0}, .state = ENABLED, .charByte = 7};

  for(byte i = 0; i < MAX_PROJ; i++)
    projectiles[i] = {.xPos = 0, .yPos = 0, .level = UP, .sprite = {0,0,0,0,0,0,0,0}, .state = DISABLED, .charByte = i}; 

  for(byte i = 0; i < MAX_EN; i++)
    enemies[i] = {.xPos = 0, .yPos = 0, .level = UP, .sprite = {0,0,0,0,0,0,0,0}, .state = DISABLED, .charByte = i+(byte)MAX_PROJ};

  score = 0;
  moveShip(DOWN);

  buttonPressed = false;
  Serial.flush();

  while(!Serial.available() && !buttonPressed){
    lcd.clear();
    lcd.home();
    lcd.write("Send data-digitl");
    lcd.setCursor(0,1);
    lcd.write("Button-analog");
    delay(20);
  }

  control = Serial.available();
  Serial.flush();

  
    lcd.clear();
    lcd.home();
  if(control){
    lcd.write("Digital mode");
    delay(2000);
    lcd.clear();
    lcd.write("w-up ; s-dwn");
    lcd.setCursor(0,1);
    lcd.write("space - shoot");
    
  } else {
    lcd.write("Analog mode");
    delay(2000);
    lcd.clear();
    lcd.write("pot changes dir");
    lcd.setCursor(0,1);
    lcd.write("button - shoot");
  }
    delay(2000);
  
}

int enemyTimer = 50;

void loop() {
  
    lcd.clear();

    if(ship.state != ENABLED){
     
     lcd.home();
     lcd.write("GAME OVER");
     lcd.setCursor(0, 1);
     lcd.print("SCORE: " + String(score, DEC));
     tone(8, NOTE_C4, 1500);
     delay(2500);
     resetGame();
     return;
    }
          
    readInput();

    drawEntity(&ship);
    for(int i = 0; i < MAX_PROJ; i++)
      drawEntity(&projectiles[i]);
    for(int i = 0; i < MAX_EN; i++)
      drawEntity(&enemies[i]);

    for(int i = 0; i < MAX_PROJ; i++){ //projectiles go foward
      if(projectiles[i].state == ENABLED)
        projectiles[i].xPos++;
      if(projectiles[i].xPos > 15) //projectile disappears if reaches the end
        projectiles[i].state = DISABLED;
    }

    if(enemyTimer == 0){
      if(random(2))
        createEnemy();
      for(int i = 0; i < MAX_EN; i++)//enemies go foward
        if(enemies[i].state == ENABLED)
         enemies[i].xPos--;     
      
      enemyTimer = (score/2 <= 30) ? 30-score/2 : 0;
    }

    detectCollision();
    
    delay(30);
    enemyTimer--;
  
}

void  moveShip(int dir){
  //pos = 0 -> 5
   if(dir == UP){
     if(ship.yPos > 0){
        ship.yPos--;
     }
     else if(ship.level == DOWN){
        ship.yPos = 5;
        ship.level = UP;
     } else{
        return;
     }
   }
   
   if(dir == DOWN){
     if(ship.yPos < 5){
        ship.yPos++;
     }
     else if(ship.level == UP){
        ship.yPos = 0;
        ship.level = DOWN;
     } else{
        return;
     }
   }
       
    //build ship sprite for current position
    for(int i = 0; i < 8; i++){
      if(i < ship.yPos || i > ship.yPos+2)
        ship.sprite[i] = B00000;
      else if(i % 2 == ship.yPos % 2)
        ship.sprite[i] = B11100;
      else
        ship.sprite[i] = B00111;
    }

    lcd.createChar(ship.charByte, ship.sprite);
}

void shootProjectile(){

    for(int i = 0; i < MAX_PROJ; i++){
      if(projectiles[i].state == DISABLED){
       byte sprite_build[8];
       
       for(int j = 0; j < 8; j++){ //build projectile sprite
        if(j == ship.yPos)
          projectiles[i].sprite[j] = B11111;
        else 
          projectiles[i].sprite[j] = B00000;
        }
      
        projectiles[i].state = ENABLED;
        projectiles[i].yPos = ship.yPos;
        projectiles[i].level = ship.level;
        projectiles[i].xPos = 1;
        lcd.createChar(i, projectiles[i].sprite);
        break;
      } 
    }
}

void detectCollision(){
      
     for(int i = 0; i < MAX_PROJ; i++){
      for(int j = 0; j < MAX_EN; j++){
        if(projectiles[i].state == ENABLED && enemies[j].state == ENABLED && projectiles[i].xPos == enemies[j].xPos && projectiles[i].level == enemies[j].level){ //check if position is same
          if(projectiles[i].yPos >= enemies[j].yPos && projectiles[i].yPos <= enemies[j].yPos+2){ //if colides, delete both projectile and enemy
              projectiles[i].state = DISABLED;
              enemies[j].state = DESTROYED;
              score++;
            } else { 
              //projectiles[i].state = MISSED;
              //enemies[j].state = MISSED;
            }
          }
      }

    for(int i = 0; i < MAX_EN; i++){ //if the ship colides with an enemy, destroy the ship
      
      if(enemies[i].state != DISABLED && enemies[i].state != DESTROYED && enemies[i].xPos == 0){
        if(enemies[i].yPos >= ship.yPos && enemies[i].yPos <= ship.yPos+2 && ship.level == enemies[i].level){
          ship.state = DESTROYED;
        }
        enemies[i].state = DISABLED;     
    }
   }
 }
}


void createEnemy(){

    for(int i = 0; i < MAX_EN; i++){
      if(enemies[i].state == DISABLED){
       byte sprite_build[8];

       enemies[i].xPos = 15;
       enemies[i].yPos = random(6);
       enemies[i].level = random(2);
       
       ///enemy sprite is 5x3
       for(int j = 0; j < 8; j++){ //build enemy sprite
        if(j < enemies[i].yPos || j > enemies[i].yPos+2)
          enemies[i].sprite[j] = B00000;
        else{
          enemies[i].sprite[j] = (random(B1000, B10000) << 1)+1;
        }
       }
             
        enemies[i].state = ENABLED;
        
        lcd.createChar(i+MAX_PROJ, enemies[i].sprite);
        break;
      } 
   }
 }

void drawEntity(struct Entity * current){
   switch (current->state){
     case DISABLED:
       break;
     case ENABLED:
       lcd.setCursor(current->xPos, current->level);
       lcd.write(byte(current->charByte));
       break;
     case MISSED:
       current->state = ENABLED;
       break;
     case DESTROYED:
       lcd.setCursor(current->xPos, current->level);
       lcd.write('#');
       noTone(BUZZ_PIN);
       tone(BUZZ_PIN, NOTE_A4, 200);
       current->state = DISABLED;
       break;
     default:
       break;
  }
}

void readInput(){

  interrupts();
  
  if(control){
    char input = Serial.read();
    Serial.println("Recebido: " + input);
  
    switch(input){
      case 'w':
        moveShip(UP);
        break;
      case 's':
        moveShip(DOWN);
        break;
      case ' ':
        shootProjectile();
        break;
      default:
        break;    
    }
    Serial.flush();
    return;
  }

  int input = map(analogRead(analogInput), 980, 1008, 0, 255);
  int damp = 30;
    if(input-damp > 255/2){
      moveShip(DOWN);
      previousInput = input;
    }
    else if(input+damp < 255/2){
      moveShip(UP);
      previousInput = input;
    }
  //previousInput = input;
    
  if(buttonPressed){
    shootProjectile();
    buttonPressed = false;
  }
}

void shootButtonISR(){
  buttonPressed = true;
}
