#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define MAX 256
#define FULL 1
#define EMPTY 2
#define OK 0
#define H_TUREV 1e-7
#define EPSILON 1e-7

typedef enum    //token türleri
{
    NUMBER,     //0   
    OPERATOR,   //1  "+","-","*","/","^"
    VARIABLE,   //2  "x","y","z" gibi değişkenler
    FUNCTION,   //3  "sin","cos","log_" gibi fonksiyonlar
    LPAREN,     //4  "("
    RPAREN     //5   ")"
} tokenType; 

char* tokenPrinter(tokenType type)
{
    char *str[6] = {"NUMBER", "OPERATOR", "VARIABLE", "FUNCTION", "LPAREN", "RPAREN"};
    return str[type];
}

typedef struct  //token yapısı
{
    tokenType type; //token türü
    char *value;    // "+", "x", "3", "sin" gibi değerleri tutar.
    double x;       //sayilari double a cevirmek icin.(converting)
} Token;

typedef struct       //shunting yard algoritması icin stack (tokenleri tutar)
{
    Token data[MAX]; //stack elementlerini tutması icin array
    int top;         //stackte en usttteki operatoru tutar.
} Stack;

typedef struct    //evaluatePostfix fonksiyonu icin stack (double tutar)
{
    double data[MAX];
    int top;
} StackDouble;

typedef struct
{
    double **data;
    int row;
    int col;
} Matrix;

typedef struct   //gregory-newton icin
{
    double x;
    double y;
} DataPoint;


//yardimci fonksiyonlar
int is_digit(char c) 
{
    return c >= '0' && c <= '9';
}

int is_letter(char c) 
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int is_whitespace(char c) 
{
    return c == ' ' || c == '\t' || c == '\n';
}

int is_operator(char c) 
{
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
}

void tokenize(char *str, Token *tokens, int *countToken)
{ 
    int i = 0;
    int j = 0;
    int k = 0;
    int start;
    int len;
    double decimal;
    double divider;
    char tmp[MAX];

    while(str[i] != '\0')
    {
        if(is_whitespace(str[i])) // White spaceleri atlar.
        {
            i++;
        }
        else if(is_digit(str[i]))  // Sayılara bakar (.5 veya 5. şeklinde)
        {
            start = i;
            tokens[j].x = 0;
            tokens[j].value = NULL;
            decimal = 0;
            divider = 10.0;
            tokens[j].type = NUMBER;

            // Integer part
            while(is_digit(str[i]))
            {
                tokens[j].x *= 10;
                tokens[j].x += str[i] - '0';
                i++;
            }

            // Decimal part
            if(str[i] == '.' && is_digit(str[i+1]))
            {
                i++; // Skip the '.'
                while(is_digit(str[i]))
                {
                    decimal += (str[i] - '0') / divider;
                    divider *= 10;
                    i++;
                }
            }

            tokens[j].x += decimal;
            j++;
        }
        else if(is_letter(str[i]))  // variables or functions
        {
            start = i;
            k = 0;
            while(is_letter(str[i]) && k < MAX - 1)
            {
                tmp[k++] = str[i++];
            }
            tmp[k] = '\0';

            if(strcmp(tmp, "log") == 0 && str[i] == '_') // log için özel kontrol
            {
                i++;
                tokens[j].value = strdup("log_");
                if(tokens[j].value == NULL)
                {
                    printf("Error: strdup failed.\n");
                    return;
                }
                tokens[j].type = FUNCTION;
            }
            else
            {
                tokens[j].value = strdup(tmp);
                if (tokens[j].value == NULL) 
                {
                    printf("Error: strdup failed.\n"); 
                    return; 
                }

                if (strcmp(tokens[j].value, "sin") == 0 || strcmp(tokens[j].value, "cos") == 0 ||
                    strcmp(tokens[j].value, "tan") == 0 || strcmp(tokens[j].value, "cot") == 0 ||
                    strcmp(tokens[j].value, "arcsin") == 0 || strcmp(tokens[j].value, "arccos") == 0 ||
                    strcmp(tokens[j].value, "arctan") == 0 || strcmp(tokens[j].value, "arccot") == 0 ||
                    strcmp(tokens[j].value, "ln") == 0) 
                {
                    tokens[j].type = FUNCTION;
                } 
                else 
                {
                    tokens[j].type = VARIABLE;
                }
            }
            j++;
        }
        else if(is_operator(str[i]))
        {
            tokens[j].value = (char*)malloc(2);
            if(tokens[j].value == NULL)
            {
                printf("Error: Memory allocation is failed\n");
                return;
            }
            tokens[j].value[0] = str[i];
            tokens[j].value[1] = '\0';
            tokens[j].type = OPERATOR;
            i++;
            j++;
        }
        else if(str[i] == '(')
        {
            tokens[j].value = (char*)malloc(2);
            if(tokens[j].value == NULL)
            {
                printf("Error: Memory allocation is failed.\n");
                return;
            }
            tokens[j].value[0] = '(';
            tokens[j].value[1] = '\0';
            tokens[j].type = LPAREN;
            i++;
            j++;
        }
        else if(str[i] == ')')
        {
            tokens[j].value = (char*)malloc(2);
            if(tokens[j].value == NULL)
            {
                printf("Error: Memory allocation is failed.\n");
                return;
            }
            tokens[j].value[0] = ')';
            tokens[j].value[1] = '\0';
            tokens[j].type = RPAREN;
            i++;
            j++;
        }
        else
        {
            printf("Error: Invalid character '%c'\n", str[i]);
            i++;
        }
    }
    *countToken = j;
}

void print_tokens(Token *tokens, int count) 
{
    int i;
    for (i=0; i < count; i++) 
    {
        printf("Token %d: %s -> ", i, tokenPrinter(tokens[i].type));

        if (tokens[i].type == NUMBER) 
        {
            printf("%lf\n", tokens[i].x);
        } else
        {
            printf("%s\n", tokens[i].value);
        }
    }
}

int presedence(char *op) // "+" < "*" < "^"
{
    if(strcmp(op, "^") == 0)
    {
        return 3;
    }
    else if(strcmp(op, "*") == 0 || strcmp(op, "/") == 0)
    {
        return 2;
    }
    else if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0)
    {
        return 1;
    }
    return 0;
}

int isRightAssociative(char *op) // sadece "^"
{
    if(strcmp(op, "^") == 0)
    {
     return 1;
    }
    return 0;
}

//stack fonksyionları
void initialize(Stack *s)
{
    s->top = -1; //Sets stack to empty state
}

int isEmpty(Stack *s) 
{
    return s->top == -1;
}

int isFull(Stack *s) 
{
    return s->top == MAX - 1;  //dogruysa 1 yanlissa 0 dondurur
}

int push(Stack *s, Token t)
{
    if(isFull(s) == 1)
    {
        printf("Stack is full!");
        return FULL;
    }
    s->data[++s->top] = t;
    return 0;
}

int pop(Stack *s, Token *val)
{
    if(isEmpty(s) == 1)
    {
        printf("Stack is empty!");
        return EMPTY;
    }
    *val = s->data[s->top--];

    return 0;
}

Token copyToken(Token t)  // Tokenin ayrı bir kopyası
{
    Token newToken;
    newToken.type = t.type;
    newToken.x = t.x;

    if (t.value != NULL) 
    {
        newToken.value = malloc(strlen(t.value) + 1);
        if (newToken.value != NULL) 
        {
            strcpy(newToken.value, t.value);
        }
        else 
        {
            printf("Memory allocation failed.\n");
        }
    }
    return newToken;
}

void shuntingYard(Token *input, int inputLen, Token *output, int *outputLen)
{
    int i;
    int flag;
    Stack operators;
    initialize(&operators); //stacki bosalttik.
    int outIndex = 0;       // Tracks position in output array

    for (i = 0; i < inputLen; i++)
    {
        Token current = input[i];

        if (current.type == NUMBER || current.type == VARIABLE)
        {
            output[outIndex++] = copyToken(current);  //Add numbers and variables directly to output
        }
        else if (current.type == FUNCTION)
        {
            push(&operators, current);     //Push functions to stack
        }
        else if (current.type == OPERATOR)
        {
            Token topToken;

            flag = 0;

            while (!isEmpty(&operators) && flag == 0)
            {
                topToken = operators.data[operators.top];

                if (topToken.type == OPERATOR &&
                    ((presedence(topToken.value) > presedence(current.value)) ||
                     (presedence(topToken.value) == presedence(current.value) && !isRightAssociative(current.value))))
                {
                    pop(&operators, &topToken);
                    output[outIndex++] = copyToken(topToken);
                }
                else if (topToken.type == FUNCTION)
                {
                    pop(&operators, &topToken);
                    output[outIndex++] = copyToken(topToken);
                }
                else
                {
                    flag++;
                }
            }

            push(&operators, current);
        }
        else if (current.type == LPAREN)
        {
            push(&operators, current);
        }
        else if (current.type == RPAREN)
        {
            Token topToken;
            flag = 0;
            while (!isEmpty(&operators) && flag == 0)
            {
                pop(&operators, &topToken);
                if (topToken.type == LPAREN) 
                {
                    flag++;
                }
                else
                {
                output[outIndex++] = copyToken(topToken);
                }
            }

            if (!isEmpty(&operators) && operators.data[operators.top].type == FUNCTION)
            {
                pop(&operators, &topToken);
                output[outIndex++] = copyToken(topToken);
            }
        }
    }

    Token remaining;
    while (!isEmpty(&operators))
    {
        pop(&operators, &remaining);
        output[outIndex++] = copyToken(remaining);
    }

    *outputLen = outIndex; // Set the final output size
}

