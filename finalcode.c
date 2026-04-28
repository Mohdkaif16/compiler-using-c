#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX 100

// ---------------- TOKEN ----------------
typedef struct {
    char type[10];
    char value[20];
} Token;

Token tokens[MAX];
int tokenCount, pos;

// ---------------- SYMBOL TABLE ----------------
typedef struct {
    char name;
    int value;
    int initialized;
    int valid;
} Symbol;

Symbol symTable[26];

// ---------------- AST ----------------
typedef struct Node {
    char value[10];
    struct Node *left, *right;
} Node;

// ---------------- TAC ----------------
char TAC[200][100];
int tacIndex = 0, tempCount = 0;

// ---------------- INIT ----------------
void initSymbolTable() {
    for(int i = 0; i < 26; i++) {
        symTable[i].name = 'a' + i;
        symTable[i].initialized = 0;
        symTable[i].valid = 0;
    }
}

// ---------------- WORD OPERATORS ----------------
void convertWords(char *expr) {
    char temp[200];
    int i = 0, j = 0;

    while(expr[i]) {
        if(strncmp(&expr[i], "add", 3) == 0) { temp[j++] = '+'; i += 3; }
        else if(strncmp(&expr[i], "sub", 3) == 0) { temp[j++] = '-'; i += 3; }
        else if(strncmp(&expr[i], "mul", 3) == 0) { temp[j++] = '*'; i += 3; }
        else if(strncmp(&expr[i], "div", 3) == 0) { temp[j++] = '/'; i += 3; }
        else temp[j++] = expr[i++];
    }

    temp[j] = '\0';
    strcpy(expr, temp);
}

// ---------------- LEXER ----------------
void tokenize(char *line) {
    int i = 0;
    tokenCount = 0;

    while(line[i]) {

        if(line[i] == ' ') {
            i++;
            continue;
        }

        if(isalpha(line[i])) {
            char temp[20]; int j = 0;
            while(isalnum(line[i])) temp[j++] = line[i++];
            temp[j] = '\0';
            strcpy(tokens[tokenCount].type, "ID");
            strcpy(tokens[tokenCount++].value, temp);
        }
        else if(isdigit(line[i])) {
            char temp[20]; int j = 0;
            while(isdigit(line[i])) temp[j++] = line[i++];
            temp[j] = '\0';
            strcpy(tokens[tokenCount].type, "NUM");
            strcpy(tokens[tokenCount++].value, temp);
        }
        else {
            char temp[2] = {line[i++], '\0'};
            strcpy(tokens[tokenCount].type, "SYM");
            strcpy(tokens[tokenCount++].value, temp);
        }
    }
}

// ---------------- AST ----------------
Node* newNode(char *val) {
    Node* n = (Node*)malloc(sizeof(Node));
    strcpy(n->value, val);
    n->left = n->right = NULL;
    return n;
}

int match(char *val) {
    if(pos < tokenCount && strcmp(tokens[pos].value, val) == 0) {
        pos++;
        return 1;
    }
    return 0;
}

Node* F();
Node* T();
Node* E();

Node* F() {
    if(pos < tokenCount &&
       (strcmp(tokens[pos].type, "NUM") == 0 || strcmp(tokens[pos].type, "ID") == 0)) {
        Node* n = newNode(tokens[pos].value);
        pos++;
        return n;
    }
    else if(match("(")) {
        Node* n = E();
        if(!match(")")) return NULL;
        return n;
    }
    return NULL;
}

Node* T() {
    Node* left = F();
    if(!left) return NULL;

    while(pos < tokenCount &&
         (strcmp(tokens[pos].value, "*") == 0 || strcmp(tokens[pos].value, "/") == 0)) {

        char op[5];
        strcpy(op, tokens[pos].value);
        pos++;

        Node* right = F();
        if(!right) return NULL;

        Node* new = newNode(op);
        new->left = left;
        new->right = right;
        left = new;
    }
    return left;
}

Node* E() {
    Node* left = T();
    if(!left) return NULL;

    while(pos < tokenCount &&
         (strcmp(tokens[pos].value, "+") == 0 || strcmp(tokens[pos].value, "-") == 0)) {

        char op[5];
        strcpy(op, tokens[pos].value);
        pos++;

        Node* right = T();
        if(!right) return NULL;

        Node* new = newNode(op);
        new->left = left;
        new->right = right;
        left = new;
    }
    return left;
}

// ---------------- TAC ----------------
void newTemp(char *t) {
    sprintf(t, "t%d", ++tempCount);
}

char* generateTACfromAST(Node* root) {
    static char result[10];

    if(root->left == NULL && root->right == NULL)
        return root->value;

    char *l = generateTACfromAST(root->left);
    char *r = generateTACfromAST(root->right);

    char temp[10];
    newTemp(temp);

    sprintf(TAC[tacIndex++], "%s = %s %s %s", temp, l, root->value, r);
    strcpy(result, temp);

    return result;
}

