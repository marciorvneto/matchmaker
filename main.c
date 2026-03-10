
#include <assert.h>
#include <ctype.h>
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
  const char *name;
  size_t index;
  size_t num_variables;
  Variable *variables[MAX_NUM_VARIABLES];
  Hash *variables_set;
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
  eq->variables_set = create_set(a);
  eq->name = NULL;
  return eq;
}

Variable *variable_create(Arena *a, const char *variable_name) {
  Variable *var = arena_allocate(a, sizeof(Variable));
  var->name = variable_name;
  return var;
}

void push_variable_to_equation(Arena *a, SystemOfEquations *sys, Equation *eq,
                               const char *variable_name) {
  if (hash_has_element(eq->variables_set, variable_name))
    return;
  // Assumes the system knows about it!
  int idx = hash_get_element(sys->variables_set, variable_name);
  if (idx < 0)
    return;
  hash_put_element(sys->variables_set, variable_name, idx);
  eq->variables[eq->num_variables++] = sys->variables[idx];
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

SystemOfEquations *system_of_equations_create(Arena *a) {
  SystemOfEquations *sys = arena_allocate(a, sizeof(SystemOfEquations));
  sys->variables_set = create_set(a);
  sys->num_variables = 0;
  sys->num_equations = 0;
  return sys;
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
    if (eq->name != NULL) {
      printf("[%s] ", eq->name);
    } else {
      printf("[%zu] ", i);
    }
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
#define MAX_NUM_SIZE 128
#define MAX_ID_SIZE 128
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
  TOKEN_LCURLY,
  TOKEN_RCURLY,
  TOKEN_COMMA,
  TOKEN_EQUALS,
  TOKEN_NEWLINE,
  TOKEN_END,
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

void read_number(Arena *a, Tokenizer *t, Token *tok) {
  assert(t->tokens_count < MAX_TOKENS);
  char *buf = arena_allocate(a, MAX_NUM_SIZE);
  int decimal = 0;
  size_t i = 0;
  for (size_t i = 0; i < MAX_NUM_SIZE; i++) {
    if (isdigit(t->string[t->pointer])) {
      buf[i] = t->string[t->pointer++];
      continue;
    } else {
      if (!decimal && t->string[t->pointer] == '.') {
        decimal = 1;
        buf[i] = t->string[t->pointer++];
        continue;
      }
      break;
    }
  }
  tok->value = buf;
}

void read_id(Arena *a, Tokenizer *t, Token *tok) {
  assert(t->tokens_count < MAX_TOKENS);
  char *buf = arena_allocate(a, MAX_NUM_SIZE);
  size_t i = 0;
  for (size_t i = 0; i < MAX_NUM_SIZE; i++) {
    char c = t->string[t->pointer];
    if (isalpha(c) || isdigit(c) || c == '_') {
      buf[i] = c;
      t->pointer++;
      continue;
    } else {
      break;
    }
  }
  tok->value = buf;
}

void tokenize(Arena *a, Tokenizer *t, const char *text) {
  t->string = text;
  while (t->pointer < strlen(text)) {
    char c = t->string[t->pointer];
    switch (c) {
    case '+': {
      Token tok;
      tok.type = TOKEN_PLUS;
      push_token(t, tok);
      t->pointer++;
      break;
    }
    case '-': {
      Token tok;
      tok.type = TOKEN_MINUS;
      push_token(t, tok);
      t->pointer++;
      break;
    }
    case '*': {
      Token tok;
      tok.type = TOKEN_STAR;
      push_token(t, tok);
      t->pointer++;
      break;
    }
    case '/': {
      Token tok;
      tok.type = TOKEN_SLASH;
      push_token(t, tok);
      t->pointer++;
      break;
    }
    case '^': {
      Token tok;
      tok.type = TOKEN_CARET;
      push_token(t, tok);
      t->pointer++;
      break;
    }
    case '(': {
      Token tok;
      tok.type = TOKEN_LPAREN;
      push_token(t, tok);
      t->pointer++;
      break;
    }
    case ')': {
      Token tok;
      tok.type = TOKEN_RPAREN;
      push_token(t, tok);
      t->pointer++;
      break;
    }
    case '{': {
      Token tok;
      tok.type = TOKEN_LCURLY;
      push_token(t, tok);
      t->pointer++;
      break;
    }
    case '}': {
      Token tok;
      tok.type = TOKEN_RCURLY;
      push_token(t, tok);
      t->pointer++;
      break;
    }
    case '=': {
      Token tok;
      tok.type = TOKEN_EQUALS;
      push_token(t, tok);
      t->pointer++;
      break;
    }
    case ',': {
      Token tok;
      tok.type = TOKEN_COMMA;
      push_token(t, tok);
      t->pointer++;
      break;
    }
    case '\n': {
      Token tok;
      tok.type = TOKEN_NEWLINE;
      push_token(t, tok);
      t->pointer++;
      break;
    }
    default: {
      if (isdigit(c)) {
        // Pointer gets incremented in read_number
        Token tok;
        tok.type = TOKEN_NUMBER;
        read_number(a, t, &tok);
        push_token(t, tok);
        break;
      }
      if (isalpha(c)) {
        // Pointer gets incremented in read_id
        Token tok;
        tok.type = TOKEN_IDENTIFIER;
        read_id(a, t, &tok);
        push_token(t, tok);
        break;
      }
      t->pointer++;
      break;
    }
    }
  }
  Token end;
  end.type = TOKEN_END;
  t->tokens[t->tokens_count++] = end;
}

void print_token(Token *t) {
  switch (t->type) {
  case TOKEN_NUMBER: {
    printf("TOKEN_NUMBER: %s\n", t->value);
    break;
  }
  case TOKEN_IDENTIFIER: {
    printf("TOKEN_IDENTIFIER: %s\n", t->value);
    break;
  }
  case TOKEN_PLUS: {
    printf("TOKEN_PLUS\n");
    break;
  }
  case TOKEN_MINUS: {
    printf("TOKEN_MINUS\n");
    break;
  }
  case TOKEN_STAR: {
    printf("TOKEN_STAR\n");
    break;
  }
  case TOKEN_SLASH: {
    printf("TOKEN_SLASH\n");
    break;
  }
  case TOKEN_CARET: {
    printf("TOKEN_CARET\n");
    break;
  }
  case TOKEN_LPAREN: {
    printf("TOKEN_LPAREN\n");
    break;
  }
  case TOKEN_RPAREN: {
    printf("TOKEN_RPAREN\n");
    break;
  }
  case TOKEN_LCURLY: {
    printf("TOKEN_LCURLY\n");
    break;
  }
  case TOKEN_RCURLY: {
    printf("TOKEN_RCURLY\n");
    break;
  }
  case TOKEN_COMMA: {
    printf("TOKEN_EQUALS\n");
    break;
  }
  case TOKEN_EQUALS: {
    printf("TOKEN_EQUALS\n");
    break;
  }
  case TOKEN_NEWLINE: {
    printf("TOKEN_NEWLINE\n");
    break;
  }
  case TOKEN_END: {
    printf("TOKEN_END\n");
    break;
  }
  }
}

SystemOfEquations *tokens_to_system(Arena *a, Token *tokens,
                                    size_t tokens_count) {
  SystemOfEquations *sys = system_of_equations_create(a);

  size_t equation_index = 0;
  Equation *eq = equation_create(a);
  const char *eq_name = NULL;
  eq->index = equation_index;
  sys->equations[sys->num_equations++] = eq;
  for (size_t i = 0; i < tokens_count; i++) {
    Token *tok = &tokens[i];
    switch (tok->type) {
    case TOKEN_LCURLY: {
      Token *id = &tokens[i + 1];
      eq->name = id->value;
      i += 2;
      break;
    }
    case TOKEN_NEWLINE: {
      eq = equation_create(a);
      eq->index = ++equation_index;
      sys->equations[sys->num_equations++] = eq;
      break;
    }
    case TOKEN_IDENTIFIER: {
      if (tokens[i + 1].type == TOKEN_LPAREN) {
        // Function call
        continue;
      }
      int element = hash_get_element(sys->variables_set, tok->value);
      push_variable(a, sys, tok->value);
      push_variable_to_equation(a, sys, eq, tok->value);
      break;
    }
    default: {
      break;
    }
    }
  }
  return sys;
}

int main() {
  Arena a = arena_create();

  const char *expression = "{eq1_asd}  2.34 = cos(t) - log(x/y)\nt/x = 23\nt=2";
  Tokenizer t = {0};
  tokenize(&a, &t, expression);

  for (size_t i = 0; i < t.tokens_count; i++) {
    print_token(&t.tokens[i]);
  }

  SystemOfEquations *sys = tokens_to_system(&a, t.tokens, t.tokens_count);
  print_system_of_equations(sys);

  BipartiteGraph *g = graph_from_system_of_equations(&a, sys);
  graph_print(g);

  BipartiteGraph *matches = bipartite_matching(&a, g);
  graph_print(matches);

  arena_destroy(&a);

  return 0;
}
