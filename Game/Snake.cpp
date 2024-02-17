#include <iostream>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <cmath>
#include<GL/gl.h>
#include<GL/glu.h>
#include<GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include<random>
#include <string>
#include <sstream>
#include <chrono>
#include "lnetwork.h"
#include "../Shared/levelgen.h"
//130
using namespace std;



const int frameTime = 30;

const int boardSize=CHUNK_BOARD_SIZE*CHUNK_SIZE;
const int foodColor=0xFFFFFF;
const int emptyColor=0x000000;
const int barrierColor=0x888888;
const int boosterColor=0x50F0FF;
const int baseFoodParticles=1024; //200
const int baseBoosters=0; //20
const int hungerTimer=15;
const int baseSnakes=512; //160
const int firstTreshold=1000;
const int secondTreshold=1000;
const int startNutrientLevel=64;
const int maxNutrientLevel=256;
int playerTurn=0;
int playerBoost=0;
int boardSurface=boardSize*boardSize;
int matrix[boardSize][boardSize];
int animationMatrix[boardSize][boardSize];
int linear[boardSize*boardSize+1];
int rainbowColor;
int rainbowR=255;
int rainbowG=51;
int rainbowB=255;
int rainbowMode=0;

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
return 0;
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
return 0;
}
vector<int> foodParticles;
vector<int> boosters;


class Snake{
    public:
        bool player;
        int headPosition;
        vector<int> segments;
        double envData[SCOPE];
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
    player = false;
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
        color=rainbowColor;
        nutrientLevel--;
    }
    else{
        color=snakeID;
    }
    int tailMemory = segments[segments.size()-1];
    for(int i=segments.size()-1; i>0; i--){
        segments[i]=segments[i-1];
    }
    headPosition=movement(headPosition, direction);
    segments[0]=headPosition;
    for(int i=0; i<foodParticles.size(); i++){
        if(headPosition==foodParticles[i]){ // UWAGA
            nutrientLevel+=64;
            nutrientLevel=min(nutrientLevel,256);
            segments.push_back(tailMemory);
            foodParticles.erase(foodParticles.begin() + i);
            break;
        }
    }

    for(int i=0; i<boosters.size(); i++){
        if(headPosition==boosters[i]){ // UWAGA
            nutrientLevel+=128;
            nutrientLevel=min(nutrientLevel,256);
            boosterCountdown=12;
            boosters.erase(boosters.begin() + i);
            break;
        }
    }
    if(linear[headPosition]==barrierColor){
        collision=true;
    }
    if(nutrientLevel<=0){
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
        }
        if(snake2.headPosition==snake1.segments[i]){
            snake2.collision=true;
        }
    }
    for(int i=1; i<snake2.segments.size(); i++){
        if(snake2.headPosition==snake2.segments[i]){
            snake2.collision=true;
        }
        if(snake1.headPosition==snake2.segments[i]){
            snake1.collision=true;
        }
    }

    if(snake1.headPosition==snake2.headPosition&&snake1.color!=snake2.color){
            snake1.collision=true;
            snake2.collision=true;
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



}

