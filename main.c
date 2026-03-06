
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//==============================
//
//   Arena allocator
//
//==============================

#define MAX_ARENA_CAPACITY 1024 * 1024 // 1MB
typedef struct {
  void *base;
  size_t offset;
  size_t size;
} Arena;

Arena arena_create() {
  Arena a = {0};
  a.base = malloc(MAX_ARENA_CAPACITY);
  return a;
}

void arena_destroy(Arena *a) { free(a->base); }

void *arena_allocate(Arena *a, size_t size) {
  size_t aligned = (size + 7) & ~7;
  if (a->offset + aligned > MAX_ARENA_CAPACITY) {
    printf("Arena ran out of memory!\n");
    return NULL;
  }
  void *addr = a->base + a->offset;
  a->offset += aligned;
  return addr;
}

//==============================
//
//   Hash
//
//==============================

#define NUM_HASH_BINS 64
#define MAX_HASH_LIST_DEPTH 64

typedef struct {
  const char *key;
  size_t value;
} MappedItem;

typedef struct {
  MappedItem **items;
  size_t *num_elements;
} Hash;

Hash *create_set(Arena *a) {
  Hash *set = arena_allocate(a, sizeof(Hash));
  set->items = arena_allocate(a, NUM_HASH_BINS * sizeof(MappedItem *));
  set->num_elements = arena_allocate(a, NUM_HASH_BINS * sizeof(size_t));
  for (size_t bin_idx = 0; bin_idx < NUM_HASH_BINS; bin_idx++) {
    set->num_elements[bin_idx] = 0;
    set->items[bin_idx] =
        arena_allocate(a, MAX_HASH_LIST_DEPTH * sizeof(MappedItem));
  }
  return set;
}

size_t hash_string(const char *str) {
  unsigned long hash = 5381;
  int c;

  while ((c = *str++)) {
    // hash * 33 + c
    hash = ((hash << 5) + hash) + c;
  }

  return hash;
}

int hash_get_element_bin_index(Hash *set, const char *key) {
  size_t bin = hash_string(key) % NUM_HASH_BINS;
  size_t n_in_bin = set->num_elements[bin];
  if (set->num_elements[bin] == 0)
    return -1;
  for (size_t i = 0; i < n_in_bin; i++) {
    if (strcmp(key, set->items[bin][i].key) == 0) {
      return i;
    }
  }
  return -1;
}

int hash_get_element(Hash *set, const char *key) {
  size_t bin = hash_string(key) % NUM_HASH_BINS;
  int idx = hash_get_element_bin_index(set, key);
  if (idx < 0)
    return idx;
  return set->items[bin][idx].value;
}

int hash_has_element(Hash *set, const char *key) {
  size_t bin = hash_string(key) % NUM_HASH_BINS;
  size_t n_in_bin = set->num_elements[bin];
  if (n_in_bin == 0)
    return 0;
  int idx = hash_get_element_bin_index(set, key);
  if (idx < 0)
    return 0;
  return 1;
}

void hash_put_element(Hash *set, const char *key, size_t value) {
  if (hash_has_element(set, key))
    return;
  size_t bin = hash_string(key) % NUM_HASH_BINS;
  MappedItem item;
  item.key = key;
  item.value = value;
  set->items[bin][set->num_elements[bin]++] = item;
}

//==============================
//
//   Graphs
//
//==============================

typedef struct {
  size_t index_a;
  size_t index_b;
} Edge;

typedef struct {
  Edge *edges;
  size_t size_a;
  size_t size_b;
  size_t num_edges;
} BipartiteGraph;

BipartiteGraph *graph_create(Arena *a, size_t size_a, size_t size_b) {
  BipartiteGraph *graph = arena_allocate(a, sizeof(BipartiteGraph));
  graph->size_a = size_a;
  graph->size_b = size_b;
  graph->edges = arena_allocate(a, size_a * size_b * sizeof(Edge));
  graph->num_edges = 0;
  return graph;
}

void push_edge(BipartiteGraph *g, size_t index_a, size_t index_b) {
  Edge e = {0};
  e.index_a = index_a;
  e.index_b = index_b;
  g->edges[g->num_edges++] = e;
}

void graph_print(BipartiteGraph *g) {
  printf("> Bipartite Graph\n");
  printf("    Equations: %zu\n", g->size_a);
  printf("    Variables: %zu\n", g->size_b);
  printf("    Edges:     %zu\n", g->num_edges);
  printf("> Edges:\n");
  for (size_t i = 0; i < g->num_edges; i++) {
    Edge e = g->edges[i];
    printf("    EQ[%zu]-VAR[%zu]\n", e.index_a, e.index_b);
  }
}

