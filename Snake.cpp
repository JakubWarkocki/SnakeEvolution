#include <iostream>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <cmath>
#include<GL/gl.h>
#include<GLFW/glfw3.h>
#include<random>
#include <string>
#include <sstream>
#include "lnetwork.h"
#define boardSize 150
//130
using namespace std;





//const int boardSize=100;
const int foodColor=0xFFFFFF;
const int emptyColor=0x000000;
const int barrierColor=0x888888;
const int boosterColor=0x50F0FF;
const int baseFoodParticles=100; //200
const int baseBoosters=0; //20
const int hungerTimer=15;
const int baseSnakes=160; //160
const int firstTreshold=32;
const int secondTreshold=64;
const int foodReward=20;
const int boosterReward=20;
const int wallReward=-200;
const int selfCollisionReward=-200;
const int otherCollisionReward=-200;
const int huntingReward=1600;
const int headCollisionReward=-200;
const int starvationReward=-200;
const int survivalReward=1;
const int startNutrientLevel=64;
const int maxNutrientLevel=256;
int boardSurface=boardSize*boardSize;
int matrix[boardSize][boardSize];
int linear[boardSize*boardSize+1];
int positionId(int x, int y){
    return x+(boardSize*y)+1;
}

int xId(int id){
    return (id-1)%boardSize;
}

int yId(int id){
    return (id-1)/boardSize;
}

int movement(int id, char direction){
    switch(direction) {
    case 'U':
        return id+boardSize;
        break;
    case 'R':
        return id+1;
        break;
    case 'D':
        return id-boardSize;
        break;
    case 'L':
        return id-1;
        break;
}
}
char clockwise(char direction){
    switch(direction) {
    case 'U':
        return 'R';
        break;
    case 'R':
        return 'D';
        break;
    case 'D':
        return 'L';
        break;
    case 'L':
        return 'U';
        break;
}
}
vector<int> foodParticles;
vector<int> boosters;


class Snake{
    public:
        int headPosition;
        vector<int> segments;
        double envData[9];
        int snakeID;
        int color;
        char direction;
        bool collision;
        int hungerCountdown;
        int boosterCountdown;
        int nutrientLevel;
        lnetwork controlAI;
        Snake(int startPosition, char startDirection, int colr);
        double flag(int tile, int distance);
        void scanEnv();
        void executeMovement();
        void turnLeft();
        void turnRight();
        void checkCollision( Snake& snake1,  Snake& snake2);
        void aiControl();
        void tickHungerCountdown();

};

Snake::Snake(int startPosition, char startDirection, int colr) {
    headPosition = startPosition;
    direction = startDirection;
    snakeID = colr;
    color = colr;
    segments.push_back(startPosition);
    collision=false;
    boosterCountdown=0;
    hungerCountdown=hungerTimer;
    nutrientLevel=startNutrientLevel;

}
void Snake::executeMovement(){
    nutrientLevel--;
    if(boosterCountdown){
        color=foodColor-rand()%254-1;
        nutrientLevel--;
        controlAI.reward(3*survivalReward);
    }
    else{
        color=snakeID;
    }
    controlAI.reward(survivalReward);
    int tailMemory = segments[segments.size()-1];
    for(int i=segments.size()-1; i>0; i--){
        segments[i]=segments[i-1];
    }
    headPosition=movement(headPosition, direction);
    segments[0]=headPosition;
    for(int i=0; i<foodParticles.size(); i++){
        if(headPosition==foodParticles[i]){ // UWAGA
            controlAI.reward(foodReward);
            nutrientLevel+=64;
            nutrientLevel=min(nutrientLevel,256);
            segments.push_back(tailMemory);
            foodParticles.erase(foodParticles.begin() + i);
            break;
        }
    }

    for(int i=0; i<boosters.size(); i++){
        if(headPosition==boosters[i]){ // UWAGA
            controlAI.reward(boosterReward);
            nutrientLevel+=128;
            nutrientLevel=min(nutrientLevel,256);
            boosterCountdown=12;
            boosters.erase(boosters.begin() + i);
            break;
        }
    }
    if(linear[headPosition]==barrierColor){
        controlAI.reward(wallReward);
        collision=true;
    }
    if(nutrientLevel<=0){
        controlAI.reward(starvationReward);
        collision=true;
    }
}

void Snake::turnLeft(){
    switch(direction) {
    case 'U':
        direction='L';
        break;
    case 'R':
        direction='U';
        break;
    case 'D':
        direction='R';
        break;
    case 'L':
        direction='D';
        break;
    }
}

void Snake::turnRight(){
    switch(direction) {
    case 'U':
        direction='R';
        break;
    case 'R':
        direction='D';
        break;
    case 'D':
        direction='L';
        break;
    case 'L':
        direction='U';
        break;
    }
}