void Snake::aiControl(){
        if(!player){
            scanEnv();
            int dec=controlAI.decision(envData);
            if(dec>=3){
                boosterCountdown=1;
            }
            else{
                boosterCountdown=0;
            }
            dec%=3;
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
        }
    else{
        switch(playerTurn){
            case -1:
                turnLeft();
                playerTurn=0;
                break;
            case 1:
                turnRight();
                playerTurn=0;
                break;
            case 0:
                break;
        }
        if(playerBoost){
            boosterCountdown=1;
        }
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

void getTileColor(int tile, float brightness){
    tileColor[2]=(float)(tile%256)/(float)(255)*brightness;
    tile /=256;
    tileColor[1]=(float)(tile%256)/(float)(255)*brightness;
    tile /=256;
    tileColor[0]=(float)(tile%256)/(float)(255)*brightness;
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

void tickRainbowColor(){
    switch(rainbowMode){
        case 0:
            rainbowB-=51;
            if(rainbowB==51){
                rainbowMode++;
            }
            break;
        case 1:
            rainbowG+=51;
            if(rainbowG==255){
                rainbowMode++;
            }
            break;
        case 2:
            rainbowR-=51;
            if(rainbowR==51){
                rainbowMode++;
            }
            break;
        case 3:
            rainbowB+=51;
            if(rainbowB==255){
                rainbowMode++;
            }
            break;
        case 4:
            rainbowG-=51;
            if(rainbowG==51){
                rainbowMode++;
            }
            break;
        case 5:
            rainbowR+=51;
            if(rainbowR==255){
                rainbowMode++;
            }
            break;
    }

    rainbowColor=rainbowR;
    rainbowColor*=256;
    rainbowColor+=rainbowG;
    rainbowColor*=256;
    rainbowColor+=rainbowB;
    rainbowMode%=6;
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

void renderBar(int nl) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float barWidth = (float)(nl) / 256.0f * 800.0f;
    float red = min(1.0f, ((float)(256 - nl) / 256.0f) * 2.0f);
    float green = min(1.0f, ((float)(nl) / 256.0f) * 2.0f);

    // Render 2D bar at the top
    glBegin(GL_QUADS);
    if(playerBoost){
            getTileColor(rainbowColor,1);
            glColor3fv(tileColor);
    }
    else{
        glColor3f(red, green, 0.0f);
    }
    glVertex2f(0.0f, 590.0f);
    glVertex2f(barWidth, 590.0f);
    glVertex2f(barWidth, 600.0f);
    glVertex2f(0.0f, 600.0f);
    glEnd();
}

void animateTile(int code, int stage){
// - ->disappear towards + -> appear towards 1-x+('R') 2-
}
void displayMatrix() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    float squareSize = (2.0) / boardSize;
    float playerX=(-1+squareSize*xId(snakes[0].headPosition))+squareSize/2;
    float playerY=(-1+squareSize*yId(snakes[0].headPosition))+squareSize/2;
    float deltaX, deltaY;

    int cameraDistanceFactor=snakes[0].segments.size();
    cameraDistanceFactor=min(cameraDistanceFactor,32);
    switch(snakes[0].direction) {
        case 'U':
            deltaY=squareSize;
            deltaX=0;
            break;
        case 'R':
            deltaY=0;
            deltaX=squareSize;
            break;
        case 'D':
            deltaY=-squareSize;
            deltaX=0;
            break;
        case 'L':
            deltaY=0;
            deltaX=-squareSize;
            break;
    }
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(100.0, 1, 0.5*squareSize, 256*squareSize);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
   // gluLookAt(playerX, playerY, (25+cameraDistanceFactor/10)*squareSize, playerX, playerY, 0.0, deltaX, deltaY, 0.0);
    gluLookAt(playerX-(4+cameraDistanceFactor)*deltaX, playerY-(4+cameraDistanceFactor)*deltaY, (5+cameraDistanceFactor/2)*squareSize, playerX+3*deltaX, playerY+3*deltaY, 0.0, 0.0, 0.0, 1.0);
    int lowerY=max(0,(yId(snakes[0].headPosition)/CHUNK_SIZE-128/CHUNK_SIZE));
    int lowerX=max(0,(xId(snakes[0].headPosition)/CHUNK_SIZE-128/CHUNK_SIZE));
    int upperY=min(CHUNK_BOARD_SIZE-1,(yId(snakes[0].headPosition)/CHUNK_SIZE+128/CHUNK_SIZE));
    int upperX=min(CHUNK_BOARD_SIZE-1,(xId(snakes[0].headPosition)/CHUNK_SIZE+128/CHUNK_SIZE));
    float chunkSize=squareSize*CHUNK_SIZE;
    for (int i = lowerX; i <= upperX; i++) {
        for (int j = lowerY; j <= upperY; j++) {

            if(chunkBoard[i][j]){ //chunkBoard[i][j]
                glPushMatrix();
                glTranslatef(-1 + i * chunkSize, -1 + j * chunkSize, 0.0);
                glColor3fv(tileColor);
                glBegin(GL_QUADS);

                getTileColor(barrierColor,0.67);
                glColor3fv(tileColor);
                glVertex3f(chunkSize, 0.0, squareSize*2);
                glVertex3f(chunkSize, 0.0, 0.0);
                glVertex3f(chunkSize, chunkSize, 0.0);
                glVertex3f(chunkSize, chunkSize, squareSize*2);

                getTileColor(barrierColor,0.67);
                glColor3fv(tileColor);
                glVertex3f(0.0, 0.0, squareSize*2);
                glVertex3f(0.0, 0.0, 0.0);
                glVertex3f(0.0, chunkSize, 0.0);
                glVertex3f(0.0, chunkSize, squareSize*2);

                getTileColor(barrierColor,0.5);
                glColor3fv(tileColor);
                glVertex3f(0.0, chunkSize, squareSize*2);
                glVertex3f(chunkSize, chunkSize, squareSize*2);
                glVertex3f(chunkSize, chunkSize, 0.0);
                glVertex3f(0.0, chunkSize, 0.0);

                getTileColor(barrierColor,0.5);
                glColor3fv(tileColor);
                glVertex3f(0.0, 0.0, squareSize*2);
                glVertex3f(chunkSize, 0.0, squareSize*2);
                glVertex3f(chunkSize, 0.0, 0.0);
                glVertex3f(0.0, 0.0, 0.0);

                getTileColor(barrierColor,1);
                glColor3fv(tileColor);
                glVertex3f(0.0, 0.0, squareSize*2);
                glVertex3f(chunkSize, 0.0, squareSize*2);
                glVertex3f(chunkSize, chunkSize, squareSize*2);
                glVertex3f(0.0, chunkSize, squareSize*2);
                glEnd();
                glPopMatrix();
            }
            else{
                for (int ii = 0; ii < CHUNK_SIZE; ii++) {
                        for (int jj = 0; jj < CHUNK_SIZE; jj++) {

                            if(matrix[ii+i*CHUNK_SIZE][jj+j*CHUNK_SIZE]==rainbowColor){
                                glPushMatrix();
                                glTranslatef(-1 + ii * squareSize + i * chunkSize, -1 + jj * squareSize + j * chunkSize, 0.0);
                                glColor3fv(tileColor);
                                glBegin(GL_QUADS);

                                getTileColor(matrix[ii+i*CHUNK_SIZE][jj+j*CHUNK_SIZE],0.95);
                                glColor3fv(tileColor);
                                glVertex3f(squareSize, 0.0, squareSize);
                                glVertex3f(squareSize, 0.0, 0.0);
                                glVertex3f(squareSize, squareSize, 0.0);
                                glVertex3f(squareSize, squareSize, squareSize);

                                getTileColor(matrix[ii+i*CHUNK_SIZE][jj+j*CHUNK_SIZE],0.95);
                                glColor3fv(tileColor);
                                glVertex3f(0.0, 0.0, squareSize);
                                glVertex3f(0.0, 0.0, 0.0);
                                glVertex3f(0.0, squareSize, 0.0);
                                glVertex3f(0.0, squareSize, squareSize);

                                getTileColor(matrix[ii+i*CHUNK_SIZE][jj+j*CHUNK_SIZE],0.92);
                                glColor3fv(tileColor);
                                glVertex3f(0.0, squareSize, squareSize);
                                glVertex3f(squareSize, squareSize, squareSize);
                                glVertex3f(squareSize, squareSize, 0.0);
                                glVertex3f(0.0, squareSize, 0.0);

                                getTileColor(matrix[ii+i*CHUNK_SIZE][jj+j*CHUNK_SIZE],0.92);
                                glColor3fv(tileColor);
                                glVertex3f(0.0, 0.0, squareSize);
                                glVertex3f(squareSize, 0.0, squareSize);
                                glVertex3f(squareSize, 0.0, 0.0);
                                glVertex3f(0.0, 0.0, 0.0);

                                getTileColor(matrix[ii+i*CHUNK_SIZE][jj+j*CHUNK_SIZE],1);
                                glColor3fv(tileColor);
                                glVertex3f(0.0, 0.0, squareSize);
                                glVertex3f(squareSize, 0.0, squareSize);
                                glVertex3f(squareSize, squareSize, squareSize);
                                glVertex3f(0.0, squareSize, squareSize);
                                glEnd();
                                glPopMatrix();
                            }
                            else if(matrix[ii+i*CHUNK_SIZE][jj+j*CHUNK_SIZE]!=emptyColor){ //&&matrix[ii+i*CHUNK_SIZE][jj+j*CHUNK_SIZE]!=barrierColor
                                glPushMatrix();
                                glTranslatef(-1 + ii * squareSize + i * chunkSize, -1 + jj * squareSize + j * chunkSize, 0.0);
                                glColor3fv(tileColor);
                                glBegin(GL_QUADS);

                                getTileColor(matrix[ii+i*CHUNK_SIZE][jj+j*CHUNK_SIZE],0.67);
                                glColor3fv(tileColor);
                                glVertex3f(squareSize, 0.0, squareSize);
                                glVertex3f(squareSize, 0.0, 0.0);
                                glVertex3f(squareSize, squareSize, 0.0);
                                glVertex3f(squareSize, squareSize, squareSize);

                                getTileColor(matrix[ii+i*CHUNK_SIZE][jj+j*CHUNK_SIZE],0.67);
                                glColor3fv(tileColor);
                                glVertex3f(0.0, 0.0, squareSize);
                                glVertex3f(0.0, 0.0, 0.0);
                                glVertex3f(0.0, squareSize, 0.0);
                                glVertex3f(0.0, squareSize, squareSize);

                                getTileColor(matrix[ii+i*CHUNK_SIZE][jj+j*CHUNK_SIZE],0.5);
                                glColor3fv(tileColor);
                                glVertex3f(0.0, squareSize, squareSize);
                                glVertex3f(squareSize, squareSize, squareSize);
                                glVertex3f(squareSize, squareSize, 0.0);
                                glVertex3f(0.0, squareSize, 0.0);

                                getTileColor(matrix[ii+i*CHUNK_SIZE][jj+j*CHUNK_SIZE],0.5);
                                glColor3fv(tileColor);
                                glVertex3f(0.0, 0.0, squareSize);
                                glVertex3f(squareSize, 0.0, squareSize);
                                glVertex3f(squareSize, 0.0, 0.0);
                                glVertex3f(0.0, 0.0, 0.0);

                                getTileColor(matrix[ii+i*CHUNK_SIZE][jj+j*CHUNK_SIZE],1);
                                glColor3fv(tileColor);
                                glVertex3f(0.0, 0.0, squareSize);
                                glVertex3f(squareSize, 0.0, squareSize);
                                glVertex3f(squareSize, squareSize, squareSize);
                                glVertex3f(0.0, squareSize, squareSize);
                                glEnd();
                                glPopMatrix();
                            }
                        }
                    }
            }
        }
    }
    glLoadIdentity();
    renderBar(snakes[0].nutrientLevel);
    glFlush();
}

void addAiSnake(){
    Snake aisnake(randomEmptyTile(),'D', randomColor());
    snakes.push_back(aisnake);
}

void barrierSetup(){
    int x, y;
    for(int i=1; i<=boardSurface; i++){
        x=xId(i)/CHUNK_SIZE;
        y=yId(i)/CHUNK_SIZE;
        if(chunkBoard[x][y]){
        linear[i]=barrierColor;
        }
    }
}


bool boostTick=false;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        playerTurn=1;
    else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        playerTurn=-1;


    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
        playerBoost=1;
    }
    else if(button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE) {
        playerBoost=0;
    }
}

