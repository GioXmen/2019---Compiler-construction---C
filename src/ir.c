#include <vslc.h>
#include <string.h>

tlhash_t **scopes_n;
size_t scope_counter;

// Externally visible, for the generator
extern tlhash_t *global_names;
extern char **string_list;
extern size_t n_string_list,stringc;

/* External interface */
void create_symbol_table(void);
void print_symbol_table(void);
void find_globals(void);
void bind_names(symbol_t *function, node_t *root);
void destroy_symbol_table(void);
void destroy_symtab(void);
void bind_names_tree_sub(node_t*, node_t *, int, int*);
void print_symbols(void);
void print_bindings(node_t *root);


void
create_symbol_table ( void )
{
  find_globals();
  size_t n_globals = tlhash_size ( global_names );
  symbol_t *global_list[n_globals];
  tlhash_values ( global_names, (void **)&global_list );
  for ( size_t i=0; i<n_globals; i++ )
      if ( global_list[i]->type == SYM_FUNCTION )
          bind_names ( global_list[i], global_list[i]->node );
}


void
print_symbol_table ( void )
{
	print_symbols();
	print_bindings(root);
}

void
print_symbols ( void )
{
    printf ( "String table:\n" );
    for ( size_t s=0; s<stringc; s++ )
        printf  ( "%zu: %s\n", s, string_list[s] );
    printf ( "-- \n" );

    printf ( "Globals:\n" );
    size_t n_globals = tlhash_size(global_names);
    symbol_t *global_list[n_globals];
    tlhash_values ( global_names, (void **)&global_list );
    for ( size_t g=0; g<n_globals; g++ )
    {
        switch ( global_list[g]->type )
        {
            case SYM_FUNCTION:
                printf (
                    "%s: function %zu:\n",
                    global_list[g]->name, global_list[g]->seq
                );
                if ( global_list[g]->locals != NULL )
                {
                    size_t localsize = tlhash_size( global_list[g]->locals );
                    printf (
                        "\t%zu local variables, %zu are parameters:\n",
                        localsize, global_list[g]->nparms
                    );
                    symbol_t *locals[localsize];
                    tlhash_values(global_list[g]->locals, (void **)locals );
                    for ( size_t i=0; i<localsize; i++ )
                    {
                        printf ( "\t%s: ", locals[i]->name );
                        switch ( locals[i]->type )
                        {
                            case SYM_PARAMETER:
                                printf ( "parameter %zu\n", locals[i]->seq );
                                break;
                            case SYM_LOCAL_VAR:
                                printf ( "local var %zu\n", locals[i]->seq );
                                break;
                        }
                    }
                }
                break;
            case SYM_GLOBAL_VAR:
                printf ( "%s: global variable\n", global_list[g]->name );
                break;
        }
    }
    printf ( "-- \n" );
}



void
print_bindings ( node_t *root )
{
    if ( root == NULL )
        return;
    else if ( root->entry != NULL )
    {
        switch ( root->entry->type )
        {
            case SYM_GLOBAL_VAR:
                printf ( "Linked global var '%s'\n", root->entry->name );
                break;
            case SYM_FUNCTION:
                printf ( "Linked function %zu ('%s')\n",
                    root->entry->seq, root->entry->name
                );
                break;
            case SYM_PARAMETER:
                printf ( "Linked parameter %zu ('%s')\n",
                    root->entry->seq, root->entry->name
                );
                break;
            case SYM_LOCAL_VAR:
                printf ( "Linked local var %zu ('%s')\n",
                    root->entry->seq, root->entry->name
                );
                break;
        }
    } else if ( root->type == STRING_DATA ) {
        size_t string_index = *((size_t *)root->data);
        if ( string_index < stringc )
            printf ( "Linked string %zu\n", *((size_t *)root->data) );
        else
            printf ( "(Not an indexed string)\n" );
    }
    for ( size_t c=0; c<root->n_children; c++ )
        print_bindings ( root->children[c] );
}

void
destroy_symbol_table ( void )
{
      destroy_symtab();
}

void
find_globals ( void )
{
    int inc_count = 0;
    node_t *gbList = root->children[0];

    global_names = malloc(sizeof(tlhash_t));
    tlhash_init(global_names, 32);

    for (int i = 0; i < gbList->n_children; i++){
        if (gbList->children[i]->type == DECLARATION){

            for (int i = 0; i < gbList->n_children; i++) {
                symbol_t *variable = malloc(sizeof(symbol_t));

                variable->name = gbList->children[i]->data;
                variable->type = SYM_GLOBAL_VAR;
                variable->node = gbList->children[i];

                tlhash_insert(global_names, gbList->children[i]->data, strlen(variable->name), variable);
                inc_count++;
            }
            inc_count = 0;

          }
        else if (gbList->children[i]->type == FUNCTION){
          symbol_t *function = malloc(sizeof(symbol_t));
          tlhash_t *locals = malloc(sizeof(tlhash_t));
          tlhash_init(locals, 32);

          function->name = gbList->children[i]->children[0]->data;
          function->type = SYM_FUNCTION;
          function->node = gbList->children[i];
          function->locals = locals;
          function->seq = (size_t) inc_count;

          tlhash_insert(global_names, gbList->children[i]->children[0]->data, strlen(function->name), function);
          inc_count++;
        }
    }
}