//DoubleStack icin stack fonksyionu 
void initializeDoubleStack(StackDouble *s) 
{
    s->top = -1;
}

int isEmptyDoubleStack(StackDouble *s)
{
    return s->top == -1;
}

int isFullDoubleStack(StackDouble *s) 
{
    return s->top == MAX - 1;
}

int pushDoubleStack(StackDouble *s, double val) 
{
    if (isFullDoubleStack(s)) 
    {
        printf("Stack is full!\n");
        return FULL;
    }
    s->data[++s->top] = val;
    return 0;
}

int popDoubleStack(StackDouble *s, double *val)
{
    if (isEmptyDoubleStack(s)) 
    {
        printf("Stack is empty!\n");
        return EMPTY;
    }
    *val = s->data[s->top--];
    return OK;
}

double evaluatePostfix(Token *postfix, int len, double x_val)
{
    StackDouble evalStack;
    initializeDoubleStack(&evalStack);
    Token top;
    double op1, op2, res;
    int i;
    double base; //logaritmanin base i için
    if(len <= -1)
    {
        return NAN; // not a number from math.h
    }
    for(i=0;i<len;i++)
    {
        Token token = postfix[i];

        if(token.type == NUMBER)
        {
            if(pushDoubleStack(&evalStack, token.x) != OK)
            {
                return NAN;
            }
        }
        else if(token.type == VARIABLE)
        {
            if(token.value == NULL)
            {
                printf("Variable is null.\n");
                return NAN;
            }
            if (strcmp(token.value, "x") == 0) 
            { // Assuming 'x' is the only variable for now
                if(pushDoubleStack(&evalStack, x_val) != OK) return NAN;
            } 
            else if (strcmp(token.value, "e") == 0) 
            { // Euler's number
                if(pushDoubleStack(&evalStack, M_E) != OK) return NAN;
            } 
            else if (strcmp(token.value, "pi") == 0) 
            { // Pi
                if(pushDoubleStack(&evalStack, M_PI) != OK) return NAN;
            }
            else
            {
                printf("Unknown variable '%s'\n", token.value);
                return NAN;
            }
        }
        else if(token.type == OPERATOR)
        {
            if(token.value == NULL)
            {
                printf("Operator is null.");
            }
            if(popDoubleStack(&evalStack, &op2) != OK) return NAN;
            if(popDoubleStack(&evalStack, &op1) != OK) return NAN;

            if(strcmp(token.value, "+") == 0)
            {
                res = op1 + op2;
            }
            else if(strcmp(token.value, "-") == 0)
            {
                res = op1 - op2;
            }
            else if(strcmp(token.value, "*") == 0)
            {
                res = op1 * op2;
            }
            else if(strcmp(token.value, "/") == 0)
            {
                if(op2 != 0)
                {
                    res = op1 / op2;
                }
                else 
                {
                    printf("Invalid process (dividing by zero)\n");
                    return NAN;
                }
            }
            else if(strcmp(token.value, "^") == 0)
            {
                res = pow(op1, op2);
            }
            else 
            {
                printf("Invalid operator! '%s'\n ", token.value);
                return NAN;
            }
            if (pushDoubleStack(&evalStack, res) != OK) return NAN; // Stack is full.
        }
        else if(token.type == FUNCTION)
        {
            if(token.value == NULL)
            { 
                printf("Function is null.\n");
                return NAN; 
            }
            if(strcmp(token.value, "log_") == 0)
            {
                if(popDoubleStack(&evalStack, &op1) != OK) return NAN; //argument
                if(popDoubleStack(&evalStack, &base) != OK) return NAN; //base
                if( op1 <= 0 || base == 1 || base <= 0) //control for log
                {
                    printf("Invalid argument or base for log.\n");
                    return NAN;
                }
                res = log(op1) / log(base);
            }
            else
            {
                if(popDoubleStack(&evalStack, &op1) != OK) return NAN;

                if(strcmp(token.value, "sin") == 0) res = sin(op1);
                else if(strcmp(token.value, "cos") == 0) res = cos(op1);
                else if(strcmp(token.value, "tan") == 0) res = tan(op1);
                else if(strcmp(token.value, "cot") == 0)
                {
                    if(tan(op1) == 0)
                    {
                        printf("Division by zero in cot.\n");
                        return NAN;
                    }
                    res = 1.0 / tan(op1);
                }
                else if (strcmp(token.value, "arcsin") == 0) res = asin(op1);
                else if (strcmp(token.value, "arccos") == 0) res = acos(op1);
                else if (strcmp(token.value, "arctan") == 0) res = atan(op1);
                else if (strcmp(token.value, "arccot") == 0) res = atan(1.0/op1);
                else if (strcmp(token.value, "ln") == 0)
                {
                    if(op1 <= 0)
                    {
                        printf("Invalid argument for ln.\n");
                        return NAN;
                    }
                    res = log(op1) / log(M_E);
                }
                else
                {
                    printf("Invalid function '%s'.\n");
                    return NAN;
                }
            }
            if (pushDoubleStack(&evalStack, res) != OK) return NAN; // Stack is full.
        }
    }
    //stack kontrolü
    if(popDoubleStack(&evalStack, &res) != OK)
    {
        printf("Stack is empty.\n");
        return NAN;
    }
    if(!isEmptyDoubleStack(&evalStack))
    {
        printf("Stack is not empty.\n");
        return NAN;
    }
    return res;
}

void freeTokens(Token *tokens, int count)  
{
    int i;
    for (i=0; i<count; i++) 
    {
        if(tokens[i].value != NULL)
        {
            free(tokens[i].value);
            tokens[i].value = NULL;
        }
    }
}

void bisectionMethod(Token *postfixTokens, int numPostfixTokens) 
{
    double a, b, epsilon, c, fa, fb, fc;
    int max_iterations = 100;
    int iteration = 0;
    int proceed_flag = 1; // Flag to control loop continuation without break/continue

    printf("\n--- Bisection Method ---\n");
    printf("Araligi giriniz: [a, b]: ");
    if(scanf("%lf %lf", &a, &b) != 2) 
    {
        printf("gecersiz aralik\n");
        while (getchar() != '\n' && getchar() != EOF);
        return;
    }
    while(getchar() != '\n' && getchar() != EOF);

    printf("Hata payini giriniz: (epsilon, ornek:0.00001): ");
    if(scanf("%lf", &epsilon) != 1) 
    {
        printf("Gecersiz hata payi!\n");
        while (getchar() != '\n' && getchar() != EOF);
        return;
    }
    while(getchar() != '\n' && getchar() != EOF);

    if(a >= b)
    {
        printf("Gecersiz aralik, a b den kucuk olmali.\n");
        return;
    }
    if(epsilon <= 0) 
    {
        printf("Hata payi pozitif olmali!\n");
        return;
    }
    fa = evaluatePostfix(postfixTokens, numPostfixTokens, a);
    fb = evaluatePostfix(postfixTokens, numPostfixTokens, b);

    if(isnan(fa) || isnan(fb)) 
    {
        printf("Evaluate edilemedi! f(a)=%f or f(b)=%f.\n", fa, fb);
        return;
    }

    printf("\nBaslangic: a=%.7f, f(a)=%.7f, b=%.7f, f(b)=%.7f\n", a, fa, b, fb);

    if(fa * fb >= 0) 
    {
        printf("Bisection calisamadi!. f(a) ve f(b) ters isaretli olmali.\n");
        if(fabs(fa) < epsilon)
        {
             printf("However, 'a' itself might be a root: f(%.7f) = %.7f (within epsilon)\n", a, fa);
        }
        if(fabs(fb) < epsilon)
        {
             printf("However, 'b' itself might be a root: f(%.7f) = %.7f (within epsilon)\n", b, fb);
        }
        return;
    }

    printf("\nIteration |    a    |    b    |    c    |   f(a)   |   f(b)   |   f(c)   |  |b-a|/2  |\n");
    printf("------------------------------------------------------------------------------------------\n");

    while(iteration < max_iterations && proceed_flag == 1)
    {
        c = (a + b) / 2.0;
        fc = evaluatePostfix(postfixTokens, numPostfixTokens, c);

        if(isnan(fc))
        {
            printf("Evaluate edilemedi. = %lf  %d. iterasyonda\n", c, iteration + 1);
            proceed_flag = 0; // Stop loop due to error
        }

        if(proceed_flag == 1) 
        {
            printf("%9d | %.7f | %.7f | %.7f | %.7f | %.7f | %.7f | %.7f |\n",
                   iteration + 1, a, b, c, fa, fb, fc, fabs(b - a) / 2.0);

            if(fabs(fc) < epsilon || fabs(b - a) / 2.0 < epsilon) 
            {
                printf("\nKok %d iterasyon sonunda bulund.\n", iteration + 1);
                printf("Yaklasik kok c = %.7f\n", c);
                printf("f(c) = %.7f\n", fc);
                printf("Hata |b-a|/2 = %.7f (hata payi = %.7f)\n", fabs(b-a)/2.0, epsilon);
                proceed_flag = 0; // Root found, stop loop
            } 
            else
            {
                if (fa * fc < 0)
                {
                    b = c;
                    fb = fc;
                }
                else 
                {
                    a = c;
                    fa = fc;
                }
            }
        }
        iteration++;
    }

    if (proceed_flag == 1 && iteration == max_iterations) 
    {
        printf("Iterasyon sayisi cok fazla!");
        printf("Son yaklasik c = %.7f, f(c) = %.7f\n", c, fc);
        printf("Current error |b-a|/2 = %.7f (tolerance epsilon = %.7f)\n", fabs(b-a)/2.0, epsilon);
    }
}

