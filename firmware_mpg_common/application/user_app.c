/**********************************************************************************************************************
File: user_app.c                                                                

----------------------------------------------------------------------------------------------------------------------
To start a new task using this user_app as a template:
 1. Copy both user_app.c and user_app.h to the Application directory
 2. Rename the files yournewtaskname.c and yournewtaskname.h
 3. Add yournewtaskname.c and yournewtaskname.h to the Application Include and Source groups in the IAR project
 4. Use ctrl-h (make sure "Match Case" is checked) to find and replace all instances of "user_app" with "yournewtaskname"
 5. Use ctrl-h to find and replace all instances of "UserApp" with "YourNewTaskName"
 6. Use ctrl-h to find and replace all instances of "USER_APP" with "YOUR_NEW_TASK_NAME"
 7. Add a call to YourNewTaskNameInitialize() in the init section of main
 8. Add a call to YourNewTaskNameRunActiveState() in the Super Loop section of main
 9. Update yournewtaskname.h per the instructions at the top of yournewtaskname.h
10. Delete this text (between the dashed lines) and update the Description below to describe your task
----------------------------------------------------------------------------------------------------------------------

Description:
This is a user_app.c file template 

------------------------------------------------------------------------------------------------------------------------
API:

Public functions:


Protected System functions:
void UserAppInitialize(void)
Runs required initialzation for the task.  Should only be called once in main init section.

void UserAppRunActiveState(void)
Runs current task state.  Should only be called once in main loop.


**********************************************************************************************************************/

#include "configuration.h"

/***********************************************************************************************************************
Global variable definitions with scope across entire project.
All Global variable names shall start with "G_"
***********************************************************************************************************************/
/* New variables */
volatile u32 G_u32UserAppFlags;                       /* Global state flags */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Existing variables (defined in other files -- should all contain the "extern" keyword) */
extern volatile u32 G_u32SystemFlags;                  /* From main.c */
extern volatile u32 G_u32ApplicationFlags;             /* From main.c */

extern volatile u32 G_u32SystemTime1ms;                /* From board-specific source file */
extern volatile u32 G_u32SystemTime1s;                 /* From board-specific source file */

extern AntSetupDataType G_stAntSetupData;                         /* From ant.c */

extern u32 G_u32AntApiCurrentDataTimeStamp;                       /* From ant_api.c */
extern AntApplicationMessageType G_eAntApiCurrentMessageClass;    /* From ant_api.c */
extern u8 G_au8AntApiCurrentData[ANT_APPLICATION_MESSAGE_BYTES];  /* From ant_api.c */


/* Existing variables (defined in other files -- should all contain the "extern" keyword) */
extern AntSetupDataType G_stAntSetupData;                         /* From ant.c */

extern u32 G_u32AntApiCurrentDataTimeStamp;                       /* From ant_api.c */
extern AntApplicationMessageType G_eAntApiCurrentMessageClass;    /* From ant_api.c */
extern u8 G_au8AntApiCurrentData[ANT_APPLICATION_MESSAGE_BYTES];  /* From ant_api.c */

/***********************************************************************************************************************
Global variable definitions with scope limited to this local application.
Variable names shall start with "UserApp_" and be declared as static.
***********************************************************************************************************************/
static fnCode_type UserApp_StateMachine;            /* The state machine function pointer */
static u32 UserApp_u32Timeout;                      /* Timeout counter used across states */

static u16 UserApp_u16Frequency[]={0,523,586,658,697,783,879,987,262,293,329,349,392,440,494};/*some frequencies matched with do re mi fa .....*/
static u8 User_App_u8MusicName[] = "Music1";
static u8 User_App_u8WelcomeScreen[] = "Welcome to H_M_P!";  /*welcome screen*/
static u8 User_App_u8Start[] = "START";                      /**/
static u8 User_App_u8StartScreen[] = "NEXT LAST  PLAY BACK"; /**/
/**********************************************************************************************************************
Function Definitions
**********************************************************************************************************************/

