#import <stdio.h>
#import "world/map.hpp"
#import "game.hpp"

bool isRedPlayer(int id) {
    auto& members = GAME.state.red.members;
    return members.find(id) != members.end();
}
bool isBluPlayer(int id) {
    auto& members = GAME.state.red.members;
    return members.find(id) != members.end();
}

template<typename T>
static void setOrAdd(T& mapping, int key, int val) {
    if (mapping.find(key) == mapping.end())
        mapping[key] = val;
    else
        mapping[key] += val;
}

void playerDamaged(int assaulter, int victim, int amount) {
    if (GAME.isClient)
        return;
    
    printf("Player %d damaged player %d! It was %d effective.\n", assaulter, victim, amount);
    auto& players = GAME.world.players;
    if (assaulter < 1)
        return;
    
    if (players.find(victim) == players.end())
        return; // ???
    
    if (players.find(assaulter) == players.end())
        assaulter = (int)CauseOfDeath::PastPlayer;
    
    
    Player& victimPlayer = players[victim];
    if (victimPlayer.health < amount)
        victimPlayer.health = 0; // Bye bye...
    else
        victimPlayer.health -= amount;
    
    
    if (victimPlayer.health == 0) {
        playerDied(assaulter, victimPlayer);
    }
}

void playerDied(int killer, Player& dead) {
    if (GAME.isClient)
        return;
    
    printf("Player %d has fainted :(\n", dead.identifier);
    
    // Move them back to the spawn
    dead.position.x = 0;
    dead.position.y = 0;
    
    // Replenish health
    dead.health = Player::MaxHealth;
    
    // Add another point to each side
    setOrAdd(GAME.state.kills, killer, 1);
    setOrAdd(GAME.state.deaths, dead.identifier, 1);
    
    if (isRedPlayer(dead.identifier))
        GAME.state.red.tickets -= 1;
    else if (isBluPlayer(dead.identifier))
        GAME.state.blu.tickets -= 1;
    
    if (GAME.state.red.tickets <= 0 || GAME.state.blu.tickets <= 0) {
        gameOver();
    }
}

void gameOver() {
    printf("GAME OVER!\n");
}