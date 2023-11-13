#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

static const int MAX_INPUT_LENGTH = 100; //the length of the analyzed string
static const int MAX_RULE_LENGTH = 30;

typedef struct GrammarRule{
    char leftSide[3]; //non-terminal on the left side: A + possible ' + \0
    char rightSide[15][15]; //15 productions of the non-terminal on the left side
    char firstSet[15]; //first_1 of the non-terminal on the left side
    char followSet[15]; //follow_1 of the non-terminal on the left side
    int productionsCount; //number of rules with the non-terminal on the left side
    int firstSetCount; //number of symbols in first_1 set
    int followSetCount; //number of symbols in follow_1 set
    bool firstSetCalculated; //if = 0, then first hasn't been calculated for the non-terminal on the left side
    bool followSetCalculated; //if = 0, then follow hasn't been calculated for the non-terminal on the left side

    struct GrammarRule *next;
} GrammarRule;

typedef struct LL1Table {
    char nonTerminal[3];
    char terminal;
    char appliedRule[15]; //rule to be applied when nonTerminal meets terminal

    struct LL1Table *next;
} LL1Table;

typedef struct Stack{
    char elementValue[3];
    struct Stack *next;
} Stack;

char *createSubstring(char *initialString, int begin, int length){
    char *resultString;
    resultString = malloc((length) * sizeof(char));
    int resStringChar = 0;

    for (int i = begin; i < (begin + length); i++) {
        resultString[resStringChar] = initialString[i];
        resStringChar++;
    }

    resultString[length] = '\0';
    return resultString;
}

void createGrammar(struct GrammarRule **gRule) {
    struct GrammarRule *newRule;
    char *partOfProduction;
    int partsOfProductionsCount = 0;
    char *allRules = malloc(MAX_RULE_LENGTH * sizeof(char));

    while (1) {
        scanf("%s", allRules);

        if (!strcmp(allRules, "@"))
            return;

        newRule = malloc(sizeof(GrammarRule));
        newRule->leftSide[0] = allRules[0];
        newRule->leftSide[1] = '\0';

        //handling the case with multiple rules for a non-terminal in one row
        char *rSide = createSubstring(allRules, 3, strlen(allRules) - 3);
        partOfProduction = strtok(rSide, "|");
        while (partOfProduction != NULL) {
            strcpy(newRule->rightSide[partsOfProductionsCount], partOfProduction);
            int len = strlen(newRule->rightSide[partsOfProductionsCount]);
            newRule->rightSide[partsOfProductionsCount][len] = '\0';
            partOfProduction = strtok(NULL, "|");
            partsOfProductionsCount++;
        }

        if (*gRule == NULL) { //if we work with the first rule of the grammar
            newRule->firstSetCalculated = false;
            newRule->followSetCalculated = false;
            newRule->next = NULL;
            *gRule = newRule;
        }
        else { //if the rule is not the first, we insert it at the end of the grammar
            GrammarRule *prevRule = *gRule;
            while (prevRule->next != NULL)
                prevRule = prevRule->next;
            prevRule->next = newRule;

            newRule->firstSetCalculated = false;
            newRule->followSetCalculated = false;
            newRule->next = NULL;
        }

        newRule->productionsCount = partsOfProductionsCount;
        partsOfProductionsCount = 0;
    }
}

void displayGrammar(GrammarRule *gRule) {
    while (gRule != NULL) {
        printf("%s->", gRule->leftSide);
        for (int i = 0; i < gRule->productionsCount; i++) {
            printf("%s", gRule->rightSide[i]);
            if (i < gRule->productionsCount - 1)
                printf("|");
        }
        gRule = gRule->next;
        printf("\n");
    }
}

void createEmptyRule(struct GrammarRule **gRule, struct GrammarRule *tempRule){
    if (*gRule == NULL) {
        tempRule->firstSet[0] = '\0';
        tempRule->followSet[0] = '\0';
        tempRule->firstSetCount = 0;
        tempRule->followSetCount = 0;
        tempRule->firstSetCalculated = false;
        tempRule->followSetCalculated = false;
        tempRule->next = NULL;
        *gRule = tempRule;
    }
    else { //if rule is not the first
        struct GrammarRule *prevRule = (*gRule);
        while(prevRule->next != NULL){
            prevRule = prevRule->next;
        }
        prevRule->next = tempRule;
        tempRule->firstSet[0] = '\0';
        tempRule->followSet[0] = '\0';
        tempRule->firstSetCount = 0;
        tempRule->followSetCount = 0;
        tempRule->firstSetCalculated = false;
        tempRule->followSetCalculated = false;
        tempRule->next = NULL;
    }
}

