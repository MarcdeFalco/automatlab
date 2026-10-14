#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

typedef enum {
    REGEXP_CONST = 0,
    REGEXP_EPSILON,
    REGEXP_OR,
    REGEXP_CONCAT,
    REGEXP_STAR
} regexp_type;

// Union type could reduce the size here
typedef struct regexp {
    char letter;
    struct regexp *arg;
    struct regexp *arg2;
    regexp_type type;
} regexp;

void regexp_print(regexp *r);

void regexp_free(regexp *r)
{
    if (r->type != REGEXP_CONST && r->type != REGEXP_EPSILON)
        regexp_free(r->arg);
    if (r->type == REGEXP_OR || r->type == REGEXP_CONCAT)
        regexp_free(r->arg2);
    free(r);
}

char peek_char(const char **b)
{
    char c = **b;
    return c;
}

char next_char(const char **b)
{
    char c = **b;
    (*b)++;
    return c;
}

void unread_char(const char **b)
{
    *b -= 1;
}

// regexp = concat '|' regexp | concat
// concat = facteur | facteur concat
// facteur = base '*' | base
// base = lettre | '(' regexp ')'
regexp *regexp_parse(const char **b);

regexp *leaf_parse(const char **b)
{
    char c = peek_char(b);

    if (c != '(' && !isalnum(c))
        return NULL;

    c = next_char(b);

    if (c == '(')
    {
        regexp *r = regexp_parse(b);
        if (r == NULL) return NULL;
        char c = next_char(b);
        if (c != ')')
        {
            regexp_free(r);
            return NULL;
        }
        return r;
    }

    regexp *r = malloc(sizeof(regexp));
    if (c == 'e')
        r->type = REGEXP_EPSILON;
    else {
        r->type = REGEXP_CONST;
        r->letter = c;
    }
    return r;
}

regexp *factor_parse(const char **b)
{
    regexp *r = leaf_parse(b);
    if (r == NULL) return NULL;
    char c = peek_char(b);
    if (c != '*')
    {
        return r;
    }

    next_char(b);
    regexp *arg = r;
    r = malloc(sizeof(regexp));
    r->type = REGEXP_STAR;
    r->arg = arg;
    return r;
}

regexp *concat_parse(const char **b)
{
    regexp *r = factor_parse(b);
    if (r == NULL) return NULL;

    regexp *arg = r;
    r = malloc(sizeof(regexp));
    r->type = REGEXP_CONCAT;
    r->arg = arg;
    r->arg2 = concat_parse(b);
    if (r->arg2 == NULL)
    {
        free(r);
        return arg;
    }

    return r;
}

regexp *regexp_parse(const char **b)
{
    regexp *r = concat_parse(b);
    if (r == NULL)
        return NULL;

    if (peek_char(b) == ')' || peek_char(b) == '\0')
        return r; // ne pas consommer '\0' (évite une lecture hors chaîne)

    char c = next_char(b);
    
    if (c != '|')
    {
        regexp_free(r);
        return NULL;
    }

    regexp *arg = r;
    r = malloc(sizeof(regexp));
    r->type = REGEXP_OR;
    r->arg = arg;
    r->arg2 = regexp_parse(b);
    if (r->arg2 == NULL)
    {
        regexp_free(r->arg);
        free(r);
        return NULL;
    }

    return r;
}

void regexp_postfix(regexp *r)
{
    if (r == NULL)
    {
        printf("ERROR");
        return;
    }
    switch(r->type)
    {
        case REGEXP_EPSILON:
            putchar('e');
            break;
        case REGEXP_CONST:
            putchar(r->letter);
            break;
        case REGEXP_OR:
        case REGEXP_CONCAT:
            regexp_postfix(r->arg);
            regexp_postfix(r->arg2);
            putchar((r->type == REGEXP_OR) ? '|' : '.');
            break;
        case REGEXP_STAR: 
            regexp_postfix(r->arg);
            putchar('*');
    }
}

