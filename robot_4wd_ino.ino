/*
  =============================================================================
 * 4WD Robot — DO NOT REVERSE VERSION (Memory Spin Logic)
  =============================================================================
 */

const uint8_t IN1 = 8;   const uint8_t IN2 = 9;   
const uint8_t IN3 = 11;  const uint8_t IN4 = 10;  

const uint8_t TRIG_FRONT = 2,  ECHO_FRONT = 3;
const uint8_t TRIG_RIGHT = 4,  ECHO_RIGHT = 5;
const uint8_t TRIG_LEFT  = 6,  ECHO_LEFT  = 7;

const uint8_t LINE_RIGHT  = A0;
const uint8_t LINE_CENTER = A1;
const uint8_t LINE_LEFT   = A2;

const bool LINE_DETECTS_BLACK_AS_HIGH = true; 

const uint16_t OBSTACLE_LIMIT = 20;   
const uint16_t CRITICAL_LIMIT = 8;    

// أوقات المناورة
const unsigned long TURN_OUT_MS    = 350;  
const unsigned long PASS_OBSTACLE_MS = 650;  
const unsigned long TURN_IN_MS     = 400;  

enum RobotState { LINE_FOLLOW, AVOID_EVALUATE, AVOID_REVERSE, AVOID_TURN_OUT, AVOID_FORWARD, AVOID_TURN_IN, AVOID_RETURN_LINE };
RobotState state = LINE_FOLLOW;
unsigned long stateStartMs = 0;

// ذاكرة الاتجاه: 0=نص, 1=يمين, 2=شمال
int lastSeen = 0; 
bool avoidedToRight = true;

// =============================================================================
//  Motor Functions
// =============================================================================
void motorStop()     { digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);  digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW); }
void motorForward()  { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); }
void motorBackward() { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }

// الدوران في المكان
void pivotLeft()  { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
void pivotRight() { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); }

// =============================================================================
//  Sensors
// =============================================================================
int ping(uint8_t trig, uint8_t echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long dur = pulseIn(echo, HIGH, 20000);
  return (dur == 0) ? 200 : dur / 58;
}

bool seesLine(uint8_t pin) {
  return (digitalRead(pin) == (LINE_DETECTS_BLACK_AS_HIGH ? HIGH : LOW));
}

void enterState(RobotState s) { state = s; stateStartMs = millis(); }

// =============================================================================
//  Line Follower (THE FIX IS HERE)
// =============================================================================
void runLineFollow() {
  bool L = seesLine(LINE_LEFT);
  bool C = seesLine(LINE_CENTER);
  bool R = seesLine(LINE_RIGHT);

  if (C) {
    motorForward();
    lastSeen = 0;
  } 
  else if (L) {
    pivotLeft();
    lastSeen = 2; // حفظنا إن الخط كان شمال
  } 
  else if (R) {
    pivotRight();
    lastSeen = 1; // حفظنا إن الخط كان يمين
  } 
  else {
    // ---- حالة التوهان وفقدان الخط ----
    
    if (lastSeen == 2) {
      pivotLeft();  // كمل دوران شمال لغاية ما تمسكه
    } else if (lastSeen == 1) {
      pivotRight(); // كمل دوران يمين لغاية ما تمسكه
    } else {
      motorForward(); // لو فقدته وهي ماشية مستقيم، تكمل مستقيم ثواني لحد ما تلقطه
    }
  }
}

// =============================================================================
//  Avoidance Logic
// =============================================================================
void runAvoidance() {
  unsigned long elapsed = millis() - stateStartMs;

  switch (state) {
    case AVOID_EVALUATE:
      motorStop();
      if (elapsed >= 250) {
        int dF = ping(TRIG_FRONT, ECHO_FRONT);
        if (dF <= CRITICAL_LIMIT) { motorBackward(); enterState(AVOID_REVERSE); }
        else {
          int dR = ping(TRIG_RIGHT, ECHO_RIGHT); delay(30);
          int dL = ping(TRIG_LEFT, ECHO_LEFT);
          if (dR >= dL) { pivotRight(); avoidedToRight = true; }
          else { pivotLeft(); avoidedToRight = false; }
          enterState(AVOID_TURN_OUT);
        }
      }
      break;

    case AVOID_REVERSE: // دي للفرملة الشديدة قدام العائق فقط، مش للخط!
      if (elapsed >= 350) enterState(AVOID_EVALUATE);
      break;

    case AVOID_TURN_OUT:
      if (elapsed >= TURN_OUT_MS) { motorForward(); enterState(AVOID_FORWARD); }
      break;

    case AVOID_FORWARD:
      if (elapsed >= PASS_OBSTACLE_MS) {
        if (avoidedToRight) pivotLeft(); else pivotRight();
        enterState(AVOID_TURN_IN);
      }
      break;

    case AVOID_TURN_IN:
      if (elapsed >= TURN_IN_MS) { 
        motorForward(); // تنطلق للأمام لتقاطع الخط
        enterState(AVOID_RETURN_LINE); 
      }
      break;

    case AVOID_RETURN_LINE:
      // مجرد ما أي حساس يلمس الخط، ارجع فوراً لعملية تتبع الخط
      if (seesLine(LINE_CENTER) || seesLine(LINE_LEFT) || seesLine(LINE_RIGHT)) {
        lastSeen = 0; // نصفر الذاكرة
        enterState(LINE_FOLLOW);
      }
      // حماية: لو طولت أوي وما لقتش الخط، تقف مكانها أأمن من إنها تهرب
      else if (elapsed > 2000) {
        motorStop();
      }
      break;
  }
}

void setup() {
  const uint8_t outs[] = { IN1, IN2, IN3, IN4, TRIG_FRONT, TRIG_RIGHT, TRIG_LEFT };
  for (int i=0; i<7; i++) pinMode(outs[i], OUTPUT);
  const uint8_t ins[] = { ECHO_FRONT, ECHO_RIGHT, ECHO_LEFT, LINE_RIGHT, LINE_CENTER, LINE_LEFT };
  for (int i=0; i<6; i++) pinMode(ins[i], INPUT);
  motorStop();
}

void loop() {
  if (state == LINE_FOLLOW) {
    int dF = ping(TRIG_FRONT, ECHO_FRONT);
    if (dF > 0 && dF < OBSTACLE_LIMIT) {
      enterState(AVOID_EVALUATE);
    } else {
      runLineFollow();
    }
  } else {
    runAvoidance();
  }
}