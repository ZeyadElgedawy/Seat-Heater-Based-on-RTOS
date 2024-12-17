/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "semphr.h"
#include "event_groups.h"

#include "adc.h"
#include "gpio.h"
#include "uart0.h"
#include "GPTM.h"
#include "tm4c123gh6pm_registers.h"


#define NUMBER_OF_ITERATIONS_PER_ONE_MILI_SECOND 369

void Delay_MS(unsigned long long n)
{
    volatile unsigned long long count = 0;
    while(count++ < (NUMBER_OF_ITERATIONS_PER_ONE_MILI_SECOND * n) );
}

/*
 *  Heater is initially @ off state, each button press will
 *  move to the next state.
 *
 *  (MAKE_HEATER_OFF) --> (MAKE_HEATER_LOW_LEVEL) --> and so on...
 */


#define MAKE_HEATER_OFF              0
#define MAKE_HEATER_LOW_LEVEL        25
#define MAKE_HEATER_MEDIUM_LEVEL     30
#define MAKE_HEATER_HIGH_LEVEL       35

/*
 * If the temperature sensor reading exceeds the below range
 * the heater should be disabled and the red led should turns on
 */

#define MAXIMUM_TEMP_ACCEPTED        40
#define MINIMUM_TEMP_ACCEPTED        5

/*
 *  Heater intensity depends on the value which is coming from
 *  the temperature sensor, if current temp. less than desired temp by (Desired Temp. - Temp. Sensor value) :
 *  HEATER_HIGH_INTENSITY_LIMIT -------> high intensity needed --------> turn on cyan led
 *  HEATER_MEDIUM_INTENSITY_LOW_LIMIT to HEATER_MEDIUM_INTENSITY_HIGH_LIMIT -------> medium intensity needed --------> turn on blue led
 *  HEATER_LOW_INTENSITY_LOW_LIMIT to  HEATER_LOW_INTENSITY_HIGH_LIMIT-------> medium intensity needed --------> turn on green led
 */

#define HEATER_HIGH_INTENSITY_LIMIT             10

#define HEATER_MEDIUM_INTENSITY_LOW_LIMIT       5
#define HEATER_MEDIUM_INTENSITY_HIGH_LIMIT      10

#define HEATER_LOW_INTENSITY_LOW_LIMIT          2
#define HEATER_LOW_INTENSITY_HIGH_LIMIT         5

/*
 * Macros Mapped to the intensity of the heater
 */

#define LOW_INTENSITY           0
#define MEDIUM_INTENSITY        1
#define HIGH_INTENSITY          2
#define HEATER_DISABLED         3

/*
 * Masks for ButtonsEventGroup
 */

#define DRIVER_BUTTON_BIT_MASK                   (1UL << 0UL)
#define PASSENGER_BUTTON_BIT_MASK                (1UL << 1UL)


#define DRIVER_SENSOR_FAILURE_BIT_MASK           (1UL << 2UL)
#define PASSENGER_SENSOR_FAILURE_BIT_MASK        (1UL << 3UL)


/* The HW setup function */
static void prvSetupHardware( void );

/* Semaphores given From ADC as an indication that the conversion is done
 * to tell heater handling task it can starts its functionality
 */
xSemaphoreHandle xDriverHeaterSemaphore;
xSemaphoreHandle xPassengerHeaterSemaphore;

/* Task Handles */

TaskHandle_t xDesiredHeaterLevelHandle;
TaskHandle_t xHeaterIntensityDriverHandle;
TaskHandle_t xHeaterIntensityPassengerHandle;
TaskHandle_t xADC_GetSensorValueHandle;
TaskHandle_t xPrintTemperatureHandle;
TaskHandle_t xDriverTemperatureSensorFailureHandle;
TaskHandle_t xPassengerTemperatureSensorFailureHandle;
TaskHandle_t xRunTimeMeasurementsHandle;

/* Event Group set when the button pressed to change the heater level */

EventGroupHandle_t xButtonsEventGroups;

/* Driver and Passenger current & next states which are initially MAKE_HEATER_OFF*/

uint8_t ui8DriverSeatCurrentState = MAKE_HEATER_OFF;
uint8_t ui8PassengerSeatCurrentState = MAKE_HEATER_OFF ;

uint32_t ui32DriverTimeSample    = 0;
uint32_t ui32PassengerTimeSample = 0 ;

