#ifndef TRAFFIC_CONTROL_H_ /* Include guard */
#define TRAFFIC_CONTROL_H_

// Define the state structure
struct State {
  unsigned long Out;
  unsigned long Time;
  unsigned long Next[4];
};
typedef const struct State STyp;

// State Identifiers
#define goN 0   // North Green
#define waitN 1 // North Yellow
#define goE 2   // East Green
#define waitE 3 // East Yellow
#define goS 4   // South Green
#define waitS 5 // South Yellow
#define goW 6   // West Green
#define waitW 7 // West Yellow

// UART Configuration
#define UART_PORT_NUM UART_NUM_1
#define UART_BAUD_RATE 115200
#define TXD_PIN (GPIO_NUM_17) // Transmit pin
#define RXD_PIN (GPIO_NUM_16) // Receive pin

// Traffic Light Outputs for Y-Junction
const unsigned long Y_NS_Green_E_Red = 0x21;  // North-South Green, East Red
const unsigned long Y_NS_Yellow_E_Red = 0x22; // North-South Yellow, East Red
const unsigned long Y_NS_Red_E_Green = 0x0C;  // North-South Red, East Green
const unsigned long Y_NS_Red_E_Yellow = 0x14; // North-South Red, East Yellow
const unsigned long Y_N_Green_SE_Red = 0x28;  // North Green, South-East Red
const unsigned long Y_N_Yellow_SE_Red = 0x18; // North Yellow, South-East Red
const unsigned long Y_N_Red_SE_Green = 0x03;  // North Red, South-East Green
const unsigned long Y_N_Red_SE_Yellow = 0x04; // North Red, South-East Yellow

// Traffic Light Outputs for X-Junction
const unsigned long X_NS_Green_EW_Red =
    0x21; // North-South Green, East-West Red
const unsigned long X_NS_Yellow_EW_Red =
    0x22; // North-South Yellow, East-West Red
const unsigned long X_NS_Red_EW_Green =
    0x0C; // North-South Red, East-West Green
const unsigned long X_NS_Red_EW_Yellow =
    0x14; // North-South Red, East-West Yellow

// Traffic Light Outputs for H-Junction
const unsigned long H_NS_Green_EW_Red =
    0x21; // North-South Green, East-West Red
const unsigned long H_NS_Yellow_EW_Red =
    0x22; // North-South Yellow, East-West Red
const unsigned long H_NS_Red_EW_Green =
    0x0C; // North-South Red, East-West Green
const unsigned long H_NS_Red_EW_Yellow =
    0x14; // North-South Red, East-West Yellow
const unsigned long H_NS_Green_TurnArrow =
    0x29; // North-South Green with Turn Arrow

// Variables for current state
unsigned long state_Y; // Current state for Y-Junction
unsigned long state_X; // Current state for X-Junction
unsigned long state_H; // Current state for H-Junction

// Example input variable to simulate traffic or sensor data
unsigned long input_Y = 0; // Input for Y-Junction
unsigned long input_X = 0; // Input for X-Junction
unsigned long input_H = 0; // Input for H-Junction

// Global variables
unsigned long S; // Current state
unsigned long Input = 0;
unsigned long NUM_JUNCTIONS = 3;

// State Machine for Y-Junction
STyp FSM_Y[8] = {
    {Y_NS_Green_E_Red,
     3000,
     {waitN, waitN, waitN, waitN, waitN, waitN, waitN,
      waitN}}, // North-South Green, East Red
    {Y_NS_Yellow_E_Red,
     500,
     {goE, goE, goE, goE, goE, goE, goE, goE}}, // North-South Yellow, East Red
    {Y_NS_Red_E_Green,
     3000,
     {waitE, waitE, waitE, waitE, waitE, waitE, waitE,
      waitE}}, // North-South Red, East Green
    {Y_NS_Red_E_Yellow,
     500,
     {goN, goN, goN, goN, goN, goN, goN, goN}}, // North-South Red, East Yellow
    {Y_N_Green_SE_Red,
     3000,
     {waitN, waitN, waitN, waitN, waitN, waitN, waitN,
      waitN}}, // North Green, South-East Red
    {Y_N_Yellow_SE_Red,
     500,
     {goS, goS, goS, goS, goS, goS, goS, goS}}, // North Yellow, South-East Red
    {Y_N_Red_SE_Green,
     3000,
     {waitS, waitS, waitS, waitS, waitS, waitS, waitS,
      waitS}}, // North Red, South-East Green
    {Y_N_Red_SE_Yellow,
     500,
     {goN, goN, goN, goN, goN, goN, goN, goN}} // North Red, South-East Yellow
};

// State Machine for X-Junction
STyp FSM_X[4] = {
    {X_NS_Green_EW_Red,
     3000,
     {waitN, waitN, waitN, waitN}}, // North-South Green, East-West Red
    {X_NS_Yellow_EW_Red,
     500,
     {goE, goE, goE, goE}}, // North-South Yellow, East-West Red
    {X_NS_Red_EW_Green,
     3000,
     {waitE, waitE, waitE, waitE}}, // North-South Red, East-West Green
    {X_NS_Red_EW_Yellow,
     500,
     {goN, goN, goN, goN}} // North-South Red, East-West Yellow
};

// State Machine for H-Junction
STyp FSM_H[5] = {
    {H_NS_Green_EW_Red,
     4000,
     {waitN, waitN, waitN, waitN, goN}}, // North-South Green, East-West Red
    {H_NS_Yellow_EW_Red,
     1000,
     {goE, goE, goE, goE, goE}}, // North-South Yellow, East-West Red
    {H_NS_Red_EW_Green,
     3000,
     {waitE, waitE, waitE, waitE, waitE}}, // North-South Red, East-West Green
    {H_NS_Red_EW_Yellow,
     1000,
     {goN, goN, goN, goN, goN}}, // North-South Red, East-West Yellow
    {H_NS_Green_TurnArrow,
     2000,
     {waitN, waitN, waitN, waitN, waitN}} // North-South Green with Turn Arrow
};

// Logger tag
static const char *TAG = "traffic_light";

void generateRandomTraffic();
void printTrafficLightState(const char *junctionType, unsigned long lightState);
void handleJunctionState(const char *junctionType, unsigned long *currentState,
                         STyp *fsm, unsigned long *oldOutput,
                         unsigned long input);
void trafficLightTask(void *pvParameter);
void uart_init(void);
void uart_send_data(const char *data);

void app_main();

#endif // TRAFFIC_CONTROL_H_