GrammarRule *eliminateLeftRecursion(GrammarRule *gRule) {
    GrammarRule *gRuleNoRecursion = NULL;
    GrammarRule *currentRule = gRule;

    while (currentRule != NULL) {
        bool isRecursive = false;
        for (int i = 0; i < currentRule->productionsCount; ++i)
            if (currentRule->leftSide[0] == currentRule->rightSide[i][0])
                isRecursive = true;

        if (isRecursive) {
            //we break a left-recursive rule S->Sa|b into 1)S->bS' and 2)S'->aS'|&
            GrammarRule *newRule1 = malloc(sizeof(GrammarRule));
            strcpy(newRule1->leftSide, currentRule->leftSide);

            GrammarRule *newRule2 = malloc(sizeof(GrammarRule));
            strcpy(newRule2->leftSide, currentRule->leftSide);
            strcat(newRule2->leftSide, "'"); //creates an S'->... rule

            int rule1ProdCount = 0, rule2ProdCount = 0;
            for (int i = 0; i < currentRule->productionsCount; i++) {
                if (currentRule->rightSide[i][0] == currentRule->leftSide[0]) { //S->Sa ===> S'->aS'
                    char* afterRecursive = createSubstring(currentRule->rightSide[i], 1, strlen(currentRule->rightSide[i])); //cut the string after S
                    strcpy(newRule2->rightSide[rule2ProdCount], afterRecursive); //S'->a
                    strcat(newRule2->rightSide[rule2ProdCount], newRule2->leftSide); //S'->aS'
                    rule2ProdCount++;
                }
                else { //S->b ===> S->bS'
                    strcpy(newRule1->rightSide[rule1ProdCount], currentRule->rightSide[i]); //S->b
                    strcat(newRule1->rightSide[rule1ProdCount], newRule2->leftSide); //S->bS'
                    rule1ProdCount++;
                }
            }

            if (rule1ProdCount == 0) { //if there is no S->b rule ===> S->S'
                strcpy(newRule1->rightSide[0], newRule2->leftSide);
                rule1ProdCount++;
            }

            newRule1->productionsCount = rule1ProdCount;
            newRule2->productionsCount = rule2ProdCount + 1;
            strcpy(newRule2->rightSide[rule2ProdCount], "&"); //S'->aS'|&

            createEmptyRule(&gRuleNoRecursion, newRule1);
            createEmptyRule(&gRuleNoRecursion, newRule2);
        }
        else { //if not recursive
            GrammarRule *newRule = malloc(sizeof(GrammarRule));
            *newRule = *currentRule;
            createEmptyRule(&gRuleNoRecursion, newRule);
        }
        currentRule = currentRule->next;
    }
    return gRuleNoRecursion;
}

GrammarRule *findRuleByNonTerminal(GrammarRule *gRule, char nonTerminal){
    while (gRule != NULL) {
        if (gRule->leftSide[0] == nonTerminal && gRule->leftSide[1] != '\'')
            return gRule;
        gRule = gRule->next;
    }
    printf("There are no %c->... rules in the grammar.", nonTerminal);
    exit(0);
}

bool containsEpsilon(char set[]){
    for (int i = 0; i < strlen(set); i++)
        if (set[i] == '&')
            return true;
    return false;
}

char *eliminateEpsilon(char set[]){
    int newSetIndex = 0;
    for (int i = 0; i < strlen(set); i++) {
        if (set[i] != '&') {
            set[newSetIndex] = set[i];
            newSetIndex++;
        }
    }
    set[newSetIndex] = '\0';
    return set;
}

char *eliminateDuplicates(char initialString[]) {
    int initialStringLength = strlen(initialString);
    char *result;
    result = malloc(initialStringLength);
    int resultCount = 0;

    for (int i = 0; i < initialStringLength; i++) {
        bool inTheString = false;
        for (int j = i + 1; j < initialStringLength; j++)
            if (initialString[i] == initialString[j])
                inTheString = true;

        if (!inTheString) {
            result[resultCount] = initialString[i];
            resultCount++;
        }
    }

    result[resultCount] = '\0';
    return result;
}

