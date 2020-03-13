#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include <stdlib.h>
#include "buzzer.h"
#include "music.h"
#include "star.h"
#include "songMachine.h"

#define GREEN_LED BIT6

/*
 NEED SOME SHAPES AND SOME BOUNDS HERE
 
 BOUNDS
 PLAYER PADDLE
 OPPONENT PADDLE
 BALL
 */

int lalaland[28] = {C2sh, F2sh, G2sh, A2, G2sh, F2sh, D2,
                    D2, F2sh, G2sh, A2, G2sh, F2sh, C2sh,
                    C2sh, F2sh, G2sh, A2, G2sh, F2sh, D2,
                    B2, A2, G2sh, A2, G2sh, F2sh, C2sh};
char currNote = 0;

char str[6];

#define COLOR_MY_PURPLE 0x4175
#define backgroundColor COLOR_BLACK
u_int bgColor = COLOR_BLACK;     /**< The background color */
static short buzzerCount = 0;

//Some shapes
AbRect playerPaddle = {
    abRectGetBounds,
    abRectCheck,
    {1, 16}
}; /**< 2x15 paddle */

AbRect opponentPaddle = {
    abRectGetBounds,
    abRectCheck,
    {1, 16}
}; /**< 2x15 paddle */

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds,
  abRectOutlineCheck,   
  {screenWidth/2-1, screenHeight/2 - 1}
};

AbStar star = {
    abStarGetBounds, 
    abStarCheck, 
    7
};



//Some layers containing shapes

Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED,
  0
};

Layer layer2 = {		/**< Layer with a red ball */
  //(AbShape *)&circle4,
  (AbShape *)&star,
  {(screenWidth/2)+5, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GOLD,
  &fieldLayer
};

Layer layer1 = {
    (AbShape *) &opponentPaddle, /* opponent paddle, right */
    {screenWidth-10, screenHeight/2},
    {0,0}, {0,0},
    COLOR_PINK,
    &layer2
};

Layer layer0 = {
    (AbShape *) &playerPaddle, /* Player paddle, left */
    {10, screenHeight/2},
    {0,0}, {0,0},
    COLOR_BLUE,
    &layer1
};


//Some moving layers containing layers

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;


MovLayer ml2 = { &layer2, {0,0}, 0};  // ball layer
MovLayer ml1 = { &layer1, {0,0}, &ml2};  // right paddle layer
MovLayer ml0 = { &layer0, {0,0}, &ml1};  // left paddle layer


void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
        Vec2 pixelPos = {col, row};
        u_int color = bgColor;
        Layer *probeLayer;
        for (probeLayer = layers; probeLayer; probeLayer = probeLayer->next) { /* probe all layers, in order */
            if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
                color = probeLayer->color;
                break; 
            } /* if probe check */
        } // for checking all layers at col, row
        lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}

static char endGame = 0;

/*
 EDIT THIS TO COLLIDE WITH THE PADDLES AND TO RESTART THE GAME OR SOMETHING IF IT HITS THE BOUNDS
 */

void mlAdvance(MovLayer *ml, Region *fence){
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  
  int i = 0; // Which shape are we looking at? Used later on
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    

    //Checking screen edge bounds
    for (axis = 0; axis < 2; axis ++) {
        if (axis == 0 && ml->next == 0){
        //collision with left and right paddle
        
            //Collision with left paddle
            int midY = (shapeBoundary.topLeft.axes[1] + shapeBoundary.botRight.axes[1]) / 2;
            if (midY < layer0.pos.axes[1] + (16) && midY > layer0.pos.axes[1] - (16) && 
                shapeBoundary.topLeft.axes[0] < layer0.pos.axes[0]+1){
                buzzer_set_period(lalaland[currNote]);
                buzzerCount = 0;
                currNote = song_state_machine(currNote);
            
                if (currNote == 29){
                    shapeInit();
                    newPos.axes[0] = screenWidth/2;
                    newPos.axes[1] = screenHeight/2;
                    ml2.velocity.axes[0] = 0;
                    ml2.velocity.axes[1] = 0;
                    ml->layer->posNext = newPos;
                    continue;
                }
            
                int velocity = ml->velocity.axes[0] = -ml->velocity.axes[0];
                newPos.axes[0] += (2*velocity);
                ml->layer->posNext = newPos;
            
                int paddleMid = layer0.pos.axes[1];
                if (midY > paddleMid && ml->velocity.axes[1] < 0)
                    ml->velocity.axes[1] *= -1;
                else if (midY < paddleMid && ml->velocity.axes[1] > 0)
                    ml->velocity.axes[1] *= -1;
                
                if (abs(ml->velocity.axes[0]) < 7) ml->velocity.axes[0]++;
                continue;
            }
            
            //Collision with right paddle
            if (midY < layer1.pos.axes[1] + (16) && midY > layer1.pos.axes[1] - (16) && 
                shapeBoundary.botRight.axes[0] > layer1.pos.axes[0]-1) {
                buzzer_set_period(lalaland[currNote]);
                buzzerCount = 0;
                currNote = song_state_machine(currNote);
            
                if (currNote == 29){
                    shapeInit();
                    newPos.axes[0] = screenWidth/2;
                    newPos.axes[1] = screenHeight/2;
                    ml2.velocity.axes[0] = 0;
                    ml2.velocity.axes[1] = 0;
                    ml->layer->posNext = newPos;
                    continue;
                }
        
                int velocity = ml->velocity.axes[0] = -ml->velocity.axes[0];
                newPos.axes[0] += (2*velocity);
                ml->layer->posNext = newPos;
            
                int paddleMid = layer1.pos.axes[1];
                if (midY > paddleMid && ml->velocity.axes[1] < 0)
                    ml->velocity.axes[1] *= -1;
                else if (midY < paddleMid && ml->velocity.axes[1] > 0)
                    ml->velocity.axes[1] *= -1;
            
                if (abs(ml->velocity.axes[0]) < 7) ml->velocity.axes[0]--;
                continue;
            }
        }
        
        
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
          (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis])) {
        
        int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
        newPos.axes[axis] += (2*velocity);
      
        //ball collision with left or right of screen, aka loss
        if (axis == 0){
            shapeInit();
            newPos.axes[0] = screenWidth/2;
            newPos.axes[1] = screenHeight/2;
            ml2.velocity.axes[0] = 0;
            ml2.velocity.axes[1] = 0;
            endGame = 1;
        }
      }	/**< if outside of fence */
      
    } /**< for axis */
 
        ml->layer->posNext = newPos;
    } /**< for ml */
  
  //buzzer_set_period(0);
}