uint8_t ui8DriverSeatNextState     = MAKE_HEATER_OFF;
uint8_t ui8PassengerSeatNextState  = MAKE_HEATER_OFF;

/* Driver and Passenger Heater states which is initially HEATER_DISABLED*/

uint8_t ui8DriverHeaterIntensity    = HEATER_DISABLED;
uint8_t ui8PassengerHeaterIntensity = HEATER_DISABLED;

/* Driver and Passenger states before failure and time of failure */
uint8   ui8DriverFailureFlag;
uint8_t ui8DriverStateBeforeFailure;
uint32_t ui32DriverFailureTime;

uint8   ui8PassengerFailureFlag;
uint8_t ui8PassengerStateBeforeFailure;
uint32_t ui32PassengerFailureTime;

/* Variables to store the ADC output (Temperature values) */
uint32_t u32ADC_DriverValue;
uint32_t u32ADC_PassengerValue;

/* The HW setup function */
static void prvSetupHardware( void );

/* FreeRTOS tasks */
void vDesiredHeaterLevelTask(void *pvParameters);
void vHeaterIntensityDriverTask(void *pvParameters);
void vHeaterIntensityPassengerTask(void *pvParameters);
void vADC_StartConversionTask(void *pvParameters);
void vPrintTemperatureTask(void *pvParameters);
void vDriverTemperatureSensorFailureTask(void *pvParameters);
void vPassengerTemperatureSensorFailureTask(void *pvParameters);
void vRunTimeMeasurementsTask(void *pvParameters);


void ADC_Init(void);
void vHeater_Action(uint8_t Temp_diff,uint8_t *Intensity_ReturnState,
                    uint8 Seat_CurrentState,uint32_t UserTemperature,uint8 *TempSensorFailure_Flag);
/*
 * Arrays for Run Time Measurements
 */
uint32 ullTasksOutTime[9];
uint32 ullTasksInTime[9];
uint32 ullTasksTotalTime[9];

int main()
{
    /* Setup the hardware for use with the Tiva C board. */
    prvSetupHardware();

    /* Create a binary semaphore */

    xDriverHeaterSemaphore = xSemaphoreCreateBinary();
    xPassengerHeaterSemaphore = xSemaphoreCreateBinary();
    xButtonsEventGroups = xEventGroupCreate();

    /* Create Tasks here */
    xTaskCreate(vDesiredHeaterLevelTask,"Heater Level Task", 256 , NULL, 2, &xDesiredHeaterLevelHandle);

    xTaskCreate(vHeaterIntensityDriverTask,"Heater Intensity Task1", 256 , NULL, 2, &xHeaterIntensityDriverHandle);
    xTaskCreate(vHeaterIntensityPassengerTask,"Heater Intensity Task2", 256 , NULL, 2, &xHeaterIntensityPassengerHandle);

    xTaskCreate(vADC_StartConversionTask,"ADC get Value", 256 , NULL, 4, &xADC_GetSensorValueHandle);
    xTaskCreate(vPrintTemperatureTask,"Temp. print", 256 , NULL, 3, &xPrintTemperatureHandle);

    xTaskCreate(vDriverTemperatureSensorFailureTask,"Temperature Sensor failure", 256 , NULL, 3, &xDriverTemperatureSensorFailureHandle);
    xTaskCreate(vPassengerTemperatureSensorFailureTask,"Temperature Sensor failure", 256 , NULL, 3, &xPassengerTemperatureSensorFailureHandle);

    xTaskCreate(vRunTimeMeasurementsTask, "Run time", 256, NULL, 1, &xRunTimeMeasurementsHandle);

    /* Set Tag for each task */
    vTaskSetApplicationTaskTag( xDesiredHeaterLevelHandle, ( TaskHookFunction_t ) 1 );
    vTaskSetApplicationTaskTag( xHeaterIntensityDriverHandle, ( TaskHookFunction_t ) 2 );
    vTaskSetApplicationTaskTag( xHeaterIntensityPassengerHandle, ( TaskHookFunction_t ) 3 );
    vTaskSetApplicationTaskTag( xADC_GetSensorValueHandle, ( TaskHookFunction_t ) 4 );
    vTaskSetApplicationTaskTag( xPrintTemperatureHandle, ( TaskHookFunction_t ) 5 );
    vTaskSetApplicationTaskTag( xDriverTemperatureSensorFailureHandle, ( TaskHookFunction_t ) 6 );
    vTaskSetApplicationTaskTag( xPassengerTemperatureSensorFailureHandle, ( TaskHookFunction_t ) 7 );
    vTaskSetApplicationTaskTag( xRunTimeMeasurementsHandle, ( TaskHookFunction_t ) 8 );

    vTaskStartScheduler();

    /* Should never reach here!  If you do then there was not enough heap
    available for the idle task to be created. */
    for (;;);

}