// ---------------- OPTIMIZATION ----------------
void optimizeTAC() {
    printf("\n--- Optimized TAC ---\n");

    char exprMap[100][50], tempMap[100][10];
    int mapSize = 0;

    for(int i = 0; i < tacIndex; i++) {
        char left[10], op1[10], op2[10], op;
        sscanf(TAC[i], "%s = %s %c %s", left, op1, &op, op2);

        char expr[50];
        sprintf(expr, "%s%c%s", op1, op, op2);

        int found = -1;

        for(int j = 0; j < mapSize; j++) {
            if(strcmp(exprMap[j], expr) == 0) {
                found = j;
                break;
            }
        }

        if(found != -1)
            printf("%s = %s\n", left, tempMap[found]);
        else {
            strcpy(exprMap[mapSize], expr);
            strcpy(tempMap[mapSize], left);
            mapSize++;
            printf("%s\n", TAC[i]);
        }
    }
}

// ---------------- EVALUATION ----------------
int evaluate(Node* root, int *errorFlag) {

    if(root->left == NULL && root->right == NULL) {

        if(isdigit(root->value[0]))
            return atoi(root->value);

        char var = root->value[0];

        if(!symTable[var - 'a'].initialized || !symTable[var - 'a'].valid) {
            *errorFlag = 1;
            return 0;
        }

        return symTable[var - 'a'].value;
    }

    int l = evaluate(root->left, errorFlag);
    int r = evaluate(root->right, errorFlag);

    if(*errorFlag) return 0;

    if(strcmp(root->value, "+") == 0) return l + r;
    if(strcmp(root->value, "-") == 0) return l - r;
    if(strcmp(root->value, "*") == 0) return l * r;
    if(strcmp(root->value, "/") == 0) {
        if(r == 0) { *errorFlag = 1; return 0; }
        return l / r;
    }

    return 0;
}

// ---------------- PROCESS ----------------
void processLine(char *line) {

    convertWords(line);

    if(strncmp(line, "print", 5) == 0) {
        char var;
        sscanf(line, "print(%c)", &var);

        if(!symTable[var - 'a'].initialized || !symTable[var - 'a'].valid)
            printf("Semantic Error: %c not initialized\n", var);
        else
            printf("Output: %d\n", symTable[var - 'a'].value);

        return;
    }

    tokenize(line);

    // invalid patterns
    for(int i = 0; i < tokenCount - 1; i++) {
        if(
            (strcmp(tokens[i].type,"NUM")==0 && strcmp(tokens[i+1].type,"NUM")==0) ||
            (strcmp(tokens[i].type,"ID")==0 && strcmp(tokens[i+1].type,"NUM")==0) ||
            (strcmp(tokens[i].type,"NUM")==0 && strcmp(tokens[i+1].type,"ID")==0)
        ) {
            printf("Syntax Error\n");
            return;
        }
    }

    if(tokenCount < 3 || strcmp(tokens[1].value, "=") != 0) {
        printf("Syntax Error\n");
        return;
    }

    char var = tokens[0].value[0];

    pos = 2;
    Node* root = E();

    if(root == NULL || pos != tokenCount) {
        printf("Syntax Error\n");
        return;
    }

    tacIndex = 0;
    tempCount = 0;

    printf("\n--- TAC ---\n");
    generateTACfromAST(root);

    for(int i = 0; i < tacIndex; i++)
        printf("%s\n", TAC[i]);

    optimizeTAC();

    int errorFlag = 0;
    int val = evaluate(root, &errorFlag);

    if(errorFlag) {
        printf("Semantic Error in expression\n");
        symTable[var - 'a'].initialized = 0;
        symTable[var - 'a'].valid = 0;
        return;
    }

    symTable[var - 'a'].value = val;
    symTable[var - 'a'].initialized = 1;
    symTable[var - 'a'].valid = 1;

    printf("%c = %d\n", var, val);
}

// ---------------- FILE ----------------
void runFile(char *filename) {

    // 🔥 RESET EVERYTHING BEFORE EACH RUN
    initSymbolTable();

    if(strstr(filename, ".scl") == NULL) {
        printf("Error: Only .scl files allowed\n");
        return;
    }

    FILE *fp = fopen(filename, "r");

    if(fp == NULL) {
        printf("Error: File not found\n");
        return;
    }

    char line[100];

    printf("\nProcessing file: %s\n\n", filename);

    while(fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = '\0';

        if(strlen(line) == 0) continue;

        printf(">> %s\n", line);
        processLine(line);
        printf("\n");
    }

    fclose(fp);

    printf("Execution Finished\n");
}

// ---------------- MAIN ----------------
int main() {

    char filename[100];

    initSymbolTable();

    while(1) {
        printf("\nEnter .scl file name (or exit): ");
        scanf("%s", filename);

        if(strcmp(filename, "exit") == 0)
            break;

        runFile(filename);
    }

    return 0;
}