int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */

u_int p1Score = 0;
u_int p2Score = 0;

void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  configureClocks();
  buzzer_init();
  lcd_init();
  shapeInit();
  p2sw_init(15);

  shapeInit();

  layerInit(&layer0);
  layerDraw(&layer0);


  layerGetBounds(&fieldLayer, &fieldFence);
  
  //enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */
  
  str[0] = '0';
  str[1] = '0';
  str[2] = '/';
  str[3] = '2';
  str[4] = '5';
  str[5] = '\0';
  
  for(;;){
        movLayerDraw(&ml0, &layer0);
        drawString5x7(10, 30, "Work with a friend", COLOR_WHITE, backgroundColor);
        drawString5x7(13, 40, "to play the song!", COLOR_WHITE, backgroundColor);
        
        int switches = p2sw_read();
        if ((switches & 1<<0) == 0|| (switches & 1<<1) == 0 || (switches & 1<<2) == 0 || (switches & 1<<3) == 0) break;
    }
    drawString5x7(10, 30, "Work with a friend", backgroundColor, backgroundColor);
    drawString5x7(13, 40, "to play the song!", backgroundColor, backgroundColor);
    
    ml2.velocity.axes[0] = -5;
    ml2.velocity.axes[1] = -3;
    
    enableWDTInterrupts();
    
  for(;;) {
      if (endGame){
          currNote = 0;
        while(1){
            drawString5x7(40, 50, "You lost!", COLOR_WHITE, backgroundColor);
            drawString5x7(40, 60, "Try again", COLOR_WHITE, backgroundColor);
            int switches = p2sw_read();
            if ((switches & 1<<0) == 0 || (switches & 1<<1) == 0 || (switches & 1<<2) == 0 || (switches & 1<<3) == 0) break;
        }
        drawString5x7(40, 50, "You lost!", backgroundColor, backgroundColor);
        drawString5x7(40, 60, "Try again", backgroundColor, backgroundColor);
        endGame = 0;
        ml2.velocity.axes[0] = -5;
        ml2.velocity.axes[1] = -3;
    }
    
    if (currNote == 29){
        while(1){
            drawString5x7(40, 50, "YOU WON!!!", COLOR_WHITE, backgroundColor);
            drawString5x7(40, 60, "Play again", COLOR_WHITE, backgroundColor);
            int switches = p2sw_read();
            if ((switches & 1<<0) == 0 || (switches & 1<<1) == 0 || (switches & 1<<2) == 0 || (switches & 1<<3) == 0) break;
        }
        drawString5x7(40, 50, "YOU WON!!!", backgroundColor, backgroundColor);
        drawString5x7(40, 60, "Play again", backgroundColor, backgroundColor);
        currNote = song_state_machine(currNote);
        ml2.velocity.axes[0] = -5;
        ml2.velocity.axes[1] = -3;
    }
    
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    
    
    int switches = p2sw_read();
    
    ml0.velocity.axes[1] = (switches & 1<<0)? 0 : -10;
    ml0.velocity.axes[1] = (switches & 1<<1)? ml0.velocity.axes[1] : 10;
    
    ml1.velocity.axes[1] = (switches & 1<<2)? 0 : -10;
    ml1.velocity.axes[1] = (switches & 1<<3)? ml1.velocity.axes[1] : 10;
    
    itoa(currNote, str, 10);
    if (currNote < 10){
        str[1] = str[0];
        str[0] = '0';
    }
        str[2] = '/';
        str[3] = '2';
        str[4] = '8';
        str[5] = '\0';
    movLayerDraw(&ml0, &layer0);
    drawString5x7(50, 3, str, COLOR_WHITE, backgroundColor);
  }
  
}



/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
    static short count = 0;
    
    P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
    count ++;
    buzzerCount++;
    if (count == 15) {
        mlAdvance(&ml0, &fieldFence);
        
        redrawScreen = 1;
        count = 0;
    } 
    
    if (buzzerCount == 100){
        buzzer_set_period(0);
        buzzerCount = 0;
    }
    P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