void regulaFalsiMethod(Token *postfixTokens, int numPostfixTokens)
{
    double a, b, epsilon, c = NAN, c_old = NAN, fa, fb, fc = NAN;
    int iteration = 0;
    int max_iterations = 100;
    int proceed_flag = 1;

    printf("\n--- Regula-Falsi Method ---\n");
    printf("Araligi giriniz [a, b]: ");
    if(scanf("%lf %lf", &a, &b) != 2)
    {
        printf("Gecersiz aralik.\n");
        while (getchar() != '\n' && getchar() != EOF); return;
    }
    while (getchar() != '\n' && getchar() != EOF);

    printf("Hata payini giriniz (epsilon, e.g., 0.00001): ");
    if(scanf("%lf", &epsilon) != 1)
    {
        printf("Invalid input for epsilon.\n");
        while (getchar() != '\n' && getchar() != EOF); return;
    }
    while(getchar() != '\n' && getchar() != EOF);

    if (a >= b)
    {
        printf("Error: Invalid interval. 'a' must be less than 'b'.\n");
        return;
    }
    if (epsilon <= 0)
    {
        printf("Error: Epsilon must be a positive value.\n");
        return;
    }

    fa = evaluatePostfix(postfixTokens, numPostfixTokens, a);
    fb = evaluatePostfix(postfixTokens, numPostfixTokens, b);

    if(isnan(fa) || isnan(fb))
    {
        printf("Error: Could not evaluate function at initial points f(a)=%g or f(b)=%g.\n", fa, fb);
        return;
    }
    printf("\nInitial: a=%.7f, f(a)=%.7f, b=%.7f, f(b)=%.7f\n", a, fa, b, fb);

    if (fa * fb >= 0) 
    {
        printf("Regula-Falsi calisamadi. f(a) ve f(b) ters isaretli olmali.\n");
        if (fabs(fa) < epsilon) printf("However, 'a' (%.7f) might be a root: f(a) = %.7f\n", a, fa);
        if (fabs(fb) < epsilon) printf("However, 'b' (%.7f) might be a root: f(b) = %.7f\n", b, fb);
        return;
    }

    printf("\nIteration |    a    |    b    |    c    |   f(a)   |   f(b)   |   f(c)   | |c-c_old| |\n");
    printf("------------------------------------------------------------------------------------------\n");
    
    c_old = (a+b)/2; // Initial c_old for first iteration's error check, can be anything not a or b.

    while(iteration < max_iterations && proceed_flag == 1)
    {
        if(fabs(fb - fa) < 1e-9)
        {
            printf("f(a) ve f(b) cok yakin, division by zero or instability in Regula-Falsi. fa=%g, fb=%g\n", fa, fb);
            proceed_flag = 0;
        }
        
        if(proceed_flag == 1) 
        {
            c = (a * fb - b * fa) / (fb - fa); //regula falsi turev acilimi
            fc = evaluatePostfix(postfixTokens, numPostfixTokens, c);

            if (isnan(fc))
            {
                printf("Error: Could not evaluate function at c = %lf in iteration %d.\n", c, iteration + 1);
                proceed_flag = 0;
            }
        }

        if(proceed_flag == 1)
        {
            printf("%9d | %.7f | %.7f | %.7f | %.7f | %.7f | %.7f | %.7f |\n",
                   iteration + 1, a, b, c, fa, fb, fc, iteration > 0 ? fabs(c - c_old) : NAN);

            if (fabs(fc) < epsilon || (iteration > 0 && fabs(c - c_old) < epsilon))
            {
                printf("\nKok %d iterasyon sonra bulundu.\n", iteration + 1);
                proceed_flag = 0;
            }
            else 
            {
                if (fa * fc < 0)
                {
                    b = c;
                    fb = fc;
                }
                else
                {
                    a = c;
                    fa = fc;
                }
                c_old = c;
            }
        }
        iteration++;
    }
    
    if(proceed_flag == 0 && !isnan(c)) 
    {
         if (!isnan(fc)) 
         {
            printf("Yaklasik kok c = %.7f\n", c);
            printf("f(c) = %.7f\n", fc);
            printf("Error |c-c_old| = %.7f (tolerance epsilon = %.7f)\n", fabs(c-c_old), epsilon);
         }
    } 
    else if(iteration == max_iterations && !isnan(c))
    {
        printf("\nCok fazla iterasyon!\n", max_iterations);
        printf("Son yaklasik c = %.7f, f(c) = %.7f\n", c, fc);
        printf("Current error |c-c_old| = %.7f (tolerance epsilon = %.7f)\n", fabs(c-c_old), epsilon);
    }
}

double centralDerivativeFunctions(Token *postfixTokens, int numPostfixTokens, double x_val, double h)
{
    if (h == 0) 
    {
        printf("h sifir olamaz.\n");
        return NAN;
    }
    double f_x_plus_h = evaluatePostfix(postfixTokens, numPostfixTokens, x_val + h);
    double f_x_minus_h = evaluatePostfix(postfixTokens, numPostfixTokens, x_val - h);
   
    if (isnan(f_x_plus_h) || isnan(f_x_minus_h))
    {
        printf("Error");
        return NAN;
    }
    return (f_x_plus_h - f_x_minus_h) / (2.0 * h);
}

double forwardDerivativeFunctions(Token *postfixTokens, int numPostfixTokens, double x_val, double h)
{
    
    double f_x_plus_h = evaluatePostfix(postfixTokens, numPostfixTokens, x_val + h);
    double f_x = evaluatePostfix(postfixTokens, numPostfixTokens, x_val);

    if (isnan(f_x_plus_h) || isnan(f_x))
    {
        printf("Error");
        return NAN;
    }
    return(f_x_plus_h - f_x) / h;
}

double backwardDerivativeFunctions(Token *postfixTokens, int numPostfixTokens, double x_val, double h)
{
    double f_x = evaluatePostfix(postfixTokens, numPostfixTokens, x_val);
    printf("%.6f", f_x);
    double f_x_minus_h = evaluatePostfix(postfixTokens, numPostfixTokens, x_val - h);

    if(isnan(f_x) || isnan(f_x_minus_h))
    {
        printf("Error");
        return NAN;
    }
    return (f_x - f_x_minus_h) / h;
}

void newtonRaphsonMethod(Token *postfixTokens, int numPostfixTokens) 
{
    double x0, x1=NAN, epsilon, fx0, f_prime_x0;
    int iteration = 0;
    int max_iterations = 100;
    int proceed_flag = 1;
    double h_turev = H_TUREV;

    printf("\n--- Newton-Raphson Method ---\n");
    printf("Enter initial guess x0: ");
    if(scanf("%lf", &x0) != 1)
    {
        printf("Invalid input for initial guess x0.\n");
        while(getchar() != '\n' && getchar() != EOF); return;
    }
    while(getchar() != '\n' && getchar() != EOF);

    printf("Hata payini giriniz (epsilon): ");
    if(scanf("%lf", &epsilon) != 1)
    {
        printf("Invalid input for epsilon.\n");
        while (getchar() != '\n' && getchar() != EOF); return;
    }
    while(getchar() != '\n' && getchar() != EOF);

    if (epsilon <= 0)
    {
        printf("Hata payi pozitif olmali.\n");
        return;
    }

    printf("\nIteration |    x_k    |   f(x_k)  |  f'(x_k)  |  x_{k+1}  | |x_{k+1}-x_k| |\n");
    printf("--------------------------------------------------------------------------------\n");

    while(iteration < max_iterations && proceed_flag == 1)
    {
        fx0 = evaluatePostfix(postfixTokens, numPostfixTokens, x0);
        f_prime_x0 = centralDerivativeFunctions(postfixTokens, numPostfixTokens, x0, h_turev);

        if(isnan(fx0))
        {
            printf("Error: Could not evaluate f(x0)=%g or f'(x0)=%g at x0 = %lf in iteration %d.\n", fx0, f_prime_x0, x0, iteration + 1);
            proceed_flag = 0;
        }
        else if(isnan(f_prime_x0))
        {
            printf("Error: f'(x0) could not be numerically evaluated at x0 = %lf, iter %d.\n", x0, iteration+1);
            proceed_flag = 0;
        }
        else if(fabs(f_prime_x0) < 1e-12)
        { 
            printf("Error: Numerical derivative f'(x0) is too small (f'(%lf) = %g). Method fails/unstable.\n", x0, f_prime_x0);
            proceed_flag = 0;
        }

        if(proceed_flag == 1)
        {
            x1 = x0 - fx0 / f_prime_x0;
            printf("%9d | %.7f | %.7f | %.7f | %.7f | %.7f |\n",
                   iteration + 1, x0, fx0, f_prime_x0, x1, fabs(x1 - x0));

            if(fabs(fx0) < epsilon || fabs(x1 - x0) < epsilon)  // Check f(x_k) or change in x
            {
                printf("\nKok %d iterasyon sonra bulundu.\n", iteration + 1);
                proceed_flag = 0;
            }
            else
            {
                x0 = x1;
            }
        }
        iteration++;
    }

    if(proceed_flag == 0 && !isnan(x1)) // Root found or error
    {
        if(!isnan(fx0) && fabs(f_prime_x0) >= 1e-9)  // If root found path
        {
            printf("Yaklasik kok x = %.7f\n", x1);
            printf("f(x) = %.7f\n", evaluatePostfix(postfixTokens, numPostfixTokens, x1)); // f(x1)
        } // Error messages already printed
    }
    else if(iteration == max_iterations && !isnan(x1))
    { // Max iterations reached
        printf("\nMethod failed to converge to the desired tolerance within %d iterations.\n", max_iterations);
        printf("Last approximation x = %.7f, f(x) = %.7f\n", x1, evaluatePostfix(postfixTokens, numPostfixTokens, x1));
    }
    else if(isnan(x1) && proceed_flag == 1) 
    { // Loop finished but x1 is NAN (e.g. first iteration failed)
        printf("\nNewton-Raphson method could not proceed (likely evaluation error on first step).\n");
    }
}

