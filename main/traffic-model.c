#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdio.h>

// Define the state structure
struct State
{
  unsigned long Out;
  unsigned long Time;
  unsigned long Next[4];
};
typedef const struct State STyp;

// Define states
#define goN 0
#define waitN 1
#define goE 2
#define waitE 3

// Define the state machine
STyp FSM[4] = {
    {0x21, 3000, {goN, waitN, goN, waitN}}, // North Green, East Red
    {0x22, 500, {goE, goE, goE, goE}},      // North Yellow, East Red
    {0x0C, 3000, {goE, goE, waitE, waitE}}, // North Red, East Green
    {0x14, 500, {goN, goN, goN, goN}}       // North Red, East Yellow
};

// Global variables
unsigned long S; // Current state
unsigned long Input = 0;

// Logger tag
static const char *TAG = "traffic_light";

// Function to generate random random traffic data
void generateRandomTraffic()
{
  // generate random number between 0 and 3
  Input = esp_random() % 4;
}

// Function to print the state of traffic lights
void printTrafficLightState(unsigned long lightState)
{
  ESP_LOGI(TAG, "-------------------------------");
  ESP_LOGI(TAG, "Current lights value: 0x%02lX", lightState);

  ESP_LOGI(TAG, "East Green Light: %s", (lightState & 0x01) ? "On" : "Off");
  ESP_LOGI(TAG, "East Yellow Light: %s", (lightState & 0x02) ? "On" : "Off");
  ESP_LOGI(TAG, "East Red Light: %s", (lightState & 0x04) ? "On" : "Off");
  ESP_LOGI(TAG, "North Green Light: %s", (lightState & 0x08) ? "On" : "Off");
  ESP_LOGI(TAG, "North Yellow Light: %s", (lightState & 0x10) ? "On" : "Off");
  ESP_LOGI(TAG, "North Red Light: %s", (lightState & 0x20) ? "On" : "Off");
}

// Traffic light task
void trafficLightTask(void *pvParameter)
{
  S = goN; // Initial state
  unsigned long oldOutput = -1;

  while (1)
  {
    // Simulate input (This can will be replaced with SUMO+logic algorithm later)
    generateRandomTraffic(); // generate random traffic data

    // Transition to the next state based on input
    S = FSM[S].Next[Input];

    // If the state has changed, print the new state and delay
    if (S != oldOutput)
    {
      printTrafficLightState(FSM[S].Out);
      oldOutput = S;
      vTaskDelay(FSM[S].Time / portTICK_PERIOD_MS); // Delay in FreeRTOS
    }

    // Reset the watchdog timer
    vTaskDelay(10 / portTICK_PERIOD_MS); // Ensure the task yields regularly

    // Check remaining stack size
    UBaseType_t remainingStack = uxTaskGetStackHighWaterMark(NULL);
    if (remainingStack < 100)
    {
      ESP_LOGW(TAG, "Stack is running low: %d words remaining", remainingStack);
    }
  }
}

// Main application
void app_main()
{
  // Create the traffic light control task
  xTaskCreate(&trafficLightTask, "trafficLightTask", 2048, NULL, 5, NULL);
}
