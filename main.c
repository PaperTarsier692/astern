#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <float.h>
#include <math.h>

#define X 1440
#define Y 750
#define FPS 60
#define SPEED 0.25f
#define POINTS 600
#define RADIUS 5
#define CONNECTION_DIST 5000
#define MAX_CONNECTIONS 2

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

Path currentPath;

Vector2 getRandomPos()
{
    return (Vector2){GetRandomValue(0, X), GetRandomValue(0, Y)};
}

Vector2 getRandomVel()
{
    return (Vector2){(float)GetRandomValue(-SPEED * 100, SPEED * 100) / 100, (float)GetRandomValue(-SPEED * 100, SPEED * 100) / 100};
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
            DrawLineV(point.pos, point.connected[i].point->pos, (Color){100, 255, 100, 255 - Clamp(point.connected[i].distance / 300, 200, 250)});
}

void drawConnections()
{
    for (unsigned int i = 0; i < POINTS; i++)
        drawConnection(points[i], i);
}

void reconstructPath(int cameFrom[], int current, Path *out)
{
    Vector2 rev[POINTS];
    int count = 0;
    while (current != -1)
    {
        rev[count++] = points[current].pos;
        current = cameFrom[current];
    }
    out->pointCount = 0;
    out->length = 0;
    if (count == 0)
        return;
    out->pointCount = count;
    for (int i = 0; i < count; i++)
        out->points[i] = rev[count - 1 - i];
    for (int i = 1; i < out->pointCount; i++)
        out->length += Vector2Distance(out->points[i - 1], out->points[i]);
}

float heuristic(int a, int b)
{
    return Vector2Distance(points[a].pos, points[b].pos);
}

bool AStar(int startIdx, int goalIdx, Path *out)
{
    int cameFrom[POINTS];
    float gScore[POINTS];
    float fScore[POINTS];
    bool openSet[POINTS];
    bool closedSet[POINTS];

    for (int i = 0; i < POINTS; i++)
    {
        cameFrom[i] = -1;
        gScore[i] = FLT_MAX;
        fScore[i] = FLT_MAX;
        openSet[i] = false;
        closedSet[i] = false;
    }

    gScore[startIdx] = 0.0f;
    fScore[startIdx] = heuristic(startIdx, goalIdx);
    openSet[startIdx] = true;

    while (true)
    {
        int current = -1;
        float bestF = FLT_MAX;
        for (int i = 0; i < POINTS; i++)
        {
            if (openSet[i] && fScore[i] < bestF)
            {
                bestF = fScore[i];
                current = i;
            }
        }
        if (current == -1)
            break;

        if (current == goalIdx)
        {
            reconstructPath(cameFrom, current, out);
            return true;
        }

        openSet[current] = false;
        closedSet[current] = true;

        for (int neighbor = 0; neighbor < POINTS; neighbor++)
        {
            if (neighbor == current || closedSet[neighbor])
                continue;

            float distSqr = Vector2DistanceSqr(points[current].pos, points[neighbor].pos);
            if (distSqr > CONNECTION_DIST)
                continue;

            float tentative_g = gScore[current] + sqrtf(distSqr);

            if (!openSet[neighbor])
                openSet[neighbor] = true;
            else if (tentative_g >= gScore[neighbor])
                continue;

            cameFrom[neighbor] = current;
            gScore[neighbor] = tentative_g;
            fScore[neighbor] = tentative_g + heuristic(neighbor, goalIdx);
        }
    }

    out->pointCount = 0;
    out->length = 0;
    return false;
}

void drawPath(const Path *p)
{
    if (p->pointCount < 2)
        return;
    for (int i = 1; i < p->pointCount; i++)
    {
        DrawLineV(p->points[i - 1], p->points[i], WHITE);
        DrawCircleV(p->points[i - 1], RADIUS + 1, ORANGE);
    }
    DrawCircleV(p->points[p->pointCount - 1], RADIUS + 1, ORANGE);
}

int main(void)
{
    srand(time(NULL));
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(X, Y, "Kein Stern, nur A - Raylib");
    SetTargetFPS(FPS);
    SetRandomSeed(time(NULL));
    initPoints();

    currentPath.pointCount = 0;
    currentPath.length = 0;

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_F))
            ToggleFullscreen();

        bool ok = AStar(0, 1, &currentPath);
        if (!ok)
            currentPath.pointCount = 0;

        BeginDrawing();
        ClearBackground(GRAY);
        movePoints();
        drawConnections();
        drawPoints();
        drawPath(&currentPath);
        DrawFPS(10, 10);
        EndDrawing();
    }
    CloseWindow();
    return EXIT_SUCCESS;
}
