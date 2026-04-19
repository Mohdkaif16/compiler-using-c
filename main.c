#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINES 100
#define MAX_ERRORS 50
#define MAX_TAC 200

char program[MAX_LINES][100];
int values[26];
int totalLines;

// -------- ERROR --------
int errorCount;
int errorLines[MAX_ERRORS];
char errorChars[MAX_ERRORS];

char *input;
int pos;
int isValid;
int currentLine;

// -------- TAC --------
char TAC[MAX_TAC][100];
int tacIndex = 0;
int tempCount = 0;

// -------- ERROR FUNCTION --------
void error() {
    if(errorCount < MAX_ERRORS) {
        errorLines[errorCount] = currentLine + 1;
        errorChars[errorCount] = input[pos] ? input[pos] : '#';
        errorCount++;
    }
    isValid = 0;
}

// -------- PARSER --------
int E();

int F() {
    if(input[pos] == '(') {
        pos++;
        E();
        if(input[pos] == ')') pos++;
        else error();
    }
    else if(isdigit(input[pos])) {
        while(isdigit(input[pos])) pos++;
    }
    else if(isalpha(input[pos])) {
        pos++;
    }
    else error();
    return 0;
}

int T() {
    F();
    while(input[pos] == '*' || input[pos] == '/') {
        pos++;
        F();
    }
    return 0;
}

int E() {
    T();
    while(input[pos] == '+' || input[pos] == '-') {
        pos++;
        T();
    }
    return 0;
}

// -------- REMOVE SPACES --------
void removeSpaces(char *str) {
    char temp[100];
    int j = 0;
    for(int i = 0; str[i]; i++) {
        if(str[i] != ' ') temp[j++] = str[i];
    }
    temp[j] = '\0';
    strcpy(str, temp);
}

// -------- CONVERT mul/add → symbols --------
void convertExpr(char *expr) {
    char temp[100];
    int i = 0, j = 0;

    while(expr[i]) {
        if(strncmp(&expr[i], "add", 3) == 0) {
            temp[j++] = '+';
            i += 3;
        }
        else if(strncmp(&expr[i], "sub", 3) == 0) {
            temp[j++] = '-';
            i += 3;
        }
        else if(strncmp(&expr[i], "mul", 3) == 0) {
            temp[j++] = '*';
            i += 3;
        }
        else if(strncmp(&expr[i], "div", 3) == 0) {
            temp[j++] = '/';
            i += 3;
        }
        else {
            temp[j++] = expr[i++];
        }
    }

    temp[j] = '\0';
    strcpy(expr, temp);
}

// -------- TAC GENERATION --------
void newTemp(char *temp) {
    sprintf(temp, "t%d", ++tempCount);
}

char* generateTAC(char *expr) {
    static char result[10];
    char temp[10];

    if(strlen(expr) == 1) {
        strcpy(result, expr);
        return result;
    }

    for(int i = 0; expr[i]; i++) {
        if(expr[i] == '+' || expr[i] == '-' ||
           expr[i] == '*' || expr[i] == '/') {

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

// -------- OPTIMIZATION --------
void optimizeTAC() {
    printf("\n--- Optimized TAC ---\n");

    char exprMap[100][50];
    char tempMap[100][10];
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

        if(found != -1) {
            printf("%s = %s\n", left, tempMap[found]);
        } else {
            strcpy(exprMap[mapSize], expr);
            strcpy(tempMap[mapSize], left);
            mapSize++;
            printf("%s\n", TAC[i]);
        }
    }
}

// -------- PARSE LINE --------
void parseLine(char *line) {

    if(strncmp(line, "print", 5) == 0) return;

    if(isalpha(line[0]) && strchr(line, '=')) {

        char tempExpr[100];
        strcpy(tempExpr, strchr(line, '=') + 1);

        convertExpr(tempExpr);

        input = tempExpr;
        pos = 0;

        E();
        if(input[pos] != '\0') error();
    }
    else {
        error();
    }
}

// -------- EVALUATE --------
int evaluate(char *expr) {

    int i = 0, result = 0;

    if(isalpha(expr[i])) result = values[expr[i++] - 'a'];
    else while(isdigit(expr[i])) result = result*10 + (expr[i++]-'0');

    while(expr[i]) {
        char op = expr[i++];
        int val = 0;

        if(isalpha(expr[i])) val = values[expr[i++] - 'a'];
        else while(isdigit(expr[i])) val = val*10 + (expr[i++]-'0');

        if(op == '+') result += val;
        else if(op == '-') result -= val;
        else if(op == '*') result *= val;
        else if(op == '/') result /= val;
    }

    return result;
}

// -------- EXECUTION --------
void executeLine(int *i) {

    char *line = program[*i];

    if(strncmp(line, "print", 5) == 0) {
        char var;
        sscanf(line, "print(%c)", &var);
        printf("Output: %d\n", values[var - 'a']);
    }

    else if(isalpha(line[0]) && strchr(line, '=')) {

        char var = line[0];
        char expr[100];

        strcpy(expr, strchr(line, '=') + 1);

        convertExpr(expr);

        tacIndex = 0;
        tempCount = 0;

        generateTAC(expr);

        printf("\n--- TAC ---\n");
        for(int i = 0; i < tacIndex; i++)
            printf("%s\n", TAC[i]);

        optimizeTAC();

        int val = evaluate(expr);
        values[var - 'a'] = val;

        printf("%c = %d\n", var, val);
    }
}

// -------- RUN FILE --------
void runFile(char *filename) {

    FILE *fp = fopen(filename, "r");

    if(fp == NULL) {
        printf("File not found\n");
        return;
    }

    totalLines = 0;
    isValid = 1;
    errorCount = 0;

    while(fgets(program[totalLines], 100, fp)) {
        program[totalLines][strcspn(program[totalLines], "\n")] = '\0';
        removeSpaces(program[totalLines]);

        currentLine = totalLines;
        parseLine(program[totalLines]);

        totalLines++;
    }

    fclose(fp);

    printf("\nSyntax Status:\n");

    if(isValid) {
        printf("Valid Syntax\n\n");

        for(int i = 0; i < totalLines; i++)
            executeLine(&i);

        printf("\nExecution Finished\n");
    }
    else {
        printf("Syntax Errors Found\n");
    }
}

// -------- MAIN --------
int main() {

    char filename[100];

    while(1) {
        printf("\nEnter file name (or exit): ");
        scanf("%s", filename);

        if(strcmp(filename, "exit") == 0)
            break;

        runFile(filename);
    }

    return 0;
}