void Snake::checkCollision( Snake& snake1, Snake& snake2){
    for(int i=1; i<snake1.segments.size(); i++){
        if(snake1.headPosition==snake1.segments[i]){
            snake1.collision=true;
            snake1.controlAI.reward(selfCollisionReward);
        }
        if(snake2.headPosition==snake1.segments[i]){
            snake2.collision=true;
            snake1.controlAI.reward(huntingReward);
            snake2.controlAI.reward(otherCollisionReward);
        }
    }
    for(int i=1; i<snake2.segments.size(); i++){
        if(snake2.headPosition==snake2.segments[i]){
            snake2.collision=true;
            snake2.controlAI.reward(selfCollisionReward);
        }
        if(snake1.headPosition==snake2.segments[i]){
            snake1.collision=true;
            snake2.controlAI.reward(huntingReward);
            snake1.controlAI.reward(otherCollisionReward);
        }
    }

    if(snake1.headPosition==snake2.headPosition&&snake1.color!=snake2.color){
            snake1.collision=true;
            snake2.collision=true;
            snake1.controlAI.reward(headCollisionReward);
            snake2.controlAI.reward(headCollisionReward);
    }
}

double Snake::flag(int tile, int distance){
    switch(tile){
     case barrierColor:
        return -1.0/distance;
     case foodColor:
        return 1.0/distance;
     default:
        return -1.0/distance;
    }
}

void Snake::scanEnv(){
    int pos, n, dist;
    char dir;
    dir=direction;
    n=0;
    for(int i=0; i<4; i++){
        if(i!=2){
            dist=1;
            pos=movement(headPosition,dir);
            while(linear[pos]==0){
                pos=movement(pos, dir);
                dist++;
            }
            envData[n]=flag(linear[pos],dist);
            n++;
        }
//diagonal
        dist =1;
        pos=movement(headPosition,dir);
        pos=movement(headPosition,clockwise(dir));
        while(linear[pos]==0){
            pos=movement(pos, dir);
            pos=movement(pos,clockwise(dir));
            dist++;
        }
        envData[n]=flag(linear[pos],dist);
        n++;
        dir=clockwise(dir);
    }

    if(nutrientLevel>= maxNutrientLevel/2){
        envData[7]=1;
    }
    else if(nutrientLevel<startNutrientLevel){
        envData[7]=-1;
    }
    else{
        envData[7]=0;
    }

    envData[8]=boosterCountdown;


}

void Snake::aiControl(){
    scanEnv();
    int dec=controlAI.decision(envData);
    switch(dec%3){
        case 0:
            turnLeft();
            break;
        case 1:
            turnRight();
            break;
        case 2:
            break;
    }
    if(dec>=3){
        boosterCountdown++;
        cout << "boost activated\n";
    }
}

void Snake::tickHungerCountdown(){
    if(segments.size()>firstTreshold){
        hungerCountdown--;
        if(hungerCountdown==0){
            hungerCountdown=hungerTimer;
            segments.erase(segments.begin()+segments.size()-1);
        }
    }
    if(segments.size()>secondTreshold){
        hungerCountdown--;
        if(hungerCountdown==0){
            hungerCountdown=hungerTimer;
            segments.erase(segments.begin()+segments.size()-1);
        }
    }
}



vector<Snake> snakes;



void renderLinear(){
    for(int i=1; i<=boardSurface; i++){
        if(linear[i]!=barrierColor){
        linear[i]=emptyColor;
        }
    }
    for(auto& snake : snakes){
        for(int segment : snake.segments){
            linear[segment]=snake.color;
        }
    }
    for(int particle : foodParticles){
        linear[particle]=foodColor;
    }
    for(int particle : boosters){
        linear[particle]=boosterColor;
    }
}

void checkAllCollisions(){
    for (int i = 0; i < snakes.size(); i++) {
        for (int j = i; j < snakes.size(); j++) {
            snakes[i].Snake::checkCollision(snakes[i], snakes[j]);
        }
    }
}

void executeAllMovements(){
    for (auto& snake : snakes) {
        snake.Snake::executeMovement();
    }
}

void executeBoosterMovements(){
    for (auto& snake : snakes) {
        if(snake.boosterCountdown){
        snake.Snake::executeMovement();
        snake.boosterCountdown--;
        }
    }
}



void tickHungerCountdowns(){
    for (auto& snake : snakes) {
        snake.tickHungerCountdown();
    }
}

void renderMatrix(){
    for(int i=1; i<=boardSurface; i++){
        matrix[xId(i)][yId(i)]=linear[i];
    }
}

int randomEmptyTile(){
    int n=(rand()%boardSurface)+1;
    while(linear[n]!=emptyColor){
        n=(rand()%boardSurface)+1;
    }
    return n;
}

void regenerateFood(){
    int n=randomEmptyTile();
    linear[n]=foodColor;
    foodParticles.push_back(n);
}

void regenerateBoosters(){
    int n=randomEmptyTile();
    linear[n]=boosterColor;
    boosters.push_back(n);
}

bool pressed = false;

void executeAiControl(){
    for(int i=0; i<snakes.size(); i++){
        snakes[i].aiControl();
    }
}

void executeBoosterAiControl(){
    for(int i=0; i<snakes.size(); i++){
        if(snakes[i].boosterCountdown){
        snakes[i].aiControl();
        }
    }
}

