#include <float.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define FPS 60
#define SPEED_MIN 10
#define SPEED_MAX 100
#define POINTS 500
#define RADIUS 5
#define CONNECTION_DIST 100
#define CONNECTIONS 3
#define MAX_CONNECTIONS 30

typedef struct Point Point;

typedef struct Entry {
    Point* point;
    float distance;
} Entry;

typedef struct Point {
    Vector2 pos;
    Vector2 vel;
    double value;
    uint8_t connected_count;
    Entry best;
    Entry connected[MAX_CONNECTIONS];
} Point;

Point points[POINTS];
unsigned int X = 1920;
unsigned int Y = 1080;
Point* start;
Point* end;

Vector2 getRandomPos()
{
    return (Vector2) { GetRandomValue(0, X), GetRandomValue(0, Y) };
}

Vector2 getRandomVel()
{
    float dir = GetRandomValue(0, 359);
    float rad = dir * DEG2RAD;
    float mult = (float)GetRandomValue(SPEED_MIN, SPEED_MAX) / 1000.0f;
    return (Vector2) { cosf(rad) * mult, sinf(rad) * mult };
}

void initPoint(Point* point)
{
    *point = (Point) {
        getRandomPos(),
        getRandomVel(), 0
    };
}

void initPoints()
{
    for (unsigned int i = 2; i < POINTS; i++)
        initPoint(&points[i]);
    points[0] = (Point) { (Vector2) { 20, 20 }, (Vector2) { 0, 0 } };
    points[1] = (Point) { (Vector2) { X - 20, Y - 20 }, (Vector2) { 0, 0 } };
    start = &points[0];
    end = &points[1];
}

bool posInBounds(Vector2* pos)
{
    return !(pos->x > X || pos->x < 0 || pos->y > Y || pos->y < 0);
}

void movePoint(Point* point)
{
    point->pos = Vector2Add(point->pos, point->vel);
    if (!posInBounds(&point->pos))
        initPoint(point);
}

void movePoints()
{
    for (unsigned int i = 0; i < POINTS; i++)
        movePoint(&points[i]);
}

void drawPoint(Point point)
{
    DrawCircleV(point.pos, RADIUS, (Color) { 200, 200, 200, 100 });
}

void drawPoints()
{
    for (unsigned int i = 0; i < POINTS; i++)
        drawPoint(points[i]);
}

void preparePoint(Point* point)
{
    point->value = DBL_MAX;
    point->connected_count = 0;
    point->best = (Entry) { NULL, FLT_MAX };
    for (uint8_t i = 0; i < MAX_CONNECTIONS; i++)
        point->connected[i] = (Entry) { NULL, FLT_MAX };
}

uint8_t getFreeEntry(Point* point)
{
    for (uint8_t i = 0; i < CONNECTIONS; i++) {
        if (point->connected[i].point == NULL)
            return i;
    }
    return 0;
}

uint8_t getFreeEntryMax(Point* point)
{
    for (uint8_t i = CONNECTIONS - 1; i < MAX_CONNECTIONS; i++) {
        if (point->connected[i].point == NULL)
            return i;
    }
    return 0;
}

uint8_t getMaxDistanceIndex(Point* point)
{
    Entry* entries = point->connected;
    uint8_t index = 0;
    float distance = entries[0].distance;
    for (uint8_t i = 1; i < CONNECTIONS; i++)
        if (entries[i].distance > distance) {
            distance = entries[i].distance;
            index = i;
        }
    return index;
}

void addEntryIfNotExists(Point* point, Entry entry)
{
    if (point == NULL || entry.point == NULL)
        return;
    Entry* entries = point->connected;
    for (uint8_t i = 0; i < MAX_CONNECTIONS; i++) {
        if (entries[i].point == entry.point)
            return;

        if (entries[i].point == NULL) {
            entries[i] = entry;
            return;
        }
    }
}

void getNearestPoints(Point* point, int index)
{
    Point* other;
    float distance;
    Entry* entries = point->connected;
    uint8_t max_index = getMaxDistanceIndex(point);
    float max_distance = entries[max_index].distance;

    for (uint16_t i = 0; i < POINTS; i++) {
        if (i == index)
            continue;
        other = &points[i];
        distance = Vector2Distance(point->pos, other->pos);
        if (distance > CONNECTION_DIST)
            continue;

        if (distance < max_distance) {
            entries[max_index] = (Entry) { other, distance };
            max_index = getMaxDistanceIndex(point);
            max_distance = entries[max_index].distance;
        }
    }
}

void completeConnections(Point* point)
{
    Point* other;
    Entry entry;
    Entry* entries = point->connected;

    for (uint8_t i = 0; i < CONNECTIONS; i++) {
        other = entries[i].point;
        if (other == NULL || entries[i].distance == FLT_MAX)
            continue;
        entry = (Entry) { point, entries[i].distance };
        addEntryIfNotExists(other, entry);
    }
}

void DrawArrowV(Vector2 startPos, Vector2 endPos, Color color, float size, float deg)
{
    DrawLineV(startPos, endPos, color);
    Vector2 dir = Vector2Subtract(endPos, startPos);
    Vector2 base = Vector2Scale(Vector2Normalize(dir), -size);
    Vector2 pos1 = Vector2Add(endPos, Vector2Rotate(base, deg * DEG2RAD));
    Vector2 pos2 = Vector2Add(endPos, Vector2Rotate(base, -deg * DEG2RAD));
    DrawTriangleLines(endPos, pos1, pos2, color);
}