int kuhn_dfs(BipartiteGraph *g, int *matches, int *visited, size_t u) {
  for (size_t edge_idx = 0; edge_idx < g->num_edges; edge_idx++) {
    Edge e = g->edges[edge_idx];
    if (e.index_a == u) {
      size_t v = e.index_b;
      if (!visited[v]) {
        visited[v] = 1;
        if (matches[v] == -1 || kuhn_dfs(g, matches, visited, matches[v])) {
          matches[v] = u;
          return 1;
        }
      }
    }
  }
  return 0;
}

BipartiteGraph *bipartite_matching(Arena *a, BipartiteGraph *g) {
  // Kuhn's algorithm

  int *matches = arena_allocate(a, g->size_b * sizeof(int));
  int *visited = arena_allocate(a, g->size_b * sizeof(int));
  for (size_t i = 0; i < g->size_b; i++) {
    matches[i] = -1;
  }

  for (size_t u = 0; u < g->size_a; u++) {
    memset(visited, 0, g->size_b * sizeof(int));
    kuhn_dfs(g, matches, visited, u);
  }

  BipartiteGraph *matching_graph = graph_create(a, g->size_a, g->size_b);
  for (size_t i = 0; i < g->size_b; i++) {
    int match_u = matches[i];
    if (match_u > -1) {
      push_edge(matching_graph, match_u, i);
    }
  }

  return matching_graph;
}

//==============================
//
//   Equations and Variables
//
//==============================

#define MAX_NUM_VARIABLES 64
#define MAX_NUM_EQNS 64

typedef struct {
  const char *name;
  size_t index;
} Variable;

typedef struct {
  size_t index;
  size_t num_variables;
  Variable *variables[MAX_NUM_VARIABLES];
} Equation;

typedef struct {
  Variable *variables[MAX_NUM_VARIABLES];
  size_t num_variables;
  Equation *equations[MAX_NUM_EQNS];
  size_t num_equations;
  Hash *variables_set;
} SystemOfEquations;

Equation *equation_create(Arena *a) {
  Equation *eq = arena_allocate(a, sizeof(Equation));
  return eq;
}

Variable *variable_create(Arena *a, const char *variable_name) {
  Variable *var = arena_allocate(a, sizeof(Variable));
  var->name = variable_name;
  return var;
}

void push_variable(Arena *a, SystemOfEquations *sys,
                   const char *variable_name) {
  if (hash_has_element(sys->variables_set, variable_name))
    return;
  Variable *var = variable_create(a, variable_name);
  var->index = sys->num_variables;
  sys->variables[sys->num_variables++] = var;
  hash_put_element(sys->variables_set, variable_name, var->index);
}

void push_equation(Arena *a, SystemOfEquations *sys, Equation *eq) {
  eq->index = sys->num_variables;
  sys->equations[sys->num_equations++] = eq;
}

SystemOfEquations *system_of_equations_create(Arena *a) {
  SystemOfEquations *sys = arena_allocate(a, sizeof(SystemOfEquations));
  sys->variables_set = create_set(a);
  sys->num_variables = 0;
  sys->num_equations = 0;
  return sys;
}

void init_system_of_equations(Arena *a, SystemOfEquations *sys,
                              size_t num_equations,
                              const char ***equation_variables,
                              size_t *num_equation_variables) {

  // Registering equations and binding them to variables

  for (size_t eq_idx = 0; eq_idx < num_equations; eq_idx++) {
    Equation *eq = equation_create(a);
    eq->index = eq_idx;
    sys->equations[sys->num_equations++] = eq;

    // Registering equation variables

    for (size_t var_idx = 0; var_idx < num_equation_variables[eq_idx];
         var_idx++) {
      const char *variable_name = equation_variables[eq_idx][var_idx];
      int element = hash_get_element(sys->variables_set, variable_name);
      if (element < 0) {
        push_variable(a, sys, variable_name);
        element = hash_get_element(sys->variables_set, variable_name);
      }
      size_t var_idx_in_soe = element;
      Variable *var = sys->variables[var_idx_in_soe];
      eq->variables[eq->num_variables++] = var;
    }
  }
}

void print_system_of_equations(SystemOfEquations *sys) {
  printf("Variables:\n");
  printf("  ");
  for (size_t i = 0; i < sys->num_variables; i++) {
    const char *variable_name = sys->variables[i]->name;
    printf("%s", variable_name);
    if (i < sys->num_variables - 1) {
      printf(", ");
    }
  }
  printf("\n");
  printf("Equations:\n");
  for (size_t i = 0; i < sys->num_equations; i++) {
    Equation *eq = sys->equations[i];
    printf("  ");
    printf("[%zu] ", i);
    for (size_t var_idx = 0; var_idx < eq->num_variables; var_idx++) {
      Variable *var = eq->variables[var_idx];
      printf("%s", var->name);
      if (var_idx < eq->num_variables - 1) {
        printf(", ");
      }
    }
    printf("\n");
  }
  printf("\n");
}