static void prvSetupHardware( void )
{
    /* Place here any needed HW initialization such as GPIO, UART, etc.  */
    GPTM_WTimer0Init();
    UART0_Init();
    ADC_vInit();
    GPIO_BuiltinButtonsLedsInit();
    GPIO_ButtonInit();
    GPIO_SW1EdgeTriggeredInterruptInit();
    GPIO_SW2EdgeTriggeredInterruptInit();
    GPIO_ExternalEdgeTriggeredInterruptInit();
}

/*
 * (Periodic Task)
 * Runtime Measurements task: Calculate the CPU Load
 */

void vRunTimeMeasurementsTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint32 time = 0;
    const TickType_t xDelay2100ms = pdMS_TO_TICKS(2100);
    for (;;)
    {
        uint8 ucCounter, ucCPU_Load;
        uint32 ullTotalTasksTime = 0;
        vTaskDelayUntil(&xLastWakeTime, xDelay2100ms);
        for(ucCounter = 1; ucCounter < 7; ucCounter++)
        {
            ullTotalTasksTime += ullTasksTotalTime[ucCounter];
        }

        ucCPU_Load = (ullTotalTasksTime * 100) /  GPTM_WTimer0Read();

        taskENTER_CRITICAL();
        UART0_SendString("\r\n");
        UART0_SendString("CPU Load is ");
        UART0_SendInteger(ucCPU_Load);
        UART0_SendString("%\r\n");
        UART0_SendString("---------------------------------------------------------\r\n");
        taskEXIT_CRITICAL();
    }
}


/*
 * (Event based Task)
 * Desired Heater Level Task: Waiting for Events to be set in xButtonsEventGroups
 * and depending on the bit that has been set (Driver or Passenger) , the task changes
 * the User current state to the next state and this task returns vHeaterIntensityDriverTask or vHeaterIntensityPassengerTask
 * to ready state when the state is MAKE_HEATER_LOW_LEVEL (the state after MAKE_HEATER_OFF)
 */

void vDesiredHeaterLevelTask(void *pvParameters)
{
    const EventBits_t xBitsToWaitFor = (PASSENGER_BUTTON_BIT_MASK | DRIVER_BUTTON_BIT_MASK);
    EventBits_t xEventGroupValue;

    for(;;)
    {
        xEventGroupValue = xEventGroupWaitBits(xButtonsEventGroups, xBitsToWaitFor, pdTRUE, pdFALSE, portMAX_DELAY);
        if(DRIVER_BUTTON_BIT_MASK & xEventGroupValue)
        {
            switch(ui8DriverSeatNextState)
            {
            case MAKE_HEATER_OFF:           ui8DriverSeatNextState = MAKE_HEATER_LOW_LEVEL;    break;
            case MAKE_HEATER_LOW_LEVEL:     ui8DriverSeatNextState = MAKE_HEATER_MEDIUM_LEVEL; break;
            case MAKE_HEATER_MEDIUM_LEVEL:  ui8DriverSeatNextState = MAKE_HEATER_HIGH_LEVEL;   break;
            case MAKE_HEATER_HIGH_LEVEL:    ui8DriverSeatNextState = MAKE_HEATER_OFF;          break;
            default: break;
            }

            if(ui8DriverSeatNextState == MAKE_HEATER_LOW_LEVEL)
            {
                vTaskResume(xHeaterIntensityDriverHandle);
            }
        }

        if(PASSENGER_BUTTON_BIT_MASK & xEventGroupValue)
        {
            switch(ui8PassengerSeatNextState)
            {
            case MAKE_HEATER_OFF:           ui8PassengerSeatNextState = MAKE_HEATER_LOW_LEVEL;    break;
            case MAKE_HEATER_LOW_LEVEL:     ui8PassengerSeatNextState = MAKE_HEATER_MEDIUM_LEVEL; break;
            case MAKE_HEATER_MEDIUM_LEVEL:  ui8PassengerSeatNextState = MAKE_HEATER_HIGH_LEVEL;   break;
            case MAKE_HEATER_HIGH_LEVEL:    ui8PassengerSeatNextState = MAKE_HEATER_OFF;          break;
            default: break;
            }

            if(ui8PassengerSeatNextState == MAKE_HEATER_LOW_LEVEL)
            {
                vTaskResume(xHeaterIntensityPassengerHandle);
            }
        }

    }
}

