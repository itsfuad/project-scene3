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

// Add this after the other enums
enum class TrafficLightState {
    RED,
    YELLOW,
    GREEN,
};

// Replace the global variable
TrafficLightState mainTrafficLightState = TrafficLightState::GREEN;

// Add this after the TrafficLightState enum
enum class PedestrianLightState {
    RED,
    WALK,
    BLINK
};

// Replace the global variable
PedestrianLightState pedestrianLightState = PedestrianLightState::RED;

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
const int TRAFFIC_LIGHT_BOX_WIDTH = 20;
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
void drawTrafficSignal(float x, float y, TrafficLightState state);

void drawText(float x, float y, const char* text, float scale = 0.7f) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);
    glRasterPos2f(0, 0);
    for (const char* p = text; *p; p++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *p);  // Changed from 18 to 12 for smaller size
    }
    glPopMatrix();
}

void drawRect(float x, float y, float width, float height) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

void drawLine(float x1, float y1, float x2, float y2, float thickness = 1.0f) {
    glLineWidth(thickness);
    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
}

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

void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3) {
    glBegin(GL_TRIANGLES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glVertex2f(x3, y3);
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

        // Draw backlights (red) when car is stopped
        if (current_speed < 0.1f) {
            // Backlight glow
            glColor4f(1.0f, 0.0f, 0.0f, 0.3f);
            drawCircle(x, y + height * 0.35f, 15.0f);

            // Backlight bulb
            glColor3f(1.0f, 0.0f, 0.0f);
            glBegin(GL_QUADS);
            glVertex2f(x + 3, y + height * 0.3f);
            glVertex2f(x, y + height * 0.3f);
            glVertex2f(x, y + height * 0.5f);
            glVertex2f(x + 3, y + height * 0.5f);
            glEnd();
        }

        if (isNight)
        {
            // Headlight glow
            glColor4f(1.0f, 1.0f, 0.7f, 0.3f);
            drawCircle(x + width, y + height * 0.35f, 25.0f);

            // Headlight beam
            glColor4f(1.0f, 1.0f, 0.7f, 0.2f);
            glBegin(GL_TRIANGLES);
            glVertex2f(x + width, y + height * 0.35f);
            glVertex2f(x + width + 50.0f, y + height * 0.35f - 15.0f);
            glVertex2f(x + width + 50.0f, y + height * 0.35f + 15.0f);
            glEnd();

            // Headlight bulb
            glColor3f(1.0f, 1.0f, 0.7f);
            glBegin(GL_QUADS);
            glVertex2f(x + width - 3, y + height * 0.3f);
            glVertex2f(x + width, y + height * 0.3f);
            glVertex2f(x + width, y + height * 0.5f);
            glVertex2f(x + width - 3, y + height * 0.5f);
            glEnd();
        }
        else
        {
            if (!(isHonking && honkTimer > 0))
                glColor3f(0.6f, 0.6f, 0.2f);
            glBegin(GL_QUADS); /*Bulb*/
            glVertex2f(x + width - 3, y + height * 0.3f);
            glVertex2f(x + width, y + height * 0.3f);
            glVertex2f(x + width, y + height * 0.5f);
            glVertex2f(x + width - 3, y + height * 0.5f);
            glEnd();
        }
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

            if (pedestrianLightState == PedestrianLightState::WALK)
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

// Add these global variables after the other global variables
struct Cloud {
    float x, y;
    float speed;
    float scale;
    Cloud(float _x, float _y, float _scale) : x(_x), y(_y), scale(_scale) {
        speed = 0.2f + (rand() % 30) / 100.0f; // Random speed between 0.2 and 0.5
    }
};
std::vector<Cloud> clouds;

// Add this struct after the Cloud struct
struct Star {
    float x, y;
    float size;
    float baseBrightness;
    float blinkPhase;
    Star(float _x, float _y, float _size) :
        x(_x), y(_y), size(_size),
        baseBrightness(0.7f + (rand() % 30) / 100.0f), // Random base brightness between 0.7 and 1.0
        blinkPhase(rand() % 628) // Random starting phase (0 to 2π)
    {}
};

// Add this global variable after the clouds vector
std::vector<Star> stars;

void drawCloud(float x, float y, float scale) {
    glColor3f(0.9f, 0.9f, 0.95f);
    float r = 15.0f * scale;
    drawCircle(x, y, r);
    drawCircle(x + r * 0.8f, y, r * 0.9f);
    drawCircle(x - r * 0.8f, y, r * 0.9f);
    drawCircle(x + r * 0.4f, y + r * 0.3f, r * 0.8f);
    drawCircle(x - r * 0.4f, y + r * 0.3f, r * 0.8f);
}

// Replace the drawStar function with this new one
void drawStars() {
    // Stars should be visible during night (currentTimeOfDay > 0.7 or < 0.2)
    // and fade in/out during transitions
    float starAlpha = 0.0f;

    if (currentTimeOfDay >= 0.7f) {
        // Night time - full visibility
        starAlpha = 1.0f;
    } else if (currentTimeOfDay >= 0.6f) {
        // Smoother transition to night
        starAlpha = (currentTimeOfDay - 0.6f) / 0.1f;
    } else if (currentTimeOfDay <= 0.2f) {
        // Night time - full visibility
        starAlpha = 1.0f;
    } else if (currentTimeOfDay <= 0.3f) {
        // Smoother transition to day
        starAlpha = (0.3f - currentTimeOfDay) / 0.1f;
    }

    if (starAlpha > 0.0f) {
        for (auto& star : stars) {
            // Update blink phase
            star.blinkPhase += 0.05f;
            if (star.blinkPhase > 628.0f) star.blinkPhase -= 628.0f;

            // Calculate blinking brightness
            float blinkFactor = 0.5f + 0.5f * sin(star.blinkPhase);
            float finalBrightness = star.baseBrightness * blinkFactor;

            glColor4f(1.0f, 1.0f, 0.9f, starAlpha * finalBrightness);
            glPointSize(star.size);
            glBegin(GL_POINTS);
            glVertex2f(star.x, star.y);
            glEnd();
        }
    }
}

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

    // Initialize clouds
    clouds.clear();
    clouds.emplace_back(WINDOW_WIDTH * 0.2f, WINDOW_HEIGHT * 0.85f, 1.0f);
    clouds.emplace_back(WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.9f, 1.2f);
    clouds.emplace_back(WINDOW_WIDTH * 0.7f, WINDOW_HEIGHT * 0.82f, 0.9f);

    // Initialize stars
    stars.clear();
    for (int i = 0; i < 100; i++) {
        float x = (rand() % WINDOW_WIDTH);
        float y = WINDOW_HEIGHT * (0.7f + (rand() % 30) / 100.0f); // Between 70% and 100% of height
        float size = 1.0f + (rand() % 30) / 10.0f; // Random size between 1.0 and 4.0
        stars.emplace_back(x, y, size);
    }
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

void drawStreetLamp(float x_pos, float y_pos_on_sidewalk)
{
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(x_pos - 3, y_pos_on_sidewalk);
    glVertex2f(x_pos + 3, y_pos_on_sidewalk);
    glVertex2f(x_pos + 3, y_pos_on_sidewalk + 120);  // Reduced from 160
    glVertex2f(x_pos - 3, y_pos_on_sidewalk + 120);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2f(x_pos, y_pos_on_sidewalk + 120);
    glVertex2f(x_pos + 30, y_pos_on_sidewalk + 120);  // Reduced from 40
    glVertex2f(x_pos + 30, y_pos_on_sidewalk + 115);  // Reduced from 155
    glVertex2f(x_pos, y_pos_on_sidewalk + 115);
    glEnd();
    if (isNight)
    {
        glColor3f(1.0f, 1.0f, 0.7f);
        drawCircle(x_pos + 30, y_pos_on_sidewalk + 110, 10);  // Reduced from 14
        glColor4f(1.0f, 1.0f, 0.7f, 0.3f);
        drawCircle(x_pos + 30, y_pos_on_sidewalk + 110, 16);  // Reduced from 22
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
    {
        glColor3f(0.6f, 0.6f, 0.5f);
        drawCircle(x_pos + 30, y_pos_on_sidewalk + 110, 8);  // Reduced from 12
    }
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

// Add these new functions before the display() function
void drawSun(float x, float y, float radius) {
    // Draw outer glow
    glColor4f(1.0f, 0.8f, 0.0f, 0.3f);
    drawCircle(x, y, radius * 1.5f);

    // Draw sun body
    glColor3f(1.0f, 0.8f, 0.0f);
    drawCircle(x, y, radius);
}

void drawMoon(float x, float y, float radius) {
    // Draw outer glow
    glColor4f(0.9f, 0.9f, 0.95f, 0.3f);
    drawCircle(x, y, radius * 1.5f);

    // Draw moon body
    glColor3f(0.9f, 0.9f, 0.95f);
    drawCircle(x, y, radius);

    // Draw craters
    glColor3f(0.8f, 0.8f, 0.85f);
    drawCircle(x - radius * 0.3f, y + radius * 0.2f, radius * 0.15f);
    drawCircle(x + radius * 0.2f, y - radius * 0.3f, radius * 0.1f);
    drawCircle(x + radius * 0.1f, y + radius * 0.1f, radius * 0.08f);
}

// Add these new functions before the display() function
void drawModernBuilding(float x, float y, float width, float height) {
    // Main building
    glColor3f(0.7f, 0.7f, 0.75f);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();

    // Windows
    glColor3f(0.9f, 0.9f, 0.95f);
    float windowWidth = width * 0.15f;
    float windowHeight = height * 0.1f;
    float windowSpacing = width * 0.2f;
    float windowRows = height * 0.15f;

    for (float wx = x + windowSpacing; wx < x + width - windowWidth; wx += windowSpacing) {
        for (float wy = y + windowRows; wy < y + height - windowRows; wy += windowRows) {
            glBegin(GL_QUADS);
            glVertex2f(wx, wy);
            glVertex2f(wx + windowWidth, wy);
            glVertex2f(wx + windowWidth, wy + windowHeight);
            glVertex2f(wx, wy + windowHeight);
            glEnd();
        }
    }
}

void drawClassicBuilding(float x, float y, float width, float height) {
    // Main building
    glColor3f(0.8f, 0.6f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();

    // Roof
    glColor3f(0.5f, 0.3f, 0.2f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x - width * 0.1f, y + height);
    glVertex2f(x + width * 0.5f, y + height + height * 0.2f);
    glVertex2f(x + width + width * 0.1f, y + height);
    glEnd();

    // Windows
    glColor3f(0.9f, 0.9f, 0.7f);
    float windowWidth = width * 0.2f;
    float windowHeight = height * 0.15f;
    float windowSpacing = width * 0.3f;
    float windowRows = height * 0.2f;

    for (float wx = x + windowSpacing; wx < x + width - windowWidth; wx += windowSpacing) {
        for (float wy = y + windowRows; wy < y + height - windowRows; wy += windowRows) {
            glBegin(GL_QUADS);
            glVertex2f(wx, wy);
            glVertex2f(wx + windowWidth, wy);
            glVertex2f(wx + windowWidth, wy + windowHeight);
            glVertex2f(wx, wy + windowHeight);
            glEnd();
        }
    }
}

void drawSkyscraper(float x, float y, float width, float height) {
    // Main building
    glColor3f(0.6f, 0.7f, 0.8f);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();

    // Glass windows
    glColor3f(0.8f, 0.9f, 1.0f);
    float windowWidth = width * 0.1f;
    float windowHeight = height * 0.05f;
    float windowSpacing = width * 0.15f;
    float windowRows = height * 0.08f;

    for (float wx = x + windowSpacing; wx < x + width - windowWidth; wx += windowSpacing) {
        for (float wy = y + windowRows; wy < y + height - windowRows; wy += windowRows) {
            glBegin(GL_QUADS);
            glVertex2f(wx, wy);
            glVertex2f(wx + windowWidth, wy);
            glVertex2f(wx + windowWidth, wy + windowHeight);
            glVertex2f(wx, wy + windowHeight);
            glEnd();
        }
    }

    // Antenna
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(x + width * 0.45f, y + height);
    glVertex2f(x + width * 0.55f, y + height);
    glVertex2f(x + width * 0.55f, y + height + height * 0.1f);
    glVertex2f(x + width * 0.45f, y + height + height * 0.1f);
    glEnd();
}

void drawBasicTree(float x_pos, float y_pos_on_sidewalk) {
    glColor3f(0.5f, 0.3f, 0.05f);
    glBegin(GL_QUADS);
    glVertex2f(x_pos - 8, y_pos_on_sidewalk);  // Reduced from 12
    glVertex2f(x_pos + 8, y_pos_on_sidewalk);
    glVertex2f(x_pos + 6, y_pos_on_sidewalk + 50);  // Reduced from 70
    glVertex2f(x_pos - 6, y_pos_on_sidewalk + 50);
    glEnd();

    glColor3f(0.0f, 0.45f, 0.05f);
    drawCircle(x_pos, y_pos_on_sidewalk + 75, 25);  // Reduced from 35
    drawCircle(x_pos - 15, y_pos_on_sidewalk + 65, 20);  // Reduced from 30
    drawCircle(x_pos + 15, y_pos_on_sidewalk + 65, 20);  // Reduced from 30
    glColor3f(0.1f, 0.55f, 0.1f);
    drawCircle(x_pos, y_pos_on_sidewalk + 80, 18);  // Reduced from 25
}


// Add these new tree drawing functions before the display() function
void drawPineTree(float x_pos, float y_pos_on_sidewalk) {
    // Trunk
    glColor3f(0.5f, 0.3f, 0.05f);
    glBegin(GL_QUADS);
    glVertex2f(x_pos - 6, y_pos_on_sidewalk);  // Reduced from 8
    glVertex2f(x_pos + 6, y_pos_on_sidewalk);
    glVertex2f(x_pos + 4, y_pos_on_sidewalk + 60);  // Reduced from 80
    glVertex2f(x_pos - 4, y_pos_on_sidewalk + 60);
    glEnd();

    // Pine needles - multiple layers with varying sizes
    glColor3f(0.0f, 0.4f, 0.1f);
    
    // Bottom layer - widest
    glBegin(GL_TRIANGLES);
    glVertex2f(x_pos - 35, y_pos_on_sidewalk + 60);  // Reduced from 45
    glVertex2f(x_pos + 35, y_pos_on_sidewalk + 60);
    glVertex2f(x_pos, y_pos_on_sidewalk + 85);  // Reduced from 110
    glEnd();

    // Second layer
    glBegin(GL_TRIANGLES);
    glVertex2f(x_pos - 30, y_pos_on_sidewalk + 75);  // Reduced from 40
    glVertex2f(x_pos + 30, y_pos_on_sidewalk + 75);
    glVertex2f(x_pos, y_pos_on_sidewalk + 100);  // Reduced from 130
    glEnd();

    // Third layer
    glBegin(GL_TRIANGLES);
    glVertex2f(x_pos - 25, y_pos_on_sidewalk + 90);  // Reduced from 35
    glVertex2f(x_pos + 25, y_pos_on_sidewalk + 90);
    glVertex2f(x_pos, y_pos_on_sidewalk + 115);  // Reduced from 150
    glEnd();

    // Fourth layer
    glBegin(GL_TRIANGLES);
    glVertex2f(x_pos - 20, y_pos_on_sidewalk + 105);  // Reduced from 30
    glVertex2f(x_pos + 20, y_pos_on_sidewalk + 105);
    glVertex2f(x_pos, y_pos_on_sidewalk + 130);  // Reduced from 170
    glEnd();

    // Top layer - smallest
    glBegin(GL_TRIANGLES);
    glVertex2f(x_pos - 15, y_pos_on_sidewalk + 120);  // Reduced from 25
    glVertex2f(x_pos + 15, y_pos_on_sidewalk + 120);
    glVertex2f(x_pos, y_pos_on_sidewalk + 145);  // Reduced from 190
    glEnd();

    // Add some darker green details
    glColor3f(0.0f, 0.35f, 0.05f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x_pos - 25, y_pos_on_sidewalk + 70);  // Reduced from 35
    glVertex2f(x_pos + 25, y_pos_on_sidewalk + 70);
    glVertex2f(x_pos, y_pos_on_sidewalk + 95);  // Reduced from 120
    glEnd();
}

void drawMapleTree(float x_pos, float y_pos_on_sidewalk) {
    // Trunk
    glColor3f(0.5f, 0.3f, 0.05f);
    glBegin(GL_QUADS);
    glVertex2f(x_pos - 10, y_pos_on_sidewalk);
    glVertex2f(x_pos + 10, y_pos_on_sidewalk);
    glVertex2f(x_pos + 8, y_pos_on_sidewalk + 80);
    glVertex2f(x_pos - 8, y_pos_on_sidewalk + 80);
    glEnd();

    // Main branches
    glBegin(GL_QUADS);
    glVertex2f(x_pos - 8, y_pos_on_sidewalk + 60);
    glVertex2f(x_pos + 8, y_pos_on_sidewalk + 60);
    glVertex2f(x_pos + 6, y_pos_on_sidewalk + 80);
    glVertex2f(x_pos - 6, y_pos_on_sidewalk + 80);
    glEnd();

    // Maple leaves - multiple layers
    glColor3f(0.0f, 0.5f, 0.1f);
    for (int i = 0; i < 3; i++) {
        float y = y_pos_on_sidewalk + 80 + i * 20;
        float radius = 35 - i * 5;
        drawCircle(x_pos, y, radius);
    }
}

void drawTree(float x, float y, int type) {
    switch(type) {
        case 0: 
            drawBasicTree(x, y);
            break;
        case 1: 
            drawPineTree(x, y); 
            break;
    }
}

void drawBackgroundScenes() {
        // Draw celestial bodies with smooth transitions
    float sunX = WINDOW_WIDTH * 0.2f;  // Sun on the left
    float moonX = WINDOW_WIDTH * 0.8f; // Moon on the right
    float celestialY = WINDOW_HEIGHT * 1.2f; // Start sun above the window

    if (currentTimeOfDay >= 0.05 && currentTimeOfDay < 0.80) {
        // Day time - sun visible
        // Sun starts high (noon) and moves down
        float sunY = celestialY - (currentTimeOfDay - 0.2f) * WINDOW_HEIGHT * 1.4f;
        drawSun(sunX, sunY, 30.0f);
    }

    if (currentTimeOfDay >= 0.70 || currentTimeOfDay < 0.20) {
        // Night time - moon visible
        // Moon rises from bottom
        float moonY = (WINDOW_HEIGHT/3) + (currentTimeOfDay > 0.5f ?
            (currentTimeOfDay - 0.7f) : (currentTimeOfDay + 0.3f)) * WINDOW_HEIGHT * 1.5;
        drawMoon(moonX, moonY, 25.0f);
    }

    // Draw clouds
    for (const auto& cloud : clouds) {
        drawCloud(cloud.x, cloud.y, cloud.scale);
    }

    // Draw stars
    drawStars();

    // Draw buildings on the top side
    float buildingY = SIDEWALK_TOP_Y_END;

    // Modern building
    drawModernBuilding(60.0f, buildingY, 80.0f, 120.0f);

    // Classic building
    drawClassicBuilding(180.0f, buildingY, 100.0f, 100.0f);

    // Skyscraper
    drawSkyscraper(340.0f, buildingY, 60.0f, 180.0f);

    // Another modern building
    drawModernBuilding(460.0f, buildingY, 90.0f, 140.0f);

    // Another classic building
    drawClassicBuilding(620.0f, buildingY, 110.0f, 110.0f);

    // Another skyscraper
    drawSkyscraper(840.0f, buildingY, 70.0f, 200.0f);
}

void drawGround() {
    glColor3f(0.35f, 0.7f, 0.25f); // green ground
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(WINDOW_WIDTH, 0);
    glVertex2f(WINDOW_WIDTH, SIDEWALK_BOTTOM_Y_START);
    glVertex2f(0, SIDEWALK_BOTTOM_Y_START);
    glEnd();

    // Draw grass clumps (deterministic, evenly spaced)
    for (int i = 0; i < 25; ++i) {
        float gx = 20 + i * 38;
        float gy = 10 + (i * 13) % (int)(SIDEWALK_BOTTOM_Y_START * 0.5f);
        float scale = 0.8f + ((i * 7) % 10) / 20.0f;
        drawGrass(gx, gy, scale);
    }

    // Draw flowers (deterministic, less frequent)
    for (int i = 0; i < 10; ++i) {
        float fx = 35 + i * 90;
        float fy = 18 + (i * 23) % (int)(SIDEWALK_BOTTOM_Y_START * 0.5f);
        float scale = 0.8f + ((i * 5) % 10) / 20.0f;
        drawFlower(fx, fy, scale);
    }
}


void drawHumanShape(float x, float y, float scale, int walkState) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);
    
    if (walkState == 1) {
        // WALK state - walking figure
        //color 
        glColor3f(0.0f, 0.9f, 0.0f); // Green for "WALK"
        //left hand
        drawLine(-7, -4, -3, 4, 2.0f);
        drawLine(6, 0, 3, 4, 2.0f);
        drawLine(6, -4, 6, 0, 2.0f);
        //legs as ^
        drawLine(-5, -10, -1, -2, 2.0f);
        drawLine(5, -10, 1, -2, 2.0f);
    } else {
        // STOP state - standing figure
        glColor3f(0.9f, 0.0f, 0.0f); // Red for "STOP"
        //left hand
        drawLine(-3, -4, -3, 4, 2.0f);
        //right hand
        drawLine(3.2, -4, 3.2, 4, 2.0f);
        //left leg
        drawLine(-1, -10, -1, -2, 2.0f);
        //right leg
        drawLine(1.2, -10, 1.2, -2, 2.0f);
    }

    drawCircle(0, 7.5, 3);
    drawRect(-2, -4, 4, 8); // Body
    
    glPopMatrix();
}

void drawHumanSign(float x, float y, PedestrianLightState state) {
    if (state == PedestrianLightState::WALK)
        glColor3f(0.0f, 1.0f, 0.0f); // Green for "WALK"
    else if (state == PedestrianLightState::BLINK && pedBlinkOn)
        glColor3f(1.0f, 0.5f, 0.0f); // Orange when blinking on
    else
        glColor3f(0.9f, 0.0f, 0.0f); // Red for "STOP"

    drawHumanShape(x + 5, y + 85, 0.8, state == PedestrianLightState::WALK ? 1 : 0);
}

void drawTrafficSignal(float x, float y, TrafficLightState state) {
    const float height = 60.0f;
    const float width = 20.0f;
    const float lightRadius = 8.0f;
    float spacing = height / 3.0f;
    int translateY = 60;
    int translateX = -(width / 4);
    const float gap = 18;

    // Draw three segments (top, middle, bottom)
    for (int i = 0; i < 3; i++) {
        float centerY = y + height - (i + 0.5f) * spacing;
        // Determine bulb color based on segment and current state
        if(i == 0 && state == TrafficLightState::RED)
            glColor3f(1.0f, 0.0f, 0.0f);
        else if(i == 1 && state == TrafficLightState::YELLOW)
            glColor3f(1.0f, 1.0f, 0.0f);
        else if(i == 2 && state == TrafficLightState::GREEN)
            glColor3f(0.0f, 1.0f, 0.0f);
        else
            glColor3f(0.3f, 0.3f, 0.3f); // dim bulb when off

        // Draw left and right semi-circles for the bulb
        drawCircle(x + translateX + gap, centerY + translateY - lightRadius, lightRadius);
        drawCircle(x + translateX + width - gap, centerY + translateY - lightRadius, lightRadius);

        bool drawGlow = true;

        if(i == 0 && state == TrafficLightState::RED)
            glColor4f(1.0f, 0.0f, 0.0f, 0.3f);
        else if(i == 1 && state == TrafficLightState::YELLOW)
            glColor4f(1.0f, 1.0f, 0.0f, 0.3f);
        else if(i == 2 && state == TrafficLightState::GREEN)
            glColor4f(0.0f, 1.0f, 0.0f, 0.3f);
        else
            drawGlow = false;
    
        if (drawGlow) {
            drawCircle(x + translateX + gap, centerY + translateY - lightRadius, lightRadius * 1.8f);
            drawCircle(x + translateX + width - gap, centerY + translateY - lightRadius, lightRadius * 1.8f);
        }

        // Draw triangle shapes on each side
        glColor3f(0.0f, 0.0f, 0.0f);
        drawTriangle(x + translateX, centerY + translateY, x + translateX - 10, centerY + translateY, x + translateX, centerY - 10 + translateY);
        drawTriangle(x + translateX + width, centerY + translateY, x + translateX + width + 10, centerY + translateY, x + translateX + width, centerY - 10 + translateY);
    }

    // Draw the main black signal box
    glColor3f(0.0f, 0.0f, 0.0f);
    drawRect(x + translateX, y + translateY - 8, width, height);
    drawRect(x, y, 10, 100);

    drawHumanSign(x, y, pedestrianLightState);
}



void drawSceneObjects() {
    // Draw trees
    drawTree(180, SIDEWALK_TOP_Y_START + 30, 0); // Regular tree
    drawTree(500, SIDEWALK_TOP_Y_START + 30, 1); // Pine tree
    drawTree(700, SIDEWALK_TOP_Y_START + 30, 1); // Pine tree

    // Draw street lamps
    drawStreetLamp(80, SIDEWALK_TOP_Y_START);
    drawStreetLamp(380, SIDEWALK_TOP_Y_START);
    drawStreetLamp(780, SIDEWALK_TOP_Y_START);

    // Draw cars
    for (const auto &car : cars) {
        car.draw();
    }

    // Draw humans
    for (const auto &p : activeHumans) {
        p.draw();
    }


    drawTrafficSignal(TRAFFIC_LIGHT_X, SIDEWALK_TOP_Y_START, mainTrafficLightState);

    // Draw bottom trees and street lamp
    drawTree(850, SIDEWALK_BOTTOM_Y_START, 0);   // Regular tree
    drawTree(300, SIDEWALK_BOTTOM_Y_START, 1);   // Pine tree
    drawStreetLamp(950, SIDEWALK_BOTTOM_Y_START);
}

void transition() {
    float dayR = 0.52f, dayG = 0.8f, dayB = 0.92f;
    float duskR = 0.8f, duskG = 0.5f, duskB = 0.4f;
    float nightR = 0.05f, nightG = 0.05f, nightB = 0.2f;
    float r, g, b;

    if (currentTimeOfDay >= 0.0 && currentTimeOfDay < 0.20)
    {
        // Night to dawn transition
        float t = currentTimeOfDay / 0.20f;
        r = nightR * (1 - t) + dayR * t;
        g = nightG * (1 - t) + dayG * t;
        b = nightB * (1 - t) + dayB * t;
    }
    else if (currentTimeOfDay >= 0.20 && currentTimeOfDay < 0.45)
    {
        r = dayR;
        g = dayG;
        b = dayB;
    }
    else if (currentTimeOfDay >= 0.45 && currentTimeOfDay < 0.70)
    {
        // Day to dusk transition
        float t = (currentTimeOfDay - 0.45f) / 0.25f;
        r = dayR * (1 - t) + duskR * t;
        g = dayG * (1 - t) + duskG * t;
        b = dayB * (1 - t) + duskB * t;
    }
    else if (currentTimeOfDay >= 0.70 && currentTimeOfDay < 0.85)
    {
        // Dusk to night transition
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
}

// Modify the display function to include the new elements
void display()
{

    transition();
    
    drawBackgroundScenes();
    drawGround();
    drawRoadAndSidewalks();
    drawZebraCrossing(ROAD_Y_BOTTOM, ROAD_Y_TOP, HUMAN_CROSSING_X_START, HUMAN_CROSSING_WIDTH);
    drawSceneObjects();

    if (showWarningMessage)
    {
        glColor3f(1.0f, 0.0f, 0.0f);
        drawText(TRAFFIC_LIGHT_X - 100, SIDEWALK_TOP_Y_START + 180, "People are still passing the road", 0.7f);
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

        if (mainTrafficLightState == TrafficLightState::RED || mainTrafficLightState == TrafficLightState::YELLOW)
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

    switch (mainTrafficLightState) {
        case TrafficLightState::GREEN:
            // Check if we should transition to yellow
            if (pedestrianIsWaitingToCross && trafficLightTimer > MIN_CAR_GREEN_TIME) {
                // Only transition if no pedestrians are currently crossing
                bool pedestrianCrossing = false;
                for (const auto &ped : activeHumans) {
                    if (ped.state == CROSSING_ROAD) {
                        pedestrianCrossing = true;
                        break;
                    }
                }

                if (!pedestrianCrossing) {
                    mainTrafficLightState = TrafficLightState::YELLOW;
                    trafficLightTimer = 0;
                }
            }
            break;

        case TrafficLightState::YELLOW:
            if (trafficLightTimer > USER_TL_YELLOW_DURATION) {
                // If we came from green, go to red
                // If we came from red, go to green
                if (pedestrianLightState == PedestrianLightState::RED && !pedestrianIsWaitingToCross) {
                    mainTrafficLightState = TrafficLightState::GREEN;
                    trafficLightTimer = 0;
                } else {
                    mainTrafficLightState = TrafficLightState::RED;
                    trafficLightTimer = 0;
                    pedestrianLightState = PedestrianLightState::RED;
                    pedestrianLightTimer = 0;
                }
            }
            break;

        case TrafficLightState::RED:
            // Handle pedestrian light states
            switch (pedestrianLightState) {
                case PedestrianLightState::RED:
                    // Check if we should start the walk phase
                    if (trafficLightTimer > USER_PED_WALK_START_DELAY_AFTER_RED) {
                        bool anyPedWaiting = false;
                        for (const auto &ped : activeHumans) {
                            if (ped.state == WAITING_AT_CROSSING_EDGE) {
                                anyPedWaiting = true;
                                break;
                            }
                        }

                        if (anyPedWaiting || pedestrianIsWaitingToCross) {
                            pedestrianLightState = PedestrianLightState::WALK;
                            pedestrianLightTimer = 0;
                            pedestrianIsWaitingToCross = false;
                        }
                        // If no pedestrians are waiting and we've waited long enough, go back to green
                        else if (trafficLightTimer > USER_TL_RED_DURATION) {
                            mainTrafficLightState = TrafficLightState::YELLOW;
                            trafficLightTimer = 0;
                        }
                    }
                    break;

                case PedestrianLightState::WALK:
                    pedestrianLightTimer++;
                    if (pedestrianLightTimer > USER_PED_WALK_DURATION) {
                        pedestrianLightState = PedestrianLightState::BLINK;
                        pedestrianLightTimer = 0;
                        pedBlinkCounter = 0;
                        pedBlinkOn = true;
                    }
                    break;

                case PedestrianLightState::BLINK:
                    pedestrianLightTimer++;
                    pedBlinkCounter = (pedBlinkCounter + 1) % 25;
                    if (pedBlinkCounter == 0) {
                        pedBlinkOn = !pedBlinkOn;
                    }
                    if (pedestrianLightTimer > USER_PED_BLINK_DURATION) {
                        pedestrianLightState = PedestrianLightState::RED;
                        pedBlinkOn = true;
                        // After blinking is done, go back to green
                        mainTrafficLightState = TrafficLightState::YELLOW;
                        trafficLightTimer = 0;
                    }
                    break;
            }
            break;
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

    // Update cloud positions
    for (auto& cloud : clouds) {
        cloud.x += cloud.speed;
        if (cloud.x > WINDOW_WIDTH + 100.0f) {
            cloud.x = -100.0f;
            cloud.y = WINDOW_HEIGHT * (0.8f + (rand() % 20) / 100.0f); // Random height between 0.8 and 1.0
        }
    }
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
                int nextState = (static_cast<int>(mainTrafficLightState) + 1) % 3;
                mainTrafficLightState = static_cast<TrafficLightState>(nextState);
                trafficLightTimer = 0;
                pedestrianLightTimer = 0;
                if (mainTrafficLightState == TrafficLightState::GREEN)
                {
                    pedestrianLightState = PedestrianLightState::RED;
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