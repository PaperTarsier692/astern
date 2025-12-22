#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <float.h>

#define FPS 60
#define SPEED_MIN 500
#define SPEED_MAX 1000
#define POINTS 500
#define RADIUS 5
#define CONNECTION_DIST 50000
#define MAX_CONNECTIONS 10

typedef struct Point Point;

typedef struct Entry
{
    Point *point;
    float distance;
} Entry;

typedef struct Point
{
    Vector2 pos;
    Vector2 vel;
    Entry connected[MAX_CONNECTIONS];
} Point;

typedef struct
{
    Vector2 points[POINTS];
    unsigned int pointCount;
    size_t length;
} Path;

Point points[POINTS];
unsigned int ballCounter;
unsigned int X = 1920;
unsigned int Y = 1080;

Vector2 getRandomPos()
{
    return (Vector2){GetRandomValue(0, X), GetRandomValue(0, Y)};
}

Vector2 getRandomVel()
{
    float dir = GetRandomValue(0, 359);
    float rad = dir * RAD2DEG;
    float mult = (float)GetRandomValue(SPEED_MIN, SPEED_MAX) / 1000.0f;
    return (Vector2){cosf(rad) * mult, sinf(rad) * mult};
}

void initPoint(Point *point)
{
    *point = (Point){
        getRandomPos(),
        getRandomVel()};
}

void initPoints()
{
    for (unsigned int i = 0; i < POINTS; i++)
        points[i] = (Point){
            getRandomPos(),
            getRandomVel()};
}

void movePoint(Point *point)
{
    point->pos = Vector2Add(point->pos, point->vel);
    if (point->pos.x > X || point->pos.x < 0 || point->pos.y > Y || point->pos.y < 0)
        initPoint(point);
}
void movePoints()
{
    for (unsigned int i = 0; i < POINTS; i++)
        movePoint(&points[i]);
}

void drawPoint(Point point)
{
    DrawCircleV(point.pos, RADIUS, (Color){200, 200, 200, 100});
}

void getNearestPoints(Point *point, int index)
{
    Point *other;
    float distance;
    Entry *entries = point->connected;
    for (int j = 0; j < MAX_CONNECTIONS; j++)
        entries[j].distance = FLT_MAX;
    for (int i = 0; i < POINTS; i++)
    {
        if (i == index)
            continue;
        other = &points[i];
        distance = Vector2DistanceSqr(point->pos, other->pos);
        if (distance > CONNECTION_DIST)
            continue;
        int maxIdx = 0;
        float maxDist = entries[0].distance;
        for (int j = 1; j < MAX_CONNECTIONS; j++)
        {
            if (entries[j].distance > maxDist)
            {
                maxDist = entries[j].distance;
                maxIdx = j;
            }
        }
        if (distance < maxDist)
            entries[maxIdx] = (Entry){other, distance};
    }
}

void drawPoints()
{
    for (unsigned int i = 0; i < POINTS; i++)
        drawPoint(points[i]);
}

void drawConnection(Point point, int index)
{
    getNearestPoints(&point, index);
    for (int i = 0; i < MAX_CONNECTIONS; i++)
        if (point.connected[i].distance < FLT_MAX)
            DrawLineV(point.pos, point.connected[i].point->pos, (Color){100, 255, 100, 255 - Clamp(point.connected[i].distance / 100, 20, 255)});
}

void getPath(Point start, Point end)
{
}

void drawConnections()
{
    for (unsigned int i = 0; i < POINTS; i++)
        drawConnection(points[i], i);
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE + FLAG_WINDOW_HIGHDPI);
    srand(time(NULL));
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(X, Y, "Kein Stern, nur A - Raylib");
    SetTargetFPS(FPS);
    SetRandomSeed(time(NULL));
    initPoints();

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_F))
            ToggleFullscreen();
        if (IsWindowResized())
        {
            X = GetScreenWidth();
            Y = GetScreenHeight();
        }
        BeginDrawing();
        ClearBackground(GRAY);
        movePoints();
        drawConnections();
        drawPoints();
        DrawFPS(10, 10);
        EndDrawing();
    }
    CloseWindow();
    return EXIT_SUCCESS;
}
