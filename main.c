#include <float.h>
#include <math.h>
#include <string.h>
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>
#include <raylib.h>
#include <raymath.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

typedef struct {
    float speed;
    float radius;
    float connection_dist;
    uint8_t connections;
    uint16_t points;
} Settings;

Settings settings = (Settings) { 100, 5, 100, 3, 500 };

#define FPS 60
#define MAX_POINTS 1000
#define MAX_CONNECTIONS 30
#define MIN_SPEED 1
#define MAX_SPEED 5

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

Point points[MAX_POINTS];
unsigned int X = 1920;
unsigned int Y = 1080;
Point* start;
Point* end;
bool menuShown = false;

Vector2 getRandomPos()
{
    return (Vector2) { GetRandomValue(0, X), GetRandomValue(0, Y) };
}

Vector2 getRandomVel()
{
    float dir = GetRandomValue(0, 359);
    float rad = dir * DEG2RAD;
    float mult = (float)GetRandomValue(MIN_SPEED, MAX_SPEED) / 1000.0f;
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
    for (unsigned int i = 2; i < settings.points; i++)
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
    point->pos = Vector2Add(point->pos, Vector2Scale(point->vel, settings.speed));
    if (!posInBounds(&point->pos))
        initPoint(point);
}

void movePoints()
{
    for (unsigned int i = 0; i < settings.points; i++)
        movePoint(&points[i]);
}

void drawPoint(Point point)
{
    DrawCircleV(point.pos, settings.radius, (Color) { 150, 150, 150, 255 });
}

void drawPoints()
{
    for (unsigned int i = 0; i < settings.points; i++)
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
    for (uint8_t i = 0; i < settings.connections; i++) {
        if (point->connected[i].point == NULL)
            return i;
    }
    return 0;
}