void regexp_print(regexp *r)
{
    if (r == NULL)
    {
        printf("ERROR");
        return;
    }
    switch(r->type)
    {
        case REGEXP_EPSILON:
            putchar('e');
            break;
        case REGEXP_CONST:
            putchar(r->letter);
            break;
        case REGEXP_OR:
        case REGEXP_CONCAT:
            putchar('(');
            regexp_print(r->arg);
            if (r->type == REGEXP_OR)
                putchar('|');
            regexp_print(r->arg2);
            putchar(')');
            break;
        case REGEXP_STAR: 
            putchar('(');
            regexp_print(r->arg);
            putchar(')');
            putchar('*');
    }
}


struct state;

typedef struct {
    char letter;
    int tgt;
} edge;

typedef struct state {
    char name[256];
    bool initial;
    bool final;
    edge *transitions;
    int ntransitions;
    double x;
    double y;
} state;

typedef struct {
    int nstates;
    state *states;
} automaton;

automaton *automaton_init()
{
    automaton *a = malloc(sizeof(automaton));
    a->nstates = 0;
    a->states = NULL;
    return a;
}

void automaton_add_transition(automaton *a, 
    int isrc,
    int itgt, char letter)
{
    state *s = &a->states[isrc];
    s->ntransitions++;
    s->transitions = realloc(s->transitions, sizeof(edge) * s->ntransitions);
    edge *e = &s->transitions[s->ntransitions-1];
    e->letter = letter;
    e->tgt = itgt;
}

int automaton_add_state(automaton *a, 
    const char *name, bool initial, bool final)
{
    a->nstates++;
    a->states = realloc(a->states, 
        a->nstates * sizeof(state));
    state *s = &a->states[a->nstates - 1];
    s->initial = initial; 
    s->final = final;
    strncpy(s->name, name, 256);
    s->transitions = NULL;
    s->ntransitions = 0;
    s->x = 0.;
    s->y = 0.;
    return a->nstates-1;
}

void automaton_free(automaton *a)
{
    for(int i = 0; i < a->nstates; i++)
        free(a->states[i].transitions);
    free(a->states);
    free(a);
}

void automaton_print(automaton *a)
{
    for(int i = 0; i < a->nstates; i++)
    {
        printf("[%d]%d | ", i, a->states[i].ntransitions);
        for(int j = 0; j < a->states[i].ntransitions; j++)
            printf("%c->[%d] ", 
                    a->states[i].transitions[j].letter, 
                    a->states[i].transitions[j].tgt);
        printf("\n");
    }
}

int automaton_find_initial(automaton *a)
{
    for(int i = 0; i < a->nstates; i++)
        if (a->states[i].initial)
            return i;
    return -1;
}

int automaton_find_final(automaton *a)
{
    for(int i = 0; i < a->nstates; i++)
        if (a->states[i].final)
            return i;
    return -1;
}

void automaton_extend(automaton *a, automaton *a2)
{
    int n1 = a->nstates;
    int n2 = a2->nstates;
    a->states = realloc(a->states, 
        sizeof(state) * (n1+n2));
    a->nstates += n2;
    memcpy(a->states + n1, 
        a2->states, sizeof(state) * n2);

    for(int i = 0; i < n2; i++)
    {
        a->states[i+n1].transitions = malloc(sizeof(edge) * a->states[i+n1].ntransitions);
        for(int j = 0; j < a2->states[i].ntransitions; j++)
        {
            a->states[i+n1].transitions[j].letter = a2->states[i].transitions[j].letter;
            a->states[i+n1].transitions[j].tgt = n1 + a2->states[i].transitions[j].tgt;
        }
    }
}