/*--------------------------------------------------------------------------------------------------------------------*/
/* Public functions                                                                                                   */
/*--------------------------------------------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------------------------------------------*/
/* Protected functions                                                                                                */
/*--------------------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------------------
Function: UserAppInitialize

Description:
Initializes the State Machine and its variables.

Requires:
  -

Promises:
  - 
*/
void UserAppInitialize(void)
{
  /* Configure ANT for this application */
  G_stAntSetupData.AntChannel          = ANT_CHANNEL_USERAPP;
  G_stAntSetupData.AntSerialLo         = ANT_SERIAL_LO_USERAPP;
  G_stAntSetupData.AntSerialHi         = ANT_SERIAL_HI_USERAPP;
  G_stAntSetupData.AntDeviceType       = ANT_DEVICE_TYPE_USERAPP;
  G_stAntSetupData.AntTransmissionType = ANT_TRANSMISSION_TYPE_USERAPP;
  G_stAntSetupData.AntChannelPeriodLo  = ANT_CHANNEL_PERIOD_LO_USERAPP;
  G_stAntSetupData.AntChannelPeriodHi  = ANT_CHANNEL_PERIOD_HI_USERAPP;
  G_stAntSetupData.AntFrequency        = ANT_FREQUENCY_USERAPP;
  G_stAntSetupData.AntTxPower          = ANT_TX_POWER_USERAPP;
  /* If good initialization, set state to Idle */
  if( AntChannelConfig(ANT_SLAVE) )
  {
    /* Channel is configured */
    UserApp_StateMachine = UserAppSM_Idle;
    AntOpenChannel();
  }
  else
  {
    /* The task isn't properly initialized, so don't run */
    UserApp_StateMachine = UserAppSM_Error;
  }
  LedOff(LCD_RED);
  LedOn(LCD_GREEN);
  LedOn(LCD_BLUE);
  LCDClearChars(LINE1_START_ADDR, 20);
  LCDClearChars(LINE2_START_ADDR, 20);
  LCDMessage(LINE1_START_ADDR, User_App_u8WelcomeScreen);
  LCDMessage(LINE2_START_ADDR + 15, User_App_u8Start);
} /* end UserAppInitialize() */


/*----------------------------------------------------------------------------------------------------------------------
Function UserAppRunActiveState()

Description:
Selects and runs one iteration of the current state in the state machine.
All state machines have a TOTAL of 1ms to execute, so on average n state machines
may take 1ms / n to execute.

Requires:
  - State machine function pointer points at current state

Promises:
  - Calls the function to pointed by the state machine function pointer
*/
void UserAppRunActiveState(void)
{
  UserApp_StateMachine();
} /* end UserAppRunActiveState */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Private functions                                                                                                  */
/*--------------------------------------------------------------------------------------------------------------------*/