GrammarRule* calculateFirstSet(GrammarRule *rule, GrammarRule *gRules) {
    if (!rule->firstSetCalculated) {
        int firstSetIndex = 0;
        for (int i = 0; i < rule->productionsCount; i++) {
            int characterIndex = 0;
            if (isupper(rule->rightSide[i][characterIndex])) { //if the character is non-terminal
                GrammarRule *nonTerminalRule = findRuleByNonTerminal(gRules, rule->rightSide[i][characterIndex]); //we find its productions
                GrammarRule *nonTerminalRule1 = calculateFirstSet(nonTerminalRule, gRules); //and first_1
                strcat(rule->firstSet, nonTerminalRule1->firstSet); //then we add its first_1 set to the first_1 set of the initial non-terminal

                while(containsEpsilon(nonTerminalRule1->firstSet) && isupper(rule->rightSide[i][characterIndex + 1])) { //if next character is non-terminal
                    characterIndex++;
                    GrammarRule *nextNonTerminalRule = findRuleByNonTerminal(gRules, rule->rightSide[i][characterIndex]); //we find its productions
                    GrammarRule *nextNonTerminalRule1 = calculateFirstSet(nextNonTerminalRule, gRules); //and first_1
                    strcat(rule->firstSet,nextNonTerminalRule1->firstSet); //then we add its first_1 set to the first_1 set of the initial non-terminal
                }

                strcpy(rule->firstSet, eliminateEpsilon(rule->firstSet)); //remove epsilon
                strcpy(rule->firstSet, eliminateDuplicates(rule->firstSet)); //and remove duplicates
            }
            else {
                if (strchr(rule->firstSet,rule->rightSide[i][0]) == NULL) { //if first set does not contain the first symbol from the right side
                    rule->firstSet[firstSetIndex] = rule->rightSide[i][0];
                    firstSetIndex++;
                    rule->firstSet[firstSetIndex] = '\0';
                }
            }
        }
        rule->firstSetCount = strlen(rule->firstSet);
        rule->firstSetCalculated = true;
        return rule;
    }
    return rule;
}

void displayFirstSet(GrammarRule* gRule){
    while (gRule != NULL) {
        printf("First_1(%s) = {", gRule->leftSide);
        for (int i = 0; i < gRule->firstSetCount; i++) {
            if (i == gRule->firstSetCount - 1)
                printf("%c", gRule->firstSet[i]);
            else
                printf("%c,", gRule->firstSet[i]);
        }
        printf("}\n");

        gRule = gRule->next;
    }

}

int findNonTerminalPositionInProduction(char production[], char nonTerminalName[]) {
    if (strlen(nonTerminalName) == 1) { //if non-terminal is S
        for (int i = 0; i < strlen(production); i++)
            if (nonTerminalName[0] == production[i] && production[i + 1] != '\'') //if we find non-terminal in production
                return i + 1;
    }
    else { //if non-terminal is S'
        for (int i = 0; i < strlen(production) - 1; i++)
            if (nonTerminalName[0] == production[i] && nonTerminalName[1] == production[i + 1])
                return i + 2;
    }
    return 0;
}

GrammarRule *findRuleByLeftSideName(GrammarRule *gRule, char *name){
    while (gRule != NULL) {
        if (strcmp(name, gRule->leftSide) == 0)
            return gRule;
        gRule = gRule->next;
    }
    return NULL;
}

