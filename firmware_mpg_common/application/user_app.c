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

static u16 UserApp_u16Frequency[]={0,523,586,658,697,783,879,987,262,293,329,349,392,440,494,1045,1171,1316,1393,1563,1755,1971};/*some frequencies matched with do re mi fa .....*/
static u16 UserApp_u16MusicNameNumber[50] = {0};              /* store how many characters of names*/
static u16 UserApp_u16MusicName[100] = {0};                    /*store every characters of all the names */
static u8 UserApp_u8EmptyList[] = "Music list is empty!";     /* if the list is empty,show it */
static u8 UserApp_u8WelcomeScreen[] = "Welcome to H_M_P!";  /* welcome screen */
static u8 UserApp_u8Open[] = "OPEN";                      
static u8 UserApp_u8OpenScreen[] = "NEXT LAST PLAY CLOSE"; 
static u8 UserApp_u8CloseScreen[] = "NEXT LAST PLAY  OPEN"; 
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
  }
  else
  {
    /* The task isn't properly initialized, so don't run */
    UserApp_StateMachine = UserAppSM_Error;
  }
  /* initialize the lcd */
  LedOff(LCD_RED);
  LedOn(LCD_GREEN);
  LedOn(LCD_BLUE);
  LCDClearChars(LINE1_START_ADDR, 20);
  LCDClearChars(LINE2_START_ADDR, 20);
  LCDMessage(LINE1_START_ADDR, UserApp_u8WelcomeScreen);
  LCDMessage(LINE2_START_ADDR + 16, UserApp_u8Open);
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
  static u16 u16Notes[100]={0};       /* save all notes have been sent */
  static u8 u8LastData[]={0};         /* save the last notes have been sent to compare with the current one*/
  static u8 u8Music[10]={0};          /* save how many notes a music have */ 
  static u8 u8MusicName[20] = "";     /* to save the current name be selected on lcd*/
  static u8 u8MiddleVariable;         /* a MiddleVariable to send the name in UserApp_u16MusicName to u8MusicName */
  static u8 u8NameCount = 0;          /* represent the position of the character in the name  */
  static u16 u16NotesCount = 0;       /* a counter to count how many notes have been sent */
  static u8 u8MusicCount = 1;         /* a counter to count how many music have finished ,start from 1*/
  static u8 u8MusicCountOnLCD = 0;    /* a counter to count which song have been selected on lcd */
  static bool bOpenBuzzer = FALSE;    /* to judge if the buzzer should be turned on */
  static bool bClose = FALSE;         /* to judge if the channel is closed */
  static bool bOpen = FALSE;          /* to judge if the channel have been  opened by user */
  static u8 u8Time = 5;               /* little loop to make the buzzer play well*/
  static u8 u8LoopTime = 50;          /* big loop to make the system work well*/
  static u8 u8NotesPlace = 0;         /* record the place of the notes of the music which have been selected */
  static LedNumberType aeCurrentLed[]  = {LCD_GREEN, LCD_RED, LCD_BLUE, LCD_GREEN, LCD_RED, LCD_BLUE};
  static bool abLedRateIncreasing[]   = {TRUE,      FALSE,   TRUE,     FALSE,     TRUE,    FALSE};
  static u8 u8CurrentLedIndex  = 0;
  static u8 u8LedCurrentLevel  = 0;
  static u8 u8DutyCycleCounter = 0;
  static u8 u8LCDChangeTime = 20;
  u8LoopTime--;
  u8LCDChangeTime--;
  /*  Get the notes sent from the master  */
  if( AntReadData() )
  {
     /*  New data message: check what it is  */
    if(G_eAntApiCurrentMessageClass == ANT_DATA)
    {
      /*  We got some data,save it to an array  */
      if(G_au8AntApiCurrentData[0] != u8LastData[0])
      {
        /*  when a song is finished ,the master send a message means he wants to name the song, so go to
        UserAppSM_Name to name it , and use u8Music[] to record how many notes the song needs  */
        if( G_au8AntApiCurrentData[0] == 50)
        {
          UserApp_StateMachine = UserAppSM_Name;
          u8Music[u8MusicCount] = u16NotesCount;
          u8MusicCount++;
          u8LastData[0] = G_au8AntApiCurrentData[0];
        }
        else if(G_au8AntApiCurrentData[0] == 200)
        {
          /* 200 reprents name have finished,don't reserve it in the array */
        }
        /* in order to get the notes correctly, we'd better get different messeages each time,
           so,if the master wants to send same note,we can make a symbol such as 20 here*/
        else if(G_au8AntApiCurrentData[0] != 30)
        {
          u16NotesCount++;
          u16Notes[u16NotesCount-1] = G_au8AntApiCurrentData[0];
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
    /* A slave channel can close on its own, so check channel status when i opened the channel , 
    if the channel closed,show on the lcd to let user open the channel*/
    if(bOpen)
    {
      if(AntRadioStatus() != ANT_OPEN)
      {
        LCDClearChars(LINE2_START_ADDR, 20);
        LCDMessage(LINE2_START_ADDR, UserApp_u8CloseScreen);
        bClose = FALSE;
        bOpen = FALSE;
      }
    }
    /* use button0 to choose next song */
    if( WasButtonPressed(BUTTON0) )
    {
      /* Be sure to acknowledge the button press */
      ButtonAcknowledge(BUTTON0);
      u8NameCount = 0;
      /* if u8MusicCountOnLCD == u8MusicCount-1,press button 0 means start from first music,else the next one */
      if(u8MusicCountOnLCD == u8MusicCount - 1 )
      {
        u8MusicCountOnLCD = 1;
        u8NotesPlace = 0;
      }
      else
      {
        u8MusicCountOnLCD++;
        u8NotesPlace = u8Music[u8MusicCountOnLCD-1];
      }
      for(u8 j = UserApp_u16MusicNameNumber[u8MusicCountOnLCD -1]; j<UserApp_u16MusicNameNumber[u8MusicCountOnLCD] ;j++)
      {
        u8MiddleVariable = UserApp_u16MusicName[j];
        u8MusicName[u8NameCount] = u8MiddleVariable;
        u8NameCount++;
      }
      u8MusicName[u8NameCount] = '\0';
      if( u8MusicCount != 1)
      {
        LCDClearChars(LINE1_START_ADDR, 20);
        LCDMessage(LINE1_START_ADDR, u8MusicName);
      }

    }
    /* use button1 to choose last song */
    if( WasButtonPressed(BUTTON1) )
    {
      /* Be sure to acknowledge the button press */
      ButtonAcknowledge(BUTTON1);
      u8NameCount = 0;
      /* u8MusicCountOnLCD == 1 ,represents next one is the final one  */
      if(u8MusicCountOnLCD == 1)
      {
        u8MusicCountOnLCD = u8MusicCount - 1;
        u8NotesPlace = u8Music[u8MusicCountOnLCD-1];
      }
      else
      {
        u8MusicCountOnLCD--;
        if(u8MusicCountOnLCD == 1)
        {
          u8NotesPlace = 0;
        }
        else
        {
          u8NotesPlace = u8Music[u8MusicCountOnLCD-1];
        }
      }
      /* move name from UserApp_u16MusicName to u8MusicName*/
      for(u8 j = UserApp_u16MusicNameNumber[u8MusicCountOnLCD -1]; j<UserApp_u16MusicNameNumber[u8MusicCountOnLCD] ;j++)
      {
        u8MiddleVariable = UserApp_u16MusicName[j];
        u8MusicName[u8NameCount] = u8MiddleVariable;
        u8NameCount++;
      }
      u8MusicName[u8NameCount] = '\0';
      /* to make sure we have music */
      if( u8MusicCount != 1)
      {
        LCDClearChars(LINE1_START_ADDR, 20);
        LCDMessage(LINE1_START_ADDR, u8MusicName);
      }
    }
    /* use button2 to play or end the song */
    if( WasButtonPressed(BUTTON2) )
    {
      /*  Be sure to acknowledge the button press */
      ButtonAcknowledge(BUTTON2);
      if(bOpenBuzzer == FALSE)
      {
        bOpenBuzzer = TRUE;
        /* Start with red LED on 100%, green and blue off */
        LedPWM(LCD_RED, LED_PWM_100);
        LedPWM(LCD_GREEN, LED_PWM_0);
        LedPWM(LCD_BLUE, LED_PWM_0);
      }
      /*  when stop the music, turn off all leds too,change lcd to the start color */
      else
      {
        PWMAudioOff(BUZZER1);
        bOpenBuzzer = FALSE;
        LedOff(WHITE);
        LedOff(PURPLE);
        LedOff(BLUE);
        LedOff(CYAN);
        LedOff(GREEN);
        LedOff(YELLOW);
        LedOff(ORANGE);
        LedOff(RED);
        LedOn(LCD_BLUE);
        LedOn(LCD_GREEN);
        LedOff(LCD_RED);
      }
    }
    /* use button3 to get into play screen or back to welcome screen */
    if( WasButtonPressed(BUTTON3) )
    {
      /* Be sure to acknowledge the button press */
      ButtonAcknowledge(BUTTON3);
      /* back to welcome screen if*/
      if(bClose)
      {
        AntCloseChannel();
        bOpen = FALSE;
        LCDClearChars(LINE2_START_ADDR, 20);
        LCDMessage(LINE2_START_ADDR, UserApp_u8CloseScreen);
        bClose = FALSE;
      }
      /* get into play_screen */
      else
      {
        AntOpenChannel();
        bOpen  = TRUE;        
        LCDClearChars(LINE1_START_ADDR, 20);
        LCDClearChars(LINE2_START_ADDR, 20);
        LCDMessage(LINE2_START_ADDR, UserApp_u8OpenScreen);
        bClose = TRUE;
        /* u8MusicCount == 1 means no music have finished,so the list is empty */
        if(u8MusicCount == 1)
        {
          LCDMessage(LINE1_START_ADDR, UserApp_u8EmptyList);
        }
        else
        {
          LCDMessage(LINE1_START_ADDR, u8MusicName);
        }
      }
    }
    /*   Buzzer's function  */
    if(bOpenBuzzer)
    {
      u8Time--;
      if(u8Time == 0)
      {
        u8Time = 5;
        /* turn off all leds first, to make sure when a frequency is played, only one led is on*/
        LedOff(WHITE);
        LedOff(PURPLE);
        LedOff(BLUE);
        LedOff(CYAN);
        LedOff(GREEN);
        LedOff(YELLOW);
        LedOff(ORANGE);
        LedOff(RED);
        /* set the frequency and open the buzzer */
        PWMAudioSetFrequency(BUZZER1, UserApp_u16Frequency[u16Notes[u8NotesPlace]] );
        PWMAudioOn(BUZZER1);        
        /*  when buzzer is on ,let an led on ,correspond to some frequencies  */
        switch(UserApp_u16Frequency[u16Notes[u8NotesPlace]])
        {
        case 523:
          LedOn(WHITE);
          break;
        case 586:
          LedOn(PURPLE);
          break;
        case 658:
          LedOn(BLUE);
          break;
        case 697:
          LedOn(CYAN);
          break;
        case 783:
          LedOn(GREEN);
          break;
        case 879:
          LedOn(YELLOW);
          break;
        case 987:
          LedOn(ORANGE);
          break;
        case 262:
          LedOn(WHITE);
          break;
        case 293:
          LedOn(PURPLE);
          break;
        case 329:
          LedOn(BLUE);
          break;
        case 349:
          LedOn(CYAN);
          break;
        case 392:
          LedOn(GREEN);
          break;
        case 440:
          LedOn(YELLOW);
          break;
        case 494:
          LedOn(ORANGE);
          break;
        case 1045:
          LedOn(WHITE);
          break;
        case 1171:
          LedOn(PURPLE);
          break;
        case 1316:
          LedOn(BLUE);
          break;
        case 1393:
          LedOn(CYAN);
          break;
        case 1563:
          LedOn(GREEN);
          break;
        case 1755:
          LedOn(YELLOW);
          break;
        case 1971:
          LedOn(ORANGE);
          break;
        }
        u8NotesPlace++;        
        if(u8MusicCountOnLCD == 0)
        {
          u8NotesPlace = 0;
        }
        /* the song have finished */
        else if( u8NotesPlace == u8Music[u8MusicCountOnLCD])
        {
          u8NotesPlace = u8Music[u8MusicCountOnLCD-1];
          for(u8 i = 0; i<200; i++)
          {
            PWMAudioOff(BUZZER1);
          }
        }
      } /*end u8Time */
    } /* end bOnBuzzer*/    
  } /*end loop time*/
  /*  make lcd's color changes in a speed  */
  if(u8LCDChangeTime == 0)
  {
    u8LCDChangeTime = 20;
    if(bOpenBuzzer)
    {
      LedPWM( (LedNumberType)aeCurrentLed[u8CurrentLedIndex], (LedRateType)u8LedCurrentLevel);
      /* Update the current level based on which way it's headed */
      if( abLedRateIncreasing[u8CurrentLedIndex] )
      {
        u8LedCurrentLevel++;
      }
      else
      {
        u8LedCurrentLevel--;
      }

    /* Change direction once we're at the end */
      u8DutyCycleCounter++;
      if(u8DutyCycleCounter == 20)
      {
        u8DutyCycleCounter = 0;
        
        /* Watch for the indexing variable to reset */
        u8CurrentLedIndex++;
        if(u8CurrentLedIndex == sizeof(aeCurrentLed))
        {
          u8CurrentLedIndex = 0;
        }
        
        /* Set the current level based on what direction we're now going */
        u8LedCurrentLevel = 20;
        if(abLedRateIncreasing[u8CurrentLedIndex])
        {
           u8LedCurrentLevel = 0;
        }
      }/* end u8DutyCycleCounter*/
    }/* end bOpenBuzzer */
  }/* end u8LCDChangeTime */
} /* end UserAppSM_Idle() */
/*-------------------------------------------------------------------------------------------------------------------*/
/* Name a song */
static void UserAppSM_Name(void)          
{
  static u16 u16MusicNameNumberCount = 0;  /**/
  static u8 u8MusiNameCount = 1;           /**/
  static u8 u8LastData[] = {0};            /**/
   /* Always check for ANT messages */
  if( AntReadData() )
  {
     /* New data message: check what it is */
    if(G_eAntApiCurrentMessageClass == ANT_DATA)
    {
      /*  We got some data,judge if it is the same with the last one  */
      if(G_au8AntApiCurrentData[0] != u8LastData[0])
      {
        /* 200 means the master has finished the name */
        if(G_au8AntApiCurrentData[0] == 200)
        {
          UserApp_u16MusicNameNumber[u8MusiNameCount] = u16MusicNameNumberCount;
          u8MusiNameCount++;
          UserApp_StateMachine = UserAppSM_Idle;
        }
        else if(G_au8AntApiCurrentData[0] == 50)
        {
          /*  50 represents a music finished , do not save it in the name array*/
        }
        else if(G_au8AntApiCurrentData[0] != 30)
        {
          UserApp_u16MusicName[u16MusicNameNumberCount] = G_au8AntApiCurrentData[0];
          u16MusicNameNumberCount++;
          u8LastData[0] = G_au8AntApiCurrentData[0];
        }
        /* 30 represents the master need to send the same message with the last one */
        else
        { 
          UserApp_u16MusicName[u16MusicNameNumberCount] = UserApp_u16MusicName[u16MusicNameNumberCount - 1] ;
          u16MusicNameNumberCount++;
          u8LastData[0] = G_au8AntApiCurrentData[0];
        }
      }
    } /* end if(G_eAntApiCurrentMessageClass == ANT_DATA) */

    else if(G_eAntApiCurrentMessageClass == ANT_TICK)
    {
      
    } /* end else if(G_eAntApiCurrentMessageClass == ANT_TICK) */
  } /* end AntReadData() */
} /* end UserAppSM_Name() */

/*-------------------------------------------------------------------------------------------------------------------*/
/* Handle an error */
static void UserAppSM_Error(void)          
{
  
} /* end UserAppSM_Error() */

/*-------------------------------------------------------------------------------------------------------------------*/
/* State to sit in if init failed */
static void UserAppSM_FailedInit(void)          
{
  
} /* end UserAppSM_FailedInit() */


/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/