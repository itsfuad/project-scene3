#include <GL/glut.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <memory>
#include <functional>



const float USER_CAR_SPEED_BASE = 1.75f;             
const float USER_HUMAN_SPEED = 0.80f;                
const float USER_HUMAN_SIDEWALK_SPEED_FACTOR = 1.2f; 


const int MIN_CAR_GREEN_TIME = 300;     
const int USER_TL_YELLOW_DURATION = 90; 

const int USER_PED_WALK_START_DELAY_AFTER_RED = 60;                                                                           
const int USER_PED_WALK_DURATION = 400;                                                                                       
const int USER_PED_BLINK_DURATION = 150;                                                                                      
const int USER_TL_RED_DURATION = USER_PED_WALK_START_DELAY_AFTER_RED + USER_PED_WALK_DURATION + USER_PED_BLINK_DURATION + 60; 


const float MIN_CAR_SPACING_AHEAD = 20.0f;        
const float MIN_CAR_SPAWN_DISTANCE = 300.0f;      
const float CAR_SAME_LANE_Y_THRESHOLD = 5.0f;     
const float HUMAN_SAFETY_BUFFER = 3.0f;           
const float DISTANCE_TO_STOP_FROM_SIGNAL = 20.0f; 


const float USER_DAY_NIGHT_CYCLE_SPEED = 0.0008f; 
float currentTimeOfDay = 0.3f;                    
bool isNight = false;                             


const int MAX_ACTIVE_HUMANS = 10;          
const int HUMAN_SPAWN_RATE_SIDEWALK = 150; 
const int MIN_TIME_BETWEEN_SPAWNS = 30;    


const int CAR_SPAWN_RATE = 400;   
const float SPAWN_OFFSET = 50.0f; 


bool showWarningMessage = false;          
int warningMessageTimer = 0;              
const int WARNING_MESSAGE_DURATION = 120; 


bool autoCarMovement = true;             
bool scenePaused = false;                
int mainTrafficLightState = 1;           
int pedestrianLightState = 0;            
int trafficLightTimer = 0;               
int pedestrianLightTimer = 0;            
bool pedBlinkOn = true;                  
int pedBlinkCounter = 0;                 
bool pedestrianIsWaitingToCross = false; 
bool DEBUG_drawBoundingBoxes = false;    



const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 600;
const float ROAD_Y_BOTTOM = 150.0f;
const float ROAD_Y_TOP = 250.0f;
const float SIDEWALK_TOP_Y_START = 250.0f;
const float SIDEWALK_TOP_Y_END = 280.0f;
const float SIDEWALK_BOTTOM_Y_START = 120.0f;
const float SIDEWALK_BOTTOM_Y_END = 150.0f;

const float TRAFFIC_LIGHT_X = 600.0f;     
const int TRAFFIC_LIGHT_BOX_WIDTH = 18;                                                  
const float CAR_STOP_LINE_X = TRAFFIC_LIGHT_X - 20.0f;                                      
const float HUMAN_CROSSING_X_START = TRAFFIC_LIGHT_X + 35.0f;                               
const float HUMAN_CROSSING_WIDTH = 70.0f;                                                   
const float HUMAN_CROSSING_CENTER_X = HUMAN_CROSSING_X_START + HUMAN_CROSSING_WIDTH / 2.0f; 

const int HUMAN_ANIMATION_MAX_FRAMES = 4;   
const int HUMAN_ANIMATION_FRAME_DELAY = 12; 



const int Z_INDEX_BACKGROUND = 0;        
const int Z_INDEX_ROAD = 1;              
const int Z_INDEX_SIDEWALK = 2;          
const int Z_INDEX_TREE_TOP = 3;          
const int Z_INDEX_HUMAN_TOP = 4;         
const int Z_INDEX_LIGHT_POLE_TOP = 5;    
const int Z_INDEX_TRAFFIC_LIGHT = 5;     
const int Z_INDEX_CAR = 6;               
const int Z_INDEX_HUMAN_BOTTOM = 7;      
const int Z_INDEX_TREE_BOTTOM = 8;       
const int Z_INDEX_LIGHT_POLE_BOTTOM = 8; 
const int Z_INDEX_TEXT = 9;              


void drawTrafficSignal(float, float, int);     
void drawHumanSignal(float, float, int, bool); 
void renderText(float, float, const char *);   



struct Rect
{
    float x, y, w, h;
};

bool checkAABBCollision(Rect r1, Rect r2)
{
    return (r1.x < r2.x + r2.w &&
            r1.x + r1.w > r2.x &&
            r1.y < r2.y + r2.h &&
            r1.y + r1.h > r2.y);
}


struct Car;
struct Human;
std::vector<Car> cars;           
std::vector<Human> activeHumans; 
void reshape(int w, int h);      



void drawCircle(float cx, float cy, float r, int num_segments = 30)
{
    glBegin(GL_POLYGON);
    for (int ii = 0; ii < num_segments; ii++)
    {
        float theta = 2.0f * 3.1415926f * float(ii) / float(num_segments);
        float circ_x = r * cosf(theta);
        float circ_y = r * sinf(theta);
        glVertex2f(circ_x + cx, circ_y + cy);
    }
    glEnd();
}