//Matrix fonksiyonları
Matrix *createMatrix(int row, int col)
{
    int i, j, k;
    Matrix *mtr = (Matrix*)malloc(sizeof(Matrix));
    if(mtr == NULL)
    {
        printf("Memory allocation failed for Matrix struct.\n");
        return NULL;
    }
    mtr->row = row;
    mtr->col = col;
    mtr->data = (double**)malloc(row * sizeof(double*));
    if(mtr->data == NULL)
    {
        printf("Memory allocation failed for matrix rows.\n");
        free(mtr);
        return NULL;
    }
    for(i=0;i<row;i++)
    {
        mtr->data[i] = (double*)malloc(col * sizeof(double));
        if(mtr->data[i] == NULL) 
        {
            printf("Memory allocation failed for matrix columns (row %d).\n", i);
            for(k=0; k < i; k++) 
            {
                free(mtr->data[k]);
            }
            free(mtr->data);
            free(mtr);
        }
        for(j=0;j<col;j++)
        {
            mtr->data[i][j] = 0;
        }
    }
    return mtr;
}

void freeMatrix(Matrix *mat) 
{
    int i;
    if(mat != NULL) 
    {
        if(mat->data != NULL)
        {
            for(i = 0; i < mat->row; i++)
            {
                if(mat->data[i] != NULL)
                {
                    free(mat->data[i]);
                }
            }
            free(mat->data);
        }
        free(mat);
    }
}

