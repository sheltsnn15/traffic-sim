#include "traffic-control.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

static QueueHandle_t uart_queue;
static unsigned long Input;

// Function to generate random traffic data
void generateRandomTraffic() {
  // Generate random number between 0 and 3 (for 4 possible traffic patterns)
  Input = esp_random() % 4;
}

// Function to print the state of traffic lights based on the junction type
void printTrafficLightState(const char *junctionType,
                            unsigned long lightState) {
  ESP_LOGI(TAG, "-------------------------------");
  ESP_LOGI(TAG, "Junction Type: %s", junctionType);
  ESP_LOGI(TAG, "Current lights value: 0x%02lX", lightState);

  // Different junctions will have different light setups, so handle each case
  if (strcmp(junctionType, "X-Junction") == 0) {
    // For X-Junction: Focus on North-South and East-West lights only
    ESP_LOGI(TAG, "North Green Light: %s", (lightState & 0x08) ? "On" : "Off");
    ESP_LOGI(TAG, "North Yellow Light: %s", (lightState & 0x10) ? "On" : "Off");
    ESP_LOGI(TAG, "North Red Light: %s", (lightState & 0x20) ? "On" : "Off");

    ESP_LOGI(TAG, "East Green Light: %s", (lightState & 0x01) ? "On" : "Off");
    ESP_LOGI(TAG, "East Yellow Light: %s", (lightState & 0x02) ? "On" : "Off");
    ESP_LOGI(TAG, "East Red Light: %s", (lightState & 0x04) ? "On" : "Off");

  } else if (strcmp(junctionType, "Y-Junction") == 0) {
    // For Y-Junction: Focus on North-South and South-East traffic flows
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
    // For H-Junction: North-South lights and Turn Arrow
    ESP_LOGI(TAG, "North Green Light: %s", (lightState & 0x08) ? "On" : "Off");
    ESP_LOGI(TAG, "North Yellow Light: %s", (lightState & 0x10) ? "On" : "Off");
    ESP_LOGI(TAG, "North Red Light: %s", (lightState & 0x20) ? "On" : "Off");
    ESP_LOGI(TAG, "North Turn Arrow: %s", (lightState & 0x01) ? "On" : "Off");

    ESP_LOGI(TAG, "East Green Light: %s", (lightState & 0x01) ? "On" : "Off");
    ESP_LOGI(TAG, "East Yellow Light: %s", (lightState & 0x02) ? "On" : "Off");
    ESP_LOGI(TAG, "East Red Light: %s", (lightState & 0x04) ? "On" : "Off");
  } else {
    // If the junction type is not recognized, log a warning
    ESP_LOGW(TAG, "Unknown junction type: %s", junctionType);
  }
}

// Helper function to handle a junction's state transitions
void handleJunctionState(const char *junctionType, unsigned long *currentState,
                         STyp *fsm, unsigned long *oldOutput,
                         unsigned long input) {
  // Update the state of the traffic light based on the input and FSM
  *currentState = fsm[*currentState].Next[input];

  // If the output state has changed, print the new traffic light state
  if (*currentState != *oldOutput) {
    printTrafficLightState(junctionType, fsm[*currentState].Out);
    *oldOutput = *currentState;
    // Introduce a delay between state transitions to simulate real-world timing
    vTaskDelay(fsm[*currentState].Time / portTICK_PERIOD_MS); // FreeRTOS delay
  }
}

// UART initialization function
void uart_init(void) {
  // Set UART configuration parameters
  uart_config_t uart_config = {
      .baud_rate = UART_BAUD_RATE,          // Set the baud rate
      .data_bits = UART_DATA_8_BITS,        // 8 data bits
      .parity = UART_PARITY_DISABLE,        // No parity
      .stop_bits = UART_STOP_BITS_1,        // 1 stop bit
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE // No hardware flow control
  };
  // Configure UART with the settings above
  uart_param_config(UART_PORT_NUM, &uart_config);

  // Set the pins for UART communication (TX and RX)
  uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE,
               UART_PIN_NO_CHANGE);

  // Enable an event-driven queue for UART (manages incoming data)
  if (uart_driver_install(UART_PORT_NUM, 1024, 0, 10, &uart_queue, 0) !=
      ESP_OK) {
    ESP_LOGE(TAG, "Failed to install UART driver");
  }
}

// Function to send data over UART
void uart_send_data(const char *data) {
  // Send the data over the UART port
  int bytes_written = uart_write_bytes(UART_PORT_NUM, data, strlen(data));
  if (bytes_written < 0) {
    ESP_LOGE(TAG, "Failed to send data over UART");
  }
}

// Traffic light control task (FreeRTOS task)
void trafficLightTask(void *pvParameter) {
  // Initial states for each junction type
  state_Y = goN;
  state_X = goN;
  state_H = goN;

  unsigned long oldOutput_Y = -1;
  unsigned long oldOutput_X = -1;
  unsigned long oldOutput_H = -1;

  while (1) {
    // Simulate random input for traffic lights
    generateRandomTraffic();

    // Handle state changes for each junction
    handleJunctionState("Y-Junction", &state_Y, FSM_Y, &oldOutput_Y, Input);

    // Send the current state of Y-Junction over UART
    unsigned long trafficState_Y = FSM_Y[state_Y].Out;
    char buffer[32];
    int len = snprintf(buffer, sizeof(buffer), "Y-Junction State: 0x%lX\n",
                       trafficState_Y);
    uart_send_data(buffer);

    // Clear the UART buffer periodically to avoid overflow
    if (!uart_wait_tx_done(UART_PORT_NUM, 100 / portTICK_PERIOD_MS)) {
      uart_flush(UART_PORT_NUM); // Clear UART buffer to prevent overflow
    }

    // Reset the watchdog timer and yield to other tasks
    vTaskDelay(10 / portTICK_PERIOD_MS);

    // Check remaining stack size for potential overflow issues
    UBaseType_t remainingStack = uxTaskGetStackHighWaterMark(NULL);
    if (remainingStack < 100) {
      ESP_LOGW(TAG, "Stack is running low: %d words remaining", remainingStack);
    }
  }
}

// Main application entry point
void app_main() {
  // Initialize the UART
  uart_init();

  // Create the traffic light control task (FreeRTOS task)
  if (xTaskCreate(&trafficLightTask, "trafficLightTask", 2048, NULL, 5, NULL) !=
      pdPASS) {
    ESP_LOGE(TAG, "Failed to create traffic light task");
  }
}
