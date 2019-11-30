#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct myStruct {
    int x;
    int y;
};

int main() {
    
    struct myStruct** myStructArr__1 = (struct myStruct **) calloc(2, sizeof(struct myStruct));
    
    struct myStruct* myStruct_1 = (struct myStruct *) calloc(1, sizeof(struct myStruct));
    myStruct_1->x = 1;
    myStruct_1->y = 2;

    struct myStruct* myStruct_2 = (struct myStruct *) calloc(1, sizeof(struct myStruct));
    myStruct_2->x = 2;
    myStruct_2->y = 4;

    myStructArr__1[0] = myStruct_1;
    myStructArr__1[1] = myStruct_2;

    for (int i = 0; i < 2; i++) {
        printf("%d: %d %d\n", i, myStructArr__1[i]->x, myStructArr__1[i]->y);
    }
    printf("\n");


    struct myStruct** myStructArr__2 = (struct myStruct **) calloc(3, sizeof(struct myStruct));
    
    struct myStruct* myStruct_3 = (struct myStruct *) calloc(1, sizeof(struct myStruct));
    myStruct_3->x = 3;
    myStruct_3->y = 6;

    struct myStruct* myStruct_4 = (struct myStruct *) calloc(1, sizeof(struct myStruct));
    myStruct_4->x = 4;
    myStruct_4->y = 8;

    struct myStruct* myStruct_5 = (struct myStruct *) calloc(1, sizeof(struct myStruct));
    myStruct_5->x = 5;
    myStruct_5->y = 10;

    myStructArr__2[0] = myStruct_3;
    myStructArr__2[1] = myStruct_4;
    myStructArr__2[2] = myStruct_5;

    for (int i = 0; i < 3; i++) {
        printf("%d: %d %d\n", i, myStructArr__2[i]->x, myStructArr__2[i]->y);
    }
    printf("\n");



    struct myStruct** myStructArr__3 = (struct myStruct **) calloc(5, sizeof(struct myStruct));
    memcpy(myStructArr__3, myStructArr__1, sizeof(struct myStruct) * 2);

    for (int i = 0; i < 2; i++) {
        printf("%d: %d %d\n", i, myStructArr__3[i]->x, myStructArr__3[i]->y);
    }
    printf("\n");



    memcpy(&myStructArr__3[2], myStructArr__2, sizeof(struct myStruct) * 3);

    for (int i = 0; i < 5; i++) {
        printf("%d: %d %d\n", i, myStructArr__3[i]->x, myStructArr__3[i]->y);
    }
    printf("\n");

    // clean up and exit
    free(myStructArr__3);
    free(myStruct_5);
    free(myStruct_4);
    free(myStruct_3);
    free(myStructArr__2);
    free(myStruct_2);
    free(myStruct_1);
    free(myStructArr__1);
    return 0;
}