//GRAPHICS

float tileColor[3] = {0.0, 0.0, 0.0};

void getTileColor(int tile){
    tileColor[2]=(float)(tile%256)/(float)(255);
    tile /=256;
    tileColor[1]=(float)(tile%256)/(float)(255);
    tile /=256;
    tileColor[0]=(float)(tile%256)/(float)(255);
}

int randomColor(){
    int output=0;
    int randomizer=rand()%6;
    int channel1, channel2;
    channel1=255;
    channel2=rand()%256;
    switch(randomizer){
        case 0: // red and blue
            output+=channel1;
            output*=256;
            output*=256;
            output+=channel2;
            break;
        case 1: //red and green
            output+=channel1;
            output*=256;
            output+=channel2;
            output*=256;
            break;
        case 2: // green and blue
            output+=channel1;
            output*=256;
            output+=channel2;
            break;
        case 3: // green and red
            output+=channel2;
            output*=256;
            output+=channel1;
            output*=256;
            break;
        case 4: // blue and green
            output+=channel2;
            output*=256;
            output+=channel1;
            break;
        case 5: // blue and red
            output+=channel2;
            output*=256;
            output*=256;
            output+=channel1;
            break;
    }
    return output;

}
void purgeDeadSnakes(){
    for (int j=0; j<snakes.size(); j++) {
        if(snakes[j].collision==true){
            for(int i=0; i<snakes[j].segments.size(); i++){
                if(i%3==2){
                    foodParticles.push_back(snakes[j].segments[i]);
                }
            }
            //snakes.erase(remove(snakes.begin()+j));
            int randDir=rand()%3;
            snakes[j].controlAI.~lnetwork();
            Snake aisnake(randomEmptyTile(),'U', randomColor());
            snakes[j]=aisnake;
            switch(randDir){
                case 0:
                    break;
                case 1:
                    snakes[j].turnRight();
                    break;
                case 2:
                    snakes[j].turnRight();
                    snakes[j].turnRight();
                    break;
                case 3:
                    snakes[j].turnRight();
                    snakes[j].turnRight();
                    snakes[j].turnRight();
                    break;
            }
        }
    }
}

void displayMatrix() {
    glClear(GL_COLOR_BUFFER_BIT);
    float squareSize = (2.0)/boardSize;
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    for (int i = 0; i < boardSize; i++) {
        for (int j = 0; j < boardSize; j++) {
            getTileColor(matrix[i][j]);
            glLoadIdentity();
            glTranslatef(-1 + i * squareSize, -1 + j * squareSize, 0.0);
            glColor3fv(tileColor);
            glBegin(GL_POLYGON);
            glVertex2f(0.0, 0.0);
            glVertex2f(squareSize, 0.0);
            glVertex2f(squareSize, squareSize);
            glVertex2f(0.0, squareSize);
            glEnd();
        }
    }

            glFlush();
}

void addAiSnake(){
    Snake aisnake(randomEmptyTile(),'D', randomColor());
    snakes.push_back(aisnake);
}

void barrierSetup(){
    for(int i=1; i<=boardSurface; i++){
        if(i<5*boardSize+1||i>boardSize*(boardSize-5)||i%boardSize<=5||i%boardSize>=(boardSize-4)){
            linear[i]=barrierColor;
        }
    }
}

bool boostTick=false;
void update() {
    if(rand()%100==0){
        barrierSetup();
    }
    if(!boostTick){
        pressed=false;
        renderLinear();
        while(foodParticles.size()<baseFoodParticles){
            regenerateFood();
        }
        while(boosters.size()<baseBoosters){
            regenerateBoosters();
        }
        renderMatrix();
        displayMatrix();
        executeAiControl();
        executeAllMovements();
        checkAllCollisions();
        purgeDeadSnakes();
        boostTick=true;
    }
    else{
        if(snakes[0].boosterCountdown){
            pressed=false;
        }
        renderLinear();
        while(foodParticles.size()<baseFoodParticles){
            regenerateFood();
        }
        while(boosters.size()<baseBoosters){
            regenerateBoosters();
        }
        renderMatrix();
        displayMatrix();
        executeBoosterAiControl();
        executeBoosterMovements();
        checkAllCollisions();
        purgeDeadSnakes();
        boostTick=false;
    }
    tickHungerCountdowns();

}



int main(){

    loadNetwork();
    if (!glfwInit()) {
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(500, 500, "Snake", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glViewport(0, 0, 500, 500);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

    glShadeModel(GL_SMOOTH);

    barrierSetup();
    renderLinear();
    while(snakes.size()<baseSnakes){;
        addAiSnake();
        renderLinear();
    }
    while(foodParticles.size()<baseFoodParticles){
        regenerateFood();
    }
    while(boosters.size()<baseBoosters){
        regenerateBoosters();
    }
    displayMatrix();
    while (!glfwWindowShouldClose(window)) {
        update();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    saveNetwork();

    glfwTerminate();

    return 0;
}

/*


  ooo
 ooooo
ooooooo
ooooo



*/
