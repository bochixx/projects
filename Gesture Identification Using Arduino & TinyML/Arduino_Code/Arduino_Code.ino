#include <Arduino_LSM9DS1.h>
#include <Chirale_TensorFlowLite.h>
#include "model.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <ArduinoBLE.h>

constexpr int kTensorArenaSize = 8 * 1024;
uint8_t tensor_arena[kTensorArenaSize];

const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

const char* labels[] = {"forehand", "backhand", "idle"};

const int SAMPLE_INTERVAL_MS = 20;
const int INPUT_SIZE = 30;
float input_buffer[INPUT_SIZE * 9];
int sample_count = 0;

BLEService tennisService("180C");
BLEStringCharacteristic resultCharacteristic("2A57", BLERead | BLENotify, 32);
BLECharacteristic commandChar("2A58", BLEWrite, 20);

bool trainingMode = false;
bool awaitingInstruction = false;
String trainingInstruction = "";

void onCommandReceived(BLEDevice central, BLECharacteristic characteristic);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  Serial.println("Starting inference...");

  model = tflite::GetModel(gesture_model_quantized_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model provided and schema version are not equal!");
    while (1);
  }

  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("Failed to allocate TFLite tensors");
    while (1);
  }

  input = interpreter->input(0);
  output = interpreter->output(0);

  if (!BLE.begin()) {
    Serial.println("Failed to initialize BLE!");
    while (1);
  }

  BLE.setLocalName("TennisTrainer");
  BLE.setAdvertisedService(tennisService);
  tennisService.addCharacteristic(resultCharacteristic);
  tennisService.addCharacteristic(commandChar);
  BLE.addService(tennisService);
  commandChar.setEventHandler(BLEWritten, onCommandReceived);
  BLE.advertise();

  Serial.println("BLE ready and advertising");
}

void loop() {
  BLEDevice central = BLE.central();
  if (central) {
    Serial.print("Connected to: ");
    Serial.println(central.address());
    while (central.connected()) {
      if (!trainingMode && !awaitingInstruction) {
        predictShotAndNotify();
      }
    }
    Serial.println("Disconnected");
    trainingMode = false;
    awaitingInstruction = false;
  } else {
    BLE.advertise();
  }
}

void predictShotAndNotify() {
  static unsigned long last_sample_time = 0;
  unsigned long now = millis();
  if (now - last_sample_time >= SAMPLE_INTERVAL_MS) {
    last_sample_time = now;

    float ax, ay, az, gx, gy, gz, mx, my, mz;
    if (IMU.accelerationAvailable()) IMU.readAcceleration(ax, ay, az);
    if (IMU.gyroscopeAvailable()) IMU.readGyroscope(gx, gy, gz);
    if (IMU.magneticFieldAvailable()) IMU.readMagneticField(mx, my, mz);

    input_buffer[sample_count * 9 + 0] = ax;
    input_buffer[sample_count * 9 + 1] = ay;
    input_buffer[sample_count * 9 + 2] = az;
    input_buffer[sample_count * 9 + 3] = gx;
    input_buffer[sample_count * 9 + 4] = gy;
    input_buffer[sample_count * 9 + 5] = gz;
    input_buffer[sample_count * 9 + 6] = mx;
    input_buffer[sample_count * 9 + 7] = my;
    input_buffer[sample_count * 9 + 8] = mz;

    sample_count++;

    if (sample_count >= INPUT_SIZE) {
      sample_count = 0;

      for (int i = 0; i < INPUT_SIZE * 9; i++) {
        input->data.f[i] = input_buffer[i];
      }

      if (interpreter->Invoke() == kTfLiteOk) {
        float* output_data = output->data.f;
        int predicted_index = 0;
        float max_value = output_data[0];
        for (int i = 1; i < 3; i++) {
          if (output_data[i] > max_value) {
            max_value = output_data[i];
            predicted_index = i;
          }
        }

        String predictedShot = labels[predicted_index];
        Serial.print("Predicted: ");
        Serial.println(predictedShot);

        if (!trainingMode) {
          if (predictedShot == "forehand") {
            resultCharacteristic.writeValue("Forehand");
            Serial.println("Shot sent: Forehand");
            delay(1000);
          } else if (predictedShot == "backhand") {
            resultCharacteristic.writeValue("Backhand");
            Serial.println("Shot sent: Backhand");
            delay(1000);
          }
        }
      } else {
        Serial.println("Inference failed");
      }
    }
  }
}

