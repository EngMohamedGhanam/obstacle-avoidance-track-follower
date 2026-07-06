# 🤖 Autonomous 4WD Robot — Line Follower with Obstacle Avoidance

**Robotics Design — Challenge 2 | New Ismailia National University, AI Department**

This project is an autonomous robot that follows a black line on the ground and, if it sees an obstacle blocking its way, steers around it by itself and then finds the line again and keeps going. Built on an Arduino Uno, it uses IR sensors to "see" the line and ultrasonic sensors to "see" obstacles.

This README explains the whole project — the hardware, the idea behind the code, and how every part of the code works — so anyone can open this repo and understand it without needing to read anything else.

---

## 1. What the Robot Does

- Drives forward following a black line on a white surface.
- If an obstacle appears in front (closer than 20 cm), it stops line-following, turns away from the obstacle, drives past it, turns back toward the track, and hunts for the line until it finds it again.
- If something gets dangerously close (under 8 cm) before it has a chance to turn, it backs up first to give itself room, then re-checks and turns.
- If the line disappears for a moment (e.g. sharp curve), it remembers which side it last saw the line on and keeps turning that way until it re-catches it, instead of stopping or going in random directions.

## 2. Hardware Used

| Component | Purpose |
|---|---|
| **Arduino Uno Rev3** | The "brain" — reads all sensors and controls the motors |
| **L298N Motor Driver** | Dual H-Bridge, lets the Arduino control the 4 motors' direction and speed |
| **4x DC Geared Motors (12V, 450 RPM)** | Drive the 4 wheels |
| **3x HC-SR04 Ultrasonic Sensors** (Front, Left, Right) | Measure distance to detect obstacles |
| **3-Channel IR Line Follower Module** (Left, Center, Right) | Detects the black line on the white surface |
| **8.4V 2S Li-ion Battery (1500mAh)** | Powers the motors and electronics |
| **Rocker Switch (with LED)** | On/off power switch |
| **4WD Acrylic Chassis (26×15 cm)** | The physical body, built as a double-deck: motors on the bottom, battery + motor driver sandwiched in the middle, Arduino on top |

**Wiring — all grounds (Arduino, sensors, motor driver, battery) are connected together (common ground).**

## 3. Pin Map

| Component | Arduino Pin |
|---|---|
| Right Motors (IN1, IN2) | D8, D9 |
| Left Motors (IN3, IN4) | D10, D11 |
| Front Ultrasonic (Trig, Echo) | D2, D3 |
| Right Ultrasonic (Trig, Echo) | D4, D5 |
| Left Ultrasonic (Trig, Echo) | D6, D7 |
| Line Follower (Right, Center, Left) | A0, A1, A2 |

## 4. How the Code Works (Step by Step)

The whole robot behavior is controlled by a **state machine** — meaning the robot is always in one specific "state" (mode), and it moves between states based on what its sensors tell it. There are 7 states:

```
LINE_FOLLOW → AVOID_EVALUATE → AVOID_REVERSE (if too close)
                              → AVOID_TURN_OUT → AVOID_FORWARD → AVOID_TURN_IN → AVOID_RETURN_LINE → back to LINE_FOLLOW
```

**`LINE_FOLLOW`** (normal driving mode)
The Arduino constantly pings the front ultrasonic sensor. If nothing is in the way, it calls `runLineFollow()`, which checks the 3 IR sensors:
- Center sensor sees the line → drive straight forward.
- Left sensor sees the line → pivot left (the line drifted to the left, so the robot corrects).
- Right sensor sees the line → pivot right.
- None of them see the line → the robot uses its memory (`lastSeen`) of the last direction the line was on, and keeps turning that way until it finds the line again, instead of stopping confused.

**`AVOID_EVALUATE`** (decision point)
Once an obstacle is detected under 20 cm, the robot stops and checks:
- If it's dangerously close (under 8 cm) → go to `AVOID_REVERSE` to back up first.
- Otherwise → check which side (left or right) has more clearance using the two side ultrasonic sensors, and turn toward whichever side is clearer. This choice is saved in `avoidedToRight` so the robot remembers which way it went.

**`AVOID_REVERSE`**
Backs up for 350 ms to create space, then returns to `AVOID_EVALUATE` to try again.

**`AVOID_TURN_OUT`**
Turns away from the track for 350 ms to clear the obstacle, then moves to `AVOID_FORWARD`.

**`AVOID_FORWARD`**
Drives straight for 650 ms to get fully past the obstacle, then moves to `AVOID_TURN_IN`.

**`AVOID_TURN_IN`**
Turns back toward the track (opposite direction of the turn-out) for 400 ms, then moves to `AVOID_RETURN_LINE`.

**`AVOID_RETURN_LINE`**
Drives forward while scanning all 3 IR sensors. The instant any of them detects the black line, the robot resets its memory and jumps back into `LINE_FOLLOW`. If it can't find the line for 2 full seconds, it stops itself as a safety fallback instead of driving off blindly.

### Key functions in the code

| Function | What it does |
|---|---|
| `motorForward()` / `motorBackward()` / `motorStop()` | Basic driving |
| `pivotLeft()` / `pivotRight()` | Spins the robot in place (one side forward, other side backward) |
| `ping(trig, echo)` | Sends an ultrasonic pulse and returns the distance in cm |
| `seesLine(pin)` | Returns true if that IR sensor currently sees the black line |
| `enterState(s)` | Switches the robot to a new state and resets the state timer |
| `runLineFollow()` | The line-following logic described above |
| `runAvoidance()` | Runs whichever avoidance state the robot is currently in |

### Tunable settings (top of the code)

If your track or obstacles are different, you don't need to touch the logic — just change these numbers:

| Constant | Meaning | Default |
|---|---|---|
| `OBSTACLE_LIMIT` | Distance (cm) that triggers avoidance | 20 |
| `CRITICAL_LIMIT` | Distance (cm) that triggers an emergency reverse | 8 |
| `TURN_OUT_MS` | How long the robot turns away from the obstacle | 350 ms |
| `PASS_OBSTACLE_MS` | How long it drives straight to clear the obstacle | 650 ms |
| `TURN_IN_MS` | How long it turns back toward the track | 400 ms |

## 5. How to Run It

1. Wire everything according to the pin map in section 3.
2. Open `src/robot_control.ino` in the Arduino IDE.
3. Select **Arduino Uno** as the board, connect via USB, and hit Upload.
4. Place the robot on a white surface with a black-tape track, power it on, and it starts driving automatically.
5. If it doesn't follow the line correctly, check the IR module's height above the ground — ~1.5 cm gave the best results in testing.

## 6. Test Results

| Metric | Result |
|---|---|
| Line following accuracy (including sharp curves) | 99% |
| Obstacle bypass success rate | 100% |
| Minimum safe distance handled | 8 cm |

## 7. Repo Structure

```
├── README.md
└── src/
    └── robot_control.ino
```

## 8. Team

[Maryam EL Sayed Ellabn](https://github.com/MaryamEllaban) · Nada Ahmed Foaad · Mohamed Amir Ghannam · Farouk Mohamed Farouk · Ahmed Mohamed Khairy · Mina Basem Samir

**Supervised by:** Dr. Tamer Mansour, Eng. Wagdy