uint8_t getFreeEntryMax(Point* point)
{
    for (uint8_t i = settings.connections - 1; i < MAX_CONNECTIONS; i++) {
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
    for (uint8_t i = 1; i < settings.connections; i++)
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

    for (uint16_t i = 0; i < settings.points; i++) {
        if (i == index)
            continue;
        other = &points[i];
        distance = Vector2Distance(point->pos, other->pos);
        if (distance > settings.connection_dist)
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

    for (uint8_t i = 0; i < settings.connections; i++) {
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
        unsigned char alpha = (unsigned char)(Clamp(1.0f - (e.distance / (float)settings.connection_dist), 0.0f, 1.0f) * 255.0f);
        Color color = (Color) { 100, 255, 100, alpha };
        DrawLineV(point->pos, other, color);
    }
}

void addPointIfNotExists(Point* point, Point** points)
{
    for (unsigned int i = 0; i < settings.points; i++) {
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
    return &points[abs(rand()) % settings.points];
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

    DrawCircleV(end->pos, settings.radius + 1, (Color) { 255, 0, 0, 255 });

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
        DrawCircleV(current->pos, settings.radius + 1, (Color) { 255, currentGreen, 0, 255 });
        current = current->best.point;
        currentGreen += colorStep;
    }

    DrawCircleV(start->pos, settings.radius + 1, (Color) { 255, 255, 0, 255 });
}

void getPath()
{
    start->value = 0;
    Point* next[MAX_POINTS] = { 0 };
    Point* current[MAX_POINTS] = { 0 };
    next[0] = start;

    unsigned int count_next = 1;
    unsigned int count;
    while (1) {
        count = count_next;
        count_next = 0;
        for (unsigned int i = 0; i < count; i++)
            current[i] = next[i];
        for (unsigned int i = count; i < settings.points; i++)
            current[i] = NULL;

        for (unsigned int i = 0; i < settings.points; i++)
            next[i] = NULL;

        for (unsigned int i = 0; i < count; i++)
            visit(current[i], next);

        for (unsigned int i = 0; i < settings.points; i++) {
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
    for (unsigned int i = 0; i < settings.points; i++)
        preparePoint(&points[i]);

    for (unsigned int i = 0; i < settings.points; i++)
        getNearestPoints(&points[i], i);

    for (unsigned int i = 0; i < settings.points; i++)
        completeConnections(&points[i]);
}

void drawConnections()
{
    for (unsigned int i = 0; i < settings.points; i++)
        drawConnection(&points[i], i);
}

void drawLabeledSlider(Rectangle bounds, const char* label, const char* leftLabel, const char* rightLabel, float min, float max, float* var)
{
    int fontSize = GuiGetStyle(DEFAULT, TEXT_SIZE);
    float gap = 10.0f;
    float labelWidth = (float)MeasureText(label, fontSize);
    float leftValueWidth = (float)MeasureText(leftLabel, fontSize) + GuiGetStyle(SLIDER, TEXT_PADDING);
    Rectangle labelBounds = (Rectangle) {
        bounds.x - leftValueWidth - gap - labelWidth,
        bounds.y,
        labelWidth,
        bounds.height,
    };
    GuiLabel(labelBounds, label);
    GuiSliderBar(bounds, leftLabel, rightLabel, var, min, max);
}

int drawLabeledSliderInt(Rectangle bounds, const char* label, const char* leftLabel, const char* rightLabel, int min, int max, int value)
{
    float temp = (float)value;
    drawLabeledSlider(bounds, label, leftLabel, rightLabel, (float)min, (float)max, &temp);
    return (int)(temp + 0.5f);
}

void drawMenu()
{
    int b_width = 500;
    int b_left = (X / 2.0f) - (b_width / 2.0f);
    int b_height = 300;
    int b_top = (Y / 2.0f) - (b_height / 2.0f);

    int width = 200;
    int left = (X / 2.0f) - (width / 2.0f);
    int height = 20;
    int top = (Y / 2.0f) - (b_top / 2.0f) + 40;

    Rectangle background = (Rectangle) {
        b_left,
        b_top,
        b_width,
        b_height
    };
    int result = GuiWindowBox(background, "Settings");
    Rectangle current = (Rectangle) {
        left,
        top,
        width,
        height
    };
    drawLabeledSlider(current, "Speed", "50", "500", 50, 500, &settings.speed);
    current.y += 40;
    drawLabeledSlider(current, "C Dist.", "10", "500", 10, 500, &settings.connection_dist);
    current.y += 40;
    settings.connections = (uint8_t)drawLabeledSliderInt(current, "Connections", "1", "10", 1, 10, settings.connections);
    current.y += 40;
    drawLabeledSlider(current, "Radius", "1", "10", 1, 10, &settings.radius);
    current.y += 40;
    uint16_t points_before = settings.points;
    settings.points = (uint16_t)drawLabeledSliderInt(current, "Points", "10", "1000", 10, MAX_POINTS, settings.points);
    if (settings.points > points_before)
        for (uint16_t i = points_before; i < settings.points; i++)
            initPoint(&points[i]);
    if (result)
        menuShown = false;
}

void tick()
{
    Vector2 mousePos;
    if (IsKeyPressed(KEY_F))
        ToggleFullscreen();

    if (!menuShown) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            mousePos = GetMousePosition();
            if (posInBounds(&mousePos))
                start->pos = GetMousePosition();
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            mousePos = GetMousePosition();
            if (posInBounds(&mousePos)) {
                end->pos = GetMousePosition();
                end->vel = getRandomVel();
            }
        }
    }

    if (IsKeyPressed(KEY_S))
        menuShown = true;

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
    if (menuShown) {
        drawMenu();
    }
    DrawFPS(10, 10);
    EndDrawing();
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE + FLAG_WINDOW_HIGHDPI);
    srand(time(NULL));
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(X, Y, "Astern - Raylib");
    SetRandomSeed(time(NULL));
    initPoints();
    GuiLoadStyleDefault();
    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);

    X = GetScreenWidth();
    Y = GetScreenHeight();

#if defined(PLATFORM_WEB)
    // Let the browser handle updates
    emscripten_set_main_loop(tick, 0, 1);
#else
    SetTargetFPS(FPS);
    while (!WindowShouldClose()) {
        tick();
    }
#endif
    CloseWindow();
    return EXIT_SUCCESS;
}