void
bind_names ( symbol_t *function, node_t *root )
{
    int inc_count = 0;
    for (int i = 0; i < root->n_children; i++) {
        if (root->children[i] != NULL) {
          if (root->children[i]->type == VARIABLE_LIST){

               function->nparms = root->children[i]->n_children;
               node_t *list = root->children[i];

                      for (int i = 0; i < list->n_children; i++) {
                          symbol_t *symbol = malloc(sizeof(symbol_t));
                          symbol->name = list->children[i]->data;
                          symbol->type = SYM_PARAMETER;
                          symbol->node = list->children[i];
                          symbol->seq = (size_t) inc_count;
                          tlhash_insert(function->locals, symbol->name, strlen(symbol->name), symbol);
                          inc_count++;
                       }
                       inc_count = 0;
                       inc_count = inc_count + root->children[i]->n_children;
            }
        }
    }

    scope_counter = 0;

    scopes_n = realloc(scopes_n, (scope_counter + 1) * sizeof(*scopes_n)); //change var names
    scopes_n[scope_counter] = global_names;
    scope_counter++;

    scopes_n = realloc(scopes_n, (scope_counter + 1) * sizeof(*scopes_n));
    scopes_n[scope_counter] = function->locals;
    scope_counter++;

    bind_names_tree_sub(root, NULL, 1, &inc_count);

    if (scope_counter > 2) {
        for (int i = 2; i < scope_counter; i++) {

            size_t size = tlhash_size(scopes_n[i]);
            char *check[size];
            tlhash_keys(scopes_n[i], (void **) &check);

            for (int j = 0; j < size; j++) {

                symbol_t *symbol;
                tlhash_lookup(scopes_n[i], check[j], strlen(check[j]), (void **) &symbol);
                tlhash_insert(function->locals, &symbol->seq, sizeof(size_t), symbol);

            }

            if (scopes_n[i] != NULL) {

                size_t size = tlhash_size(scopes_n[i]);
                char *check[size];
                tlhash_keys(scopes_n[i], (void **) &check);

                free(scopes_n[i]);
            }
        }
    }
}

void bind_names_tree_sub (node_t *node, node_t *parent_node, int scope_level, int *inc_count) {
    if (node != NULL) {
        tlhash_t *current_scope;
        symbol_t *symbol_tmp = NULL;
        node_t *child;
        if(node->type == BLOCK){

                if (scope_level > 1){
                    current_scope = malloc(sizeof(tlhash_t));
                    tlhash_init(current_scope, 32);

                    scopes_n = realloc(scopes_n, (scope_counter + 1) * sizeof(*scopes_n));
                    scopes_n[scope_counter] = current_scope;
                    scope_counter++;

                }   scope_level++;

          } else if (node->type == DECLARATION){

                child = node->children[0];

                  int temp = *inc_count;
                for (int i = 0; i < child->n_children; i++) {
                    symbol_t *symbol = malloc(sizeof(symbol_t));

                    symbol->name = child->children[i]->data;
                    symbol->type = SYM_LOCAL_VAR;
                    symbol->node = child->children[i];
                    symbol->seq = (size_t) temp;

                    tlhash_insert(scopes_n[scope_counter - 1], child->children[i]->data, strlen(symbol->name), symbol);
                    temp++;
                }

                *inc_count += child->n_children;

          } else if (node->type == IDENTIFIER_DATA){

              if (parent_node->type != 10 && parent_node->type != 6) {

                for (int i = scope_level - 1; i >= 0; i--) {
                    tlhash_t *scope = scopes_n[i];
                    if (tlhash_lookup(scope, node->data, strlen(node->data), (void *)&symbol_tmp) == TLHASH_SUCCESS) {
                        node->entry = symbol_tmp;
                    }
                }
              }

          } else if (node->type == STRING_DATA){

            string_list = realloc(string_list, (stringc + 1) * sizeof(*string_list));
            string_list[stringc] = node->data;

            size_t *point = malloc(sizeof(size_t));
            *point = stringc;
            node->data = point;

            stringc++;

          }

        for (int i = 0; i < node->n_children; i++)
        {
          bind_names_tree_sub (node->children[i], node, scope_level, inc_count);

        }
    }
}

void
destroy_symtab ( void ){
    size_t n_globals = tlhash_size(global_names);
    symbol_t *global_list[n_globals];
    tlhash_values ( global_names, (void **)&global_list );
    for ( size_t g=0; g<n_globals; g++ ) {
        symbol_t *global = global_list[g];

        if (global->locals != NULL) {
            size_t size = tlhash_size(global->locals);
            char *keys[size];
            tlhash_keys(global->locals, (void **) &keys);
            free(global->locals);
        }
    }

    if (global_names != NULL) {
        size_t size = tlhash_size(global_names);
        char *keys[size];
        tlhash_keys(global_names, (void **) &keys);
        free(global_names);
    }
    free(scopes_n);
}