/**********************************************************************************************************************
State Machine Function Definitions
**********************************************************************************************************************/

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for a message to be queued */
static void UserAppSM_Idle(void)
{
  static u16 u16Notes[100]={0};
  static u8 u8LastData[]={0};
  static u8 u8Music[10]={0};
  static u16 u16NotesCount = 0;
  static u8 u8MusicCount = 0;
  static u8 u8MusicCountOnLCD = 0;
  static bool bOpenBuzzer = FALSE;
  static bool bBack = FALSE;
  static u8 u8Time = 6;
  static u8 u8LoopTime = 50;
  static u8 i = 0;
  u8LoopTime--;
  /*  Get the notes sent from the master  */
  if( AntReadData() )
  {
     /*  New data message: check what it is  */
    if(G_eAntApiCurrentMessageClass == ANT_DATA)
    {
      /*  We got some data,save it to an array  */
      if(G_au8AntApiCurrentData[0] != u8LastData[0])
      {
        /* in order to get the notes correctly, we'd better get different messeages each time,
           so,if the master wants to send same note,we can make a symbol such as 20 here*/
        if(G_au8AntApiCurrentData[0] != 20)
        {
          u16NotesCount++;
          u16Notes[u16NotesCount-1] = G_au8AntApiCurrentData[0];
          u8LastData[0] = G_au8AntApiCurrentData[0];
        }
        /*  when a song is finished ,the master send a message means he wants to name the song,  
            and use u8Music[] to record how many notes the song needs */
        else if( G_au8AntApiCurrentData[0] == 50)
        {
          UserApp_StateMachine = UserAppSM_Name;
          u8Music[u8MusicCount] = u16NotesCount;
          u8MusicCount++;
          u8LastData[0] = G_au8AntApiCurrentData[0];
        }
        /* if the message == 20,means the master send the same message as the last one*/
        else
        {
          u16NotesCount++;
          u16Notes[u16NotesCount-1] = u16Notes[u16NotesCount-2] ;
          u8LastData[0] = G_au8AntApiCurrentData[0];
        }
      }      
    }
    else if(G_eAntApiCurrentMessageClass == ANT_TICK)
    {
     /* A channel period has gone by: typically this is when new data should be queued to be sent */
    }
  } /* end AntReadData() */ 
  /*   Buttons and buzzers' function  */
  if(u8LoopTime == 0)
  {
    u8LoopTime = 50;
    /* use button0 to choose next song */
    if( WasButtonPressed(BUTTON0) )
    {
      /* Be sure to acknowledge the button press */
      ButtonAcknowledge(BUTTON0);
      if(u8MusicCountOnLCD == u8MusicCount)
      {
        u8MusicCountOnLCD = 0;
        i = 0;
      }
      else
      {
        u8MusicCountOnLCD++;
        i = u8Music[u8MusicCountOnLCD-1];
      }
    }
    /* use button1 to choose last song */
    if( WasButtonPressed(BUTTON1) )
    {
      /* Be sure to acknowledge the button press */
      ButtonAcknowledge(BUTTON1);
      if(u8MusicCountOnLCD == 0)
      {
        u8MusicCountOnLCD = u8MusicCount;
        i = u8Music[u8MusicCountOnLCD-1];
      }
      else
      {
        u8MusicCountOnLCD--;
        if(u8MusicCountOnLCD == 0)
        {
          i = 0;
        }
        else
        {
          i = u8Music[u8MusicCountOnLCD-1];
        }
      }
    }
    /* use button2 to play or end the song */
    if( WasButtonPressed(BUTTON2) )
    {
      /* Be sure to acknowledge the button press */
      ButtonAcknowledge(BUTTON2);
      if(bOpenBuzzer == FALSE)
      {
        bOpenBuzzer = TRUE;
      }
      else
      {
        PWMAudioOff(BUZZER1);
        bOpenBuzzer = FALSE; 
      }
    }
    /* use button3 to get into play screen or back to welcome screen */
    if( WasButtonPressed(BUTTON3) )
    {
      /* Be sure to acknowledge the button press */
      ButtonAcknowledge(BUTTON3);
      /* back to welcome screen */
      if(bBack)
      {
        LCDClearChars(LINE1_START_ADDR, 20);
        LCDClearChars(LINE2_START_ADDR, 20);
        LCDMessage(LINE1_START_ADDR, User_App_u8WelcomeScreen);
        LCDMessage(LINE2_START_ADDR + 15, User_App_u8Start);
        bBack = FALSE;
      }
      /* get into play screen */
      else
      {
        LCDClearChars(LINE1_START_ADDR, 20);
        LCDClearChars(LINE2_START_ADDR, 20);
        LCDMessage(LINE1_START_ADDR,User_App_u8MusicName);
        LCDMessage(LINE2_START_ADDR, User_App_u8StartScreen);
        bBack = TRUE;
      }
    }
    /*  Buzzer's function  */
    if(bOpenBuzzer)
    {
      u8Time--;
      if(u8Time == 0)
      {
        u8Time = 6;
        /**/
        PWMAudioSetFrequency(BUZZER1, UserApp_u16Frequency[u16Notes[i]] );
        PWMAudioOn(BUZZER1);
        i++;
        /* the song have finished */
        if(u8MusicCountOnLCD == 0)
        {
          i = 0;
        }
        else if( i == u8Music[u8MusicCountOnLCD]-1)
        {
          i = u8Music[u8MusicCountOnLCD-1];
        }
      } /*   */
    } /* end bOnBuzzer */
  } /**/
} /* end UserAppSM_Idle() */
/*-------------------------------------------------------------------------------------------------------------------*/
/* Handle an error */
static void UserAppSM_Error(void)          
{
  
} /* end UserAppSM_Error() */


/*-------------------------------------------------------------------------------------------------------------------*/
/* Name a song */
static void UserAppSM_Name(void)          
{
  
} /* end UserAppSM_Name() */


/*-------------------------------------------------------------------------------------------------------------------*/
/* State to sit in if init failed */
static void UserAppSM_FailedInit(void)          
{
    
} /* end UserAppSM_FailedInit() */


/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/