BipartiteGraph *graph_from_system_of_equations(Arena *a,
                                               SystemOfEquations *sys) {
  BipartiteGraph *g = graph_create(a, sys->num_equations, sys->num_variables);
  for (size_t eq_idx = 0; eq_idx < sys->num_equations; eq_idx++) {
    Equation *eq = sys->equations[eq_idx];
    for (size_t var_idx = 0; var_idx < eq->num_variables; var_idx++) {
      Variable *var = eq->variables[var_idx];
      int value = hash_get_element(sys->variables_set, var->name);
      if (value < 0) {
        printf("Could not find an element corresponding to %s in the system\n",
               var->name);
        continue;
      }
      size_t idx_in_soe = value;
      push_edge(g, eq_idx, idx_in_soe);
    }
  }
  return g;
}

//==============================
//
//   Tokenizer
//
//==============================

#define MAX_TOKENS 1024
typedef enum {
  TOKEN_NUMBER,
  TOKEN_IDENTIFIER,
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_STAR,
  TOKEN_SLASH,
  TOKEN_CARET,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_COMMA,
  TOKEN_EQUALS
} token_t;

typedef struct {
  token_t type;
  const char *value;
} Token;

typedef struct {
  size_t pointer;
  const char *string;
  Token tokens[MAX_TOKENS];
  size_t tokens_count;
} Tokenizer;

void push_token(Tokenizer *t, Token tok) {
  assert(t->tokens_count < MAX_TOKENS);
  t->tokens[t->tokens_count++] = tok;
}

void tokenize(const char *text) {
  Tokenizer t = {0};
  t.string = text;
  while (t.pointer < strlen(text)) {
    char c = t.string[t.pointer++];
    switch (c) {
    case '+': {
      Token tok;
      tok.type = TOKEN_PLUS;
      push_token(&t, tok);
      break;
    }
    case '-': {
      Token tok;
      tok.type = TOKEN_MINUS;
      push_token(&t, tok);
      break;
    }
    case '*': {
      Token tok;
      tok.type = TOKEN_STAR;
      push_token(&t, tok);
      break;
    }
    case '/': {
      Token tok;
      tok.type = TOKEN_SLASH;
      push_token(&t, tok);
      break;
    }
    case '^': {
      Token tok;
      tok.type = TOKEN_CARET;
      push_token(&t, tok);
      break;
    }
    case '(': {
      Token tok;
      tok.type = TOKEN_LPAREN;
      push_token(&t, tok);
      break;
    }
    case ')': {
      Token tok;
      tok.type = TOKEN_RPAREN;
      push_token(&t, tok);
      break;
    }
    case '=': {
      Token tok;
      tok.type = TOKEN_EQUALS;
      push_token(&t, tok);
      break;
    }
    case ',': {
      Token tok;
      tok.type = TOKEN_COMMA;
      push_token(&t, tok);
      break;
    }
    }
  }
}

int main() {
  Arena a = arena_create();

  //  e(x*y/z) - x^2 = 3
  //  x*z = 2
  //  z = 2

  // const char ***equation_vars = arena_allocate(&a, 3 * sizeof(char **));
  // equation_vars[0] = arena_allocate(&a, 3 * sizeof(char *));
  // equation_vars[0][0] = "x";
  // equation_vars[0][1] = "y";
  // equation_vars[0][2] = "z";
  //
  // equation_vars[1] = arena_allocate(&a, 2 * sizeof(char *));
  // equation_vars[1][0] = "x";
  // equation_vars[1][1] = "z";
  //
  // equation_vars[2] = arena_allocate(&a, 1 * sizeof(char *));
  // equation_vars[2][0] = "z";
  //
  // size_t *num_eq_vars = arena_allocate(&a, 3 * sizeof(size_t));
  // num_eq_vars[0] = 3;
  // num_eq_vars[1] = 2;
  // num_eq_vars[2] = 1;
  //
  // SystemOfEquations *soe = system_of_equations_create(&a);
  //
  // init_system_of_equations(&a, soe, 3, equation_vars, num_eq_vars);
  // print_system_of_equations(soe);
  // BipartiteGraph *g = graph_from_system_of_equations(&a, soe);
  // graph_print(g);
  //
  // BipartiteGraph *matches = bipartite_matching(&a, g);
  // graph_print(matches);

  const char *expression = "2.34 = cos(t) - log(x/y)";
  tokenize(expression);

  arena_destroy(&a);

  return 0;
}
