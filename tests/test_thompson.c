#define main app_main
#include "../main.c"
#undef main
#include <regex.h>

static void closure(automaton *a, bool *set) {
    bool ch = true;
    while (ch) { ch = false;
        for (int i = 0; i < a->nstates; i++) if (set[i])
            for (int j = 0; j < a->states[i].ntransitions; j++) {
                edge e = a->states[i].transitions[j];
                if (e.letter == '\0' && !set[e.tgt]) { set[e.tgt] = true; ch = true; }
            }
    }
}
static bool accepts(automaton *a, const char *w) {
    int n = a->nstates; bool *cur = calloc(n,1), *nx = calloc(n,1);
    for (int i = 0; i < n; i++) cur[i] = a->states[i].initial;
    closure(a, cur);
    for (; *w; w++) {
        memset(nx, 0, n);
        for (int i = 0; i < n; i++) if (cur[i])
            for (int j = 0; j < a->states[i].ntransitions; j++)
                if (a->states[i].transitions[j].letter == *w) nx[a->states[i].transitions[j].tgt] = true;
        closure(a, nx); memcpy(cur, nx, n);
    }
    bool r = false; for (int i = 0; i < n; i++) if (cur[i] && a->states[i].final) r = true;
    free(cur); free(nx); return r;
}
int main(int argc, char **argv) {
    // argv: regexp-in-tool-syntax  POSIX-ERE-reference
    for (int k = 1; k + 1 < argc; k += 2) {
        const char *b = argv[k]; regexp *r = regexp_parse(&b);
        printf("== %s : ", argv[k]);
        if (!r) { printf("PARSE ERROR\n"); continue; }
        regexp_print(r); printf("  postfix=");  regexp_postfix(r);
        automaton *a = regexp_thompson(r);
        int ni = 0, nf = 0; for (int i=0;i<a->nstates;i++){ni+=a->states[i].initial; nf+=a->states[i].final;}
        printf("  states=%d init=%d final=%d\n", a->nstates, ni, nf);
        regex_t re; char pat[256]; snprintf(pat, sizeof pat, "^(%s)$", argv[k+1]);
        if (regcomp(&re, pat, REG_EXTENDED|REG_NOSUB)) { printf("bad ref\n"); continue; }
        int bad = 0, tot = 0; char w[16];
        for (int len = 0; len <= 7; len++) for (int m = 0; m < (1<<len); m++) {
            for (int i = 0; i < len; i++) w[i] = (m>>i)&1 ? 'b' : 'a'; w[len]=0; tot++;
            bool x = accepts(a, w), y = regexec(&re, w, 0, NULL, 0) == 0;
            if (x != y) { if (bad < 5) printf("   MISMATCH '%s': thompson=%d ref=%d\n", w, x, y); bad++; }
        }
        printf("   %d/%d mots en desaccord\n", bad, tot);
        if (argc == 3) automaton_print(a);
        automaton_free(a); regexp_free(r);
    }
}