void onCommandReceived(BLEDevice central, BLECharacteristic characteristic) {
  String raw = String((char*)commandChar.value());
  raw.trim();

  String received = "";
  if (raw.indexOf("TRAINING_MODE") != -1) {
    received = "TRAINING_MODE";
  } else if (raw.indexOf("EXIT_TRAINING") != -1) {
    received = "EXIT_TRAINING";
  } else if (raw.indexOf("Forehand_MODE") != -1) {
    received = "Forehand_MODE";
  } else if (raw.indexOf("Backhand_MODE") != -1) {
    received = "Backhand_MODE";
  }

  Serial.println("Received command: " + received);


  if (received == "TRAINING_MODE") {
    trainingMode = true;
    awaitingInstruction = true;
  } else if (received == "EXIT_TRAINING") {
    trainingMode = false;
    awaitingInstruction = false;
  } else if (received == "Forehand_MODE" || received == "Backhand_MODE") {
    if (trainingMode) {
      trainingInstruction = (received == "Forehand_MODE") ? "Forehand" : "Backhand";
      awaitingInstruction = false;
      performTrainingSequence();
      awaitingInstruction = true;
    }
  }
}

void performTrainingSequence() {
  int fore = 0, back = 0, idle = 0;
  unsigned long start = millis();

  while (millis() - start < 2000) {
    float ax, ay, az, gx, gy, gz, mx, my, mz;
    if (IMU.accelerationAvailable()) IMU.readAcceleration(ax, ay, az);
    if (IMU.gyroscopeAvailable()) IMU.readGyroscope(gx, gy, gz);
    if (IMU.magneticFieldAvailable()) IMU.readMagneticField(mx, my, mz);

    input_buffer[sample_count * 9 + 0] = ax;
    input_buffer[sample_count * 9 + 1] = ay;
    input_buffer[sample_count * 9 + 2] = az;
    input_buffer[sample_count * 9 + 3] = gx;
    input_buffer[sample_count * 9 + 4] = gy;
    input_buffer[sample_count * 9 + 5] = gz;
    input_buffer[sample_count * 9 + 6] = mx;
    input_buffer[sample_count * 9 + 7] = my;
    input_buffer[sample_count * 9 + 8] = mz;

    sample_count++;

    if (sample_count >= INPUT_SIZE) {
      sample_count = 0;

      for (int i = 0; i < INPUT_SIZE * 9; i++) {
        input->data.f[i] = input_buffer[i];
      }

      if (interpreter->Invoke() == kTfLiteOk) {
        float* output_data = output->data.f;
        int predicted_index = 0;
        float max_value = output_data[0];
        for (int i = 1; i < 3; i++) {
          if (output_data[i] > max_value) {
            max_value = output_data[i];
            predicted_index = i;
          }
        }

        String predictedShot = labels[predicted_index];
        if (predictedShot == "forehand") fore++;
        else if (predictedShot == "backhand") back++;
        else idle++;
      }
    }
  }

  String result;
  if (fore > 0) result = "Forehand";
  else if (back > 0) result = "Backhand";
  else result = "Idle";

  if (result == "Idle") {
    Serial.println(result);
    resultCharacteristic.writeValue("You did not move!");
    Serial.println("Shot sent: You did not move!");
  } else if (result == trainingInstruction) {
    Serial.println(result);
    resultCharacteristic.writeValue("Your shot is correct!");
    Serial.println("Shot sent: Your shot is correct!");
  } else {
    Serial.println(result);
    resultCharacteristic.writeValue("Your shot is not correct!");
    Serial.println("Shot sent: Your shot is not correct!");
  }
}