automaton *regexp_thompson(regexp *r)
{
    if (r->type == REGEXP_EPSILON || r->type == REGEXP_CONST)
    {
        automaton *a = automaton_init();
        int i = automaton_add_state(a, "", true, false);
        int f = automaton_add_state(a, "", false, true);
        char c = r->letter;
        if (r->type == REGEXP_EPSILON)
            c = '\0';
        automaton_add_transition(a, i, f, c);
        return a;
    }

    automaton *a = regexp_thompson(r->arg);
    int a1_i = automaton_find_initial(a);
    int a1_f = automaton_find_final(a);
    a->states[a1_f].final = false;
    a->states[a1_i].initial = false;

    if (r->type == REGEXP_STAR)
    {
        int i = automaton_add_state(a, "", true, false);
        int f = automaton_add_state(a, "", false, true);
        automaton_add_transition(a, i, a1_i, '\0');
        automaton_add_transition(a, a1_f, f, '\0');
        automaton_add_transition(a, i, f, '\0'); // i -> f (et non f -> i) : le mot vide doit être reconnu
        automaton_add_transition(a, a1_f, a1_i, '\0');

        return a;
    }

    automaton *a2 = regexp_thompson(r->arg2);
    automaton_extend(a, a2);
    automaton_free(a2);

    int a2_i = automaton_find_initial(a);
    int a2_f = automaton_find_final(a);
    a->states[a2_f].final = false;
    a->states[a2_i].initial = false;

    if (r->type == REGEXP_CONCAT)
    {
        a->states[a1_i].initial = true;
        a->states[a2_f].final = true;
        automaton_add_transition(a, a1_f, a2_i, '\0');
        return a;
    }
    if (r->type == REGEXP_OR)
    {
        int i = automaton_add_state(a, "", true, false);
        int f = automaton_add_state(a, "", false, true);
        automaton_add_transition(a, i, a1_i, '\0');
        automaton_add_transition(a, i, a2_i, '\0');
        automaton_add_transition(a, a1_f, f, '\0');
        automaton_add_transition(a, a2_f, f, '\0');
        return a;
    }
  
    // unreachable
    return NULL;
}

void automaton_place_random(automaton *a, double W, double H)
{
    for(int i = 0; i < a->nstates; i++)
    {
        a->states[i].x = W * (drand48() - 0.5);
        a->states[i].y = H * (drand48() - 0.5);
    }
}

#define MAX(a,b) ((a>b)?(a):(b))
#define MIN(a,b) ((a>b)?(b):(a))
#define NITER 50
double transition_length(automaton *a, int i, int j)
{
    double x1 = a->states[i].x;
    double x2 = a->states[j].x;
    double y1 = a->states[i].y;
    double y2 = a->states[j].y;
    return sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
}

void automaton_place_force(automaton *a, double W, double H)
{
    double area = W * H;
    // Fruchterman-Reingold : k = sqrt(aire / n), température initiale ~ W/10
    double temp = W * 0.1;
    double dt = temp / (NITER+1);
    double k_ = 0.5 * sqrt(area / a->nstates);

    double *vx = malloc(sizeof(double)*a->nstates);
    double *vy = malloc(sizeof(double)*a->nstates);
    memset(vx, 0, sizeof(double)*a->nstates);
    memset(vy, 0, sizeof(double)*a->nstates);
    
    automaton_place_random(a, W/2, H/2);

    for (int i = 0; i < NITER; i++)
    {
        for (int j = 0; j < a->nstates; j++)
        {
            double xj = a->states[j].x;
            double yj = a->states[j].y;

            vx[j] = 0.;
            vy[j] = 0.;

            for(int k = 0; k < a->nstates; k++)
            {
                double xk = a->states[k].x;
                double yk = a->states[k].y;
                double dx = xj - xk;
                double dy = yj - yk;

                double d = sqrt(dx * dx + dy * dy) + 0.01; 

                if (k != j)
                {
                    vx[j] += (dx/d) * (k_*k_/d);
                    vy[j] += (dy/d) * (k_*k_/d);
                }
            }
        }

    
        for (int j = 0; j < a->nstates; j++)
        {
            state *s = &a->states[j];
            for (int k = 0; k < s->ntransitions; k++)
            {
                int ik = s->transitions[k].tgt;
                double d = transition_length(a, j, ik) + 0.01;
                if (true)
                {
                    double sx = s->x;
                    double sy = s->y;
                    double tx = a->states[ik].x;
                    double ty = a->states[ik].y;
                    double dx = sx - tx;
                    double dy = sy - ty;
                    vx[ik] += (dx/d) * (d*d/k_);
                    vy[ik] += (dy/d) * (d*d/k_);
                    vx[j] -= (dx/d) * (d*d/k_);
                    vy[j] -= (dy/d) * (d*d/k_);
                }
            }
        }

        for (int j = 0; j < a->nstates; j++)
        {
            state *s = &a->states[j];
            double xi = s->x;
            double yi = s->y;
            double dp = sqrt(vx[j]*vx[j] + vy[j]*vy[j]) + 0.01;
            vx[j] *= MIN(dp, temp) / dp;
            vy[j] *= MIN(dp, temp) / dp;
            xi += vx[j];
            yi += vy[j];
            s->x = MIN(W/2.,MAX(-W/2.,xi));
            s->y = MIN(H/2.,MAX(-H/2.,yi));
        }

        temp -= dt;
        if (temp < 0) break;
    }
}

