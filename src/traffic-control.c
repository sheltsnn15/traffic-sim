#include "traffic-control.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

// Function to generate random random traffic data
void generateRandomTraffic() {
  // generate random number between 0 and 3
  Input = esp_random() % 4;
}

// Function to print the state of traffic lights based on the junction type
void printTrafficLightState(const char *junctionType,
                            unsigned long lightState) {
  ESP_LOGI(TAG, "-------------------------------");
  ESP_LOGI(TAG, "Junction Type: %s", junctionType);
  ESP_LOGI(TAG, "Current lights value: 0x%02lX", lightState);

  if (strcmp(junctionType, "X-Junction") == 0) {
    // X-Junction: Focus on North-South and East-West lights only
    ESP_LOGI(TAG, "North Green Light: %s", (lightState & 0x08) ? "On" : "Off");
    ESP_LOGI(TAG, "North Yellow Light: %s", (lightState & 0x10) ? "On" : "Off");
    ESP_LOGI(TAG, "North Red Light: %s", (lightState & 0x20) ? "On" : "Off");

    ESP_LOGI(TAG, "East Green Light: %s", (lightState & 0x01) ? "On" : "Off");
    ESP_LOGI(TAG, "East Yellow Light: %s", (lightState & 0x02) ? "On" : "Off");
    ESP_LOGI(TAG, "East Red Light: %s", (lightState & 0x04) ? "On" : "Off");

  } else if (strcmp(junctionType, "Y-Junction") == 0) {
    // Y-Junction: Handle North-South and South-East traffic flows
    ESP_LOGI(TAG, "North Green Light: %s", (lightState & 0x08) ? "On" : "Off");
    ESP_LOGI(TAG, "North Yellow Light: %s", (lightState & 0x10) ? "On" : "Off");
    ESP_LOGI(TAG, "North Red Light: %s", (lightState & 0x20) ? "On" : "Off");

    ESP_LOGI(TAG, "South-East Green Light: %s",
             (lightState & 0x03) ? "On" : "Off");
    ESP_LOGI(TAG, "South-East Yellow Light: %s",
             (lightState & 0x04) ? "On" : "Off");
    ESP_LOGI(TAG, "South-East Red Light: %s",
             (lightState & 0x01) ? "On" : "Off");

  } else if (strcmp(junctionType, "H-Junction") == 0) {
    // H-Junction: Include North-South lights and Turn Arrow
    ESP_LOGI(TAG, "North Green Light: %s", (lightState & 0x08) ? "On" : "Off");
    ESP_LOGI(TAG, "North Yellow Light: %s", (lightState & 0x10) ? "On" : "Off");
    ESP_LOGI(TAG, "North Red Light: %s", (lightState & 0x20) ? "On" : "Off");
    ESP_LOGI(TAG, "North Turn Arrow: %s", (lightState & 0x01) ? "On" : "Off");

    ESP_LOGI(TAG, "East Green Light: %s", (lightState & 0x01) ? "On" : "Off");
    ESP_LOGI(TAG, "East Yellow Light: %s", (lightState & 0x02) ? "On" : "Off");
    ESP_LOGI(TAG, "East Red Light: %s", (lightState & 0x04) ? "On" : "Off");
  } else {
    ESP_LOGW(TAG, "Unknown junction type: %s", junctionType);
  }
}

// Helper function to handle a junction's state transitions
void handleJunctionState(const char *junctionType, unsigned long *currentState,
                         STyp *fsm, unsigned long *oldOutput,
                         unsigned long input) {
  *currentState = fsm[*currentState].Next[input];
  if (*currentState != *oldOutput) {
    printTrafficLightState(junctionType, fsm[*currentState].Out);
    *oldOutput = *currentState;
    vTaskDelay(fsm[*currentState].Time /
               portTICK_PERIOD_MS); // Delay in FreeRTOS
  }
}

void uart_init(void) {
  uart_config_t uart_config = {.baud_rate = UART_BAUD_RATE,
                               .data_bits = UART_DATA_8_BITS,
                               .parity = UART_PARITY_DISABLE,
                               .stop_bits = UART_STOP_BITS_1,
                               .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
  uart_param_config(UART_PORT_NUM, &uart_config);
  uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE,
               UART_PIN_NO_CHANGE);
  uart_driver_install(UART_PORT_NUM, 1024, 0, 0, NULL, 0);
}

void uart_send_data(const char *data) {
  uart_write_bytes(UART_PORT_NUM, data, strlen(data));
}

// Traffic light task for multiple junctions
void trafficLightTask(void *pvParameter) {
  // Initial states for each junction type
  state_Y = goN;
  state_X = goN;
  state_H = goN;

  unsigned long oldOutput_Y = -1;
  unsigned long oldOutput_X = -1;
  unsigned long oldOutput_H = -1;

  while (1) {
    // Simulate input (This will be replaced with SUMO+logic algorithm later)
    generateRandomTraffic(); // Generate random traffic data

    // Handle each junction
    handleJunctionState("Y-Junction", &state_Y, FSM_Y, &oldOutput_Y, input_Y);
    /*handleJunctionState("X-Junction", &state_X, FSM_X, &oldOutput_X,
     * input_X);*/
    /*handleJunctionState("H-Junction", &state_H, FSM_H, &oldOutput_H,
     * input_H);*/

    // Send the state of Y-Junction over UART
    unsigned long trafficState_Y = FSM_Y[state_Y].Out;
    char buffer[32];
    int len = snprintf(buffer, sizeof(buffer), "Y-Junction State: 0x%lX\n",
                       trafficState_Y);
    uart_send_data(buffer);

    // Send the state of X-Junction over UART
    /*unsigned long trafficState_X = FSM_X[state_X].Out;*/
    /*len = snprintf(buffer, sizeof(buffer), "X-Junction State: 0x%lX\n",*/
    /*               trafficState_X);*/
    /*uart_send_data(buffer);*/

    // Send the state of H-Junction over UART
    /*unsigned long trafficState_H = FSM_H[state_H].Out;*/
    /*len = snprintf(buffer, sizeof(buffer), "H-Junction State: 0x%lX\n",*/
    /*               trafficState_H);*/
    /*uart_send_data(buffer);*/

    // Reset the watchdog timer and yield
    vTaskDelay(10 / portTICK_PERIOD_MS); // Ensure the task yields regularly

    // Check remaining stack size for potential stack overflow issues
    UBaseType_t remainingStack = uxTaskGetStackHighWaterMark(NULL);
    if (remainingStack < 100) {
      ESP_LOGW(TAG, "Stack is running low: %d words remaining", remainingStack);
    }
  }
}

// Main application
void app_main() {
  uart_init(); // Initialize UART
  // Create the traffic light control task
  xTaskCreate(&trafficLightTask, "trafficLightTask", 2048, NULL, 5, NULL);
}