struct Car
{
    float x, y;          
    float width, height; 
    float r, g, b;       
    bool isHonking;      
    int honkTimer;       
    float current_speed; 
    float speedFactor;   

    
    Car(float _x, float _y, float _w, float _h, float _r, float _g, float _b)
        : x(_x), y(_y), width(_w), height(_h), r(_r), g(_g), b(_b),
          isHonking(false), honkTimer(0), current_speed(0)
    {
        speedFactor = 0.85f + (rand() % 31) / 100.0f; 
        current_speed = USER_CAR_SPEED_BASE * speedFactor;
    }

    
    void draw() const
    {
        
        float original_r = r, original_g = g, original_b = b;
        if (isHonking && honkTimer > 0)
        {
            glColor3f(1.0f, 1.0f, 0.2f);
        }
        else
        {
            glColor3f(r, g, b);
        }
        glBegin(GL_QUADS); /*Body*/
        glVertex2f(x, y);
        glVertex2f(x + width, y);
        glVertex2f(x + width, y + height);
        glVertex2f(x, y + height);
        glEnd();
        glBegin(GL_QUADS); /*Roof*/
        glVertex2f(x + width * 0.15f, y + height);
        glVertex2f(x + width * 0.85f, y + height);
        glVertex2f(x + width * 0.75f, y + height + height * 0.4f);
        glVertex2f(x + width * 0.25f, y + height + height * 0.4f);
        glEnd();
        glColor3f(0.1f, 0.1f, 0.1f); /*Wheels*/
        drawCircle(x + width * 0.25f, y, height * 0.35f);
        drawCircle(x + width * 0.75f, y, height * 0.35f);
        if (isNight)
        {
            glColor3f(1.0f, 1.0f, 0.7f);
            glBegin(GL_POLYGON);
            glVertex2f(x + width, y + height * 0.35f);
            glVertex2f(x + width + 15, y + height * 0.35f - 5);
            glVertex2f(x + width + 15, y + height * 0.35f + 5);
            glEnd();
        }
        else
        {
            if (!(isHonking && honkTimer > 0))
                glColor3f(0.6f, 0.6f, 0.2f);
        }
        glBegin(GL_QUADS); /*Bulb*/
        glVertex2f(x + width - 3, y + height * 0.3f);
        glVertex2f(x + width, y + height * 0.3f);
        glVertex2f(x + width, y + height * 0.5f);
        glVertex2f(x + width - 3, y + height * 0.5f);
        glEnd();
        glColor3f(original_r, original_g, original_b);

        if (DEBUG_drawBoundingBoxes)
        {
            Rect b = getBounds();
            glColor3f(1.0f, 0.0f, 0.0f); 
            glBegin(GL_LINE_LOOP);
            glVertex2f(b.x, b.y);
            glVertex2f(b.x + b.w, b.y);
            glVertex2f(b.x + b.w, b.y + b.h);
            glVertex2f(b.x, b.y + b.h);
            glEnd();
        }
    }
    
    void honk()
    {
        if (!isHonking)
        {
            isHonking = true;
            honkTimer = 45;
        }
    }
    
    void updateHonk()
    {
        if (isHonking && honkTimer > 0)
        {
            honkTimer--;
            if (honkTimer == 0)
                isHonking = false;
        }
    }
    
    bool isClicked(int mouseX, int mouseY) { return mouseX >= x && mouseX <= x + width && mouseY >= y && mouseY <= y + height + height * 0.4f; }
    
    Rect getBounds() const { return {x, y, width, height}; } 
};



enum HumanState
{
    WALKING_ON_SIDEWALK_TO_CROSSING, 
    WAITING_AT_CROSSING_EDGE,        
    CROSSING_ROAD,                   
    REACHED_OTHER_SIDEWALK,          
    WALKING_AWAY_ON_SIDEWALK,        
    DESPAWNED                        
};

struct Human
{
    float x, y;                      
    float targetX, currentSidewalkY; 
    bool onBottomSidewalkInitially;  
    HumanState state;                
    int animationFrame;              
    int animationTimer;              
    float visualWidth, visualHeight; 
    float speedFactor;               

    
    Human(bool startsOnBottomSidewalk) : animationFrame(0), animationTimer(0),
                                         visualWidth(10.0f), visualHeight(40.0f) 
    {
        speedFactor = 0.85f + (rand() % 31) / 100.0f; 
        onBottomSidewalkInitially = startsOnBottomSidewalk;
        state = WALKING_ON_SIDEWALK_TO_CROSSING;
        if (startsOnBottomSidewalk)
            currentSidewalkY = (SIDEWALK_BOTTOM_Y_START + SIDEWALK_BOTTOM_Y_END) / 2.0f;
        else
            currentSidewalkY = (SIDEWALK_TOP_Y_START + SIDEWALK_TOP_Y_END) / 2.0f;
        y = currentSidewalkY;
        targetX = HUMAN_CROSSING_CENTER_X + (rand() % (int)(HUMAN_CROSSING_WIDTH / 2) - (int)(HUMAN_CROSSING_WIDTH / 4)); 
        if (rand() % 2 == 0)
            x = (float)(rand() % (int)(targetX - 50.0f));
        else
            x = targetX + 50.0f + (float)(rand() % (int)(WINDOW_WIDTH - (targetX + 50.0f)));
        if (x < visualWidth / 2.0f)
            x = visualWidth / 2.0f + 20.f;
        if (x > WINDOW_WIDTH - visualWidth / 2.0f)
            x = WINDOW_WIDTH - visualWidth / 2.0f - 20.f;
    }

    
    void update()
    {
        float effectiveSpeed = USER_HUMAN_SPEED * speedFactor;
        bool isMoving = false;

        switch (state)
        {
        case WALKING_ON_SIDEWALK_TO_CROSSING:
            isMoving = true;
            y = currentSidewalkY;
            effectiveSpeed *= USER_HUMAN_SIDEWALK_SPEED_FACTOR;
            if (fabs(x - targetX) < effectiveSpeed * 1.5f)
            {
                x = targetX;
                state = WAITING_AT_CROSSING_EDGE;
            }
            else if (x < targetX)
                x += effectiveSpeed;
            else
                x -= effectiveSpeed;
            break;
        case WAITING_AT_CROSSING_EDGE:
            
            if (pedestrianLightState == 1)
                state = CROSSING_ROAD;
            break;
        case CROSSING_ROAD:
            isMoving = true;
            if (onBottomSidewalkInitially)
            { 
                y += effectiveSpeed;
                if (y >= (SIDEWALK_TOP_Y_START + SIDEWALK_TOP_Y_END) / 2.0f)
                {
                    y = (SIDEWALK_TOP_Y_START + SIDEWALK_TOP_Y_END) / 2.0f;
                    currentSidewalkY = y;
                    state = REACHED_OTHER_SIDEWALK;
                }
            }
            else
            { 
                y -= effectiveSpeed;
                if (y <= (SIDEWALK_BOTTOM_Y_START + SIDEWALK_BOTTOM_Y_END) / 2.0f)
                {
                    y = (SIDEWALK_BOTTOM_Y_START + SIDEWALK_BOTTOM_Y_END) / 2.0f;
                    currentSidewalkY = y;
                    state = REACHED_OTHER_SIDEWALK;
                }
            }
            break;
        case REACHED_OTHER_SIDEWALK:
            targetX = (x < WINDOW_WIDTH / 2) ? -visualWidth * 2 : WINDOW_WIDTH + visualWidth * 2; 
            state = WALKING_AWAY_ON_SIDEWALK;
            break;
        case WALKING_AWAY_ON_SIDEWALK:
            isMoving = true;
            y = currentSidewalkY;
            effectiveSpeed *= USER_HUMAN_SIDEWALK_SPEED_FACTOR;
            if (fabs(x - targetX) < effectiveSpeed * 1.5f || (targetX < 0 && x < 0) || (targetX > WINDOW_WIDTH && x > WINDOW_WIDTH))
            {
                state = DESPAWNED;
            }
            else if (x < targetX)
                x += effectiveSpeed;
            else
                x -= effectiveSpeed;
            break;
        case DESPAWNED:
            break;
        }

        
        if (isMoving)
        {
            animationTimer++;
            if (animationTimer >= HUMAN_ANIMATION_FRAME_DELAY)
            {
                animationFrame = (animationFrame + 1) % HUMAN_ANIMATION_MAX_FRAMES;
                animationTimer = 0;
            }
        }
        else
        {
            animationFrame = 0;
        }
    }

    
    void draw() const
    {
        glColor3f(0.2f, 0.5f, 0.8f);
        float bodyActualHeight = visualHeight * 0.55f;
        float headRadius = visualHeight * 0.15f;
        float legLength = visualHeight * 0.30f;
        float hipY = y + legLength;
        float torsoBottomY = y + legLength;
        float torsoTopY = torsoBottomY + bodyActualHeight;
        float shoulderY = torsoTopY - bodyActualHeight * 0.1f;
        float headCenterY = torsoTopY + headRadius;
        drawCircle(x, headCenterY, headRadius); 
        glBegin(GL_QUADS);                      /*Torso*/
        glVertex2f(x - visualWidth * 0.2f, torsoBottomY);
        glVertex2f(x + visualWidth * 0.2f, torsoBottomY);
        glVertex2f(x + visualWidth * 0.2f, torsoTopY);
        glVertex2f(x - visualWidth * 0.2f, torsoTopY);
        glEnd();
        glLineWidth(2.0f);
        glBegin(GL_LINES); /*Limbs*/
        bool pose1 = animationFrame < HUMAN_ANIMATION_MAX_FRAMES / 2;
        if (pose1)
        {
            glVertex2f(x, hipY);
            glVertex2f(x - legLength * 0.4f, y);
            glVertex2f(x, hipY);
            glVertex2f(x + legLength * 0.3f, y + legLength * 0.2f);
            glVertex2f(x, shoulderY);
            glVertex2f(x + visualWidth * 0.3f, shoulderY - bodyActualHeight * 0.2f);
            glVertex2f(x, shoulderY);
            glVertex2f(x - visualWidth * 0.3f, shoulderY + bodyActualHeight * 0.2f * 0.5f);
        }
        else
        {
            glVertex2f(x, hipY);
            glVertex2f(x + legLength * 0.4f, y);
            glVertex2f(x, hipY);
            glVertex2f(x - legLength * 0.3f, y + legLength * 0.2f);
            glVertex2f(x, shoulderY);
            glVertex2f(x - visualWidth * 0.3f, shoulderY - bodyActualHeight * 0.2f);
            glVertex2f(x, shoulderY);
            glVertex2f(x + visualWidth * 0.3f, shoulderY + bodyActualHeight * 0.2f * 0.5f);
        }
        glEnd();
        glLineWidth(1.0f);

        if (DEBUG_drawBoundingBoxes)
        {
            Rect b = getBounds();
            glColor3f(0.0f, 1.0f, 0.0f); 
            glBegin(GL_LINE_LOOP);
            glVertex2f(b.x, b.y);
            glVertex2f(b.x + b.w, b.y);
            glVertex2f(b.x + b.w, b.y + b.h);
            glVertex2f(b.x, b.y + b.h);
            glEnd();
        }
    }
    