GrammarRule *calculateFollowSet(GrammarRule *targetRule, GrammarRule *gRule, GrammarRule *allRules) {
    if (!targetRule->followSetCalculated){
        while (gRule != NULL) {
            for (int i = 0; i < gRule->productionsCount; i++) {
                int position = findNonTerminalPositionInProduction(gRule->rightSide[i], targetRule->leftSide);
                if (position == 0) //there is no target non-terminal in gRule
                    continue;

                if (gRule->rightSide[i][position] == '\0') { //if non-terminal is the last in the production
                    if (strcmp(targetRule->leftSide, gRule->leftSide) != 0) { //if target non-terminal and left side of gRule are different
                        if (gRule->rightSide[i][position - 1] != '\'' && !isupper(gRule->rightSide[i][position - 1])) {
                            //if there is no non-terminal before the target non-terminal in gRule, and it is not ' before the target non-terminal in gRule
                            targetRule->followSet[targetRule->followSetCount] = gRule->rightSide[i][position - 1]; //add terminal to follow_1 set
                            targetRule->followSetCount++;
                        }
                        else {
                            //if there is a non-terminal before the target non-terminal in gRule, calculate its follow set
                            GrammarRule *tempRule = calculateFollowSet(gRule, allRules, allRules);
                            if (targetRule->followSet[0] == '\0') //if follow_1 set of the target non-terminal is empty
                                strcpy(targetRule->followSet, tempRule->followSet);
                            else
                                strcat(targetRule->followSet, tempRule->followSet);
                            targetRule->followSetCount = targetRule->followSetCount + strlen(tempRule->followSet);
                        }
                    }
                }

                else { //if non-terminal is not the last in the production
                    while (position < strlen(gRule->rightSide[i])) {
                        if (isupper(gRule->rightSide[i][position])) {
                            GrammarRule *tempRule;
                            if (gRule->rightSide[i][position + 1] == '\'') { //if S'
                                char nonTerminalName[3];
                                nonTerminalName[0] = gRule->rightSide[i][position];
                                nonTerminalName[1] = '\'';
                                //we find the corresponding grammar rule for the target non-terminal
                                tempRule = findRuleByLeftSideName(allRules, nonTerminalName);
                                position = position + 2;
                            }
                            else { //if S
                                tempRule = findRuleByNonTerminal(allRules, gRule->rightSide[i][position]);
                                position++;
                            }
                            if (containsEpsilon(tempRule->firstSet)) {
                                //if the first set of the non-terminal contains epsilon, add it to the target rule's follow set
                                strcat(targetRule->followSet, tempRule->firstSet);
                                strcpy(targetRule->followSet, eliminateEpsilon(targetRule->followSet));
                                targetRule->followSetCount = targetRule->followSetCount + tempRule->firstSetCount - 1;
                                if (gRule->rightSide[i][position] == '\0' && strcmp(tempRule->leftSide, gRule->leftSide) != 0) {
                                    //if the non-terminal is at the end of the production, add the follow set of the current grammar rule
                                    strcat(targetRule->followSet, gRule->followSet);
                                    targetRule->followSetCount = targetRule->followSetCount + strlen(gRule->followSet);
                                }
                            }
                            else {
                                strcat(targetRule->followSet, tempRule->firstSet);
                                targetRule->followSetCount = targetRule->followSetCount + tempRule->firstSetCount;
                                break;
                            }
                        }
                        else {
                            //if the character following the target non-terminal is not an upper case letter, add it to the follow set
                            targetRule->followSet[targetRule->followSetCount] = gRule->rightSide[i][position];
                            targetRule->followSetCount++;
                            targetRule->followSet[targetRule->followSetCount] = '\0';
                            break;
                        }
                    }
                }
            }
            gRule = gRule->next;
        }

        strcpy(targetRule->followSet, eliminateDuplicates(targetRule->followSet));
        targetRule->followSetCount = strlen(targetRule->followSet);
        targetRule->followSetCalculated = true;
        return targetRule;
    }
    return targetRule;
}

void displayFollowSet(GrammarRule *gRule){
    while (gRule != NULL) {
        printf("Follow_1(%s) = {",gRule->leftSide);
        for (int i = 0; i < gRule->followSetCount; ++i) {
            if (i == gRule->followSetCount - 1)
                printf("%c", gRule->followSet[i]);
            else
                printf("%c,", gRule->followSet[i]);
        }
        printf("}\n");
        gRule = gRule->next;
    }
}

