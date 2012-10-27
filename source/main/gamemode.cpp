#import <stdio.h>

void playerDamaged(int assaulter, int victim, int amount) {
    printf("Player %d damaged player %d! It was %d effective.\n", assaulter, victim, amount);
}
void playerDied(int killer, int dead) {
    printf("Player %d has fainted :(\n", dead);
}