/*
 * (Periodic Task)
 * ADc Start Converion Task: Start the Conversion of the ADC
 *
 */

void vADC_StartConversionTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xDelay500ms = pdMS_TO_TICKS(500);
    for(;;)
    {

        ADC0_vStartConversion();
        ADC1_vStartConversion();

        vTaskDelayUntil(&xLastWakeTime, xDelay500ms);
    }
}

/*
 * (Event based task)
 * The Task is waiting for the Passenger semaphore
 * when it becomes true the passenger heater should start working.
 * If the user state is(MAKE_HEATER_OFF) the task suspends itself.
 * The task also saves the state and time when a failure occurs.
 */

void vHeaterIntensityDriverTask(void *pvParameters)
{
    uint32_t DriverTempDifference    = 0;
    for(;;)
    {
       if( pdTRUE == xSemaphoreTake(xDriverHeaterSemaphore,portMAX_DELAY) )
        {

            DriverTempDifference    = ui8DriverSeatCurrentState - u32ADC_DriverValue;
            vHeater_Action(DriverTempDifference,&ui8DriverHeaterIntensity,
                           ui8DriverSeatNextState,u32ADC_DriverValue,&ui8DriverFailureFlag);
            if(ui8DriverSeatNextState == MAKE_HEATER_OFF)
            {
                vTaskSuspend(NULL);
            }
            if(!ui8DriverFailureFlag)
            {
                ui8DriverSeatCurrentState = ui8DriverSeatNextState;
                ui32DriverTimeSample = GPTM_WTimer0Read();
            }
            if(ui8DriverFailureFlag)
            {
                xEventGroupSetBits(xButtonsEventGroups, DRIVER_SENSOR_FAILURE_BIT_MASK);
            }

        }

    }
}

/*
 * (Event based task)
 * The Task is waiting for the Passenger semaphore
 * when it becomes true the passenger heater should start working.
 * If the user state is(MAKE_HEATER_OFF) the task suspends itself.
 * The task also saves the state and time when a failure occurs.
 */

void vHeaterIntensityPassengerTask(void *pvParameters)
{
    uint32_t PassengerTempDifference = 0;
    for(;;)
    {

        if( pdTRUE == xSemaphoreTake(xPassengerHeaterSemaphore,portMAX_DELAY) )
        {
            PassengerTempDifference = ui8PassengerSeatCurrentState - u32ADC_PassengerValue;
            vHeater_Action(PassengerTempDifference,&ui8PassengerHeaterIntensity,
                           ui8PassengerSeatNextState,u32ADC_PassengerValue,&ui8PassengerFailureFlag);

            if(ui8PassengerSeatNextState == MAKE_HEATER_OFF)
            {
                vTaskSuspend(NULL);
            }
            if(!ui8PassengerFailureFlag)
            {
                ui8PassengerSeatCurrentState = ui8PassengerSeatNextState;
                ui32PassengerTimeSample = GPTM_WTimer0Read();
            }

            if(ui8PassengerFailureFlag)
            {
                xEventGroupSetBits(xButtonsEventGroups, PASSENGER_SENSOR_FAILURE_BIT_MASK);

            }

        }
    }
}

/*(Periodic task)
 * Print application information of Driver & Passenger
 * (Temperature Current state,Heater intensity Level)
 */