void drawConnection(Point* point, int index)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        Entry e = point->connected[i];
        if (e.distance == FLT_MAX)
            continue;

        Vector2 other = e.point->pos;
        unsigned char alpha = (unsigned char)(Clamp(1.0f - (e.distance / (float)CONNECTION_DIST), 0.0f, 1.0f) * 255.0f);
        Color color = (Color) { 100, 255, 100, alpha };
        Vector2 offset = Vector2Scale(Vector2Normalize(Vector2Subtract(other, point->pos)), RADIUS);
        Vector2 startPos = Vector2Add(point->pos, offset);
        Vector2 endPos = Vector2Subtract(other, offset);
        DrawLineV(startPos, endPos, color);
    }
}

void addPointIfNotExists(Point* point, Point** points)
{
    for (unsigned int i = 0; i < POINTS; i++) {
        if (points[i] == point)
            return;
        if (points[i] == NULL) {
            points[i] = point;
            return;
        }
    }
}

Point* getRandomPoint()
{
    return &points[abs(rand()) % POINTS];
}

void visit(Point* point, Point** next)
{
    if (point == NULL)
        return;
    double value;
    Entry* entries = point->connected;
    Entry* entry;
    for (uint8_t i = 0; i < MAX_CONNECTIONS; i++) {
        entry = &entries[i];
        if (!entry->point)
            continue;
        value = point->value + entry->distance;
        if (value < entry->point->value) {
            entry->point->value = value;
            entry->point->best = (Entry) { point, entry->distance };
            addPointIfNotExists(entry->point, next);
        }
    }
}

int comparePoints(const void* a, const void* b)
{
    const Point* pointA = *(const Point* const*)a;
    const Point* pointB = *(const Point* const*)b;
    if (pointA == NULL && pointB == NULL)
        return 0;
    if (pointA == NULL)
        return 1;
    if (pointB == NULL)
        return -1;
    if (pointA->value < pointB->value)
        return -1;
    if (pointA->value > pointB->value)
        return 1;
    return 0;
}

void drawPath(Point* end)
{
    unsigned int pathLength = 1;
    Point* current = end;

    DrawCircleV(end->pos, RADIUS + 1, (Color) { 255, 0, 0, 255 });

    while (current->best.point != NULL) {
        current = current->best.point;
        pathLength++;
    }

    float colorStep = 255.0f / (float)pathLength;
    float currentGreen = 0;
    current = end;

    while (current->best.point != NULL) {
        DrawLineEx(current->pos, current->best.point->pos, 3,
            (Color) { 255, currentGreen, 0, 255 });
        DrawCircleV(current->pos, RADIUS + 1, (Color) { 255, currentGreen, 0, 255 });
        current = current->best.point;
        currentGreen += colorStep;
    }

    DrawCircleV(start->pos, RADIUS + 1, (Color) { 255, 255, 0, 255 });
}

void getPath()
{
    start->value = 0;
    Point* next[POINTS] = { 0 };
    Point* current[POINTS] = { 0 };
    next[0] = start;

    unsigned int count_next = 1;
    unsigned int count;
    while (1) {
        count = count_next;
        count_next = 0;
        for (unsigned int i = 0; i < count; i++)
            current[i] = next[i];
        for (unsigned int i = count; i < POINTS; i++)
            current[i] = NULL;

        for (unsigned int i = 0; i < POINTS; i++)
            next[i] = NULL;

        for (unsigned int i = 0; i < count; i++)
            visit(current[i], next);

        for (unsigned int i = 0; i < POINTS; i++) {
            if (next[i] == NULL)
                break;
            count_next++;
        }
        if (count_next == 0)
            break;
        qsort(next, count_next, sizeof(Point*), comparePoints);
    }
}

void preparePoints()
{
    for (unsigned int i = 0; i < POINTS; i++)
        preparePoint(&points[i]);

    for (unsigned int i = 0; i < POINTS; i++)
        getNearestPoints(&points[i], i);

    for (unsigned int i = 0; i < POINTS; i++)
        completeConnections(&points[i]);
}

void drawConnections()
{
    for (unsigned int i = 0; i < POINTS; i++)
        drawConnection(&points[i], i);
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE + FLAG_WINDOW_HIGHDPI);
    srand(time(NULL));
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(X, Y, "Meh - Raylib");
    SetTargetFPS(FPS);
    SetRandomSeed(time(NULL));
    initPoints();

    X = GetScreenWidth();
    Y = GetScreenHeight();

    Vector2 mousePos;

    char time_buffer[33];
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F))
            ToggleFullscreen();

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            mousePos = GetMousePosition();
            if (posInBounds(&mousePos))
                start->pos = GetMousePosition();
        }

        if (IsKeyPressed(KEY_C))
            initPoints();

        if (IsWindowResized()) {
            X = GetScreenWidth();
            Y = GetScreenHeight();
        }
        BeginDrawing();
        ClearBackground(GRAY);
        movePoints();
        preparePoints();
        getPath();
        drawConnections();
        drawPoints();
        drawPath(end);
        DrawFPS(10, 10);
        EndDrawing();
    }
    CloseWindow();
    return EXIT_SUCCESS;
}