void printMatrix(Matrix *mtr)
{
    int i, j;
    if(mtr == NULL || mtr->data == NULL) 
    {
        printf("NULL matrix");
        return;
    }
    printf("\nMatrix (%d x %d):\n", mtr->row, mtr->col);
    for(i=0; i < mtr->row; i++) 
    {
        for(j=0; j < mtr->col; j++) 
        {
            printf("%10.4f ", mtr->data[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

Matrix *createIdentityMatrix(int N) //birim matris olusturur
{
    int i;
    Matrix *I = createMatrix(N, N);
    for(i=0;i<N;i++)
    {
        I->data[i][i] = 1;
    }
    return I;
}

int calculateInverse(Matrix *originalMtr, Matrix *inverseMtrOutput, int N)
{
    int i, j, k;
    double tmp; // For swapping elements
    double pivot_element;
    double factor;
    int pivot_row_ind;

    Matrix *copyMtr = createMatrix(N, N);
    if(copyMtr == NULL)
    {
        printf("Hata: Ters alma için kopya matris oluşturulamadi.\n");
        return 0;
    }

    for(i = 0; i < N; i++) //initializing the inverse as an identity matrix
    {
        for(j = 0; j < N; j++)
        {
            copyMtr->data[i][j] = originalMtr->data[i][j];
            if(i == j)
            {
                inverseMtrOutput->data[i][j] = 1.0;
            }
            else
            {
                inverseMtrOutput->data[i][j] = 0.0;
            }
        }
    }

    for(j=0; j<N; j++)  //once sutunlar
    {
        int pivot_row_ind = j; //her sutun icin pivot bulma
        for(i = j + 1; i < N; i++)
        {
            if (fabs(copyMtr->data[i][j]) > fabs(copyMtr->data[pivot_row_ind][j]))
            {
                pivot_row_ind = i;
            }
        }
        if (pivot_row_ind != j) //satır degistirme
        {
            for(k=0; k<N; k++)
            {
                tmp = copyMtr->data[j][k];
                copyMtr->data[j][k] = copyMtr->data[pivot_row_ind][k];  //swapping 
                copyMtr->data[pivot_row_ind][k] = tmp;

                tmp = inverseMtrOutput->data[j][k];
                inverseMtrOutput->data[j][k] = inverseMtrOutput->data[pivot_row_ind][k]; //swapping the inverse
                inverseMtrOutput->data[pivot_row_ind][k] = tmp;
            }
        }
        pivot_element = copyMtr->data[j][j];
        // Tekillik kontrolü
        if(fabs(pivot_element) < EPSILON)
        {
            printf("Hata: Matris tekil.(satir %d, sütun %d: %g).\n", j, j, pivot_element);
            freeMatrix(copyMtr);
            return 0; // Başarısız
        }

        for(k=0; k<N; k++) //pivot elemanın 1 yapılması
        {
            copyMtr->data[j][k] /= pivot_element;
            inverseMtrOutput->data[j][k] /= pivot_element;
        }

        for(i=0; i<N; i++) //last part diger satirlardaki j. sutun elemanlari sıfırlanir
        {
            if(i != j)
            {
                factor = copyMtr->data[i][j];
                for(k=0; k < N; k++)
                {
                    copyMtr->data[i][k] -= factor * copyMtr->data[j][k];
                    inverseMtrOutput->data[i][k] -= factor * inverseMtrOutput->data[j][k];
                }
            }
        }
    }

    freeMatrix(copyMtr);
    return 1; //basarili
}

void displayInverveMatrix()
{
    int n;
    int i, j;
    int c;

    printf("\n--- NxN Matrisin Tersi ---\n");
    printf("Matrisin boyutunu girin (N): ");
    if (scanf("%d", &n) != 1 || n <= 0)
    {
        printf("Gecersiz boyut. N pozitif bir tamsayi olmalidir.\n");
        while ((c = getchar()) != '\n' && c != EOF);
        return;
    }
    while((c = getchar()) != '\n' && c != EOF);

    if (n > 30)
    {
        printf("Uyari: %dx%d matrisler icin hesaplama biraz zaman alabilir.\n", n,n);
    }

    Matrix *inputMatrix = createMatrix(n, n);
    Matrix *inverseMatrixResult = createMatrix(n, n);

    if(inputMatrix == NULL || inverseMatrixResult == NULL)
    {
        printf("Matrisler icin bellek ayrilamadi.\n");
        freeMatrix(inputMatrix); 
        freeMatrix(inverseMatrixResult);
        return;
    }

    printf("Matris elemanlarini girin (%dx%d):\n", n, n);
    int flag = 1;
    for(i = 0; i<n && flag == 1; i++)
    { 
        printf("Satir %d: ", i + 1);
        for (j = 0; j < n && flag == 1; j++)
        {
            if(scanf("%lf", &inputMatrix->data[i][j]) != 1)
            {
                printf("\nGecersiz eleman girisi. Islem iptal edildi.\n");
                flag = 0; 
                while ((c = getchar()) != '\n' && c != EOF); 
            }
        }
        if(flag == 1) 
        {
            while((c = getchar()) != '\n' && c != EOF && c != ' '); // Satır sonu veya sonraki sayıya kadar kadar kalanlari temizledik
        }
    }
    
    if(flag == 1)
    {
        printf("\nGirdiğiniz Matris:");
        printMatrix(inputMatrix);
        if(calculateInverse(inputMatrix, inverseMatrixResult, n)) 
        {
            printf("Matrisin Tersi:");
            printMatrix(inverseMatrixResult);
        }
        else
        {
            printf("Matrisin tersi hesaplanamadi.\n");
        }
    }
    freeMatrix(inputMatrix);
    freeMatrix(inverseMatrixResult);
}

int luDecomposition(Matrix *A, Matrix *L, Matrix *U, int N)
{
    int i, j, k;
    double sum;

    if(A == NULL || L == NULL || U == NULL || A->row != N || A->col != N || L->row != N || L->col != N || U->row != N || U->col != N)
    {
        printf("Hata: L ve U matrisleri dekomposize edilemedi. Gecersiz boyut veya matrisler.\n");
        return 0; // Başarısız
    }
    if(N <= 0)
    {
         printf("Hata: Matris boyutu N pozitif olmali.\n");
         return 0;
    }

    // L ve U matrislerini sıfırla (U'nun köşegeni hariç)
    for(i = 0; i < N; i++)
    {
        for(j = 0; j < N; j++)
        {
            L->data[i][j] = 0.0;
            U->data[i][j] = 0.0;
        }
    }

    for(j = 0; j < N; j++)
    { // Her sütun için (veya pivot adımı)
        // L matrisinin j. sütununu hesapla
        for (i = j; i < N; i++) // L_ij, i >= j
        {
            sum = 0.0;
            for (k = 0; k < j; k++)
            {
                sum += L->data[i][k] * U->data[k][j];
            }
            L->data[i][j] = A->data[i][j] - sum;
        }

        // U matrisinin j. satırını hesapla (U_jj = 1)
        U->data[j][j] = 1.0;
        if(fabs(L->data[j][j]) < EPSILON)
        {
            printf("Hata: L[%d][%d] (pivot) elemani çok küçük (%.4g). LU dekompozisyonu basarisiz.\n", j,j, L->data[j][j]);
            return 0; // Başarısız (tekil olabilir veya pivotlama gerekir)
        }
        for(k = j + 1; k < N; k++) // U_jk, k > j
        {
            sum = 0.0;
            for (i = 0; i < j; i++)
            {
                sum += L->data[j][i] * U->data[i][k];
            }
            U->data[j][k] = (A->data[j][k] - sum) / L->data[j][j];
        }
    }
    return 1; // Başarılı
}

void forwardSubstitution(Matrix *L, double y[], double b[], int N) //Lx = b
{
    int i, j, k;
    double sum;
    for (i = 0; i < N; i++)
    {
        sum = 0.0;
        for (j = 0; j < i; j++)
        {
            sum += L->data[i][j] * y[j];
        }
        if(fabs(L->data[i][i]) < EPSILON)
        {
            printf("Hata: İleri yerine koymada L[%d][%d] sifir. Sistem çözülemiyor.\n", i, i);
            for(k=0; k<N; k++)
            {
                y[k] = NAN; // Çözümü geçersiz kıl
                return;
            }
        }
        y[i] = (b[i] - sum) / L->data[i][i];
    }
}

void backwardSubstitution(Matrix *U, double x_sol[], double y[], int N) // Ux = y
{
    int i, j;
    double sum;
    for (i = N - 1; i >= 0; i--)
    {
        sum = 0.0;
        for (j = i + 1; j < N; j++)
        {
            sum += U->data[i][j] * x_sol[j];
        }
        x_sol[i] = (y[i] - sum); // U->data[i][i] (yani U_ii) 1.0 olduğu için bölmeye gerek yok
    }
}

void cholesky()
{
    int N, i, j, c;
    int y_flag;
    printf("\n--- LU Dekompozisyonu ile Denklem Sistemi Çözümü (Ax=b) ---\n");
    printf("Katsayilar matrisi A'nin boyutunu girin (N): ");
    if(scanf("%d", &N) != 1 || N <= 0) 
    {
        printf("Geçersiz boyut.\n");
        while((c = getchar()) != '\n' && c != EOF);
        return;
    }
    while((c = getchar()) != '\n' && c != EOF);
    Matrix *A = createMatrix(N, N);
    Matrix *L = createMatrix(N, N);
    Matrix *U = createMatrix(N, N);
    double *b_vec = (double*)malloc(N * sizeof(double)); // b_vec olarak adlandırdım, bisection'daki b ile karışmasın diye
    double *x_sol = (double*)malloc(N * sizeof(double)); // x_sol
    double *y_vec = (double*)malloc(N * sizeof(double)); // y_vec

    if(!A || !L || !U || !b_vec || !x_sol || !y_vec)
    {
        printf("LU dekompozisyonu için bellek ayrilamadi.\n");
        freeMatrix(A); freeMatrix(L); freeMatrix(U);
        free(b_vec); free(x_sol); free(y_vec);
        return;
    }
    printf("A matrisinin elemanlarini girin (%dx%d):\n", N, N);
    for(i = 0; i < N; i++)
    {
        printf("Satir %d: ", i + 1);
        for(j = 0; j < N; j++)
        {
            if(scanf("%lf", &A->data[i][j]) != 1)
            {
                printf("\nGeçersiz eleman. İşlem iptal.\n");
                while ((c = getchar()) != '\n' && c != EOF);
                freeMatrix(A); 
                freeMatrix(L);
                freeMatrix(U);
                free(b_vec); 
                free(x_sol);
                free(y_vec);
                return;
            }
        }
        while ((c = getchar()) != '\n' && c != EOF && c != ' ');
    }
    printf("b vektörünün elemanlarini girin (%d eleman):\n", N);
    for (i = 0; i < N; i++)
    {
        printf("b[%d]: ", i);
        if (scanf("%lf", &b_vec[i]) != 1)
        {
            printf("\nGeçersiz eleman. İşlem iptal.\n"); 
            while ((c = getchar()) != '\n' && c != EOF);
            freeMatrix(A); freeMatrix(L); freeMatrix(U);
            free(b_vec);
            free(x_sol);
            free(y_vec);
            return;
        }
    }
    while((c = getchar()) != '\n' && c != EOF);

    printf("\nOrijinal A Matrisi:");
    printMatrix(A);
    printf("Orijinal b Vektörü:\n");
    for(i=0; i<N; i++)
    {
        printf("%.4f ", b_vec[i]);
        printf("\n");
    }

    if(luDecomposition(A, L, U, N))
    {
        printf("\nL Matrisi:");
        printMatrix(L);
        printf("U Matrisi (Köşegenler 1):");
        printMatrix(U);
        forwardSubstitution(L, y_vec, b_vec, N);
        y_flag = 0;
        for(i=0; i<N; i++)
        {
            if(isnan(y_vec[i]))
            {
                y_flag = 1;
            }   
        }
        if(!y_flag)
        {
            printf("\nAra y Vektörü (Ly=b):\n");
            for(i = 0; i < N; i++)
            {
                printf("y[%d] = %.4f\n", i, y_vec[i]);
            }
            backwardSubstitution(U, x_sol, y_vec, N);
            printf("\nÇözüm x Vektörü (Ux=y):\n");
            for(i = 0; i < N; i++)
            {
                printf("x[%d] = %.4f\n", i, x_sol[i]);
            }
        }
        else
        {
            printf("İleri yerine koyma sirasinda hata oluştuğu için çözüm bulunamadi.\n");
        }
    }
    else
    {
        printf("LU dekompozisyonu basarisiz. Sistem çözülemiyor.\n");
    }
    freeMatrix(A); freeMatrix(L); freeMatrix(U);
    free(b_vec); free(x_sol); free(y_vec);
}

void freeAllGaussSeidel(Matrix* A_mtr, double* b_vec, double* x_current, double* x_prev_iter)
{
    freeMatrix(A_mtr);
    free(b_vec);
    free(x_current);
    free(x_prev_iter);
}

void gaussSeidelMethod()
{
    int N, i, j, iter = 0, max_iter = 100, c_buffer_clear; 
    double epsilon_tol, sum_val, current_x_value; 
    int flag_loop = 1;
    double max_diff_this_iter, diff;
    int can_print_solution = 1;

    printf("\n--- Gauss-Seidel Yöntemi (Ax=b) ---\n");
    printf("Katsayilar matrisi A'nin boyutunu girin (N): ");
    if(scanf("%d", &N) != 1 || N <= 0)
    { 
        printf("Geçersiz boyut.\n"); 
        while((c_buffer_clear = getchar()) != '\n' && c_buffer_clear != EOF); 
        return; 
    }
    while((c_buffer_clear = getchar()) != '\n' && c_buffer_clear != EOF);

    Matrix *A_matrix_gs = createMatrix(N, N); 
    double *b_vector_gs = (double*)malloc(N * sizeof(double)); 
    double *x_current_iter_gs = (double*)malloc(N * sizeof(double)); 
    double *x_previous_iter_gs = (double*)malloc(N * sizeof(double)); 
   
    if(!A_matrix_gs || !b_vector_gs || !x_current_iter_gs || !x_previous_iter_gs)
    {
        printf("Gauss-Seidel için bellek ayrilamadi.\n");
        freeAllGaussSeidel(A_matrix_gs, b_vector_gs, x_current_iter_gs, x_previous_iter_gs);
        return;
    }

    printf("A matrisinin elemanlarini girin (%dx%d):\n", N, N);
    for(i = 0; i < N; i++)
    {
        printf("Satir %d: ", i + 1);
        for (j = 0; j < N; j++)
        {
            if(scanf("%lf", &A_matrix_gs->data[i][j]) != 1)
            {
                printf("\nGeçersiz eleman. İşlem iptal.\n");
                while((c_buffer_clear = getchar()) != '\n' && c_buffer_clear != EOF);
                freeAllGaussSeidel(A_matrix_gs, b_vector_gs, x_current_iter_gs, x_previous_iter_gs);
                return;
            }
        }
        while ((c_buffer_clear = getchar()) != '\n' && c_buffer_clear != EOF);
    }

    printf("b vektörünün elemanlarini girin (%d eleman):\n", N);
    for(i = 0; i < N; i++)
    {
        printf("b[%d]: ", i);
        if(scanf("%lf", &b_vector_gs[i]) != 1)
        {
            printf("\nGeçersiz eleman. İşlem iptal.\n");
            while((c_buffer_clear = getchar()) != '\n' && c_buffer_clear != EOF);
            freeAllGaussSeidel(A_matrix_gs, b_vector_gs, x_current_iter_gs, x_previous_iter_gs);
            return;
        }
    }
    while((c_buffer_clear = getchar()) != '\n' && c_buffer_clear != EOF);

    printf("x için başlangic tahminlerini girin (%d eleman, örn: hepsi 0):\n", N);
    for(i = 0; i < N; i++)
    {
        printf("x_baslangic[%d]: ", i);
        if(scanf("%lf", &x_current_iter_gs[i]) != 1)
        {
            printf("\nGeçersiz eleman. İşlem iptal.\n"); 
            while((c_buffer_clear = getchar()) != '\n' && c_buffer_clear != EOF);
            freeAllGaussSeidel(A_matrix_gs, b_vector_gs, x_current_iter_gs, x_previous_iter_gs);
            return;
        }
    }
    while((c_buffer_clear = getchar()) != '\n' && c_buffer_clear != EOF);

    printf("Hata payini giriniz (epsilon, örn: 0.00001): ");
    if(scanf("%lf", &epsilon_tol) != 1 || epsilon_tol <= 0)
    { 
        printf("Geçersiz hata payi.\n"); 
        while((c_buffer_clear = getchar()) != '\n' && c_buffer_clear != EOF); 
        freeAllGaussSeidel(A_matrix_gs, b_vector_gs, x_current_iter_gs, x_previous_iter_gs); 
        return; 
    }
    while((c_buffer_clear = getchar()) != '\n' && c_buffer_clear != EOF);

    printf("\nIter |"); for(i=0; i<N; i++) printf("    x%d    |", i+1); printf(" Maks.Fark|\n");
    printf("-----------------------------------------------------------\n");

    while(flag_loop == 1 && iter < max_iter)
    {
        max_diff_this_iter = 0.0; 
        for(i = 0; i < N; i++)
        {
            x_previous_iter_gs[i] = x_current_iter_gs[i]; 
        }

        for(i = 0; i < N; i++)
        { 
            sum_val = 0.0;
            for (j = 0; j < N; j++)
            {
                if (i != j)
                {
                    sum_val += A_matrix_gs->data[i][j] * x_current_iter_gs[j]; 
                }
            }
            if (fabs(A_matrix_gs->data[i][i]) < EPSILON)
            {
                printf("\nHata: Köşegen elemanı A[%d][%d] sıfıra çok yakın. Gauss-Seidel başarısız.\n", i, i);
                flag_loop = 0; 
            }
            if (flag_loop == 1) { // Check flag before calculation
                current_x_value = (b_vector_gs[i] - sum_val) / A_matrix_gs->data[i][i];
                x_current_iter_gs[i] = current_x_value; 
            }
        }
       
        if(flag_loop == 1)
        { 
            for (i = 0; i < N; i++)
            {
                diff = fabs(x_current_iter_gs[i] - x_previous_iter_gs[i]);
                if (diff > max_diff_this_iter)
                {
                    max_diff_this_iter = diff;
                }
            }
            printf("%4d |", iter + 1); for(i=0; i<N; i++) printf(" %.7f |", x_current_iter_gs[i]); printf(" %.7f |\n", max_diff_this_iter);

            if (max_diff_this_iter < epsilon_tol)
            {
                printf("\n%d iterasyonda yakınsadı.\n", iter + 1);
                flag_loop = 0; 
            }
        }
        iter++;
    }

    if (flag_loop == 1 && iter == max_iter)
    {
        printf("\nGauss-Seidel %d iterasyonda verilen toleransla yakınsayamadı.\n", max_iter);
    }
   
    if (flag_loop == 0 && iter <= max_iter)
    {
        can_print_solution = 1;
        for(i=0; i<N; i++){ 
            if(isnan(x_current_iter_gs[i]))
            {
                can_print_solution = 0;
                flag_loop = 1;
            }
        }
        if(can_print_solution)
        {
            printf("\nÇözüm x:\n");
            for (i = 0; i < N; i++) printf("x[%d] = %.7f\n", i, x_current_iter_gs[i]);
        } 
        else if(iter <= max_iter && !can_print_solution){ 
        }
    }
    freeAllGaussSeidel(A_matrix_gs, b_vector_gs, x_current_iter_gs, x_previous_iter_gs);
}

void simpsonOneThirdMethod(Token *postfixTokens, int numPostfixTokens)
{
    double a, b, h, sum_total, integral_val;
    int n_intervals, i, c;
    double fa, fb;
    double xi, fxi;
    int multiplier;

    printf("\n--- Simpson 1/3 Kuralı ile İntegral Hesabı ---\n");
    printf("İntegrasyonun alt limitini girin (a): ");
    if (scanf("%lf", &a) != 1)
    {
        printf("Geçersiz girdi.\n");
        while ((c = getchar()) != '\n' && c != EOF); return;
    }
    while ((c = getchar()) != '\n' && c != EOF);

    printf("İntegrasyonun üst limitini girin (b): ");
    if (scanf("%lf", &b) != 1)
    {
        printf("Geçersiz girdi.\n");
        while ((c = getchar()) != '\n' && c != EOF);
        return;
    }
    while ((c = getchar()) != '\n' && c != EOF);

    printf("Alt aralık sayısını girin (n, ÇİFT ve >0 olmalı): ");
    if (scanf("%d", &n_intervals) != 1 || n_intervals <= 0 || n_intervals % 2 != 0)
    {
        printf("Geçersiz n. Pozitif ÇİFT bir tamsayı olmalıdır.\n");
        while ((c = getchar()) != '\n' && c != EOF); return;
    }
    while ((c = getchar()) != '\n' && c != EOF);

    if (a >= b) { printf("Geçersiz aralık: a < b olmalıdır.\n"); return; }

    h = (b - a) / n_intervals;
    sum_total = 0.0;

    fa = evaluatePostfix(postfixTokens, numPostfixTokens, a);
    fb = evaluatePostfix(postfixTokens, numPostfixTokens, b);

    if(isnan(fa) || isnan(fb)) { printf("Hata: Fonksiyon uç noktalarda değerlendirilemedi.\n"); return; }

    sum_total = fa + fb; // y0 + yn

    printf("\nAra noktaların hesaplanması:\n");
    printf(" i |    x_i    |   f(x_i)   | Katsayı\n");
    printf("-----------------------------------------\n");
    printf("%2d | %.7f | %.7f |    1    (y0)\n", 0, a, fa);

    for (i = 1; i < n_intervals; i++)
    {
        xi = a + i * h;
        fxi = evaluatePostfix(postfixTokens, numPostfixTokens, xi);
        if(isnan(fxi)) { printf("Hata: Fonksiyon x = %.7f noktasında değerlendirilemedi.\n", xi); return; }
        
        if (i % 2 == 1)
        {
            sum_total += 4 * fxi;
            multiplier = 4;
        }
        else 
        {
            sum_total += 2 * fxi;
            multiplier = 2;
        }
        printf("%2d | %.7f | %.7f |    %d    (y%d)\n", i, xi, fxi, multiplier, i);
    }
    printf("%2d | %.7f | %.7f |    1    (y%d)\n", n_intervals, b, fb, n_intervals);

    integral_val = (h / 3.0) * sum_total;
    
    printf("\n-----------------------------------------\n");
    printf("h = (b-a)/n = (%.4f - %.4f)/%d = %.7f\n", b, a, n_intervals, h);
    printf("Simpson 1/3 Kuralı ile integralin yaklaşık değeri: %.7f\n", integral_val);
}

void simpsonThreeEighthMethod(Token *postfixTokens, int numPostfixTokens)
{
    double a, b, h, sum_total, integral_val;
    int n_intervals, i, c;
    double fa, fb;
    double xi, fxi;

    printf("\n--- Simpson 3/8 Kuralı ile İntegral Hesabı ---\n");
    printf("İntegrasyonun alt limitini girin (a): ");
    if (scanf("%lf", &a) != 1) { /* validation */ return; }
    while ((c = getchar()) != '\n' && c != EOF);
    printf("İntegrasyonun üst limitini girin (b): ");
    if (scanf("%lf", &b) != 1) { /* validation */ return; }
    while ((c = getchar()) != '\n' && c != EOF);
    printf("Alt aralık sayısını girin (n, 3'ÜN KATI ve >0 olmalı): ");
    if (scanf("%d", &n_intervals) != 1 || n_intervals <= 0 || n_intervals % 3 != 0) {
        printf("Geçersiz n. Pozitif ve 3'ün katı bir tamsayı olmalıdır.\n");
        while ((c = getchar()) != '\n' && c != EOF); return;
    }
    while ((c = getchar()) != '\n' && c != EOF);

    if (a >= b) { return; }

    h = (b - a) / n_intervals;
    sum_total = 0.0;

    fa = evaluatePostfix(postfixTokens, numPostfixTokens, a);
    fb = evaluatePostfix(postfixTokens, numPostfixTokens, b);

    if (isnan(fa) || isnan(fb)) { /* validation */ return; }

    sum_total = fa + fb; // y0 + yn

    printf("\nAra noktaların hesaplanması:\n");
    printf(" i |    x_i    |   f(x_i)   | Katsayı\n");
    printf("-----------------------------------------\n");
    printf("%2d | %.7f | %.7f |    1    (y0)\n", 0, a, fa);

    for (i = 1; i < n_intervals; i++)
    {
        xi = a + i * h;
        fxi = evaluatePostfix(postfixTokens, numPostfixTokens, xi);
        if(isnan(fxi)) { /* validation */ return; }
        
        int multiplier;
        if (i % 3 == 0)
        {
            sum_total += 2 * fxi;
            multiplier = 2;
        } else
        {
            sum_total += 3 * fxi;
            multiplier = 3;
        }
         printf("%2d | %.7f | %.7f |    %d    (y%d)\n", i, xi, fxi, multiplier, i);
    }
    printf("%2d | %.7f | %.7f |    1    (y%d)\n", n_intervals, b, fb, n_intervals);


    integral_val = (3.0 * h / 8.0) * sum_total;
    
    printf("\n-----------------------------------------\n");
    printf("h = (b-a)/n = (%.4f - %.4f)/%d = %.7f\n", b, a, n_intervals, h);
    printf("Simpson 3/8 Kuralı ile integralin yaklaşık değeri: %.7f\n", integral_val);
}

void trapezoidalMethod(Token *postfixTokens, int numPostfixTokens)
{
    double a, b, h, sum_intermediate_y, integral_val;
    int n_intervals, i, c;
    double fa, fb;
    double xi, fxi;


    printf("\n--- Trapez Kuralı ile İntegral Hesabı ---\n");
    printf("İntegrasyonun alt limitini girin (a): ");
    if (scanf("%lf", &a) != 1) { /* validation */ return; }
    while ((c = getchar()) != '\n' && c != EOF);
    printf("İntegrasyonun üst limitini girin (b): ");
    if (scanf("%lf", &b) != 1) { /* validation */ return; }
    while ((c = getchar()) != '\n' && c != EOF);
    printf("Alt aralık sayısını girin (n, >0 olmalı): ");
    if (scanf("%d", &n_intervals) != 1 || n_intervals <= 0) {
        printf("Geçersiz n. Pozitif bir tamsayı olmalıdır.\n");
        while ((c = getchar()) != '\n' && c != EOF); return;
    }
    while ((c = getchar()) != '\n' && c != EOF);

    if (a >= b) { return; }

    h = (b - a) / n_intervals;
    sum_intermediate_y = 0.0; // Sadece ara terimlerin toplamı (y1'den y(n-1)'e kadar)

    fa = evaluatePostfix(postfixTokens, numPostfixTokens, a); // y0
    fb = evaluatePostfix(postfixTokens, numPostfixTokens, b); // yn

    if (isnan(fa) || isnan(fb)) {return; }

    printf("\nAra noktaların hesaplanması:\n");
    printf(" i |    x_i    |   f(x_i)   \n");
    printf("---------------------------\n");
    printf("%2d | %.7f | %.7f (y0)\n", 0, a, fa);

    for (i = 1; i < n_intervals; i++) { // y1'den y(n-1)'e kadar
        xi = a + i * h;
        fxi = evaluatePostfix(postfixTokens, numPostfixTokens, xi);
        if(isnan(fxi)) { /* validation */ return; }
        sum_intermediate_y += fxi;
        printf("%2d | %.7f | %.7f (y%d)\n", i, xi, fxi, i);
    }
    printf("%2d | %.7f | %.7f (y%d)\n", n_intervals, b, fb, n_intervals);

    integral_val = (h / 2.0) * (fa + fb + 2.0 * sum_intermediate_y);
    // Alternatif formül: (h/2.0) * (y0 + 2y1 + 2y2 + ... + 2y(n-1) + yn)

    printf("\n---------------------------\n");
    printf("h = (b-a)/n = (%.4f - %.4f)/%d = %.7f\n", b, a, n_intervals, h);
    printf("Trapez Kuralı ile integralin yaklaşık değeri: %.7f\n", integral_val);
}

double factorial(int n)
{
    double fact;
    int i;
    if(n < 0)
    {
        printf("Hata: Faktöriyel negatif sayılar için tanımsızdır.\n");
        return NAN; // Not a Number
    }
    if (n == 0) return 1.0;
    fact = 1.0;
    for(i = 1; i <= n; i++)
    {
        fact *= i;
    }
    return fact;
}

int calculateForwardDifferences(DataPoint *points, int num_points, double **diff_table_first_elements_ptr)
{
    double *diff_table_first_elements;
    int i, j, k;

    if(num_points <= 0 || points == NULL)
    {
        printf("Hata: Geçersiz nokta sayısı veya nokta dizisi (calculateForwardDifferences).\n");
        return 0;
    }

    *diff_table_first_elements_ptr = (double *)malloc(num_points * sizeof(double));
    if (*diff_table_first_elements_ptr == NULL)
    {
        printf("Hata: İleri farklar için bellek ayrılamadı (diff_table_first_elements_ptr).\n");
        return 0;
    }
    diff_table_first_elements = *diff_table_first_elements_ptr;

    // Create a temporary 2D array for the full difference table
    double **table = (double **)malloc(num_points * sizeof(double *));
    if(table == NULL)
    {
        free(diff_table_first_elements);
        *diff_table_first_elements_ptr = NULL;
        printf("Hata: Geçici fark tablosu için bellek ayrılamadı (satırlar).\n");
        return 0;
    }
    for(i = 0; i < num_points; i++)
    {
        table[i] = (double *)malloc(num_points * sizeof(double));
        if (table[i] == NULL)
        {
            printf("Hata: Geçici fark tablosu için bellek ayrılamadı (sütun %d).\n", i);
            for (int k_free = 0; k_free < i; k_free++) free(table[k_free]);
            free(table);
            free(diff_table_first_elements);
            *diff_table_first_elements_ptr = NULL;
            return 0;
        }
    }

    for(i = 0; i < num_points; i++) 
    {
        table[i][0] = points[i].y;
    }

    for(j = 1; j < num_points; j++) 
    {
        for(i = 0; i < num_points - j; i++)
        { 
            table[i][j] = table[i + 1][j - 1] - table[i][j - 1];
        }
    }

    printf("\nİleri Farklar Tablosu:\n");
    printf("x_i      | y_i      |");
    for(j = 1; j < num_points; j++)
    {
        printf("   Δ^%d y   |", j);
    }
    printf("\n---------|----------|");
    for(j = 1; j < num_points; j++)
    {
        printf("----------|");
    }
    printf("\n");

    for(i = 0; i < num_points; i++)
    {
        printf("%8.4f | %8.4f |", points[i].x, table[i][0]);
        if (i == 0)
        {
            for(k=0; k < num_points; k++)
            {
                 if (k < num_points - i)
                 {
                    diff_table_first_elements[k] = table[i][k];
                 }
                 else
                 {
                    diff_table_first_elements[k] = 0;
                 }
            }
        }
        for (int j = 1; j < num_points - i; j++) {
            printf(" %8.4f |", table[i][j]);
        }
        printf("\n");
    }
    printf("\n");

    for(i = 0; i < num_points; i++)
    {
        free(table[i]);
    }
    free(table);

    return 1;
}

double gregoryNewtonForwardInterpolation(DataPoint *points, int num_points, double x_interpolate, double **forward_diffs_ptr)
{
    int i, k;
    double h;
    double *y_diffs;
    double s;
    double interpolated_y;
    double term_s;
    double current_term_value;

    if (num_points <= 0 || points == NULL)
    {
        printf("Hata: Enterpolasyon için yetersiz nokta veya NULL point dizisi.\n");
        return NAN;
    }
    *forward_diffs_ptr = NULL;

    if (num_points > 1)
    {
        h = points[1].x - points[0].x;
        for(i = 1; i < num_points - 1; i++)
        {
            if (fabs((points[i+1].x - points[i].x) - h) > EPSILON * 1000)
            {
                printf("Hata: Gregory-Newton İleri Enterpolasyonu için x değerleri eşit aralıklı olmalıdır.\n");
                printf("Fark: x[%d]-x[%d] = %.5f, x[%d]-x[%d] = %.5f, beklenen h=%.5f\n", i+1, i, points[i+1].x - points[i].x, i, i-1, points[i].x - points[i-1].x, h);
                return NAN;
            }
        }
        if (fabs(h) < EPSILON)
        {
            printf("Hata: h değeri (x noktaları arası fark) çok küçük veya sıfır (Gregory-Newton).\n");
            return NAN;
        }
    }
    else
    {
        if(fabs(x_interpolate - points[0].x) < EPSILON) return points[0].y;
        else
        {
            printf("Uyarı: Enterpolasyon için sadece bir nokta var. x_interpolate noktadaki x ile eşleşmiyorsa, enterpolasyon yapılamaz.\n");
            return NAN;
        }
    }

    if(!calculateForwardDifferences(points, num_points, forward_diffs_ptr) || *forward_diffs_ptr == NULL)
    {
        printf("Hata: İleri farklar hesaplanamadı.\n");
        if (*forward_diffs_ptr != NULL)
        {
            free(*forward_diffs_ptr);
            *forward_diffs_ptr = NULL;
        }
        return NAN;
    }
    y_diffs = *forward_diffs_ptr;
    s = (x_interpolate - points[0].x) / h;
    interpolated_y = y_diffs[0]; 
    term_s = 1.0;

    printf("\nEnterpolasyon adımları (s = %.4f):\n", s);
    printf("P_0(x) = y0 = %.4f\n", interpolated_y);

    for(i = 1; i < num_points; i++)
    {
        term_s = term_s * (s - (i - 1)); // s, s(s-1), s(s-1)(s-2), ...
        double current_term_value = (term_s / factorial(i)) * y_diffs[i];
        interpolated_y += current_term_value;
        printf("Terim %d: (s",i);
        for(k=1; k<i; ++k) printf("(s-%d)", k);
        printf(")/(%d!) * Δ^%d y0 = (%.4f / %.0f) * %.4f = %.4f\n", i, i, term_s, factorial(i), y_diffs[i], current_term_value);
        printf("P_%d(x) = P_%d(x) + Terim %d = %.4f\n",i, i-1, i, interpolated_y);
    }
    return interpolated_y;
}

void displayGregoryNewtonInterpolation()
{
    int num_points, i, c_clear_buffer;
    double x_interpolate, interpolated_y_val;
    DataPoint *points = NULL;
    double *forward_differences = NULL; // To store y0, Δy0, Δ²y0, etc.

    printf("\n--- Değişken Dönüşümsüz Gregory Newton Enterpolasyonu ---\n");
    printf("Veri noktası sayısını girin (N): ");
    if(scanf("%d", &num_points) != 1 || num_points <= 0)
    {
        printf("Geçersiz nokta sayısı. Pozitif bir tamsayı olmalıdır.\n");
        while ((c_clear_buffer = getchar()) != '\n' && c_clear_buffer != EOF);
        return;
    }
    while ((c_clear_buffer = getchar()) != '\n' && c_clear_buffer != EOF);

    if(num_points == 1)
    {
         printf("Uyarı: Sadece bir nokta ile enterpolasyon genellikle anlamlı değildir, ancak yine de devam edilecek.\n");
    }

    points = (DataPoint*)malloc(num_points * sizeof(DataPoint));
    if (points == NULL)
    {
        printf("Hata: Veri noktaları için bellek ayrılamadı.\n");
        return;
    }

    printf("Veri noktalarını (x y) formatında girin:\n");
    for(i = 0; i < num_points; i++)
    {
        printf("Nokta %d (x y): ", i + 1);
        if(scanf("%lf %lf", &points[i].x, &points[i].y) != 2)
        {
            printf("Geçersiz girdi. Lütfen sayısal değerler girin.\n");
            while ((c_clear_buffer = getchar()) != '\n' && c_clear_buffer != EOF);
            free(points);
            return;
        }
        while((c_clear_buffer = getchar()) != '\n' && c_clear_buffer != EOF);
    }

    printf("Enterpolasyon yapılacak x değerini girin: ");
    if(scanf("%lf", &x_interpolate) != 1)
    {
        printf("Geçersiz x değeri.\n");
        while ((c_clear_buffer = getchar()) != '\n' && c_clear_buffer != EOF);
        free(points);
        return;
    }
    while((c_clear_buffer = getchar()) != '\n' && c_clear_buffer != EOF);

    interpolated_y_val = gregoryNewtonForwardInterpolation(points, num_points, x_interpolate, &forward_differences);

    if(!isnan(interpolated_y_val))
    {
        printf("\n--------------------------------------------------\n");
        printf("x = %.4f için enterpole edilmiş y değeri: %.7f\n", x_interpolate, interpolated_y_val);
        printf("--------------------------------------------------\n");
    }
    else
    {
        printf("\nEnterpolasyon hesaplanamadı.\n");
    }

    if(forward_differences != NULL)
    {
        free(forward_differences);
    }
    free(points);
}

int main()
{
    int i = 0, j = 0, k = 0;
    int countToken = 0; //ayristirilan token sayisi
    int outputLen = 0;  //postfix token sayisi 
    double x;
    double result;
    int choice;
    int simpson_choice;
    int contains_x; //flag for x
    int c_clear;
    int c_consume;
    int max_iterations = 100;
    double h; //ileri geri merkezi turev icin

    Token tokens[MAX]; //array for infix tokens
    Token output[MAX]; //array for postfix 
    
    for(k = 0; k < MAX; k++)
    {
        tokens[k].value = NULL;
        output[k].value = NULL;
    }

    char *str = malloc(256 * sizeof(char));       // Allocate memory for 256 characters
    if (str == NULL) // Check if memory allocation was successful
    {
        printf("Memory allocation failed!\n");
        return 1;
    }

    do
    {
        printf("\n SAYISAL ANALIZ PROJE MENU\n");
        printf("1. Bisection Method\n");
        printf("2. Regula-Falsi \n");
        printf("3. Newton-Raphson Method\n");
        printf("4. NxN'lik bir matrisin tersi\n");
        printf("5. Cholesky (ALU ) Yöntemi\n");
        printf("6. Gauss Seidal Yöntemi\n");
        printf("7. Sayisal Türev (merkezi, ileri ve geri farklar)\n");
        printf("8. Simpson Yöntemi(1/3 ve 3/8)\n");
        printf("9. Trapez Yöntemi\n");
        printf("10. Değişken dönüşümsüz Gregory Newton Enterpolasyonu\n");
        printf("0. Exit\n");
        printf("Enter your choice: ");

        if (countToken > 0)
        {
            freeTokens(tokens, countToken);
            countToken = 0;
        }
        if (outputLen > 0)
        {
            freeTokens(output, outputLen);
            outputLen = 0; 
        }

        if(scanf(" %d", &choice) != 1)
        {
            printf("Invalid choice. Please enter a number!\n");
            while((c_clear = getchar()) != '\n' && c_clear != EOF);
                choice = -99;
        }
        else
        {
            while((c_consume = getchar()) != '\n' && c_consume != EOF);
            
        }

        if(choice >= 1 && choice <= 3 || choice == 7 || choice == 8 || choice == 9)
        {
            printf("\nf(x) fonksiyonunu giriniz (örn: x^3 - x - 2): ");
            fgets(str, MAX, stdin);
            if(str == NULL)
            {
                printf("Function could not be read\n");
            }
            str[strcspn(str, "\n")] = 0;
            if (countToken > 0) freeTokens(tokens, countToken); //onceki tokenleri temizlemek icin
            if (outputLen > 0) freeTokens(output, outputLen);
            
            tokenize(str, tokens, &countToken);
            if(countToken>0)
            {
                shuntingYard(tokens, countToken, output, &outputLen);
            }
            else
            {
                printf("Fonksiyon ayristirilamadi.\n");
                outputLen = 0; // Postfix ifade yok
            }
        }
        if(choice == 7) // sayisal turev icin
        {
            printf("Enter h:\n");
            scanf("%lf", &h);
            while((getchar()) != '\n' && getchar() != EOF);
            printf("Turev alinacak x degerini giriniz:\n");
            scanf("%lf", &x);
            while((getchar()) != '\n' && getchar() != EOF);
            printf("%ld",x);
        }
       
        switch (choice) 
        {
            case 0:
                printf("Cikis yapiliyor.\n");
                break;

            case 1: // Bisection Method
                bisectionMethod(output, outputLen);
                break;

            case 2: // Regula-Falsi
                regulaFalsiMethod(output, outputLen);
                break;

            case 3: // Newton-Raphson
                newtonRaphsonMethod(output, outputLen);
                break;

            case 4: // NxN Matrix Inverse
                displayInverveMatrix();
                break;

            case 5: // Cholesky
                cholesky();
                break;

            case 6: // Gauss-Seidel
                gaussSeidelMethod();
                break;
                

            case 7: // Numerical Derivative
                printf("Merkezi Turev: %.6f\n",centralDerivativeFunctions(output, outputLen, x, h));
                printf("Ileri Turev: %.6f\n",forwardDerivativeFunctions(output, outputLen, x, h));
                printf("Geri Turev: %.6f\n",backwardDerivativeFunctions(output, outputLen, x, h));
                break;

            case 8: // Simpson's Method
                printf("\n--- Simpson Yöntemi Seçimi ---\n");
                printf("1. Simpson 1/3 Kurali\n");
                printf("2. Simpson 3/8 Kurali\n");
                printf("Seçiminizi girin: ");
                scanf("%d", &simpson_choice);
                while((getchar()) != '\n' && getchar() != EOF);
                if(simpson_choice == 1)
                {
                    simpsonOneThirdMethod(output, outputLen);
                }
                else if(simpson_choice == 2)
                {
                    simpsonThreeEighthMethod(output, outputLen);
                }
                else
                {
                    printf("Yanlis Simpson yontemi secimi.\n");
                }
                break;

            case 9: // Trapezoidal Method
                trapezoidalMethod(output, outputLen);
                break;

            case 10: // Gregory-Newton Interpolation
                displayGregoryNewtonInterpolation();
                break;

            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 0);

    if(countToken > 0) 
        {
            freeTokens(tokens, countToken);   
        }
        if(outputLen > 0)
        {
            freeTokens(output, outputLen);
        }

    free(str); // Free the allocated memory
    printf("Program bitti.\n");
    return 0;
}