void update() {
    tickRainbowColor();
    if(rand()%100==0){
        barrierSetup();
    }
    if(!boostTick){
        renderLinear();
        while(foodParticles.size()<baseFoodParticles){
            regenerateFood();
        }
        while(boosters.size()<baseBoosters){
            regenerateBoosters();
        }
        renderMatrix();
      //  displayMatrix();
        executeAiControl();
        executeAllMovements();
        checkAllCollisions();
        purgeDeadSnakes();
        boostTick=true;
       // renderMatrix();
       // displayMatrix();
    }
    else{
        renderLinear();
        while(foodParticles.size()<baseFoodParticles){
            regenerateFood();
        }
        while(boosters.size()<baseBoosters){
            regenerateBoosters();
        }
        renderMatrix();
      // displayMatrix();
        executeBoosterAiControl();
        executeBoosterMovements();
        checkAllCollisions();
        purgeDeadSnakes();
        boostTick=false;
      //  renderMatrix();
       // displayMatrix();
    }
    tickHungerCountdowns();
    snakes[0].player=true;

}



int main(){
    generateLevel(2137);
    loadNetwork();
    if (!glfwInit()) {
        return -1;
    }
    GLFWwindow* window = glfwCreateWindow(1000, 1000, "Snake", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSwapInterval(1);
    glShadeModel(GL_SMOOTH);
    barrierSetup();
    renderLinear();
    while(snakes.size()<baseSnakes){;
        addAiSnake();
        renderLinear();
    }
    snakes[0].player=true;
    while(foodParticles.size()<baseFoodParticles){
        regenerateFood();
    }
    while(boosters.size()<baseBoosters){
        regenerateBoosters();
    }
    renderMatrix();
    displayMatrix();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        update();
        renderLinear();
        renderMatrix();
        displayMatrix();
        glfwSwapBuffers(window);
        usleep(frameTime*1000);
    }

    glfwTerminate();

    return 0;
}

