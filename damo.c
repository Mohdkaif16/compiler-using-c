#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX 100

// ---------------- TOKEN ----------------
typedef struct {
    char type[10];   // ID, NUM, OP
    char value[20];
} Token;

Token tokens[MAX];
int tokenCount, pos;

// ---------------- SYMBOL TABLE ----------------
typedef struct {
    char name;
    int value;
    int initialized;
} Symbol;

Symbol symTable[26];

// ---------------- TAC ----------------
char TAC[200][100];
int tacIndex = 0, tempCount = 0;

// ---------------- INIT ----------------
void initSymbolTable() {
    for(int i = 0; i < 26; i++) {
        symTable[i].name = 'a' + i;
        symTable[i].initialized = 0;
    }
}

// ---------------- LEXICAL ANALYSIS ----------------
void tokenize(char *line) {
    int i = 0;
    tokenCount = 0;

    while(line[i]) {
        if(isalpha(line[i])) {
            char temp[20]; int j = 0;
            while(isalnum(line[i])) temp[j++] = line[i++];
            temp[j] = '\0';

            strcpy(tokens[tokenCount].type, "ID");
            strcpy(tokens[tokenCount].value, temp);
            tokenCount++;
        }
        else if(isdigit(line[i])) {
            char temp[20]; int j = 0;
            while(isdigit(line[i])) temp[j++] = line[i++];
            temp[j] = '\0';

            strcpy(tokens[tokenCount].type, "NUM");
            strcpy(tokens[tokenCount].value, temp);
            tokenCount++;
        }
        else {
            char temp[2] = {line[i++], '\0'};
            strcpy(tokens[tokenCount].type, "SYM");
            strcpy(tokens[tokenCount].value, temp);
            tokenCount++;
        }
    }
}

// ---------------- PARSER ----------------
int match(char *expected) {
    if(pos < tokenCount && strcmp(tokens[pos].value, expected) == 0) {
        pos++;
        return 1;
    }
    return 0;
}

int F() {
    if(strcmp(tokens[pos].type, "NUM") == 0 || strcmp(tokens[pos].type, "ID") == 0) {
        pos++;
        return 1;
    }
    else if(match("(")) {
        if(!E()) return 0;
        if(!match(")")) return 0;
        return 1;
    }
    return 0;
}

int T() {
    if(!F()) return 0;
    while(pos < tokenCount && 
         (strcmp(tokens[pos].value, "*") == 0 || strcmp(tokens[pos].value, "/") == 0)) {
        pos++;
        if(!F()) return 0;
    }
    return 1;
}

int E() {
    if(!T()) return 0;
    while(pos < tokenCount && 
         (strcmp(tokens[pos].value, "+") == 0 || strcmp(tokens[pos].value, "-") == 0)) {
        pos++;
        if(!T()) return 0;
    }
    return 1;
}

// ---------------- TAC ----------------
void newTemp(char *t) {
    sprintf(t, "t%d", ++tempCount);
}

char* generateTAC(char *expr) {
    static char result[10];
    char temp[10];

    if(strlen(expr) == 1) {
        strcpy(result, expr);
        return result;
    }

    for(int i = 0; expr[i]; i++) {
        if(strchr("+-*/", expr[i])) {
            char left[50], right[50];

            strncpy(left, expr, i);
            left[i] = '\0';
            strcpy(right, expr + i + 1);

            char *l = generateTAC(left);
            char *r = generateTAC(right);

            newTemp(temp);
            sprintf(TAC[tacIndex++], "%s = %s %c %s", temp, l, expr[i], r);

            strcpy(result, temp);
            return result;
        }
    }
    strcpy(result, expr);
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

// ---------------- ASSEMBLY ----------------
void generateAssembly() {
    printf("\n--- Assembly Code ---\n");

    for(int i = 0; i < tacIndex; i++) {
        char res[10], op1[10], op2[10], op;
        sscanf(TAC[i], "%s = %s %c %s", res, op1, &op, op2);

        printf("MOV R1, %s\n", op1);

        if(op == '+') printf("ADD R1, %s\n", op2);
        else if(op == '-') printf("SUB R1, %s\n", op2);
        else if(op == '*') printf("MUL R1, %s\n", op2);
        else if(op == '/') printf("DIV R1, %s\n", op2);

        printf("MOV %s, R1\n", res);
    }
}

// ---------------- EVALUATION ----------------
int evaluate(char *expr) {
    int i = 0, result = 0;

    if(isalpha(expr[i])) result = symTable[expr[i++] - 'a'].value;
    else while(isdigit(expr[i])) result = result*10 + (expr[i++]-'0');

    while(expr[i]) {
        char op = expr[i++];
        int val = 0;

        if(isalpha(expr[i])) val = symTable[expr[i++] - 'a'].value;
        else while(isdigit(expr[i])) val = val*10 + (expr[i++]-'0');

        if(op == '+') result += val;
        else if(op == '-') result -= val;
        else if(op == '*') result *= val;
        else if(op == '/') result /= val;
    }

    return result;
}

// ---------------- MAIN EXECUTION ----------------
void processLine(char *line) {

    if(strncmp(line, "print", 5) == 0) {
        char var;
        sscanf(line, "print(%c)", &var);

        if(!symTable[var - 'a'].initialized)
            printf("Semantic Error: %c not initialized\n", var);
        else
            printf("Output: %d\n", symTable[var - 'a'].value);

        return;
    }

    tokenize(line);

    if(strcmp(tokens[1].value, "=") != 0) {
        printf("Syntax Error\n");
        return;
    }

    char var = tokens[0].value[0];

    pos = 2;
    if(!E()) {
        printf("Syntax Error\n");
        return;
    }

    // TAC
    tacIndex = 0; tempCount = 0;
    char expr[100];
    strcpy(expr, line + 2);

    printf("\n--- TAC ---\n");
    generateTAC(expr);

    for(int i = 0; i < tacIndex; i++)
        printf("%s\n", TAC[i]);

    optimizeTAC();
    generateAssembly();

    int val = evaluate(expr);
    symTable[var - 'a'].value = val;
    symTable[var - 'a'].initialized = 1;

    printf("%c = %d\n", var, val);
}

// ---------------- MAIN ----------------
int main() {
    char line[100];

    initSymbolTable();

    printf("Enter statements (type exit to stop):\n");

    while(1) {
        printf(">> ");
        fgets(line, sizeof(line), stdin);
        line[strcspn(line, "\n")] = 0;

        if(strcmp(line, "exit") == 0)
            break;

        processLine(line);
    }

    return 0;
}
