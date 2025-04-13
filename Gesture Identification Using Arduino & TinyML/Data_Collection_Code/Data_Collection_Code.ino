#include <Arduino_LSM9DS1.h>

// ----- USER SETTINGS -----

// Label for the shots in this run.
//   Use 'F' for Forehand, 'B' for Backhand, etc.
char label = 'b';

// How many shots you want to capture in one run
const int TOTAL_SHOTS = 101;

// How long each shot is recorded (in milliseconds)
const unsigned long SHOT_DURATION_MS = 1500; // 2 seconds

// How long to wait before each shot starts (in milliseconds)
const unsigned long PREPARE_WAIT_MS = 1000; // 2 seconds

// Interval for reading sensors within that shot window (ms)
const unsigned long SAMPLE_INTERVAL_MS = 50; // ~50 samples/sec

// How long to wait before the *entire* shot process begins (ms)
const unsigned long INITIAL_PREPARE_MS = 5000; // 20 seconds

// -------------------------

int shotCount = 0;
unsigned long shotStartTime = 0;
unsigned long previousSampleTime = 0;
bool shotInProgress = false;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for the Serial port to be ready
  }

  // Initialize IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  // Print a CSV header (optional).
  // We have 11 columns in total:
  // label, shot, ax, ay, az, gx, gy, gz, mx, my, mz
  Serial.println("label,shot,ax,ay,az,gx,gy,gz,mx,my,mz");

  // Initial info
  Serial.print("Capturing label '");
  Serial.print(label);
  Serial.print("' for ");
  Serial.print(TOTAL_SHOTS);
  Serial.println(" shots.");

  // Wait 20 seconds before collecting any shots
  Serial.print("\n[INITIAL PREPARATION] Waiting ");
  Serial.print(INITIAL_PREPARE_MS / 1000);
  Serial.println(" seconds. Get ready...");
  delay(INITIAL_PREPARE_MS);
  Serial.println("Starting shots now...");
}

void loop() {
  // If we've completed all shots, do nothing else.
  if (shotCount >= TOTAL_SHOTS) {
    // Optionally print a done message and halt.
    // Serial.println("All shots captured.");
    return;
  }

  // If we're NOT currently recording a shot,
  // let's initiate the next shot after a small wait.
  if (!shotInProgress) {
    shotInProgress = true;
    shotCount++;

    // Give user time to prepare for this shot
    //Serial.print("\nGet ready for shot #");
    //Serial.println(shotCount);
    delay(PREPARE_WAIT_MS);

    // Start timing for this shot
    shotStartTime = millis();
    previousSampleTime = shotStartTime;

    //Serial.print("=== STARTING shot #");
    //Serial.print(shotCount);
    //Serial.println(" ===");
  }

  // Now we are "in progress" for this shot,
  // record for SHOT_DURATION_MS.
  unsigned long now = millis();
  if (now - shotStartTime <= SHOT_DURATION_MS) {

    // We are within the 2-second window
    // Check if it's time for the next sample
    if (now - previousSampleTime >= SAMPLE_INTERVAL_MS) {
      previousSampleTime = now; 

      float ax, ay, az;
      float gx, gy, gz;
      float mx, my, mz;

      bool gotAccel = false;
      bool gotGyro = false;
      bool gotMag = false;

      // Check each sensor in turn
      if (IMU.accelerationAvailable()) {
        IMU.readAcceleration(ax, ay, az);
        gotAccel = true;
      }

      if (IMU.gyroscopeAvailable()) {
        IMU.readGyroscope(gx, gy, gz);
        gotGyro = true;
      }

      if (IMU.magneticFieldAvailable()) {
        IMU.readMagneticField(mx, my, mz);
        gotMag = true;
      }

      // If any sensor data was read, print line
      if (gotAccel || gotGyro || gotMag) {
        // If a sensor wasn't read, it will remain 0.0 
        // (or old value if you want to keep it)
        
        // Print CSV line: 
        // label, shot#, ax, ay, az, gx, gy, gz, mx, my, mz
        Serial.print(label);
        Serial.print(",");
        Serial.print(shotCount);
        Serial.print(",");
        Serial.print(ax);
        Serial.print(",");
        Serial.print(ay);
        Serial.print(",");
        Serial.print(az);
        Serial.print(",");
        Serial.print(gx);
        Serial.print(",");
        Serial.print(gy);
        Serial.print(",");
        Serial.print(gz);
        Serial.print(",");
        Serial.print(mx);
        Serial.print(",");
        Serial.print(my);
        Serial.print(",");
        Serial.println(mz);
      }
    }

  } else {
    // The 2-second window ended; finish this shot
    shotInProgress = false;
    //Serial.print("=== END of shot #");
    //Serial.print(shotCount);
    //Serial.println(" ===\n");
  }
}