void vPrintTemperatureTask(void *pvParameters)
{
    const TickType_t xDelay1000ms = pdMS_TO_TICKS(1000);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for(;;)
    {
        UART0_SendString("Driver Temperature: ");
        UART0_SendInteger(u32ADC_DriverValue);
        UART0_SendString("\r\n");
        switch(ui8DriverSeatNextState)
        {
            case MAKE_HEATER_OFF:          UART0_SendString("Driver -> Heater is Disabled\r\n");                break;
            case MAKE_HEATER_LOW_LEVEL:    UART0_SendString("Driver -> Low Heating Mode\r\n");                  break;
            case MAKE_HEATER_MEDIUM_LEVEL: UART0_SendString("Driver -> Medium Heating Mode\r\n");               break;
            case MAKE_HEATER_HIGH_LEVEL:   UART0_SendString("Driver -> Maximum Heating Mode\r\n");              break;
            default: break;
        }
        switch(ui8DriverHeaterIntensity)
        {
            case LOW_INTENSITY:     UART0_SendString("Driver Heater Intensity is Low Intensity\r\n");           break;
            case MEDIUM_INTENSITY:  UART0_SendString("Driver Heater Intensity is Medium Intensity\r\n");        break;
            case HIGH_INTENSITY:    UART0_SendString("Driver Heater Intensity is High Intensity\r\n");          break;
            case HEATER_DISABLED:   UART0_SendString("Driver Heater is Disabled\r\n");                          break;
            default: break;
        }
        UART0_SendString("\r\n");
        UART0_SendString("---------------------------------------------------------\r\n");
        UART0_SendString("\r\n");
        vTaskDelayUntil(&xLastWakeTime,xDelay1000ms);
        UART0_SendString("Passenger Temperature: ");
        UART0_SendInteger(u32ADC_PassengerValue);
        UART0_SendString("\r\n");
        switch(ui8PassengerSeatNextState)
        {
            case MAKE_HEATER_OFF:          UART0_SendString("Passenger -> Heater is Disabled\r\n");                break;
            case MAKE_HEATER_LOW_LEVEL:    UART0_SendString("Passenger -> Low Heating Mode\r\n");                  break;
            case MAKE_HEATER_MEDIUM_LEVEL: UART0_SendString("Passenger -> Medium Heating Mode\r\n");               break;
            case MAKE_HEATER_HIGH_LEVEL:   UART0_SendString("Passenger -> Maximum Heating Mode\r\n");              break;
            default: break;
        }
        switch(ui8PassengerHeaterIntensity)
        {
            case LOW_INTENSITY:     UART0_SendString("Passenger Heater Intensity is Low Intensity\r\n");           break;
            case MEDIUM_INTENSITY:  UART0_SendString("Passenger Heater Intensity is Medium Intensity\r\n");        break;
            case HIGH_INTENSITY:    UART0_SendString("Passenger Heater Intensity is High Intensity\r\n");          break;
            case HEATER_DISABLED:   UART0_SendString("Passenger Heater is Disabled\r\n");                          break;
            default: break;
        }
        UART0_SendString("\r\n");
        UART0_SendString("---------------------------------------------------------\r\n");
        UART0_SendString("\r\n");
        vTaskDelayUntil(&xLastWakeTime,xDelay1000ms);
    }
}


/*
 * (Event Based Task)
 * When error occurs in Driver Temperature sensor this task should run
 * to save the state & time at which last failure occurs
 */

void vDriverTemperatureSensorFailureTask(void *pvParameters)
{
    const EventBits_t xBitsToWaitFor = (DRIVER_SENSOR_FAILURE_BIT_MASK);
    EventBits_t xEventGroupValue;
    for(;;)
    {
        xEventGroupValue = xEventGroupWaitBits(xButtonsEventGroups, xBitsToWaitFor, pdTRUE, pdFALSE, portMAX_DELAY);

        if(xEventGroupValue & DRIVER_SENSOR_FAILURE_BIT_MASK)
        {
            ui32DriverFailureTime = ui32DriverTimeSample;
            ui8DriverStateBeforeFailure = ui8DriverSeatCurrentState;
        }
    }
}

/*
 * (Event Based Task)
 * When error occurs in Passenger Temperature sensor this task should run
 * to save the state & time at which last failure occurs
 */
void vPassengerTemperatureSensorFailureTask(void *pvParameters)
{
    const EventBits_t xBitsToWaitFor = (PASSENGER_SENSOR_FAILURE_BIT_MASK);
    EventBits_t xEventGroupValue;
    for(;;)
    {
        xEventGroupValue = xEventGroupWaitBits(xButtonsEventGroups, xBitsToWaitFor, pdTRUE, pdFALSE, portMAX_DELAY);

        if(xEventGroupValue & PASSENGER_SENSOR_FAILURE_BIT_MASK)
        {
            ui32PassengerFailureTime = ui32PassengerTimeSample;
            ui8PassengerStateBeforeFailure = ui8PassengerSeatCurrentState;
        }
    }
}


/*************************************************/

