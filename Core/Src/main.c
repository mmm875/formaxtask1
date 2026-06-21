/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : ???:????RGB????
  ******************************************************************************
  * ????:
  *   PA2 -> ?? (PWM ??)
  *   PA3 -> ?? (??IO)
  *   PA4 -> ?? (???)
  *   PA5 -> ???
  *   PB0 -> ??1
  *   PB1 -> ??2
  ******************************************************************************
  */
/* USER CODE END Header */
#include "main.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    GREEN_ON = 0,
    RED_ON,
    GREEN_BREATH
} RGB_Mode_t;

typedef enum {
    BEEP_ON = 0,
    BEEP_OFF,
    BEEP_BLINK
} Beep_Mode_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PWM_MAX     1000
#define PWM_STEP    15
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static RGB_Mode_t rgbMode = GREEN_ON;
static Beep_Mode_t beepMode = BEEP_ON;

// ???
static uint16_t pwmValue = 0;
static uint8_t pwmDir = 1;
static uint32_t lastBreathTime = 0;

// ?????
static uint32_t lastBeepTime = 0;
static uint8_t beepState = 0;

// ???????
static uint8_t key1State = 0;
static uint8_t key2State = 0;
static uint32_t key1PressTick = 0;
static uint32_t key2PressTick = 0;
static uint8_t key2Triggered = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void SetRGBMode(RGB_Mode_t mode);
void SetBeepMode(Beep_Mode_t mode);
void Key_Scan(void);
void Breath_Process(void);
void Beep_Process(void);
void Enable_Key_PullUp(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// ????????????
void Enable_Key_PullUp(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void SetRGBMode(RGB_Mode_t mode)
{
    rgbMode = mode;
    
    if(key2Triggered) return;
    
    switch(mode) {
        case GREEN_ON:
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, PWM_MAX);
            break;
        case RED_ON:
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 0);
            break;
        case GREEN_BREATH:
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
            pwmValue = 0;
            pwmDir = 1;
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 0);
            break;
    }
}

void SetBeepMode(Beep_Mode_t mode)
{
    beepMode = mode;
    
    switch(mode) {
        case BEEP_ON:
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
            break;
        case BEEP_OFF:
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
            break;
        case BEEP_BLINK:
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
            beepState = 0;
            lastBeepTime = HAL_GetTick();
            break;
    }
}

void Key_Scan(void)
{
    uint8_t key1Now = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
    uint8_t key2Now = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1);
    uint32_t now = HAL_GetTick();
    
    // ??1:????
    switch(key1State) {
        case 0:
            if(key1Now == 0) {
                key1State = 1;
                key1PressTick = now;
            }
            break;
        case 1:
            if(key1Now == 0) {
                if(now - key1PressTick >= 30) {
                    switch(rgbMode) {
                        case GREEN_ON: SetRGBMode(RED_ON); break;
                        case RED_ON: SetRGBMode(GREEN_BREATH); break;
                        case GREEN_BREATH: SetRGBMode(GREEN_ON); break;
                    }
                    switch(beepMode) {
                        case BEEP_ON: SetBeepMode(BEEP_OFF); break;
                        case BEEP_OFF: SetBeepMode(BEEP_BLINK); break;
                        case BEEP_BLINK: SetBeepMode(BEEP_ON); break;
                    }
                    key1State = 2;
                }
            } else {
                key1State = 0;
            }
            break;
        case 2:
            if(key1Now == 1) {
                key1State = 0;
            }
            break;
    }
    
    // ??2:??????
    switch(key2State) {
        case 0:
            if(key2Now == 0) {
                key2State = 1;
                key2PressTick = now;
            }
            break;
        case 1:
            if(key2Now == 0) {
                if(now - key2PressTick >= 1000) {
                    key2Triggered = 1;
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
                    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, PWM_MAX);
                    key2State = 2;
                }
            } else {
                key2State = 0;
            }
            break;
        case 2:
            if(key2Now == 1) {
                key2Triggered = 0;
                SetRGBMode(rgbMode);
                key2State = 0;
            }
            break;
    }
}

void Breath_Process(void)
{
    if(rgbMode != GREEN_BREATH) return;
    if(key2Triggered) return;
    
    uint32_t now = HAL_GetTick();
    if(now - lastBreathTime >= 20) {
        lastBreathTime = now;
        
        if(pwmDir) {
            pwmValue += PWM_STEP;
            if(pwmValue >= PWM_MAX) {
                pwmValue = PWM_MAX;
                pwmDir = 0;
            }
        } else {
            if(pwmValue >= PWM_STEP) {
                pwmValue -= PWM_STEP;
            } else {
                pwmValue = 0;
                pwmDir = 1;
            }
        }
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, pwmValue);
    }
}

void Beep_Process(void)
{
    if(beepMode != BEEP_BLINK) return;
    if(key2Triggered) return;
    
    uint32_t now = HAL_GetTick();
    if(now - lastBeepTime >= 500) {
        lastBeepTime = now;
        beepState = !beepState;
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, beepState ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

/* USER CODE END 0 */

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();

    /* USER CODE BEGIN 2 */
    // ??????????
    Enable_Key_PullUp();
    
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    SetRGBMode(GREEN_ON);
    SetBeepMode(BEEP_ON);
    /* USER CODE END 2 */

    while (1)
    {
        Key_Scan();
        Breath_Process();
        Beep_Process();
        HAL_Delay(10);
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
    if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) { Error_Handler(); }
}

void Error_Handler(void)
{
    __disable_irq();
    while(1) {}
}