    Rect getBounds() const { return {x - visualWidth / 2.0f, y, visualWidth, visualHeight}; } 
};


struct DrawableObject
{
    int zIndex;
    virtual void draw() = 0;
    virtual ~DrawableObject() = default;
};


struct Tree : public DrawableObject
{
    float x, y;
    Tree(float _x, float _y) : x(_x), y(_y) { zIndex = Z_INDEX_TREE_TOP; }
    void draw() override
    {
        glColor3f(0.5f, 0.3f, 0.05f);
        glBegin(GL_QUADS);
        glVertex2f(x - 12, y);
        glVertex2f(x + 12, y);
        glVertex2f(x + 8, y + 70);
        glVertex2f(x - 8, y + 70);
        glEnd();
        glColor3f(0.0f, 0.45f, 0.05f);
        drawCircle(x, y + 100, 35);
        drawCircle(x - 20, y + 85, 30);
        drawCircle(x + 20, y + 85, 30);
        glColor3f(0.1f, 0.55f, 0.1f);
        drawCircle(x, y + 110, 25);
    }
};


struct StreetLamp : public DrawableObject
{
    float x, y;
    StreetLamp(float _x, float _y) : x(_x), y(_y) { zIndex = Z_INDEX_LIGHT_POLE_TOP; }
    void draw() override
    {
        glColor3f(0.5f, 0.5f, 0.5f);
        glBegin(GL_QUADS);
        glVertex2f(x - 4, y);
        glVertex2f(x + 4, y);
        glVertex2f(x + 4, y + 160);
        glVertex2f(x - 4, y + 160);
        glEnd();
        glBegin(GL_QUADS);
        glVertex2f(x, y + 160);
        glVertex2f(x + 40, y + 160);
        glVertex2f(x + 40, y + 155);
        glVertex2f(x, y + 155);
        glEnd();
        if (isNight)
        {
            glColor3f(1.0f, 1.0f, 0.7f);
            drawCircle(x + 40, y + 150, 14);
            glColor4f(1.0f, 1.0f, 0.7f, 0.3f);
            drawCircle(x + 40, y + 150, 22);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        }
        else
        {
            glColor3f(0.6f, 0.6f, 0.5f);
            drawCircle(x + 40, y + 150, 12);
        }
    }
};


struct TrafficLight : public DrawableObject
{
    float x, y;
    int state;
    TrafficLight(float _x, float _y, int _state) : x(_x), y(_y), state(_state) { zIndex = Z_INDEX_TRAFFIC_LIGHT; }
    void draw() override
    {
        drawTrafficSignal(x, y, state);
    }
};


struct HumanSignal : public DrawableObject
{
    float x, y;
    int state;
    bool blinkOn;
    HumanSignal(float _x, float _y, int _state, bool _blinkOn)
        : x(_x), y(_y), state(_state), blinkOn(_blinkOn) { zIndex = Z_INDEX_HUMAN_TOP; }
    void draw() override
    {
        drawHumanSignal(x, y, state, blinkOn);
    }
};


struct WarningText : public DrawableObject
{
    float x, y;
    const char *text;
    WarningText(float _x, float _y, const char *_text) : x(_x), y(_y), text(_text) { zIndex = Z_INDEX_TEXT; }
    void draw() override
    {
        glColor3f(1.0f, 0.0f, 0.0f);
        renderText(x, y, text);
    }
};


