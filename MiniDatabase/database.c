#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "database.h"

//Persona constructor//

Persona* create_persona(char* name, char* surname, char* address, int age){

    Persona* p = malloc(sizeof(Persona));

    if(p == NULL){
        return NULL;
    }

    strcpy(p->name, name );
    strcpy(p->surname, surname );
    strcpy(p->address, address );
    p->age = age;

    return p;
}


//IndexNodeInt* operations//

IndexNodeInt* create_tree_Int(int root_value, Persona* persona) {
    IndexNodeInt *root = malloc(sizeof(IndexNodeInt));
    if (root == NULL) {
        return NULL;
    } 
    root->value = root_value;
    root->persona = persona;
    root->left = NULL;
    root->right = NULL;
    return root;
}

void free_tree_Int(IndexNodeInt* root) {
    if(root == NULL){
        return;
    }
    else{
        free_tree_Int(root->left);
        free_tree_Int(root->right);
        free(root);
    }
}

void insert_inorder_Int(IndexNodeInt *root, int value, Persona* persona) {
    if (root == NULL) {
        return;
    }
    if (value <= root->value) {
        if (root->left == NULL) {
            IndexNodeInt *node = create_tree_Int(value, persona);
            root->left = node;
            return;
        }
        insert_inorder_Int(root->left, value, persona);
        return;
    }
    if (root->right == NULL) {
        IndexNodeInt *node = create_tree_Int(value, persona);
        root->right = node;
        return;
    }
    insert_inorder_Int(root->right, value, persona);
}

void print_tree_Int(IndexNodeInt * root) {
    if (root == NULL) {
        return;
    }
    print_tree_Int(root->left);
    printf("%d\n", root->value);
    print_tree_Int(root->right);
}

//////

//IndexNodeString* operations//

IndexNodeString* create_tree_String(char* root_value, Persona* persona) {
    IndexNodeString *root = malloc(sizeof(IndexNodeString));
    if (root == NULL) {
        return NULL;
    } 
    root->value = root_value;
    root->persona = persona;
    root->left = NULL;
    root->right = NULL;
    return root;
}

void free_tree_String(IndexNodeString* root) {
    if(root == NULL){
        return;
    }
    else{
        free_tree_String(root->left);
        free_tree_String(root->right);
        free(root);
    }
}

void insert_inorder_String(IndexNodeString *root, char* value, Persona* persona) {
    if (root == NULL) {
        return;
    }
    if (strcmp(value, root->value) <= 0) {
        if (root->left == NULL) {
            IndexNodeString *node = create_tree_String(value, persona);
            root->left = node;
            return;
        }
        insert_inorder_String(root->left, value, persona);
        return;
    }
    if (root->right == NULL) {
        IndexNodeString *node = create_tree_String(value, persona);
        root->right = node;
        return;
    }
    insert_inorder_String(root->right, value, persona);
}

void print_tree_String(IndexNodeString * root) {
    if (root == NULL) {
        return;
    }
    print_tree_String(root->left);
    printf("%s\n", root->value);
    print_tree_String(root->right);
}


//"database.h" functions implementation//

void insert(Database * database, Persona * persona){

    char* name = persona->name;
    char* surname = persona->surname;
    char* address = persona->address;
    int age = persona->age;

    if(database->name == NULL){
        database->name = create_tree_String(name, persona);
        database->surname = create_tree_String(surname, persona);
        database->address = create_tree_String(address, persona);
        database->age = create_tree_Int(age, persona);
    }
    else{
    insert_inorder_String(database->name, name, persona);
    insert_inorder_String(database->surname, surname, persona);
    insert_inorder_String(database->address, address, persona);
    insert_inorder_Int(database->age, age, persona);
    }
}

//AUX FUNCTIONS TO FIND ELEMENT IN A BINARY TREE

Persona* findBy_AuxString(IndexNodeString* root, char* value){

    if(root == NULL) {return NULL;}
    else if(root->value == value) {return root->persona;}

    if (strcmp(value, root->value) < 0) {findBy_AuxString(root->left, value);}
    else {findBy_AuxString(root->right, value);}
}
Persona* findBy_AuxInt(IndexNodeInt* root, int value){

    if(root == NULL) {return NULL;}
    else if(root->value == value) {return root->persona;}

    if (value < root->value) {findBy_AuxInt(root->left, value);}
    else {findBy_AuxInt(root->right, value);}

}

//

Persona* findByName(Database * database, char * name){

    IndexNodeString* root = database->name;
    return findBy_AuxString(root, name);
}

Persona* findBySurname(Database * database, char * surname){

    IndexNodeString* root = database->surname;
    return findBy_AuxString(root, surname);
}

Persona* findByAddress(Database * database, char * address){

    IndexNodeString* root = database->address;
    return findBy_AuxString(root, address);
}

Persona* findByAge(Database * database, int age){
    
    IndexNodeInt* root = database->age;
    return findBy_AuxInt(root, age);
}

void free_database(Database* database){

    free_tree_String(database->name);
    free_tree_String(database->surname);
    free_tree_String(database->address);
    free_tree_Int(database->age);
}