const int screenWidth = 800;
const int screenHeight = 600;
const int statusBarHeight = 20;

const int controlPanel = 200;
const int toolBarHeight = 40;

double W = (double) (screenWidth);
double H = (double) (screenHeight - statusBarHeight - toolBarHeight);

char regexp_text[1024] = "ab(a|b)";
bool regexp_edit = false;
bool regexp_parse_error = false;

automaton *thompson = NULL;

void UpdateDrawFrame()
{
    bool regexp_change = false;
    BeginDrawing();

    ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

    GuiStatusBar((Rectangle){ 0, 0, screenWidth, toolBarHeight}, "");

    if (GuiTextBox((Rectangle){ 1, 1, 100, toolBarHeight-2 },
        regexp_text, toolBarHeight-2, regexp_edit))
    {
        if(regexp_edit)
            regexp_change = true;
        regexp_edit = !regexp_edit;
    }

    if (regexp_parse_error)
        GuiLabel((Rectangle){ 101, 1, 201, toolBarHeight-2 }, "Parsing error");

    Camera2D cam;
    cam.offset = (Vector2){screenWidth / 2., screenHeight / 2.};
    cam.rotation = 0;
    cam.target = (Vector2){0., 0.};
    cam.zoom = 1;
    BeginMode2D(cam);

    if (thompson)
    {
        for(int i = 0; i < thompson->nstates; i++)
        {
            state *s = &thompson->states[i];
            for(int j = 0; j < s->ntransitions; j++)
            {
                state *s2 = &thompson->states[s->transitions[j].tgt];
                char buf[100];
                if (s->transitions[j].letter == '\0')
                    sprintf(buf, "e"); // epsilon-transition
                else
                    sprintf(buf, "%c", s->transitions[j].letter);
                DrawLineV((Vector2){ s->x, s->y }, (Vector2){ s2->x, s2->y }, BLACK);
                DrawText(buf, (s->x+s2->x)/2., (s->y+s2->y)/2., 5, BLACK);
            }
        }
        for(int i = 0; i < thompson->nstates; i++)
        {
            state *s = &thompson->states[i];
            Color c = BLACK;
            if (s->initial) c = RED;
            DrawCircleV((Vector2){ s->x, s->y },
                    10., c);
            // l'état final est dessiné par-dessus (il était auparavant masqué)
            if (s->final)
                DrawCircleV((Vector2){ s->x, s->y },
                        7., GREEN);
        }
    }

    EndMode2D();


    EndDrawing();

    if (regexp_change || thompson == NULL) //FIXME
    {
        const char *b = regexp_text;
        regexp *r = regexp_parse(&b);
        if (r != NULL && *b != '\0')
        {
            // caractères non lus (par ex. ')' en trop) : erreur
            regexp_free(r);
            r = NULL;
        }
        regexp_parse_error = false;
        if(r != NULL)
        {
            if (thompson != NULL)
                automaton_free(thompson);
            thompson = regexp_thompson(r);
            //automaton_print(thompson);

            automaton_place_force(thompson, 400, 400);
            /*
            regexp_postfix(r);
            putchar('\n');
            regexp_print(r);
            putchar('\n');
            */
            regexp_free(r);
        }
        else
        {
            if (thompson != NULL)
                automaton_free(thompson); // évitait une fuite mémoire
            thompson = NULL;
            regexp_parse_error = true;
        }
    }
}

int main(void)
{
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Automatlab");
    
#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }
#endif

    CloseWindow();

    return 0;
}