void ADC0Seq3_Handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    SET_BIT(ADC0_ISC,3);
    ADC0_vRead(&u32ADC_DriverValue);
    xSemaphoreGiveFromISR(xDriverHeaterSemaphore, &xHigherPriorityTaskWoken);
    u32ADC_DriverValue *= 45.0/4095;
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
void ADC1Seq3_Handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    SET_BIT(ADC1_ISC,3);
    ADC1_vRead(&u32ADC_PassengerValue);
    xSemaphoreGiveFromISR(xPassengerHeaterSemaphore, &xHigherPriorityTaskWoken);
    u32ADC_PassengerValue *= 45.0/4095;
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

void GPIOPortF_Handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Simple debounce delay
    Delay_MS(300);

    if(GPIO_PORTF_RIS_REG & (1<<4))
    {

        xEventGroupSetBitsFromISR(xButtonsEventGroups, DRIVER_BUTTON_BIT_MASK,xHigherPriorityTaskWoken);
        /* Clear Trigger flag for PF4 (Interrupt Flag) */
        GPIO_PORTF_ICR_REG   |= (1<<4);
    }
    if(GPIO_PORTF_RIS_REG & (1<<0))
    {
        xEventGroupSetBitsFromISR(xButtonsEventGroups, PASSENGER_BUTTON_BIT_MASK,xHigherPriorityTaskWoken);
        /* Clear Trigger flag for PF0 (Interrupt Flag) */
        GPIO_PORTF_ICR_REG   |= (1<<0);
    }
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
void GPIOPortA_Handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Simple debounce delay
    Delay_MS(300);

    if(GPIO_PORTA_RIS_REG & (1<<4))
    {

        xEventGroupSetBitsFromISR(xButtonsEventGroups, DRIVER_BUTTON_BIT_MASK,xHigherPriorityTaskWoken);
        /* Clear Trigger flag for PF4 (Interrupt Flag) */
        GPIO_PORTA_ICR_REG   |= (1<<4);
    }

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
void ADC0Seq1_Handler(void)
{

}
void ADC1Seq1_Handler(void)
{

}

/*
 * This function responsible for changing the intensity ( colors of the LED )
 *
 * (Temp_diff) ===> parameter refers to (Desired Temp. - Temp. Sensor value)
 *
 * returns void
 */

void vHeater_Action(uint8_t Temp_diff,uint8_t *Intensity_ReturnState,
                    uint8 Seat_CurrentState,uint32_t UserTemperature,uint8 *TempSensorFailure_Flag)
{
    if((Seat_CurrentState == MAKE_HEATER_OFF))
    {
        GPIO_RedLedOff();
        GPIO_BlueLedOff();
        GPIO_GreenLedOff();
        *Intensity_ReturnState = HEATER_DISABLED;
    }
    else
    {
        if((UserTemperature > MAXIMUM_TEMP_ACCEPTED) || (UserTemperature < MINIMUM_TEMP_ACCEPTED))
        {
            GPIO_GreenLedOff();
            GPIO_BlueLedOff();
            GPIO_RedLedOn();
            *TempSensorFailure_Flag = 1;
            *Intensity_ReturnState = HEATER_DISABLED;
        }
        else if((UserTemperature > Seat_CurrentState))
        {
            *TempSensorFailure_Flag = 0;
            GPIO_RedLedOff();
            GPIO_BlueLedOff();
            GPIO_GreenLedOff();
            *Intensity_ReturnState = HEATER_DISABLED;
        }
        else
        {
            *TempSensorFailure_Flag = 0;
            if(Temp_diff >= HEATER_HIGH_INTENSITY_LIMIT)
            {
                GPIO_RedLedOff();
                GPIO_GreenLedOn();
                GPIO_BlueLedOn();
                *Intensity_ReturnState = HIGH_INTENSITY;
            }
            else if( (Temp_diff >= HEATER_MEDIUM_INTENSITY_LOW_LIMIT) && (Temp_diff < HEATER_MEDIUM_INTENSITY_HIGH_LIMIT) )
            {
                GPIO_RedLedOff();
                GPIO_GreenLedOff();
                GPIO_BlueLedOn();
                *Intensity_ReturnState = MEDIUM_INTENSITY;
            }
            else if( (Temp_diff >= HEATER_LOW_INTENSITY_LOW_LIMIT) && (Temp_diff < HEATER_LOW_INTENSITY_HIGH_LIMIT) )
            {
                GPIO_RedLedOff();
                GPIO_BlueLedOff();
                GPIO_GreenLedOn();
                *Intensity_ReturnState = LOW_INTENSITY;
            }
        }
    }

}