int findProductionIndexContainingTerminal(GrammarRule *gRule, GrammarRule *allRules, char targetTerminal) {
    for (int productionIndex = 0; productionIndex < gRule->productionsCount; productionIndex++) {
        int currentPosition = 0;
        while (currentPosition < strlen(gRule->rightSide[productionIndex])) {
            if (gRule->rightSide[productionIndex][currentPosition] == targetTerminal)
                return productionIndex;

            if (isupper(gRule->rightSide[productionIndex][currentPosition])) {
                //if current character is non-terminal
                int a = -1;
                if (gRule->rightSide[productionIndex][currentPosition + 1] != '\'') { //if S
                    a = findProductionIndexContainingTerminal(findRuleByNonTerminal(allRules, gRule->rightSide[productionIndex][currentPosition]), allRules, targetTerminal);
                    currentPosition++; //move further
                }
                else { //if S'
                    char nonTerminalName[3];
                    nonTerminalName[0] = gRule->rightSide[productionIndex][currentPosition];
                    nonTerminalName[1] = '\'';
                    a = findProductionIndexContainingTerminal(findRuleByLeftSideName(allRules, nonTerminalName), allRules, targetTerminal);
                    currentPosition = currentPosition + 2; //move further over '
                }
                if (a == 2)
                    continue;
                else
                    return productionIndex;
            }
            else if (gRule->rightSide[productionIndex][currentPosition] == '&')
                return 2;

            break;
        }
    }
    return 0;
}

void insertRecordIntoLL1Table(LL1Table **targetTable, LL1Table *tableRecord){
    tableRecord->next = NULL;
    if (*targetTable == NULL) //if target is empty
        *targetTable = tableRecord;
    else {//if not, insert at the end
        LL1Table *tempTable = (*targetTable);
        while (tempTable->next != NULL)
            tempTable = tempTable->next;
        tempTable->next = tableRecord;
    }
}

LL1Table *createLL1Table(GrammarRule *gRule, GrammarRule *allRules){
    LL1Table *analyzerTable = NULL;
    while (gRule != NULL) {
        for (int i = 0; i < gRule->firstSetCount; i++) {
            if (gRule->firstSet[i] == '&') {
                for (int j = 0; j < gRule->followSetCount; j++) {
                    LL1Table *tempTable = malloc(sizeof(LL1Table));
                    strcpy(tempTable->nonTerminal, gRule->leftSide);
                    tempTable->terminal = gRule->followSet[j];
                    tempTable->appliedRule[0] = '&';

                    insertRecordIntoLL1Table(&analyzerTable, tempTable);
                }
            }
            else {
                int productionIndex = findProductionIndexContainingTerminal(gRule, allRules, gRule->firstSet[i]);
                LL1Table *tempTable = malloc(sizeof(LL1Table));
                strcpy(tempTable->nonTerminal, gRule->leftSide);
                tempTable->terminal = gRule->firstSet[i];
                strcpy(tempTable->appliedRule, gRule->rightSide[productionIndex]);

                insertRecordIntoLL1Table(&analyzerTable, tempTable);
            }
        }
        gRule = gRule->next;
    }
    return analyzerTable;
}

void displayLL1Table(LL1Table *l) {
    while (l!= NULL) {
        printf("<%s,%c> = %s->%s \n", l->nonTerminal, l->terminal, l->nonTerminal, l->appliedRule);
        l = l->next;
    }
}

void insertElementOnTop(struct Stack **s, char *element) {
    Stack *tempElem = malloc(sizeof(Stack));
    strcpy(tempElem->elementValue, element);
    if (*s == NULL) { //if stack is empty
        tempElem->next = NULL;
        *s = tempElem;
    }
    else {
        tempElem->next = *s;
        (*s) = tempElem;
    }
}

char* getTopElement(Stack *s) {
    if (s != NULL)
        return s->elementValue;
    return NULL;
}

void removeTopElement(Stack **s) {
    Stack *tempElem = *s;
    (*s) = (*s)->next;
    free(tempElem);
}

char *findTerminalInLL1Table(LL1Table *analyzerTable, char *topElement, char targetTerminal) {
    while (analyzerTable != NULL) {
        if (analyzerTable->terminal == targetTerminal && strcmp(analyzerTable->nonTerminal, topElement) == 0)
            return analyzerTable->appliedRule;
        analyzerTable = analyzerTable->next;
    }
    return "0";
}

void displayStack(Stack *stack) {
    if (stack == NULL)
        return;
    printf("\n%s\n_", stack->elementValue);
    displayStack(stack->next);
}

