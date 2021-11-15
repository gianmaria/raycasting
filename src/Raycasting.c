#include <stdio.h>
#include <stdlib.h>

#include <raylib.h>
#include <raymath.h>

#include "Perlin_Noise.h"

typedef struct Boundary
{
    Vector2 begin;
    Vector2 end;
} Boundary;

Boundary boundary_init(float x1, float y1, float x2, float y2)
{
    Boundary wall = {0};
    
    wall.begin.x = x1;
    wall.begin.y = y1;

    wall.end.x = x2;
    wall.end.y = y2;

    return wall;
}

void boundary_draw(Boundary b)
{
    DrawLineEx(b.begin, b.end, 2.0f, WHITE);
}

typedef struct Ray2D
{
    Vector2 pos;
    Vector2 dir;
} Ray2D;

Ray2D ray2d_init(Vector2 pos, float angle_deg)
{
    Ray2D ray;

    ray.pos = pos;

    float angle_rad = angle_deg*DEG2RAD;
    
    float len = 1.0f;
    ray.dir.x = cosf(angle_rad) * len;
    ray.dir.y = sinf(angle_rad) * len;

    return ray;
}

void ray2d_draw(Ray2D ray)
{
    float len = 10.0f;

    Vector2 end_pos;
    end_pos.x = ray.pos.x + ray.dir.x * len;
    end_pos.y = ray.pos.y + ray.dir.y * len;
    
    DrawLineEx(ray.pos, end_pos, 1.0f, Fade(WHITE, 0.078f));
}

void ray2d_look_at(Ray2D* ray, float x, float y)
{
    ray->dir.x = x - ray->pos.x;
    ray->dir.y = y - ray->pos.y;
    ray->dir = Vector2Normalize(ray->dir);
}

bool ray2d_cast(Ray2D ray, Boundary wall, Vector2* intersect)
{
    // line a
    float x1 = wall.begin.x;
    float y1 = wall.begin.y;
    float x2 = wall.end.x;
    float y2 = wall.end.y;

    // line b
    float x3 = ray.pos.x;
    float y3 = ray.pos.y;
    float x4 = ray.pos.x + ray.dir.x;
    float y4 = ray.pos.y + ray.dir.y;

    float denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

    // if denom is zero, line a and line b are parallel
    if (denom == 0.0f)
        return false;

    float t = (x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4);
    t /= denom;

    float u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3));
    u /= denom;

    if (t > 0.0f && t < 1.0f &&
        u > 0.0f)
    {
        if (intersect)
        {
            intersect->x = x1 + t * (x2 - x1);
            intersect->y = y1 + t * (y2 - y1);
        }
        return true;
    }
    else
    {
        return false;
    }
    
}

#define RAYS_COUNT 360
typedef struct Particle
{
    Vector2 pos;
    Ray2D rays[RAYS_COUNT];
} Particle;

void particle_draw(const Particle* particle)
{
    DrawEllipse((int)particle->pos.x, (int)particle->pos.y, 5, 5, WHITE);

    for (int i = 0;
         i < RAYS_COUNT;
         ++i)
    {
        ray2d_draw(particle->rays[i]);
    }
}

Particle particle_init(float x, float y)
{
    Particle particle = {0};

    particle.pos = (Vector2){x, y};
    
    for (int i = 0;
         i < RAYS_COUNT;
         ++i)
    {
#define SLICE (360.0f / (float)RAYS_COUNT)
        float angle =  SLICE * (float)i;
#undef SLICE
        particle.rays[i] = ray2d_init(particle.pos, angle);
    }

    return particle;
}

void particle_look(const Particle* particle, Boundary* walls, int walls_count)
{
    for (int ray_idx = 0;
         ray_idx < RAYS_COUNT;
         ++ray_idx)
    {
        Vector2 closest_hit = {0};
        float min_distance = INFINITY;

        for (int wall_idx = 0;
             wall_idx < walls_count;
             ++wall_idx)
        {
            Vector2 intersect = {0};
            if (ray2d_cast(particle->rays[ray_idx], walls[wall_idx], &intersect))
            {
                float distance = Vector2Distance(particle->pos, intersect);

                if (distance < min_distance)
                {
                    min_distance = distance;
                    closest_hit = intersect;
                }
            }
        }
        if (!(closest_hit.x == 0.0f &&
              closest_hit.y == 0.0f))
        {
            DrawLineEx(particle->pos, closest_hit, 1.0f, Fade(WHITE, 0.30f));
        }
        
    }
}

void particle_update_pos(Particle* particle, float x, float y)
{
    particle->pos.x = x;
    particle->pos.y = y;

    for (int i = 0;
         i < RAYS_COUNT;
         ++i)
    {
        float slice = 360.0f / (float)RAYS_COUNT;
        float angle =  slice * (float)i;
        particle->rays[i] = ray2d_init(particle->pos, angle);
    }

}

#define TRAIL2D_MAX (200UL)
typedef struct Trail2D
{
    Vector2 points[TRAIL2D_MAX];
    int head;
    int tail;
} Trail2D;

Trail2D trail2d_init(void)
{
    Trail2D trail = {0};
    return trail;
}

void trail2d_add(Trail2D* trail, Vector2 new_pos)
{
    trail->points[trail->tail++] = new_pos;
    trail->tail %= TRAIL2D_MAX;

    if (trail->tail == trail->head)
    {
        ++trail->head;
        trail->head %= TRAIL2D_MAX;
    }
}

