
// This represent a record in the only schema of this database
typedef struct Persona {
    char name[20];
    char surname[50];
    char address[100];
    int age;
} Persona;

// This is a node of an index that hold a string
typedef struct IndexNodeString {
    char * value;
    Persona * persona;
    struct IndexNodeString * left;
    struct IndexNodeString * right;
} IndexNodeString;

// This is a node of an index that hold an int
typedef struct IndexNodeInt {
    int value;
    Persona * persona;
    struct IndexNodeInt * left;
    struct IndexNodeInt * right;
} IndexNodeInt;

// A database hold a set of records and a set of indexes
typedef struct {
    IndexNodeString * name;
    IndexNodeString * surname;
    IndexNodeString * address;
    IndexNodeInt * age;
} Database;


//Persona constructor//

Persona* create_persona(char* name, char* surname, char* address, int age);

//IndexNodeInt* operations//
IndexNodeInt* create_tree_Int(int root_value, Persona* persona);
void free_tree_Int(IndexNodeInt* root);
void insert_inorder_Int(IndexNodeInt *root, int value, Persona* persona);
void print_tree_Int(IndexNodeInt * root);


//IndexNodeString* operations//
IndexNodeString* create_tree_String(char* root_value, Persona* persona);
void free_tree_String(IndexNodeString* root);
void insert_inorder_String(IndexNodeString *root, char* value, Persona* persona);
void print_tree_String(IndexNodeString * root);


// TODO implement the following methods
// The method return a Persona or NULL 

void insert(Database * database, Persona * persona);
Persona* findByName(Database * database, char * name);
Persona* findBySurname(Database * database, char * surname);
Persona* findByAddress(Database * database, char * address);
Persona* findByAge(Database * database, int age);

void free_database(Database* database);