int checkIfValid(LL1Table *analyzerTable, char *stringToAnalyze, GrammarRule *gRule) {
    Stack *stack = NULL;
    strcat(stringToAnalyze, "$"); //$ indicates the end of the input
    insertElementOnTop(&stack, "$");
    insertElementOnTop(&stack, gRule->leftSide); //insert core element
    int inputIndex = 0;

    while (inputIndex < strlen(stringToAnalyze)) {
        char *stackTop = getTopElement(stack);
        if (stackTop[0] == stringToAnalyze[inputIndex]) { //if symbol on top of the stack = current symbol of the string => delete both
            inputIndex++;
            if (stackTop[0] == '$') //stack is empty but we have not reached the end of the string
                return -1;
            removeTopElement(&stack);
        }
        else {
            char *appliedRule = findTerminalInLL1Table(analyzerTable, getTopElement(stack), stringToAnalyze[inputIndex]);
            if (strcmp(appliedRule, "0") == 0) //"0" if we didn`t find the character in the list of terminals
                return inputIndex;
            if (strcmp(appliedRule, "&") == 0) //turn the top elem to epsilon
                removeTopElement(&stack);
            else if (strlen(appliedRule) > 1) {
                removeTopElement(&stack);
                int ruleLength = strlen(appliedRule) - 1;
                while (ruleLength >= 0) { //inserting production elements one by one from the last to the top of the stack
                    if (appliedRule[ruleLength] == '\'') { //if S' is at the end of the applied rule
                        char *nonTerminal = malloc(3);
                        nonTerminal[0] = appliedRule[ruleLength - 1];
                        nonTerminal[1] = appliedRule[ruleLength];
                        nonTerminal[2] = '\0';
                        ruleLength = ruleLength - 2;
                        insertElementOnTop(&stack, nonTerminal);
                    }
                    else {
                        char *terminal = malloc(2);
                        terminal[0] = appliedRule[ruleLength];
                        terminal[1] = '\0';
                        ruleLength--;
                        insertElementOnTop(&stack, terminal);
                    }
                }
            }
            else {
                removeTopElement(&stack);
                insertElementOnTop(&stack, appliedRule);
            }
        }
    }
    return strlen(stringToAnalyze);
}

int main(){
    GrammarRule *grammar = NULL;
    LL1Table *analyzerTable = NULL;

    printf("To start the program, enter grammar rules row by row in the following format:\n");
    printf("A->nN|M\n");
    printf("B->kKKk|&  -  & <=> epsilon\n");
    printf("When you finish the input, enter @ to create a grammar and begin analyzing.\n");
    printf("----------------------------------------------------------------------------\n");

    printf("Enter your grammar rules:\n");
    createGrammar(&grammar);
    printf("The grammar has been created.\n");
    printf("-----------------------------------------\n");

    GrammarRule *newGrammar = NULL;
    newGrammar = eliminateLeftRecursion(grammar);
    printf("The same grammar without left recursions:\n");
    displayGrammar(newGrammar);
    printf("-----------------------------------------\n");

    newGrammar->followSet[0] ='$';
    newGrammar->followSet[1] = '\0';
    newGrammar->followSetCount = 1;

    printf("First_1 of the non-terminals of the grammar:\n");
    GrammarRule *gRule_first = newGrammar;
    while(gRule_first != NULL){
        gRule_first = calculateFirstSet(gRule_first,newGrammar);
        gRule_first = gRule_first->next;
    }
    displayFirstSet(newGrammar);
    printf("-----------------------------------------\n");

    printf("Follow_1 of the non-terminals of the grammar:\n");
    GrammarRule *gRule_follow = newGrammar;
    while(gRule_follow != NULL){
        gRule_follow = calculateFollowSet(gRule_follow,newGrammar,newGrammar);
        gRule_follow = gRule_follow->next;
    }
    displayFollowSet(newGrammar);
    printf("-----------------------------------------\n");

    analyzerTable = createLL1Table(newGrammar, newGrammar);
    printf("LL(1) Table: <non-terminal, terminal> = rule to be applied\n");
    displayLL1Table(analyzerTable);
    printf("-----------------------------------------\n");

    char stringToAnalyze[MAX_INPUT_LENGTH];
    while (1) {
        printf("Enter a string to analyze: \n");
        scanf("%s",stringToAnalyze);
        if (strcmp(stringToAnalyze,"@") == 0)
            break;

        int isCorrect = checkIfValid(analyzerTable, stringToAnalyze, newGrammar);
        if (isCorrect == -1)
            printf("%s accepted \n", stringToAnalyze);
        else
            printf("%s rejected by syntax error at symbol: %d \n", stringToAnalyze, isCorrect + 1);
    }
    return 0;
}