void trail2d_draw(const Trail2D* trail)
{
    int head = trail->head;
    while (head != trail->tail)
    {
        int pos = head;
        int pos_next = (head + 1) % TRAIL2D_MAX;
        
        if (pos_next != trail->tail)
        {
            const Vector2* p1 = &trail->points[pos];
            const Vector2* p2 = &trail->points[pos_next];
            DrawLineEx(*p1, *p2, 1, Fade(WHITE, 0.5f));
        }

        head = (head + 1) % TRAIL2D_MAX;
    }
}

const int screen_width = 800;
const int screen_height = 600;

void init_walls(Boundary* walls, int count)
{
    for (int i = 0;
         i < count;
         ++i)
    {
        float x1 = (float)GetRandomValue(0, screen_width);
float y1 = (float)GetRandomValue(0, screen_height);

float x2 = (float)GetRandomValue(0, screen_width);
float y2 = (float)GetRandomValue(0, screen_height);

walls[i] = boundary_init(x1, y1, x2, y2);
    }
}

int main(void)
{
    InitWindow(screen_width, screen_height, "Raytracing");
    SetWindowPosition((GetMonitorWidth(0) / 2) - screen_width,
                      (GetMonitorHeight(0) / 2) - screen_height);
    HideCursor();
    SetTargetFPS(60);

#define WALLS_COUNT 5
#define BORDERS_COUNT 4
#define BOUNDARYS_COUNT (WALLS_COUNT + BORDERS_COUNT)

    Boundary walls[BOUNDARYS_COUNT] = {0};

    init_walls(walls, WALLS_COUNT);

    walls[WALLS_COUNT + 0] = boundary_init(0, 0, (float)screen_width, 0);
    walls[WALLS_COUNT + 1] = boundary_init((float)screen_width, 0, (float)screen_width, (float)screen_height);
    walls[WALLS_COUNT + 2] = boundary_init((float)screen_width, (float)screen_height, 0, (float)screen_height);
    walls[WALLS_COUNT + 3] = boundary_init(0, (float)screen_height, 0, 0);


    Particle particle = particle_init((float)(screen_width / 2),
                                      (float)(screen_height / 2));

    float x_off = 0.0f;
    float y_off = 1000.0f;
    bool manual_mode = false;
    bool draw_wall = true;

    Vector2 particle_pos = {
        (float)(screen_width / 2),
        (float)(screen_height / 2)
    };
    //Trail2D particle_trail = trail2d_init();
    Trail2D* particle_trail = calloc(1, sizeof(Trail2D));

    while (!WindowShouldClose())
    {
        // Update
        //----------------------------------------------------------------------------------
        if (IsKeyReleased(KEY_R))
        {
            // generate new walls
            init_walls(walls, WALLS_COUNT);
        }

        if (IsKeyDown(KEY_LEFT_CONTROL) &&
            IsKeyReleased(KEY_S))
        {
            TakeScreenshot("screenshot.png");
        }

        if (IsKeyReleased(KEY_UP))
        {

        }

        if (IsKeyReleased(KEY_DOWN))
        {

        }

        if (IsKeyReleased(KEY_M))
        {
            manual_mode = !manual_mode;
        }

        if (IsKeyReleased(KEY_F))
        {
            //ToggleFullscreen();
        }

        if (IsKeyReleased(KEY_W))
        {
            draw_wall = !draw_wall;
        }

        if (manual_mode)
        {
            particle_pos.x = (float)GetMouseX();
            particle_pos.y = (float)GetMouseY();

            particle_update_pos(&particle, particle_pos.x, particle_pos.y);
        }
        else
        {
            particle_pos.x = noise(x_off, 0.0f, 0.0f) * screen_width;
            particle_pos.y = noise(0.0f, y_off, 0.0f) * screen_height;

            particle_update_pos(&particle, particle_pos.x, particle_pos.y);

            x_off += 0.001f;
            y_off += 0.001f;
        }

        trail2d_add(particle_trail, particle_pos);
        //----------------------------------------------------------------------------------



        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();
        {
            ClearBackground(BLACK);

            for (int i = 0;
                 draw_wall && i < BOUNDARYS_COUNT;
                 ++i)
            {
                boundary_draw(walls[i]);
            }
       
            if (draw_wall)
            {
                particle_look(&particle, walls, BOUNDARYS_COUNT);
            }

            particle_draw(&particle);

            trail2d_draw(particle_trail);
            
            //DrawText(TextFormat("Screen  W:%d H%d", GetScreenWidth(), GetScreenHeight()),   10, 10, 10, MAGENTA);
            //DrawText(TextFormat("Monitor W:%d H%d", GetMonitorWidth(0), GetMonitorHeight(0)), 10, 30, 10, MAGENTA);
            
            //DrawText(TextFormat("Min noise: %.3f  Max noise: %.3f", min_noise, max_noise), 10, 10, 20, MAGENTA);

            //DrawRectangle(      10, 10, 250, 113, Fade(SKYBLUE, 0.5f));
            //DrawRectangleLines( 10, 10, 250, 113, BLUE);
            //DrawText("Free 2d camera controls:", 20, 20, 10, BLACK);
            //DrawText("- Right/Left to move Offset", 40, 40, 10, DARKGRAY);
            //DrawText("- Mouse Wheel to Zoom in-out", 40, 60, 10, DARKGRAY);
            //DrawText("- A / S to Rotate", 40, 80, 10, DARKGRAY);
            //DrawText("- R to reset Zoom and Rotation", 40, 100, 10, DARKGRAY);

        }
        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    CloseWindow();

    return 0;
}