std::vector<std::unique_ptr<DrawableObject>> sceneObjects;


void init()
{
    glClearColor(0.52f, 0.8f, 0.92f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, WINDOW_WIDTH, 0.0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    srand(time(0));

    
    float carW = 60.0f, carH = 28.0f;
    cars.emplace_back(-50.0f, ROAD_Y_BOTTOM + 8.0f, carW, carH, 1.0f, 0.1f, 0.1f);
    cars.emplace_back(-200.0f, ROAD_Y_TOP - 8.0f - carH, carW, carH, 0.2f, 0.7f, 0.2f);
}


int lastCarSpawnTime = 0;
int lastHumanSpawnTime = 0;


void spawnNewCar()
{
    if (cars.size() >= 4)
        return;

    
    if (trafficLightTimer - lastCarSpawnTime < MIN_TIME_BETWEEN_SPAWNS)
        return;

    float carW = 60.0f, carH = 28.0f;
    bool spawnInTopLane = rand() % 2 == 0;
    float y = spawnInTopLane ? (ROAD_Y_TOP - 8.0f - carH) : (ROAD_Y_BOTTOM + 8.0f);

    
    float minX = -carW - 100.0f; 
    for (const auto &car : cars)
    {
        if (fabs(car.y - y) < CAR_SAME_LANE_Y_THRESHOLD)
        { 
            
            if (car.x > minX)
            {
                minX = car.x - MIN_CAR_SPAWN_DISTANCE;
            }
            
            else if (car.x < minX)
            {
                minX = std::max(minX, car.x + MIN_CAR_SPAWN_DISTANCE);
            }
        }
    }

    
    minX -= (rand() % 100);

    
    float r = (rand() % 10) / 10.0f;
    float g = (rand() % 10) / 10.0f;
    float b = (rand() % 10) / 10.0f;
    if (r < 0.1f && g < 0.1f && b < 0.1f)
        r = 0.5f;

    cars.emplace_back(minX, y, carW, carH, r, g, b);
    lastCarSpawnTime = trafficLightTimer;
}


void spawnNewHuman()
{
    if (activeHumans.size() >= MAX_ACTIVE_HUMANS)
        return;

    
    if (trafficLightTimer - lastHumanSpawnTime < MIN_TIME_BETWEEN_SPAWNS)
        return;

    bool startsOnBottomSidewalk = rand() % 2 == 0;
    bool comesFromLeft = rand() % 2 == 0;

    
    float x = comesFromLeft ? -100.0f - (rand() % 50) : (WINDOW_WIDTH + 100.0f + (rand() % 50));
    float y = startsOnBottomSidewalk ? (SIDEWALK_BOTTOM_Y_START + SIDEWALK_BOTTOM_Y_END) / 2.0f : (SIDEWALK_TOP_Y_START + SIDEWALK_TOP_Y_END) / 2.0f;

    Human ped(startsOnBottomSidewalk);
    ped.x = x;
    ped.y = y;
    ped.currentSidewalkY = y;

    if (comesFromLeft)
    {
        ped.targetX = HUMAN_CROSSING_CENTER_X + (rand() % (int)(HUMAN_CROSSING_WIDTH / 2));
    }
    else
    {
        ped.targetX = HUMAN_CROSSING_CENTER_X - (rand() % (int)(HUMAN_CROSSING_WIDTH / 2));
    }

    activeHumans.push_back(ped);
    lastHumanSpawnTime = trafficLightTimer;
}


void renderText(float x, float y, const char *text)
{
    glRasterPos2f(x, y);
    for (const char *c = text; *c != '\0'; c++)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}



void drawRoadAndSidewalks()
{ 
    glColor3f(0.4f, 0.4f, 0.4f);
    glBegin(GL_QUADS);
    glVertex2f(0, ROAD_Y_BOTTOM);
    glVertex2f(WINDOW_WIDTH, ROAD_Y_BOTTOM);
    glVertex2f(WINDOW_WIDTH, ROAD_Y_TOP);
    glVertex2f(0, ROAD_Y_TOP);
    glEnd();
    glColor3f(0.75f, 0.75f, 0.75f);
    glBegin(GL_QUADS);
    glVertex2f(0, SIDEWALK_TOP_Y_START);
    glVertex2f(WINDOW_WIDTH, SIDEWALK_TOP_Y_START);
    glVertex2f(WINDOW_WIDTH, SIDEWALK_TOP_Y_END);
    glVertex2f(0, SIDEWALK_TOP_Y_END);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2f(0, SIDEWALK_BOTTOM_Y_START);
    glVertex2f(WINDOW_WIDTH, SIDEWALK_BOTTOM_Y_START);
    glVertex2f(WINDOW_WIDTH, SIDEWALK_BOTTOM_Y_END);
    glVertex2f(0, SIDEWALK_BOTTOM_Y_END);
    glEnd();
    glColor3f(0.9f, 0.9f, 0.9f);
    float road_center_y = (ROAD_Y_BOTTOM + ROAD_Y_TOP) / 2.0f;
    for (float i = 0; i < WINDOW_WIDTH; i += 60)
    {
        glBegin(GL_QUADS);
        glVertex2f(i, road_center_y - 1);
        glVertex2f(i + 30, road_center_y - 1);
        glVertex2f(i + 30, road_center_y + 1);
        glVertex2f(i, road_center_y + 1);
        glEnd();
    }
}
void drawZebraCrossing(float road_y_bottom, float road_y_top, float crossing_area_x_start, float crossing_area_width)
{ 
    glColor3f(0.95f, 0.95f, 0.95f);
    float offset = -5.0f;
    //draw 10 stripes on x axis
    const int stripCount = 10;
    const float stripe_height = (road_y_top - road_y_bottom) / stripCount;
    for (int i = 0; i < stripCount; i++)
    {
        if (i % 2 == 0) continue;
        glBegin(GL_QUADS);
        glVertex2f(crossing_area_x_start, (road_y_bottom + i * stripe_height) + offset);
        glVertex2f(crossing_area_x_start + crossing_area_width, (road_y_bottom + i * stripe_height) + offset);
        glVertex2f(crossing_area_x_start + crossing_area_width, (road_y_bottom + (i + 1) * stripe_height) + offset);
        glVertex2f(crossing_area_x_start, (road_y_bottom + (i + 1) * stripe_height) + offset);
        glEnd();
    }
}
void drawTrafficSignal(float x_pos, float y_pos_on_sidewalk, int state)
{ 
    glColor3f(0.25f, 0.25f, 0.25f);
    
    glBegin(GL_QUADS);
    glVertex2f(x_pos - 5, y_pos_on_sidewalk);
    glVertex2f(x_pos + 5, y_pos_on_sidewalk);
    glVertex2f(x_pos + 5, y_pos_on_sidewalk + 90);
    glVertex2f(x_pos - 5, y_pos_on_sidewalk + 90);
    glEnd();
    glColor3f(0.15f, 0.15f, 0.15f);
    glBegin(GL_QUADS);
    glVertex2f(x_pos - TRAFFIC_LIGHT_BOX_WIDTH, y_pos_on_sidewalk + 60);
    glVertex2f(x_pos + TRAFFIC_LIGHT_BOX_WIDTH, y_pos_on_sidewalk + 60);
    glVertex2f(x_pos + TRAFFIC_LIGHT_BOX_WIDTH, y_pos_on_sidewalk + 150);
    glVertex2f(x_pos - TRAFFIC_LIGHT_BOX_WIDTH, y_pos_on_sidewalk + 150);
    glEnd();
    float lightRadius = 8.0f;
    float lightYOffset = 28.0f;
    float topLightY = y_pos_on_sidewalk + 130;
    if (state == 0)
        glColor3f(1.0f, 0.0f, 0.0f);
    else
        glColor3f(0.4f, 0.05f, 0.05f);
    drawCircle(x_pos, topLightY, lightRadius);
    if (state == 2)
        glColor3f(1.0f, 1.0f, 0.0f);
    else
        glColor3f(0.4f, 0.4f, 0.05f);
    drawCircle(x_pos, topLightY - lightYOffset, lightRadius);
    if (state == 1)
        glColor3f(0.0f, 1.0f, 0.0f);
    else
        glColor3f(0.05f, 0.4f, 0.05f);
    drawCircle(x_pos, topLightY - 2 * lightYOffset, lightRadius);
}
void drawHumanSignal(float x_pos, float y_pos_on_sidewalk, int state, bool blinkOn)
{ 
    glColor3f(0.25f, 0.25f, 0.25f);
    glBegin(GL_QUADS);
    glVertex2f(x_pos - 4, y_pos_on_sidewalk);
    glVertex2f(x_pos + 4, y_pos_on_sidewalk);
    glVertex2f(x_pos + 4, y_pos_on_sidewalk + 40);
    glVertex2f(x_pos - 4, y_pos_on_sidewalk + 40);
    glEnd();
    glColor3f(0.15f, 0.15f, 0.15f);
    glBegin(GL_QUADS);
    glVertex2f(x_pos - 18, y_pos_on_sidewalk + 20);
    glVertex2f(x_pos + 18, y_pos_on_sidewalk + 20);
    glVertex2f(x_pos + 18, y_pos_on_sidewalk + 70);
    glVertex2f(x_pos - 18, y_pos_on_sidewalk + 70);
    glEnd();
    float iconX = x_pos;
    float iconY = y_pos_on_sidewalk + 45;
    if (state == 1)
    {
        glColor3f(0.1f, 0.9f, 0.1f);
        drawCircle(iconX, iconY + 10, 4);
        glBegin(GL_QUADS);
        glVertex2f(iconX - 3, iconY - 8);
        glVertex2f(iconX + 3, iconY - 8);
        glVertex2f(iconX + 3, iconY + 5);
        glVertex2f(iconX - 3, iconY + 5);
        glEnd();
        glBegin(GL_LINES);
        glVertex2f(iconX, iconY - 8);
        glVertex2f(iconX - 5, iconY - 15);
        glVertex2f(iconX, iconY - 8);
        glVertex2f(iconX + 5, iconY - 15);
        glEnd();
    }
    else
    {
        if (state == 2)
        {
            if (blinkOn)
                glColor3f(1.0f, 0.2f, 0.0f);
            else
                glColor3f(0.4f, 0.1f, 0.0f);
        }
        else
        {
            glColor3f(1.0f, 0.1f, 0.0f);
        }
        glBegin(GL_QUADS);
        glVertex2f(iconX - 6, iconY - 5);
        glVertex2f(iconX + 6, iconY - 5);
        glVertex2f(iconX + 6, iconY + 5);
        glVertex2f(iconX - 6, iconY + 5);
        glEnd();
        for (int i = 0; i < 4; ++i)
        {
            glBegin(GL_QUADS);
            glVertex2f(iconX - 5 + i * 3, iconY + 5);
            glVertex2f(iconX - 3 + i * 3, iconY + 5);
            glVertex2f(iconX - 3 + i * 3, iconY + 10);
            glVertex2f(iconX - 5 + i * 3, iconY + 10);
            glEnd();
        }
    }
}
void drawStreetLamp(float x_pos, float y_pos_on_sidewalk)
{
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(x_pos - 4, y_pos_on_sidewalk);
    glVertex2f(x_pos + 4, y_pos_on_sidewalk);
    glVertex2f(x_pos + 4, y_pos_on_sidewalk + 160);
    glVertex2f(x_pos - 4, y_pos_on_sidewalk + 160);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2f(x_pos, y_pos_on_sidewalk + 160);
    glVertex2f(x_pos + 40, y_pos_on_sidewalk + 160);
    glVertex2f(x_pos + 40, y_pos_on_sidewalk + 155);
    glVertex2f(x_pos, y_pos_on_sidewalk + 155);
    glEnd();
    if (isNight)
    {
        glColor3f(1.0f, 1.0f, 0.7f);
        drawCircle(x_pos + 40, y_pos_on_sidewalk + 150, 14);
        glColor4f(1.0f, 1.0f, 0.7f, 0.3f);
        drawCircle(x_pos + 40, y_pos_on_sidewalk + 150, 22);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
    {
        glColor3f(0.6f, 0.6f, 0.5f);
        drawCircle(x_pos + 40, y_pos_on_sidewalk + 150, 12);
    }
}
void drawTreeTrunk(float x_pos, float y_pos_on_sidewalk)
{
    glColor3f(0.5f, 0.3f, 0.05f);
    glBegin(GL_QUADS);
    glVertex2f(x_pos - 12, y_pos_on_sidewalk);
    glVertex2f(x_pos + 12, y_pos_on_sidewalk);
    glVertex2f(x_pos + 8, y_pos_on_sidewalk + 70);
    glVertex2f(x_pos - 8, y_pos_on_sidewalk + 70);
    glEnd();
}
void drawTreeCanopy(float x_pos, float y_pos_on_sidewalk)
{
    glColor3f(0.0f, 0.45f, 0.05f);
    drawCircle(x_pos, y_pos_on_sidewalk + 100, 35);
    drawCircle(x_pos - 20, y_pos_on_sidewalk + 85, 30);
    drawCircle(x_pos + 20, y_pos_on_sidewalk + 85, 30);
    glColor3f(0.1f, 0.55f, 0.1f);
    drawCircle(x_pos, y_pos_on_sidewalk + 110, 25);
}

// Helper to draw a clump of grass
void drawGrass(float x, float y, float scale = 1.0f) {
    glColor3f(0.2f, 0.7f, 0.2f);
    glBegin(GL_LINES);
    for (int i = -2; i <= 2; ++i) {
        glVertex2f(x, y);
        glVertex2f(x + i * 2 * scale, y + 12 * scale - abs(i) * 2 * scale);
    }
    glEnd();
}
// Helper to draw a simple flower
void drawFlower(float x, float y, float scale = 1.0f) {
    // Stem
    glColor3f(0.2f, 0.7f, 0.2f);
    glBegin(GL_LINES);
    glVertex2f(x, y);
    glVertex2f(x, y + 10 * scale);
    glEnd();
    // Petals
    glColor3f(1.0f, 0.7f, 0.2f); // yellow/orange
    for (int i = 0; i < 5; ++i) {
        float angle = i * 2 * 3.14159f / 5;
        float px = x + cosf(angle) * 4 * scale;
        float py = y + 10 * scale + sinf(angle) * 4 * scale;
        drawCircle(px, py, 2.0f * scale, 8);
    }
    // Center
    glColor3f(1.0f, 0.2f, 0.5f); // pink center
    drawCircle(x, y + 10 * scale, 2.2f * scale, 10);
}


void display()
{
    float dayR = 0.52f, dayG = 0.8f, dayB = 0.92f;
    float duskR = 0.8f, duskG = 0.5f, duskB = 0.4f;
    float nightR = 0.05f, nightG = 0.05f, nightB = 0.2f;
    float r, g, b;

    if (currentTimeOfDay >= 0.0 && currentTimeOfDay < 0.20)
    {
        float t = currentTimeOfDay / 0.20f;
        r = nightR * (1 - t) + dayR * t * 0.5f;
        g = nightG * (1 - t) + dayG * t * 0.5f;
        b = nightB * (1 - t) + dayB * t * 0.5f;
    }
    else if (currentTimeOfDay >= 0.20 && currentTimeOfDay < 0.45)
    {
        r = dayR;
        g = dayG;
        b = dayB;
    }
    else if (currentTimeOfDay >= 0.45 && currentTimeOfDay < 0.70)
    {
        float t = (currentTimeOfDay - 0.45f) / 0.25f;
        r = dayR * (1 - t) + duskR * t;
        g = dayG * (1 - t) + duskG * t;
        b = dayB * (1 - t) + duskB * t;
    }
    else if (currentTimeOfDay >= 0.70 && currentTimeOfDay < 0.85)
    {
        float t = (currentTimeOfDay - 0.70f) / 0.15f;
        r = duskR * (1 - t) + nightR * t;
        g = duskG * (1 - t) + nightG * t;
        b = duskB * (1 - t) + nightB * t;
    }
    else
    {
        r = nightR;
        g = nightG;
        b = nightB;
    }
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    glColor3f(0.35f, 0.7f, 0.25f); // green ground
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(WINDOW_WIDTH, 0);
    glVertex2f(WINDOW_WIDTH, SIDEWALK_BOTTOM_Y_START);
    glVertex2f(0, SIDEWALK_BOTTOM_Y_START);
    glEnd();
    // Draw grass clumps
    for (float gx = 20; gx < WINDOW_WIDTH; gx += 40 + rand() % 30) {
        float gy = 10 + rand() % (int)(SIDEWALK_BOTTOM_Y_START * 0.5f);
        drawGrass(gx, gy, 0.7f + (rand() % 10) / 20.0f);
    }
    // Draw flowers
    for (float fx = 35; fx < WINDOW_WIDTH; fx += 60 + rand() % 40) {
        float fy = 18 + rand() % (int)(SIDEWALK_BOTTOM_Y_START * 0.5f);
        drawFlower(fx, fy, 0.7f + (rand() % 10) / 20.0f);
    }
    
    drawRoadAndSidewalks();
    drawZebraCrossing(ROAD_Y_BOTTOM, ROAD_Y_TOP, HUMAN_CROSSING_X_START, HUMAN_CROSSING_WIDTH);

    
    struct SceneDrawItem
    {
        int zIndex;
        float y;
        std::function<void()> drawFunc;
    };
    std::vector<SceneDrawItem> drawItems;

    
    auto addTree = [&](float x, float y)
    {
        if (y > ROAD_Y_TOP)
            drawItems.push_back({Z_INDEX_TREE_TOP, y, [=]()
                                 { drawTreeTrunk(x, y); drawTreeCanopy(x, y); }});
        else if (y < ROAD_Y_BOTTOM)
            drawItems.push_back({Z_INDEX_TREE_BOTTOM, y, [=]()
                                 { drawTreeTrunk(x, y); drawTreeCanopy(x, y); }});
        else
            drawItems.push_back({Z_INDEX_TREE_TOP, y, [=]()
                                 { drawTreeTrunk(x, y); drawTreeCanopy(x, y); }});
    };
    addTree(180, SIDEWALK_TOP_Y_START + 30);
    addTree(500, SIDEWALK_TOP_Y_START + 30);
    addTree(850, SIDEWALK_BOTTOM_Y_START);

    
    drawItems.push_back({Z_INDEX_TRAFFIC_LIGHT, SIDEWALK_TOP_Y_START, []()
                         { drawTrafficSignal(TRAFFIC_LIGHT_X, SIDEWALK_TOP_Y_START, mainTrafficLightState); }});
    drawItems.push_back({Z_INDEX_TRAFFIC_LIGHT, SIDEWALK_TOP_Y_START, []()
                         { drawHumanSignal(TRAFFIC_LIGHT_X + 120, SIDEWALK_TOP_Y_START, pedestrianLightState, pedBlinkOn); }});

    
    drawItems.push_back({Z_INDEX_LIGHT_POLE_TOP, SIDEWALK_TOP_Y_START, []()
                         { drawStreetLamp(80, SIDEWALK_TOP_Y_START); }});
    drawItems.push_back({Z_INDEX_LIGHT_POLE_TOP, SIDEWALK_TOP_Y_START, []()
                         { drawStreetLamp(380, SIDEWALK_TOP_Y_START); }});
    drawItems.push_back({Z_INDEX_LIGHT_POLE_TOP, SIDEWALK_TOP_Y_START, []()
                         { drawStreetLamp(780, SIDEWALK_TOP_Y_START); }});
    drawItems.push_back({Z_INDEX_LIGHT_POLE_BOTTOM, SIDEWALK_BOTTOM_Y_START, []()
                         { drawStreetLamp(950, SIDEWALK_BOTTOM_Y_START); }});

    
    for (const auto &p : activeHumans)
    {
        if (p.y > ROAD_Y_TOP)
            drawItems.push_back({Z_INDEX_HUMAN_TOP, p.y, [&p]()
                                 { p.draw(); }});
        else if (p.y < ROAD_Y_BOTTOM)
            drawItems.push_back({Z_INDEX_HUMAN_BOTTOM, p.y, [&p]()
                                 { p.draw(); }});
        else
            drawItems.push_back({Z_INDEX_HUMAN_TOP, p.y, [&p]()
                                 { p.draw(); }});
    }
    
    for (const auto &car : cars)
    {
        drawItems.push_back({Z_INDEX_CAR, car.y, [&car]()
                             { car.draw(); }});
    }

    
    std::sort(drawItems.begin(), drawItems.end(), [](const SceneDrawItem &a, const SceneDrawItem &b)
              {
        if (a.zIndex != b.zIndex) return a.zIndex < b.zIndex;
        return a.y < b.y; });
    for (const auto &item : drawItems)
        item.drawFunc();

    
    if (showWarningMessage)
    {
        glColor3f(1.0f, 0.7f, 0.7f);
        renderText(TRAFFIC_LIGHT_X - 100, SIDEWALK_TOP_Y_START + 180, "People are still passing the road");
    }

    glDisable(GL_BLEND);
    glutSwapBuffers();
}


void updateScene()
{
    
    currentTimeOfDay += USER_DAY_NIGHT_CYCLE_SPEED;
    if (currentTimeOfDay >= 1.0f)
        currentTimeOfDay -= 1.0f;
    isNight = (currentTimeOfDay > 0.75 || currentTimeOfDay < 0.22);

    
    pedestrianIsWaitingToCross = false;
    for (const auto &ped : activeHumans)
    {
        if (ped.state == WAITING_AT_CROSSING_EDGE)
        {
            pedestrianIsWaitingToCross = true;
            break;
        }
    }

    
    for (size_t i = 0; i < cars.size(); ++i)
    {
        Car &car1 = cars[i];
        float targetSpeed = USER_CAR_SPEED_BASE * car1.speedFactor * (car1.y > (ROAD_Y_BOTTOM + ROAD_Y_TOP) / 2.0f ? 0.90f : 1.0f); 
        car1.current_speed = targetSpeed;                                                                                           

        bool stoppedByLight = false;
        
        if (mainTrafficLightState == 0 || mainTrafficLightState == 2)
        { 
            
            if (car1.x + car1.width < CAR_STOP_LINE_X + 5.0f &&
                car1.x + car1.width > CAR_STOP_LINE_X - DISTANCE_TO_STOP_FROM_SIGNAL)
            { 
                car1.current_speed = 0;
                stoppedByLight = true;
            }
        }

        bool blockedByCarAhead = false;
        if (autoCarMovement && !stoppedByLight)
        { 
            for (size_t j = 0; j < cars.size(); ++j)
            {
                if (i == j)
                    continue;
                Car &car2 = cars[j];
                if (fabs(car1.y - car2.y) < CAR_SAME_LANE_Y_THRESHOLD)
                { 
                    if (car2.x > car1.x)
                    { 
                        float distance = car2.x - (car1.x + car1.width);
                        if (distance < MIN_CAR_SPACING_AHEAD)
                        {
                            if (car2.current_speed < car1.current_speed)
                            {                                            
                                car1.current_speed = car2.current_speed; 
                            }
                            if (car2.current_speed < 0.1f && distance < MIN_CAR_SPACING_AHEAD * 0.75f)
                            { 
                                car1.current_speed = 0;
                                blockedByCarAhead = true; 
                            }
                            if (distance < 2.0f)
                            { 
                                car1.current_speed = 0;
                                blockedByCarAhead = true;
                            }
                        }
                    }
                }
            }
        }

        bool blockedByHuman = false;
        
        if (autoCarMovement && !stoppedByLight && !blockedByCarAhead && car1.y < (ROAD_Y_BOTTOM + ROAD_Y_TOP) / 2.0f)
        {
            Rect carNextBounds = {car1.x + car1.current_speed, car1.y, car1.width, car1.height};
            
            Rect crossingAreaForCarCheck = {HUMAN_CROSSING_X_START - car1.width / 2.0f, ROAD_Y_BOTTOM,
                                            HUMAN_CROSSING_WIDTH + car1.width, ROAD_Y_TOP - ROAD_Y_BOTTOM};

            if (checkAABBCollision(carNextBounds, crossingAreaForCarCheck))
            {
                for (const auto &ped : activeHumans)
                {
                    if (ped.state == CROSSING_ROAD)
                    { 
                        Rect pedBounds = ped.getBounds();
                        
                        Rect carCheckBounds = {carNextBounds.x + HUMAN_SAFETY_BUFFER, carNextBounds.y + HUMAN_SAFETY_BUFFER,
                                               carNextBounds.w - 2 * HUMAN_SAFETY_BUFFER, carNextBounds.h - 2 * HUMAN_SAFETY_BUFFER};

                        if (checkAABBCollision(carCheckBounds, pedBounds))
                        {
                            blockedByHuman = true;
                            car1.current_speed = 0;
                            break;
                        }
                    }
                }
            }
        }

        if (autoCarMovement)
        {
            if (stoppedByLight || blockedByCarAhead || blockedByHuman)
            {
                car1.current_speed = 0;
            }
            car1.x += car1.current_speed;
        }

        if (car1.x > WINDOW_WIDTH + car1.width + 20)
        { 
            
            float respawnX = -car1.width - 100.0f;
            for (const auto &car2 : cars)
            {
                if (fabs(car1.y - car2.y) < CAR_SAME_LANE_Y_THRESHOLD)
                { 
                    if (car2.x < respawnX)
                    {
                        respawnX = std::max(respawnX, car2.x + MIN_CAR_SPAWN_DISTANCE);
                    }
                }
            }
            car1.x = respawnX - (rand() % 100);                
            car1.speedFactor = 0.85f + (rand() % 31) / 100.0f; 
            car1.r = (rand() % 10) / 10.0f;
            car1.g = (rand() % 10) / 10.0f;
            car1.b = (rand() % 10) / 10.0f; 
            if (car1.r < 0.1f && car1.g < 0.1f && car1.b < 0.1f)
                car1.r = 0.5f; 
        }
        car1.updateHonk();
    }

    
    if (showWarningMessage)
    {
        warningMessageTimer--;
        if (warningMessageTimer <= 0)
        {
            showWarningMessage = false;
        }
    }

    
    trafficLightTimer++;
    if (mainTrafficLightState == 1)
    { 
        if (pedestrianIsWaitingToCross && trafficLightTimer > MIN_CAR_GREEN_TIME)
        {
            
            bool pedestrianCrossing = false;
            for (const auto &ped : activeHumans)
            {
                if (ped.state == CROSSING_ROAD)
                {
                    pedestrianCrossing = true;
                    break;
                }
            }

            if (!pedestrianCrossing)
            {
                mainTrafficLightState = 2; 
                trafficLightTimer = 0;
            }
        }
    }
    else if (mainTrafficLightState == 2)
    { 
        if (trafficLightTimer > USER_TL_YELLOW_DURATION)
        {
            mainTrafficLightState = 0; 
            trafficLightTimer = 0;
            pedestrianLightState = 0; 
            pedestrianLightTimer = 0;
        }
    }
    else if (mainTrafficLightState == 0)
    { 
        
        if (trafficLightTimer > USER_PED_WALK_START_DELAY_AFTER_RED && pedestrianLightState == 0)
        {
            bool anyPedStillWaiting = false; 
            for (const auto &ped : activeHumans)
                if (ped.state == WAITING_AT_CROSSING_EDGE)
                    anyPedStillWaiting = true;

            if (anyPedStillWaiting || pedestrianIsWaitingToCross)
            {                             
                pedestrianLightState = 1; 
                pedestrianLightTimer = 0;
                pedestrianIsWaitingToCross = false; 
            }
            else if (trafficLightTimer > USER_PED_WALK_START_DELAY_AFTER_RED + 100 && pedestrianLightState == 0)
            {
                
                
                if (trafficLightTimer > USER_PED_WALK_START_DELAY_AFTER_RED + USER_PED_WALK_DURATION / 2)
                {                              
                    mainTrafficLightState = 1; 
                    trafficLightTimer = 0;
                }
            }
        }
        
        if (pedestrianLightState == 1)
        { 
            pedestrianLightTimer++;
            if (pedestrianLightTimer > USER_PED_WALK_DURATION)
            {
                pedestrianLightState = 2; 
                pedestrianLightTimer = 0;
                pedBlinkCounter = 0;
                pedBlinkOn = true;
            }
        }
        else if (pedestrianLightState == 2)
        { 
            pedestrianLightTimer++;
            pedBlinkCounter = (pedBlinkCounter + 1) % 25;
            if (pedBlinkCounter == 0)
                pedBlinkOn = !pedBlinkOn;
            if (pedestrianLightTimer > USER_PED_BLINK_DURATION)
            {
                pedestrianLightState = 0; 
                pedBlinkOn = true;
            }
        }

        
        if (trafficLightTimer > USER_TL_RED_DURATION && pedestrianLightState == 0)
        {                              
            mainTrafficLightState = 1; 
            trafficLightTimer = 0;
        }
    }

    
    if (cars.size() < 4 && rand() % CAR_SPAWN_RATE == 0)
    {
        spawnNewCar();
    }

    
    if (activeHumans.size() < MAX_ACTIVE_HUMANS && rand() % HUMAN_SPAWN_RATE_SIDEWALK == 0)
    {
        spawnNewHuman();
    }

    
    for (auto &p : activeHumans)
        p.update();
    activeHumans.erase(
        std::remove_if(activeHumans.begin(), activeHumans.end(),
                       [](const Human &p)
                       { return p.state == DESPAWNED; }),
        activeHumans.end());
}


void timer(int)
{ 
    if (!scenePaused)
    {
        updateScene();
    }
    glutPostRedisplay();
    glutTimerFunc(1000 / 60, timer, 0);
}
void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 'c':
    case 'C':
        autoCarMovement = !autoCarMovement;
        break;
    case 'p':
    case 'P':
        scenePaused = !scenePaused;
        break;
    case 'b':
    case 'B':
        DEBUG_drawBoundingBoxes = !DEBUG_drawBoundingBoxes;
        break; 
    case 'd':
    case 'D':
        if (isNight)
        {
            currentTimeOfDay = 0.3f; 
            isNight = false;
        }
        else
        {
            currentTimeOfDay = 0.8f; 
            isNight = true;
        }
        break;
    case 27:
        exit(0);
        break;
    }
    glutPostRedisplay();
}
void mouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        int glutY = WINDOW_HEIGHT - y;
        float sigClickY_min = SIDEWALK_TOP_Y_START + 60;
        float sigClickY_max = SIDEWALK_TOP_Y_START + 150;

        if (x >= TRAFFIC_LIGHT_X - TRAFFIC_LIGHT_BOX_WIDTH && x <= TRAFFIC_LIGHT_X + TRAFFIC_LIGHT_BOX_WIDTH &&
            glutY >= sigClickY_min && glutY <= sigClickY_max)
        {

            
            bool pedestrianCrossing = false;
            for (const auto &ped : activeHumans)
            {
                if (ped.state == CROSSING_ROAD)
                {
                    pedestrianCrossing = true;
                    break;
                }
            }

            if (pedestrianCrossing)
            {
                showWarningMessage = true;
                warningMessageTimer = WARNING_MESSAGE_DURATION;
            }
            else
            {
                mainTrafficLightState = (mainTrafficLightState + 1) % 3;
                trafficLightTimer = 0;
                pedestrianLightTimer = 0;
                if (mainTrafficLightState == 1)
                {
                    pedestrianLightState = 0;
                    pedestrianIsWaitingToCross = false;
                }
                std::cout << "Traffic Light Manually Cycled." << std::endl;
            }
        }

        for (auto &car : cars)
        {
            if (car.isClicked(x, glutY))
                car.honk();
        }
    }
}


void reshape(int w, int h)
{ 
    if (h == 0)
        h = 1;
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, WINDOW_WIDTH, 0.0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void transitionToScene4() { std::cout << "Transitioning to Scene 4..." << std::endl; }


int main(int argc, char **argv)
{ 
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_ALPHA);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Scene 3: Enhanced Traffic System");
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutTimerFunc(0, timer, 0);
    glutMainLoop();
    return 0;
}
