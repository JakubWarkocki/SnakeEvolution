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
#define boardSize 160
//130
using namespace std;



const int frameTime = 50;

//const int boardSize=100;
const int foodColor=0xFFFFFF;
const int emptyColor=0x000000;
const int barrierColor=0x888888;
const int boosterColor=0x50F0FF;
const int baseFoodParticles=70; //200
const int baseBoosters=0; //20
const int hungerTimer=15;
const int baseSnakes=50; //160
const int firstTreshold=32;
const int secondTreshold=64;
const int startNutrientLevel=64;
const int maxNutrientLevel=256;
int playerTurn=0;
int playerBoost=0;
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
        bool player;
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
        color=foodColor-rand()%254-1;
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

    envData[8]=boosterCountdown;


}

void Snake::aiControl(){
    if(!player){
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
            boosterCountdown=1;
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
    // Set up orthographic projection for 2D rendering
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);   // Adjust based on your window size

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float barWidth = (float)(nl) / 256.0f * 800.0f;  // Adjust as needed
    float red = min(1.0f, ((float)(256 - nl) / 256.0f) * 2.0f);
    float green = min(1.0f, ((float)(nl) / 256.0f) * 2.0f);

    // Render 2D bar at the top
    glBegin(GL_QUADS);
    glColor3f(red, green, 0.0f);
    glVertex2f(0.0f, 590.0f);                // Top-left corner
    glVertex2f(barWidth, 590.0f);            // Top-right corner (width depends on nutrientLevel)
    glVertex2f(barWidth, 600.0f);            // Bottom-right corner
    glVertex2f(0.0f, 600.0f);                // Bottom-left corner
    glEnd();
}


void displayMatrix() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // Add GL_DEPTH_BUFFER_BIT for depth testing
    glEnable(GL_DEPTH_TEST);
    float squareSize = (2.0) / boardSize;
    float playerX=(-1+squareSize*xId(snakes[0].headPosition))+squareSize/2;
    float playerY=(-1+squareSize*yId(snakes[0].headPosition))+squareSize/2;
    float deltaX, deltaY;
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
    gluPerspective(70.0, 1, squareSize, 100.0);  // Adjust these values based on your scene requirements
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(playerX-(2+snakes[0].segments.size())*deltaX, playerY-(2+snakes[0].segments.size())*deltaY, (2+snakes[0].segments.size()/2)*squareSize, playerX+3*deltaX, playerY+3*deltaY, 0.0, 0.0, 0.0, 1.0);
    for (int i = 0; i < boardSize; i++) {
        for (int j = 0; j < boardSize; j++) {
            glPushMatrix();
            glTranslatef(-1 + i * squareSize, -1 + j * squareSize, 0.0);
            glColor3fv(tileColor);
            if(matrix[i][j]!=emptyColor){
                glBegin(GL_QUADS);

                // Bottom face
                getTileColor(matrix[i][j],0.1);
                glColor3fv(tileColor);
                glVertex3f(0.0, 0.0, 0.0);
                glVertex3f(squareSize, 0.0, 0.0);
                glVertex3f(squareSize, squareSize, 0.0);
                glVertex3f(0.0, squareSize, 0.0);

                // face
                getTileColor(matrix[i][j],0.67);
                glColor3fv(tileColor);
                glVertex3f(squareSize, 0.0, squareSize);
                glVertex3f(squareSize, 0.0, 0.0);
                glVertex3f(squareSize, squareSize, 0.0);
                glVertex3f(squareSize, squareSize, squareSize);

                //  face face
                getTileColor(matrix[i][j],0.67);
                glColor3fv(tileColor);
                glVertex3f(0.0, 0.0, squareSize);
                glVertex3f(0.0, 0.0, 0.0);
                glVertex3f(0.0, squareSize, 0.0);
                glVertex3f(0.0, squareSize, squareSize);

                // face
                getTileColor(matrix[i][j],0.5);
                glColor3fv(tileColor);
                glVertex3f(0.0, squareSize, squareSize);
                glVertex3f(squareSize, squareSize, squareSize);
                glVertex3f(squareSize, squareSize, 0.0);
                glVertex3f(0.0, squareSize, 0.0);

                // face
                getTileColor(matrix[i][j],0.5);
                glColor3fv(tileColor);
                glVertex3f(0.0, 0.0, squareSize);
                glVertex3f(squareSize, 0.0, squareSize);
                glVertex3f(squareSize, 0.0, 0.0);
                glVertex3f(0.0, 0.0, 0.0);

                // Top face
                getTileColor(matrix[i][j],1);
                glColor3fv(tileColor);
                glVertex3f(0.0, 0.0, squareSize);
                glVertex3f(squareSize, 0.0, squareSize);
                glVertex3f(squareSize, squareSize, squareSize);
                glVertex3f(0.0, squareSize, squareSize);
                glEnd();
            }
            glPopMatrix();
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
    for(int i=1; i<=boardSurface; i++){
        if(i<5*boardSize+1||i>boardSize*(boardSize-5)||i%boardSize<=5||i%boardSize>=(boardSize-4)){
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

}



int main(){

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
        snakes[0].player=true;
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

/*


  ooo
 ooooo
ooooooo
ooooo



*/
